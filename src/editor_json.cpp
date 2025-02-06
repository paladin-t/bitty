/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "editing.h"
#include "editor_json.h"
#include "encoding.h"
#include "platform.h"
#include "theme.h"
#include "workspace.h"
#include "../lib/imgui_code_editor/imgui_code_editor.h"
#include <SDL.h>

/*
** {===========================================================================
** JSON editor
*/

class EditorJsonImpl : public EditorJson, public ImGui::CodeEditor {
private:
	bool _opened = false;

	std::string _name;
	Json::Ptr _object = nullptr;
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

		Json::Error error;
		bool hasError = false;

		void clear(void) {
			text.clear();
			overdue = true;

			error = Json::Error();
			hasError = false;
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

public:
	EditorJsonImpl() {
		_checkpoint.fill();

		SetLanguageDefinition(languageDefinition());
	}
	virtual ~EditorJsonImpl() override {
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

		_object = Object::as<Json::Ptr>(obj);
		if (_object) {
			std::string txt;
			_object->toString(txt);
			text(txt.c_str(), txt.length());
		}
		Editing::Data::toCheckpoint(project, _name.c_str(), _checkpoint);

		DisableShortcut(All);

		SetTooltipEnabled(false);

		SetModifiedHandler(std::bind(&EditorJsonImpl::modified, this));

		fprintf(stdout, "JSON editor opened: \"%s\".\n", _name.c_str());
	}
	virtual void close(const class Project* project) override {
		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "JSON editor closed: \"%s\".\n", _name.c_str());

		if (!_checkpoint.empty()) {
			if (hasUnsavedChanges())
				Editing::Data::fromCheckpoint(project, _name.c_str(), _checkpoint);
			_checkpoint.clear();
		}

		SetModifiedHandler(nullptr);

		_cache.clear();
		_status.clear();

		_object = nullptr;
		_name.clear();
	}

	virtual void flush(void) const override {
		if (!_object)
			return;

		size_t len = 0;
		const char* txt = text(&len);
		if (txt && len) {
			Json::Error error;
			if (!_object->fromString(txt, &error)) {
				int ln = 1;
				const char* tmp = txt;
				for (unsigned i = 0; i < error.position && *tmp; ++i, ++tmp) {
					if (*tmp == '\n')
						++ln;
				}
				_cache.error = error;
				_cache.error.message = "JSON error:\n  " + Text::toString(ln) + ": " + _cache.error.message;
				_cache.hasError = true;
			}
		} else {
			_object->fromString("null");
		}
	}

	virtual const char* text(size_t* len) const override {
		if (_cache.overdue) {
			_cache.text = GetText("\n");
			_cache.overdue = false;
		}
		if (len)
			*len = _cache.text.length();

		return _cache.text.c_str();
	}
	virtual void text(const char* txt, size_t /* len */) override {
		SetText(txt);
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
				_tools.initialized = false;

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
				_tools.initialized = false;

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
		class Workspace* ws, const class Project* /* project */, class Executable* /* exec */,
		const char* title,
		float /* x */, float /* y */, float width, float height,
		float /* scaleX */, float /* scaleY */,
		bool pending,
		double /* delta */
	) override {
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

		renderStatus(wnd, rnd, ws, width, statusBarHeight, pending);

		if (_cache.hasError) {
			_cache.hasError = false;
			ws->error(_cache.error.message.c_str());
		}
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
	void renderStatus(Window* wnd, Renderer* rnd, Workspace* ws, float width, float height, bool pending) {
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
		if (actived)
			ImGui::PopStyleColor();
	}

	void modified(void) {
		_cache.overdue = true;
	}

	bool tokenize(const char* inBegin, const char* inEnd, const char* &outBegin, const char* &outEnd, PaletteIndex &paletteIndex) const {
		paletteIndex = PaletteIndex::Max;

		while (inBegin < inEnd && isascii(*inBegin) && isblank(*inBegin))
			inBegin++;

		if (inBegin == inEnd) {
			outBegin = inEnd;
			outEnd = inEnd;
			paletteIndex = PaletteIndex::Default;
		} else if (tokenizeString(inBegin, inEnd, outBegin, outEnd, '"')) {
			paletteIndex = PaletteIndex::String;
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

		constexpr const char* const keywords[] = {
			"false", "true", "null"
		};
		for (const char* const k : keywords)
			langDef.Keys.insert(k);

		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[\\[\\]\\{\\}\\-\\+\\:\\,]", PaletteIndex::Punctuation));

		langDef.Tokenize = std::bind(
			&EditorJsonImpl::tokenize, this,
			std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4,
			std::placeholders::_5
		);

		langDef.CaseSensitive = true;

		langDef.Name = "JSON";

		return langDef;
	}
};

EditorJson* EditorJson::create(void) {
	EditorJsonImpl* result = new EditorJsonImpl();

	return result;
}

void EditorJson::destroy(EditorJson* ptr) {
	EditorJsonImpl* impl = static_cast<EditorJsonImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
