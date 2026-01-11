/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "platform.h"
#include "text_box.h"
#include "theme.h"
#include "workspace.h"
#include "../lib/imgui_code_editor/imgui_code_editor.h"

/*
** {===========================================================================
** Text box
*/

class TextBoxImpl : public TextBox, public ImGui::CodeEditor {
private:
	bool _opened = false;

	std::string _id;

	bool _acquireFocus = false;
	mutable struct {
		std::string text;
		bool overdue = true;

		void clear(void) {
			text.clear();
			overdue = true;
		}
	} _cache;

public:
	TextBoxImpl() {
	}
	virtual ~TextBoxImpl() override {
		close(nullptr);
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Object** ptr) const override { // Non-clonable.
		if (ptr)
			*ptr = nullptr;

		return false;
	}

	virtual void open(const class Project* /* project */, const char* name, Object::Ptr /* obj */, const char* /* ref */) override {
		if (_opened)
			return;
		_opened = true;

		_id = name;

		DisableShortcut((ShortcutType)(UndoRedo | CopyCutPasteDelete | IndentUnindent));

		SetTooltipEnabled(false);

		SetModifiedHandler(std::bind(&TextBoxImpl::modified, this));

		fprintf(stdout, "Text box opened: \"%s\".\n", _id.c_str());
	}
	virtual void close(const class Project* /* project */) override {
		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "Text box closed: \"%s\".\n", _id.c_str());

		SetModifiedHandler(nullptr);

		_cache.clear();

		_id.clear();
	}

	virtual void flush(void) const override {
		// Do nothing.
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
	}

	virtual bool hasUnsavedChanges(void) const override {
		return !IsChangesSaved();
	}
	virtual void markChangesSaved(const class Project* /* project */) override {
		SetChangesSaved();
	}

	virtual void copy(void) override {
		Copy();
	}
	virtual void cut(void) override {
		if (ReadOnly) {
			copy();

			return;
		}

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

		Paste();
	}
	virtual void del(void) override {
		if (ReadOnly)
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
			SelectAll();

			return Variant(true);
		case SELECT_WORD:
			SelectWordUnderCursor();

			return Variant(true);
		case INDENT: {
				const bool byKey = unpack<bool>(argc, argv, 0, true);

				Indent(byKey);
			}

			return Variant(true);
		case UNINDENT: {
				const bool byKey = unpack<bool>(argc, argv, 0, true);

				Unindent(byKey);
			}

			return Variant(true);
		case TOGGLE_COMMENT:
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
			MoveLineUp();

			return Variant(true);
		case MOVE_DOWN:
			MoveLineDown();

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
		class Workspace* ws, const class Project* /* project */, class Executable* /* exec */,
		const char* title,
		float /* x */, float /* y */, float width, float height,
		float /* scaleX */, float /* scaleY */,
		bool /* pending */,
		double /* delta */
	) override {
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
		Render(title, ImVec2(width, height));
		if (fontCode && fontCode->IsLoaded()) {
			SetFont(nullptr);
			ImGui::PopFont();
		}

		context(wnd, rnd, ws);
	}
	virtual void update(
		class Window* wnd, class Renderer* rnd,
		class Workspace* ws,
		float x, float y, float width, float height
	) override {
		update(
			wnd, rnd,
			ws, nullptr, nullptr,
			_id.c_str(),
			x, y, width, height,
			1.0f, 1.0f,
			false,
			0.0
		);
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
			if (ImGui::MenuItem(ws->theme()->menuEdit_Cut(), nullptr, nullptr, HasSelection())) {
				cut();
			}
			if (ImGui::MenuItem(ws->theme()->menuEdit_Copy(), nullptr, nullptr, HasSelection())) {
				copy();
			}
			if (ImGui::MenuItem(ws->theme()->menuEdit_Paste(), nullptr, nullptr, pastable())) {
				paste();
			}
			if (ImGui::MenuItem(ws->theme()->menuEdit_Delete(), nullptr, nullptr, HasSelection())) {
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

	void modified(void) {
		_cache.overdue = true;
	}
};

TextBox* TextBox::create(void) {
	TextBoxImpl* result = new TextBoxImpl();

	return result;
}

void TextBox::destroy(TextBox* ptr) {
	TextBoxImpl* impl = static_cast<TextBoxImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
