/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2023 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "code.h"
#include "editing.h"
#include "editor_code.h"
#include "encoding.h"
#include "platform.h"
#include "project.h"
#include "theme.h"
#include "workspace.h"
#include "../lib/imgui_code_editor/imgui_code_editor.h"
#include <SDL.h>

/*
** {===========================================================================
** Code editor
*/

class EditorCodeImpl : public EditorCode, public ImGui::CodeEditor {
private:
	bool _opened = false;

	std::string _name;
	Code::Ptr _object = nullptr;
	Editing::Data::Checkpoint _checkpoint;
	int _index = -1;

	bool _acquireFocus = false;
	int _breaking = -1;
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

		Editing::Tools::Marker marker;
		int direction = 0;

		void clear(void) {
			initialized = false;
			focused = false;

			jumping = -1;
		}
	} _tools;

	static struct Shared {
		struct Cache {
			typedef std::vector<Cache> Array;

			std::string text;
			bool valid = false;

			Cache() {
			}
			Cache(const std::string &txt, bool valid_) : text(txt), valid(valid_) {
			}
		};

		bool finding = false;
		Editing::Tools::Marker marker;
		mutable std::string* wordPtr = nullptr;
		mutable Cache::Array* cacheStr = nullptr;

		Shared() {
		}
		~Shared() {
			clear();
		}

		const std::string &word(void) const {
			if (wordPtr == nullptr)
				wordPtr = new std::string();

			return *wordPtr;
		}
		std::string &word(void) {
			if (wordPtr == nullptr)
				wordPtr = new std::string();

			return *wordPtr;
		}

		const Cache::Array &cache(void) const {
			if (cacheStr == nullptr)
				cacheStr = new Cache::Array();

			return *cacheStr;
		}
		Cache::Array &cache(void) {
			if (cacheStr == nullptr)
				cacheStr = new Cache::Array();

			return *cacheStr;
		}

		void clear(void) {
			finding = false;
			marker.clear();
			if (wordPtr) {
				delete wordPtr;
				wordPtr = nullptr;
			}
			if (cacheStr) {
				delete cacheStr;
				cacheStr = nullptr;
			}
		}
	} _shared;

public:
	EditorCodeImpl() {
		_checkpoint.fill();

		SetLanguageDefinition(languageDefinition());

		for (int i = 0; i < BITTY_COUNTOF(EDITOR_CODE_KEYWORDS); ++i)
			addKeyword(EDITOR_CODE_KEYWORDS[i]);
		for (int i = 0; i < BITTY_COUNTOF(EDITOR_CODE_MODULES); ++i)
			addIdentifier(EDITOR_CODE_MODULES[i]);
		for (int i = 0; i < BITTY_COUNTOF(EDITOR_CODE_PRIMITIVES); ++i)
			addIdentifier(EDITOR_CODE_PRIMITIVES[i]);
	}
	virtual ~EditorCodeImpl() override {
		close(nullptr);
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	void initialize(int refCount_) {
		(void)refCount_;

		// Do nothing.
	}
	void dispose(int refCount_) {
		if (refCount_ == 0)
			_shared.clear();
	}

	virtual void open(const class Project* project, const char* name, Object::Ptr obj, const char* /* ref */) override {
		if (_opened)
			return;
		_opened = true;

		_name = name;

		_object = Object::as<Code::Ptr>(obj);
		if (_object) {
			size_t len = 0;
			const char* txt = _object->text(&len);
			if (txt && len)
				text(txt, len);
		}
		Editing::Data::toCheckpoint(project, _name.c_str(), _checkpoint);

		SetHeadClickEnabled(true);

		DisableShortcut(All);

		SetTooltipEnabled(false);

		SetModifiedHandler(std::bind(&EditorCodeImpl::modified, this));

#if BITTY_DEBUG_ENABLED
		SetHeadClickedHandler(std::bind(&EditorCodeImpl::headClicked, this, std::placeholders::_1));
#endif /* BITTY_DEBUG_ENABLED */

		fprintf(stdout, "Code editor opened: \"%s\".\n", _name.c_str());
	}
	virtual void close(const class Project* project) override {
		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "Code editor closed: \"%s\".\n", _name.c_str());

		if (!_checkpoint.empty()) {
			if (hasUnsavedChanges())
				Editing::Data::fromCheckpoint(project, _name.c_str(), _checkpoint);
			_checkpoint.clear();
		}

		SetModifiedHandler(nullptr);

		SetHeadClickedHandler(nullptr);

		_cache.clear();
		_status.clear();
		_breaking = -1;

		_object = nullptr;
		_name.clear();
	}

	virtual void flush(void) const override {
		if (!_object)
			return;

		size_t len = 0;
		const char* txt = text(&len);
		if (txt && len)
			_object->text(txt, len);
		else
			_object->text("", 0);
	}

	virtual void addKeyword(const char* str) override {
		LanguageDefinition &def = GetLanguageDefinition();
		def.Keys.insert(str);
	}
	virtual void addIdentifier(const char* str) override {
		LanguageDefinition &def = GetLanguageDefinition();
		Identifier id;
		id.Declaration = "Bitty function";
		def.Ids.insert(std::make_pair(std::string(str), id));
	}
	virtual void addPreprocessor(const char* str) override {
		LanguageDefinition &def = GetLanguageDefinition();
		Identifier id;
		id.Declaration = "Bitty preprocessor";
		def.PreprocIds.insert(std::make_pair(std::string(str), id));
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
		case TOGGLE_COMMENT:
			if (_tools.focused)
				return Variant(false);

			if (HasSelection()) {
				if (GetCommentLines() == GetSelectionLines())
					Uncomment();
				else
					Comment();
			} else {
				if (GetCommentLines() > 0)
					Uncomment();
				else
					Comment();
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

				_shared.finding = true;

				Coordinates begin, end;
				GetSelection(begin, end);
				if (begin == end)
					_shared.word() = GetWordUnderCursor(&begin, &end);
				else
					_shared.word() = GetSelectionText();
				SetSelection(begin, end);

				_tools.direction = 0;
			}

			return Variant(true);
		case FIND_NEXT:
			_tools.jumping = -1;

			if (_shared.word().empty()) {
				_shared.finding = true;

				_shared.word() = GetWordUnderCursor();
			}

			_tools.direction = 1;

			return Variant(true);
		case FIND_PREVIOUS:
			_tools.jumping = -1;

			if (_shared.word().empty()) {
				_shared.finding = true;

				_shared.word() = GetWordUnderCursor();
			}

			_tools.direction = -1;

			return Variant(true);
		case GOTO: {
				_tools.initialized = false;

				_shared.finding = false;

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
		case GET_PROGRAM_POINTER:
			return Variant((Variant::Int)GetProgramPointer());
		case SET_PROGRAM_POINTER: {
				const Variant::Int ln = unpack<Variant::Int>(argc, argv, 0, -1);
				if (ln < 0 || ln >= GetTotalLines()) {
					SetProgramPointer(-1);

					break;
				}

				SetProgramPointer(ln);
			}

			return Variant(true);
		case GET_BREAKPOINT: {
				const Variant::Int ln = unpack<Variant::Int>(argc, argv, 0, -1);
				if (ln < 0 || ln >= GetTotalLines())
					break;

				Breakpoints::iterator it = Brks.find(ln);
				if (it == Brks.end())
					break;
			}

			return Variant(true);
		case SET_BREAKPOINT: {
				const Variant::Int ln = unpack<Variant::Int>(argc, argv, 0, -1);
				const bool brk = unpack<bool>(argc, argv, 1, false);
				const bool enabled = unpack<bool>(argc, argv, 2, true);
				if (ln < 0 || ln >= GetTotalLines())
					break;

				Breakpoints::iterator it = Brks.find(ln);
				if (brk) {
					if (it != Brks.end())
						Brks.erase(it);

					Brks.insert(std::make_pair(ln, enabled));
				} else {
					if (it == Brks.end())
						break;

					Brks.erase(it);
				}
			}

			return Variant(true);
		case GET_BREAKPOINTS: {
				IList::Ptr lst(List::create());
				for (Breakpoints::value_type brk : Brks)
					lst->add((Variant::Int)brk.first);

				return Variant(lst);
			}
		case CLEAR_BREAKPOINTS:
			Brks.clear();

			return Variant(true);
		default: // Do nothing.
			break;
		}

		return Variant(false);
	}
	using Dispatchable::post;

	virtual void update(
		class Window* wnd, class Renderer* rnd,
		class Workspace* ws, const class Project* project, class Executable* exec,
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
		if (_shared.finding || _tools.direction != 0) {
			Coordinates srcBegin, srcEnd;
			GetSelection(srcBegin, srcEnd);

			Shared::Cache::Array &strings = _shared.cache();
			Editing::Tools::TextPages cache;
			do {
				LockGuard<RecursiveMutex>::UniquePtr acquired;
				Project* prj = project->acquire(acquired);
				if (!prj)
					break;

				Asset* asset = prj->get(Asset::List::Index(_index, false));
				if (!asset)
					break;

				if (asset->type() != Code::TYPE())
					break;

				const bool readyForEditing = asset->readyFor(Asset::EDITING);
				if (readyForEditing) {
					EditorCodeImpl* editor = (EditorCodeImpl*)asset->editor();
					if (!editor)
						break;

					size_t len = 0;
					const char* txt = editor->text(&len);
					std::string str;
					str.assign(txt, len);
					if (_index < (int)strings.size())
						strings[_index] = Shared::Cache(txt, true);
				} else {
					asset->prepare(Asset::EDITING, true);
					Object::Ptr obj = asset->object(Asset::EDITING);
					asset->finish(Asset::EDITING, true);

					if (!obj)
						break;
					Code::Ptr code = Object::as<Code::Ptr>(obj);
					if (!code)
						break;

					size_t len = 0;
					const char* txt = code->text(&len);
					std::string str;
					str.assign(txt, len);
					if (_index < (int)strings.size())
						strings[_index] = Shared::Cache(txt, true);
				}
			} while (false);
			for (Shared::Cache &str : strings) {
				if (str.valid)
					cache.push_back(&str.text);
				else
					cache.push_back(nullptr);
			}
			_tools.marker = Editing::Tools::Marker(
				Editing::Tools::Marker::Coordinates(_index, srcBegin.Line, srcBegin.Column),
				Editing::Tools::Marker::Coordinates(_index, srcEnd.Line, srcEnd.Column)
			);

			const float y = ImGui::GetCursorPosY();
			const bool stepped = Editing::Tools::find(
				rnd, ws,
				&_tools.marker,
				width,
				&_tools.initialized, &_tools.focused,
				&cache, &_shared.word(),
				Editing::Tools::Marker::Coordinates(_index, GetTotalLines(), GetColumnsAt(GetTotalLines())),
				&_tools.direction,
				&ws->settings()->editorCaseSensitive, &ws->settings()->editorMatchWholeWord, &ws->settings()->editorGlobalSearch,
				_shared.finding,
				[&] (const Editing::Tools::Marker::Coordinates &pos, Editing::Tools::Marker &src) -> std::string {
					Coordinates srcBegin, srcEnd;
					const std::string result = GetWordAt(Coordinates(pos.line, pos.column), &srcBegin, &srcEnd);
					src.begin = Editing::Tools::Marker::Coordinates(_index, srcBegin.Line, srcBegin.Column);
					src.end = Editing::Tools::Marker::Coordinates(_index, srcEnd.Line, srcEnd.Column);

					return result;
				}
			);
			if (stepped && !_tools.marker.empty()) {
				if (_tools.marker.begin.index == _index) {
					const Coordinates begin(_tools.marker.begin.line, _tools.marker.begin.column);
					const Coordinates end(_tools.marker.end.line, _tools.marker.end.column);

					SetCursorPosition(begin);
					SetSelection(begin, end);
				} else {
					const int index_ = _tools.marker.begin.index;
					do {
						LockGuard<RecursiveMutex>::UniquePtr acquired;
						Project* prj = project->acquire(acquired);
						if (!prj)
							break;

						Asset* asset = prj->get(Asset::List::Index(index_, false));
						if (!asset)
							break;

						asset->prepare(Asset::EDITING, false);

						Asset::States* states = asset->states();
						states->activate(Asset::States::INSPECTABLE);
						states->focus();

						EditorCodeImpl* editor = (EditorCodeImpl*)asset->editor();
						if (!editor)
							break;

						const Coordinates begin(_tools.marker.begin.line, _tools.marker.begin.column);
						const Coordinates end(_tools.marker.end.line, _tools.marker.end.column);

						editor->SetCursorPosition(begin);
						editor->SetSelection(begin, end);
					} while (false);
				}
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

		if (_breaking >= 0 && _breaking < GetTotalLines()) {
			ws->post(
				ON_TOGGLE_BREAKPOINT,
				_name, (Variant::Int)_breaking,
				(void*)project, (void*)exec
			);

			_breaking = -1;
		}

		renderStatus(wnd, rnd, ws, width, statusBarHeight, pending);
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
		_index = -1;
		Shared::Cache::Array &strings = _shared.cache();
		strings.clear();
	}
	virtual void gainFocus(class Renderer* /* rnd */, const class Project* project) override {
		Shared::Cache::Array &strings = _shared.cache();
		do {
			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj)
				break;

			for (int i = 0; i < prj->count(); ++i) {
				Asset* asset = prj->get(Asset::List::Index(i, false));
				if (!asset)
					break;

				if (asset->type() != Code::TYPE()) {
					strings.push_back(Shared::Cache("", false));

					continue;
				}

				if (_name == asset->entry().name())
					_index = i;

				const bool readyForEditing = asset->readyFor(Asset::EDITING);
				if (readyForEditing) {
					EditorCodeImpl* editor = (EditorCodeImpl*)asset->editor();
					if (!editor)
						break;

					size_t len = 0;
					const char* txt = editor->text(&len);
					std::string str;
					str.assign(txt, len);
					strings.push_back(Shared::Cache(txt, true));
				} else {
					asset->prepare(Asset::EDITING, true);
					Object::Ptr obj = asset->object(Asset::EDITING);
					asset->finish(Asset::EDITING, true);

					if (!obj)
						break;
					Code::Ptr code = Object::as<Code::Ptr>(obj);
					if (!code)
						break;

					size_t len = 0;
					const char* txt = code->text(&len);
					std::string str;
					str.assign(txt, len);
					strings.push_back(Shared::Cache(txt, true));
				}
			}
		} while (false);
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
			ImGui::Separator();
			if (ImGui::MenuItem(ws->theme()->menuEdit_IncreaseIndent())) {
				post(Editable::INDENT, false);
			}
			if (ImGui::MenuItem(ws->theme()->menuEdit_DecreaseIndent())) {
				post(Editable::UNINDENT, false);
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
	void headClicked(int ln) {
		_breaking = ln;
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
		} else if (tokenizeString(inBegin, inEnd, outBegin, outEnd, '\'')) {
			paletteIndex = PaletteIndex::String;
		}

		return paletteIndex != PaletteIndex::Max;
	}
	LanguageDefinition languageDefinition(void) const {
		LanguageDefinition langDef;

		constexpr const char* const keywords[] = {
			"and", "break", "do", "else", "elseif", "end",
			"false", "for", "function", "goto", "if", "in",
			"local", "nil", "not", "or", "repeat", "return",
			"then", "true", "until", "while"
		};
		for (const char* const k : keywords)
			langDef.Keys.insert(k);

		constexpr const char* const identifiers[] = {
			"__add", "__sub", "__mul", "__div",
			"__mod", "__pow", "__unm", "__idiv",
			"__band", "__bor", "__bxor", "__bnot",
			"__shl", "__shr",
			"__concat", "__len",
			"__eq", "__lt", "__le",
			"__index", "__newindex", "__call",
			"__gc", "__close", "__mode", "__name", "__tostring",

			"char", "len", "pack", "type", "unpack",

			"_G", "assert", "collectgarbage", "dofile", "error", "getmetatable", "ipairs", "load", "loadfile", "next", "pairs", "pcall", "print", "rawequal", "rawget", "rawlen", "rawset", "select", "setmetatable", "tonumber", "tostring", "xpcall",
			"coroutine", "create", "isyieldable", "resume", "running", "status", "wrap", "yield",
			"require", "package", "config", "cpath", "loaded", "loadlib", "path", "preload", "searchers", "searchpath",
			"string", "byte", "dump", "find", "format", "gmatch", "gsub", "lower", "match", "packsize", "rep", "reverse", "sub", "upper",
			"utf8", "charpattern", "codes", "codepoint", "offset",
			"table", "concat", "insert", "move", "remove", "sort",
			"math", "abs", "acos", "asin", "atan", "ceil", "cos", "deg", "exp", "floor", "fmod", "huge", "log", "max", "maxinteger", "min", "mininteger", "modf", "pi", "rad", "random", "randomseed", "sin", "sqrt", "tan", "tointeger", "ult",
			// "io", "close", "flush", "input", "lines", "open", "output", "popen", "read", "tmpfile", "write",
			// "file", "close", "flush", "lines", "read", "seek", "setvbuf", "write",
			// "os", "clock", "date", "difftime", "execute", "exit", "getenv", "remove", "rename", "setlocale", "time", "tmpname",
			"debug", "gethook", "getinfo", "getlocal", "getregistry", "getupvalue", "getuservalue", "sethook", "setlocal", "setupvalue", "setuservalue", "traceback", "upvalueid", "upvaluejoin",
			"const", "self"
		};
		for (const char* const k : identifiers) {
			Identifier id;
			id.Declaration = "Built-in function";
			auto it = langDef.Ids.insert(std::make_pair(std::string(k), id));
			if (!it.second) {
				fprintf(stderr, "Duplicated identifier: \"%s\"\n", k);

				assert(false && "Duplicated identifier.");
			}
		}

		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("\\-\\-.*", PaletteIndex::Comment));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[\\[\\]\\{\\}\\!\\#\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\:\\<\\>\\?\\/\\;\\,\\.]", PaletteIndex::Punctuation));

		langDef.Tokenize = std::bind(
			&EditorCodeImpl::tokenize, this,
			std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4,
			std::placeholders::_5
		);

		langDef.CommentStart = "--[[";
		langDef.CommentEnd = "]]";
		langDef.SimpleCommentHead = "--";

		langDef.CaseSensitive = true;

		langDef.Name = "Lua";

		return langDef;
	}
};

EditorCodeImpl::Shared EditorCodeImpl::_shared;

int EditorCode::refCount = 0;

EditorCode* EditorCode::create(void) {
	EditorCodeImpl* result = new EditorCodeImpl();
	result->initialize(refCount++);

	return result;
}

void EditorCode::destroy(EditorCode* ptr) {
	EditorCodeImpl* impl = static_cast<EditorCodeImpl*>(ptr);
	impl->dispose(--refCount);
	delete impl;
}

/* ===========================================================================} */
