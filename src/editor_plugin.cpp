/*
** Bitty
**
** An itty bitty 2D game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "editing.h"
#include "editor_code.h"
#include "editor_plugin.h"
#include "encoding.h"
#include "filesystem.h"
#include "platform.h"
#include "project.h"
#include "theme.h"
#include "workspace.h"
#include "../lib/imgui_code_editor/imgui_code_editor.h"
#include <SDL.h>

/*
** {===========================================================================
** Editor customized by plugin
*/

namespace Bitty {

class EditorPluginImpl : public EditorPlugin, public ImGui::CodeEditor {
private:
	bool _opened = false;

	std::string _name;
	Bytes::Ptr _object = nullptr;
	Editing::Data::Checkpoint _checkpoint;

	bool _acquireFocus = false;
	struct {
		std::string text;
		bool filled = false;

		void clear(void) {
			text.clear();
			filled = false;
		}
	} _status;
	mutable struct {
		std::string text;
		bool overdue = true;

		void clear(void) {
			text.clear();
			overdue = true;
		}
	} _cache;

	struct Tools {
		bool initialized = false;
		bool focused = false;

		int jumping = -1;

		bool finding = false;
		Editing::Tools::Marker marker;
		std::string word;
		int direction = 0;

		void clear(void) {
			initialized = false;
			focused = false;

			jumping = -1;

			finding = false;
		}
	} _tools;
	struct Compiler {
		Text::Array quotes;

		void clear(void) {
			quotes.clear();
		}
	} _compiler;
	struct Injection {
		bool initialized = false;
		int pluginIndex = -1;
		Text::Array assets;
		int installedCount = 0;

		void clear(void) {
			initialized = false;
			pluginIndex = -1;
			assets.clear();
			installedCount = 0;
		}
	} _injection;

public:
	EditorPluginImpl() {
		_checkpoint.fill();

		SetLanguageDefinition(languageDefinition());

		for (int i = 0; i < BITTY_COUNTOF(EDITOR_CODE_KEYWORDS); ++i)
			addKeyword(EDITOR_CODE_KEYWORDS[i]);
		for (int i = 0; i < BITTY_COUNTOF(EDITOR_CODE_MODULES); ++i)
			addIdentifier(EDITOR_CODE_MODULES[i]);
		for (int i = 0; i < BITTY_COUNTOF(EDITOR_CODE_PRIMITIVES); ++i)
			addIdentifier(EDITOR_CODE_PRIMITIVES[i]);
	}
	virtual ~EditorPluginImpl() override {
		close(nullptr);
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual void open(const class Project* project, const char* name, Object::Ptr obj, const char* /* ref */) override {
		if (_opened)
			return;
		_opened = true;

		_name = name;

		_object = Object::as<Bytes::Ptr>(obj);
		if (_object) {
			std::string txt;
			const size_t pos = _object->peek();
			_object->poke(0);
			_object->readString(txt);
			_object->poke(pos);
			text(txt.c_str(), txt.length());
		}
		Editing::Data::toCheckpoint(project, _name.c_str(), _checkpoint);

		SetStickyLineNumbers(true);

		DisableShortcut((ShortcutType)(UndoRedo | CopyCutPasteDelete | IndentUnindent));

		SetTooltipEnabled(false);

		SetModifiedHandler(std::bind(&EditorPluginImpl::modified, this));

		fprintf(stdout, "Editor customized by plugin opened: \"%s\".\n", _name.c_str());
	}
	virtual void close(const class Project* project) override {
		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "Editor customized by plugin closed: \"%s\".\n", _name.c_str());

		if (!_checkpoint.empty()) {
			if (hasUnsavedChanges())
				Editing::Data::fromCheckpoint(project, _name.c_str(), _checkpoint);
			_checkpoint.clear();
		}

		SetModifiedHandler(nullptr);

		_compiler.clear();
		_injection.clear();

		_cache.clear();
		_status.clear();

		_object = nullptr;
		_name.clear();
	}

	virtual void flush(void) const override {
		if (!_object)
			return;

		_object->clear();
		size_t len = 0;
		const char* txt = text(&len);
		if (txt && len)
			_object->writeString(txt);
		else
			_object->writeString("");
		_object->poke(0);
	}

	virtual bool readonly(void) const override {
		return IsReadOnly();
	}
	virtual void readonly(bool ro) override {
		SetReadOnly(ro);

		_status.clear();
	}

	virtual bool hasUnsavedChanges(void) const override {
		return !IsChangesSaved();
	}
	virtual void markChangesSaved(const class Project* project) override {
		Editing::Data::toCheckpoint(project, _name.c_str(), _checkpoint);

		SetChangesSaved();
	}

	virtual void copy(void) override {
		if (_tools.focused)
			return;

		Copy();
	}
	virtual void cut(void) override {
		if (ReadOnly) {
			copy();

			return;
		}

		if (_tools.focused)
			return;

		Cut();
	}
	virtual bool pastable(void) const override {
		if (ReadOnly)
			return false;

		return Platform::hasClipboardText();
	}
	virtual void paste(void) override {
		if (ReadOnly)
			return;

		if (_tools.focused)
			return;

		Paste();
	}
	virtual void del(void) override {
		if (ReadOnly)
			return;

		if (_tools.focused)
			return;

		Delete();
	}
	virtual bool selectable(void) const override {
		return true;
	}

	virtual const char* redoable(void) const override {
		return CanRedo() ? "" : nullptr;
	}
	virtual const char* undoable(void) const override {
		return CanUndo() ? "" : nullptr;
	}

	virtual void redo(class Asset*) override {
		if (ReadOnly)
			return;

		Redo();
	}
	virtual void undo(class Asset*) override {
		if (ReadOnly)
			return;

		Undo();
	}

	virtual Variant post(unsigned msg, int argc, const Variant* argv) override {
		switch (msg) {
		case SET_THEME_STYLE: {
				const Variant::Int idx = unpack<Variant::Int>(argc, argv, 0, -1);
				switch (idx) {
				case Theme::DARK:
					SetPalette(ImGui::CodeEditor::GetDarkPalette());

					break;
				case Theme::CLASSIC:
					SetPalette(ImGui::CodeEditor::GetRetroBluePalette());

					break;
				case Theme::LIGHT:
					SetPalette(ImGui::CodeEditor::GetLightPalette());

					break;
				}
			}

			return Variant(true);
		case SET_INDENT_RULE: {
				const Int rule = unpack<Int>(argc, argv, 0, (Int)Workspace::Settings::TAB_4);
				switch ((Workspace::Settings::IndentRules)rule) {
				case Workspace::Settings::SPACE_2:
					SetIndentWithTab(false);
					SetTabSize(2);

					break;
				case Workspace::Settings::SPACE_4:
					SetIndentWithTab(false);
					SetTabSize(4);

					break;
				case Workspace::Settings::SPACE_8:
					SetIndentWithTab(false);
					SetTabSize(8);

					break;
				case Workspace::Settings::TAB_2:
					SetIndentWithTab(true);
					SetTabSize(2);

					break;
				case Workspace::Settings::TAB_4:
					SetIndentWithTab(true);
					SetTabSize(4);

					break;
				case Workspace::Settings::TAB_8:
					SetIndentWithTab(true);
					SetTabSize(8);

					break;
				}
			}

			return Variant(true);
		case SET_COLUMN_INDICATOR: {
				const Int rule = unpack<Int>(argc, argv, 0, (Int)Workspace::Settings::COL_80);
				switch ((Workspace::Settings::ColumnIndicator)rule) {
				case Workspace::Settings::COL_NONE:
					SetSafeColumnIndicatorOffset(0);

					break;
				case Workspace::Settings::COL_40:
					SetSafeColumnIndicatorOffset(40);

					break;
				case Workspace::Settings::COL_80:
					SetSafeColumnIndicatorOffset(80);

					break;
				case Workspace::Settings::COL_100:
					SetSafeColumnIndicatorOffset(100);

					break;
				case Workspace::Settings::COL_120:
					SetSafeColumnIndicatorOffset(120);

					break;
				}
			}

			return Variant(true);
		case SET_SHOW_SPACES: {
				const bool show = unpack<bool>(argc, argv, 0, true);
				SetShowWhiteSpaces(show);
			}

			return Variant(true);
		case FOCUS:
			_acquireFocus = true;

			return Variant(true);
		case SELECT_ALL:
			if (_tools.focused)
				return Variant(false);

			SelectAll();

			return Variant(true);
		case SELECT_WORD:
			if (_tools.focused)
				return Variant(false);

			SelectWordUnderCursor();

			return Variant(true);
		case INDENT: {
				const bool byKey = unpack<bool>(argc, argv, 0, true);

				if (_tools.focused)
					return Variant(false);

				Indent(byKey);
			}

			return Variant(true);
		case UNINDENT: {
				const bool byKey = unpack<bool>(argc, argv, 0, true);

				if (_tools.focused)
					return Variant(false);

				Unindent(byKey);
			}

			return Variant(true);
		case MOVE_UP:
			if (_tools.focused)
				return Variant(false);

			MoveLineUp();

			return Variant(true);
		case MOVE_DOWN:
			if (_tools.focused)
				return Variant(false);

			MoveLineDown();

			return Variant(true);
		case FIND: {
				_tools.jumping = -1;

				_tools.finding = true;

				Coordinates begin, end;
				GetSelection(begin, end);
				if (begin == end)
					_tools.word = GetWordUnderCursor(&begin, &end);
				else
					_tools.word = GetSelectionText();
				SetSelection(begin, end);

				_tools.direction = 0;
			}

			return Variant(true);
		case FIND_NEXT:
			_tools.jumping = -1;

			if (_tools.word.empty()) {
				_tools.finding = true;

				_tools.word = GetWordUnderCursor();
			}

			_tools.direction = 1;

			return Variant(true);
		case FIND_PREVIOUS:
			_tools.jumping = -1;

			if (_tools.word.empty()) {
				_tools.finding = true;

				_tools.word = GetWordUnderCursor();
			}

			_tools.direction = -1;

			return Variant(true);
		case GOTO: {
				_tools.finding = false;

				const Coordinates coord = GetCursorPosition();
				_tools.jumping = coord.Line;
			}

			return Variant(true);
		case GET_CURSOR:
			return Variant((Variant::Int)GetCursorPosition().Line);
		case SET_CURSOR: {
				const Variant::Int ln = unpack<Variant::Int>(argc, argv, 0, -1);
				if (ln < 0 || ln >= GetTotalLines())
					break;

				SetCursorPosition(Coordinates(ln, 0));
			}

			return Variant(true);
		default: // Do nothing.
			break;
		}

		return Variant(false);
	}
	using Dispatchable::post;

	virtual void update(
		class Window* wnd, class Renderer* rnd,
		class Workspace* ws, const class Project* project, class Executable* /* exec */,
		const char* title,
		float /* x */, float /* y */, float width, float height,
		int /* scale */,
		bool pending,
		double /* delta */
	) override {
		inject(ws, project);

		ImGuiStyle &style = ImGui::GetStyle();

		shortcuts(wnd, rnd, ws);

		const float statusBarHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;

		float toolBarHeight = 0;
		if (_tools.jumping >= 0) {
			const float posY = ImGui::GetCursorPosY();
			if (Editing::Tools::jump(rnd, ws, &_tools.jumping, width, &_tools.initialized, &_tools.focused, 0, GetTotalLines() - 1)) {
				SetCursorPosition(Coordinates(_tools.jumping, 0));
			}
			toolBarHeight += ImGui::GetCursorPosY() - posY;
		}
		if (_tools.finding || _tools.direction != 0) {
			Coordinates srcBegin, srcEnd;
			GetSelection(srcBegin, srcEnd);
			_tools.marker = Editing::Tools::Marker(
				Editing::Tools::Marker::Coordinates(srcBegin.Line, srcBegin.Column),
				Editing::Tools::Marker::Coordinates(srcEnd.Line, srcEnd.Column)
			);

			const float y = ImGui::GetCursorPosY();
			const bool stepped = Editing::Tools::find(
				rnd, ws,
				&_tools.marker,
				width,
				&_tools.initialized, &_tools.focused,
				text(nullptr), &_tools.word,
				Editing::Tools::Marker::Coordinates(GetTotalLines(), GetColumnsAt(GetTotalLines())),
				&_tools.direction,
				&ws->settings()->editorCaseSensitive, &ws->settings()->editorMatchWholeWord,
				_tools.finding,
				[&] (const Editing::Tools::Marker::Coordinates &pos, Editing::Tools::Marker &src) -> std::string {
					Coordinates srcBegin, srcEnd;
					const std::string result = GetWordAt(Coordinates(pos.line, pos.column), &srcBegin, &srcEnd);
					src.begin = Editing::Tools::Marker::Coordinates(srcBegin.Line, srcBegin.Column);
					src.end = Editing::Tools::Marker::Coordinates(srcEnd.Line, srcEnd.Column);

					return result;
				}
			);
			if (stepped && !_tools.marker.empty()) {
				const Coordinates begin(_tools.marker.begin.line, _tools.marker.begin.column);
				const Coordinates end(_tools.marker.end.line, _tools.marker.end.column);

				SetCursorPosition(begin);
				SetSelection(begin, end);
			}
			toolBarHeight += ImGui::GetCursorPosY() - y;
		}

		if (_acquireFocus) {
			if (!ws->popupBox()) {
				_acquireFocus = false;
				if (!ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
					ImGui::SetNextWindowFocus();
			}
		}

		ImFont* fontCode = ws->theme()->fontCode();
		if (fontCode && fontCode->IsLoaded()) {
			ImGui::PushFont(fontCode);
			SetFont(fontCode);
		}
		Render(title, ImVec2(width, height - statusBarHeight - toolBarHeight));
		if (fontCode && fontCode->IsLoaded()) {
			SetFont(nullptr);
			ImGui::PopFont();
		}

		context(wnd, rnd, ws);

		renderStatus(wnd, rnd, ws, project, width, statusBarHeight, pending);
	}

	virtual void played(class Renderer* /* rnd */, const class Project* /* project */) override {
		// Do nothing.
	}
	virtual void stopped(class Renderer* /* rnd */, const class Project* /* project */) override {
		// Do nothing.
	}

	virtual void resized(class Renderer* /* rnd */, const class Project* /* project */) override {
		// Do nothing.
	}

	virtual void lostFocus(class Renderer* /* rnd */, const class Project* /* project */) override {
		// Do nothing.
	}
	virtual void gainFocus(class Renderer* /* rnd */, const class Project* /* project */) override {
		// Do nothing.
	}

private:
	void addKeyword(const char* str) {
		LanguageDefinition &def = GetLanguageDefinition();
		def.Keys.insert(str);
	}
	void addIdentifier(const char* str) {
		LanguageDefinition &def = GetLanguageDefinition();
		Identifier id;
		id.Declaration = "Bitty function";
		def.Ids.insert(std::make_pair(std::string(str), id));
	}
	void addPreprocessor(const char* str) {
		LanguageDefinition &def = GetLanguageDefinition();
		Identifier id;
		id.Declaration = "Bitty preprocessor";
		def.PreprocIds.insert(std::make_pair(std::string(str), id));
	}

	const char* text(size_t* len) const {
		if (_cache.overdue) {
			_cache.text = GetText("\n");
			_cache.overdue = false;
		}
		if (len)
			*len = _cache.text.length();

		return _cache.text.c_str();
	}
	void text(const char* txt, size_t /* len */) {
		SetText(txt);
	}

	void shortcuts(Window* /* wnd */, Renderer* /* rnd */, Workspace* ws) {
		if (!ws->canUseShortcuts())
			return;

		const Editing::Shortcut esc(SDL_SCANCODE_ESCAPE);
		if (esc.pressed()) {
			_tools.clear();
		}
	}

	void context(Window* /* wnd */, Renderer* /* rnd */, Workspace* ws) {
		ImGuiStyle &style = ImGui::GetStyle();

		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			if (!HasSelection())
				SelectWordUnderMouse();

			ImGui::OpenPopup("@Ed/Ctx");
		}

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
		VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

		if (ImGui::BeginPopup("@Ed/Ctx")) {
			if (ImGui::MenuItem(ws->theme()->menuEdit_Cut())) {
				cut();
			}
			if (ImGui::MenuItem(ws->theme()->menuEdit_Copy())) {
				copy();
			}
			if (ImGui::MenuItem(ws->theme()->menuEdit_Paste(), nullptr, nullptr, pastable())) {
				paste();
			}
			if (ImGui::MenuItem(ws->theme()->menuEdit_Delete())) {
				del();
			}
			ImGui::Separator();
			if (ImGui::MenuItem(ws->theme()->menuEdit_SelectAll())) {
				post(Editable::SELECT_ALL);
			}

			ImGui::EndPopup();
		}
	}

	void refreshStatus(Window* /* wnd */, Renderer* /* rnd */, Workspace* ws) {
		if (_status.filled)
			return;

		_status.filled = true;

		if (readonly()) {
			_status.text += ws->theme()->statusTip_Readonly();
		}
	}
	void renderStatus(Window* wnd, Renderer* rnd, Workspace* ws, const Project* project, float width, float height, bool pending) {
		refreshStatus(wnd, rnd, ws);

		ImGuiStyle &style = ImGui::GetStyle();

		const bool actived = IsEditorFocused() || ImGui::IsWindowFocused();
		if (actived) {
			const ImVec2 pos = ImGui::GetCursorPos();
			ImGui::Dummy(
				ImVec2(width - style.ChildBorderSize, height - style.ChildBorderSize),
				ImGui::GetStyleColorVec4(ImGuiCol_TabActive)
			);
			ImGui::SetCursorPos(pos);
		}

		if (actived)
			ImGui::PushStyleColor(ImGuiCol_Text, pending ? ws->theme()->style()->tabTextPendingColor : ws->theme()->style()->tabTextColor);
		const Coordinates coord = GetCursorPosition();
		ImGui::Dummy(ImVec2(8, 0));
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::Text(
			"%s %d/%d  %s %d    %s",
			ws->theme()->statusItem_Ln().c_str(),
			coord.Line + 1, GetTotalLines(),
			ws->theme()->statusItem_Col().c_str(),
			coord.Column + 1,
			_status.text.c_str()
		);
		if (!readonly()) {
			ImGui::SameLine();
			const char* txt = ws->theme()->generic_Install().c_str();
			if ((int)_injection.assets.size() == _injection.installedCount)
				txt = ws->theme()->generic_Reinstall().c_str();
			if (ImGui::Url(txt, nullptr, true))
				install(ws, project);
		}
		if (actived)
			ImGui::PopStyleColor();
	}

	void modified(void) {
		_cache.overdue = true;
	}

	void inject(Workspace* ws, const Project* project) {
		if (_injection.initialized)
			return;
		_injection.initialized = true;

		auto getBool = [] (const IDictionary::Ptr &dict, const char* key, bool &out) -> bool {
			out = false;

			if (dict->contains(key)) {
				const Variant elem = dict->get(key);
				if (elem.type() != Variant::BOOLEAN)
					return false;

				out = (bool)elem;

				return true;
			}

			return false;
		};
		auto getText = [] (const IDictionary::Ptr &dict, const char* key, std::string &out) -> bool {
			out.clear();

			if (dict->contains(key)) {
				const Variant elem = dict->get(key);
				if (elem.type() != Variant::STRING)
					return false;

				out = elem.toString();

				return true;
			}

			return false;
		};
		auto getTextArray = [] (const IDictionary::Ptr &dict, const char* key, Text::Array &out) -> bool {
			out.clear();

			if (dict->contains(key)) {
				const Variant elem = dict->get(key);
				if (elem.type() != Variant::OBJECT)
					return false;

				Object::Ptr obj = (Object::Ptr)elem;
				if (!obj)
					return false;
				if (!Object::is<IList::Ptr>(obj))
					return false;

				IList::Ptr list = Object::as<IList::Ptr>(obj);
				for (int i = 0; i < list->count(); ++i) {
					const Variant elem_ = list->at(i);
					if (elem_.type() != Variant::STRING)
						continue;

					out.push_back(elem_.toString());
				}

				return true;
			}

			return false;
		};

		std::string ext;
		Path::split(_name, nullptr, &ext, nullptr);
		LanguageDefinition langDef = languageDefinition();

		for (int i = 0; i < (int)ws->plugins().size(); ++i) {
			Plugin* plugin = ws->plugins()[i];
			if (!plugin->is(Plugin::Usages::COMPILER))
				continue;

			const Plugin::Schema &schema = plugin->schema();
			if (ext != schema.extension)
				continue;

			const Variant schema_ = plugin->run(Plugin::Functions::SCHEMA, "", nullptr);
			if (schema_.type() != Variant::OBJECT)
				continue;
			Object::Ptr obj = (Object::Ptr)schema_;
			if (!obj)
				continue;
			if (!Object::is<IDictionary::Ptr>(obj))
				continue;

			IDictionary::Ptr dict = Object::as<IDictionary::Ptr>(obj);
			bool boolean = false;
			std::string str;
			Text::Array arr;
			if (getText(dict, "name", str)) {
				langDef.Name = str;
			}
			if (getTextArray(dict, "keywords", arr)) {
				for (const std::string &txt : arr)
					langDef.Keys.insert(txt);
			}
			if (getTextArray(dict, "identifiers", arr)) {
				for (const std::string &txt : arr) {
					Identifier id;
					id.Declaration = "Built-in function";
					auto it = langDef.Ids.insert(std::make_pair(txt, id));
					if (!it.second) {
						fprintf(stderr, "Duplicated identifier: \"%s\"\n", txt.c_str());

						BITTY_ASSERT(false && "Duplicated identifier.");
					}
				}
			}
			if (getTextArray(dict, "quotes", arr)) {
				_compiler.quotes = arr;
			}
			std::string str0, str1;
			if (getText(dict, "multiline_comment_start", str0) && getText(dict, "multiline_comment_end", str1)) {
				langDef.CommentStart = str0;
				langDef.CommentEnd = str1;
			}
			if (getTextArray(dict, "comment_patterns", arr)) {
				for (const std::string &txt : arr)
					langDef.TokenRegexPatterns.push_back(std::make_pair(txt, PaletteIndex::Comment));
			}
			if (getTextArray(dict, "number_patterns", arr)) {
				for (const std::string &txt : arr)
					langDef.TokenRegexPatterns.push_back(std::make_pair(txt, PaletteIndex::Number));
			}
			if (getTextArray(dict, "identifier_patterns", arr)) {
				for (const std::string &txt : arr)
					langDef.TokenRegexPatterns.push_back(std::make_pair(txt, PaletteIndex::Identifier));
			}
			if (getTextArray(dict, "punctuation_patterns", arr)) {
				for (const std::string &txt : arr)
					langDef.TokenRegexPatterns.push_back(std::make_pair(txt, PaletteIndex::Punctuation));
			}
			if (getBool(dict, "case_sensitive", boolean)) {
				langDef.CaseSensitive = boolean;
				boolean = false;
			}

			if (getTextArray(dict, "assets", arr)) {
				_injection.assets = arr;
			}
			_injection.pluginIndex = i;

			break;
		}

		SetLanguageDefinition(langDef);

		check(ws, project);
	}
	void install(Workspace* ws, const Project* project) {
		if (_injection.pluginIndex < 0 || _injection.pluginIndex >= (int)ws->plugins().size())
			return;

		Plugin* plugin = ws->plugins()[_injection.pluginIndex];
		if (!plugin->is(Plugin::Usages::COMPILER))
			return;

		std::string ext;
		Path::split(_name, nullptr, &ext, nullptr);
		const Plugin::Schema &schema = plugin->schema();
		if (ext != schema.extension)
			return;

		plugin->run(Plugin::Functions::COMPILER, "", nullptr);

		check(ws, project);
	}
	void check(Workspace*, const Project* project) {
		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = project->acquire(acquired);
		if (!prj)
			return;

		_injection.installedCount = 0;
		for (const std::string &asset : _injection.assets) {
			if (prj->get(asset.c_str()))
				++_injection.installedCount;
		}
	}

	bool tokenize(const char* inBegin, const char* inEnd, const char* &outBegin, const char* &outEnd, PaletteIndex &paletteIndex) const {
		paletteIndex = PaletteIndex::Max;

		while (inBegin < inEnd && isascii(*inBegin) && isblank(*inBegin))
			inBegin++;

		if (inBegin == inEnd) {
			outBegin = inEnd;
			outEnd = inEnd;
			paletteIndex = PaletteIndex::Default;
		} else {
			for (const std::string &quote : _compiler.quotes) {
				if (quote.length() == 1) {
					if (tokenizeString(inBegin, inEnd, outBegin, outEnd, quote.front()))
						paletteIndex = PaletteIndex::String;
				}
			}
		}

		return paletteIndex != PaletteIndex::Max;
	}
	bool tokenizeString(const char* inBegin, const char* inEnd, const char* &outBegin, const char* &outEnd, char quote) const {
		const char* p = inBegin;
		if (*p != quote)
			return false;
		++p;

		while (p < inEnd) {
			if (*p == quote) {
				outBegin = inBegin;
				outEnd = p + 1;

				return true;
			}

			if (*p == '\\' && p + 1 < inEnd && p[1] == quote)
				++p;

			p += Unicode::expectUtf8(p);
		}

		return false;
	}
	LanguageDefinition languageDefinition(void) const {
		LanguageDefinition langDef;

		langDef.Tokenize = std::bind(
			&EditorPluginImpl::tokenize, this,
			std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4,
			std::placeholders::_5
		);

		return langDef;
	}
};

EditorPlugin* EditorPlugin::create(void) {
	EditorPluginImpl* result = new EditorPluginImpl();

	return result;
}

void EditorPlugin::destroy(EditorPlugin* ptr) {
	EditorPluginImpl* impl = static_cast<EditorPluginImpl*>(ptr);
	delete impl;
}

}

/* ===========================================================================} */
