/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "bytes.h"
#include "editor_bytes.h"
#include "theme.h"
#include "workspace.h"
#include "../lib/imgui_code_editor/imgui_code_editor.h"

/*
** {===========================================================================
** Bytes editor
*/

class EditorBytesImpl : public EditorBytes, public ImGui::CodeEditor {
private:
	bool _opened = false;

	std::string _name;
	Bytes::Ptr _object = nullptr;

	bool _acquireFocus = false;
	struct {
		std::string text;
		bool filled = false;

		void clear(void) {
			text.clear();
			filled = false;
		}
	} _status;

public:
	EditorBytesImpl() {
		SetLanguageDefinition(LanguageDefinition::Text());
	}
	virtual ~EditorBytesImpl() override {
		close(nullptr);
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual void open(const class Project* /* project */, const char* name, Object::Ptr obj, const char* /* ref */) override {
		if (_opened)
			return;
		_opened = true;

		_name = name;

		_object = Object::as<Bytes::Ptr>(obj);
		if (_object) {
			constexpr const int BYTES_PER_LINE = 16;
			std::string content;
			for (int i = 0; i < (int)_object->count(); ) {
				content += "0x";
				content += Text::toHex(i, 8u, '0', true);
				content += "  ";
				for (int j = 0; i < (int)_object->count() && j < BYTES_PER_LINE; ++i, ++j) {
					content += Text::toHex(_object->get(i), 2, '0', true);
					if (j != BYTES_PER_LINE - 1)
						content += " ";
				}
				content += "\n";
			}

			SetText(content);
		}

		DisableShortcut(All);

		SetTooltipEnabled(false);

		SetReadOnly(true);

		fprintf(stdout, "Bytes editor opened: \"%s\".\n", _name.c_str());
	}
	virtual void close(const class Project* /* project */) override {
		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "Bytes editor closed: \"%s\".\n", _name.c_str());

		_status.clear();

		_object = nullptr;
		_name.clear();
	}

	virtual void flush(void) const override {
		// Do nothing.
	}

	virtual bool readonly(void) const override {
		return true;
	}
	virtual void readonly(bool) override {
		// Do nothing.
	}

	virtual bool hasUnsavedChanges(void) const override {
		return false;
	}
	virtual void markChangesSaved(const class Project*) override {
		// Do nothing.
	}

	virtual void copy(void) override {
		Copy();
	}
	virtual void cut(void) override {
		Cut();
	}
	virtual bool pastable(void) const override {
		return false;
	}
	virtual void paste(void) override {
		// Do nothing.
	}
	virtual void del(void) override {
		// Do nothing.
	}
	virtual bool selectable(void) const override {
		return true;
	}

	virtual const char* redoable(void) const override {
		return nullptr;
	}
	virtual const char* undoable(void) const override {
		return nullptr;
	}

	virtual void redo(class Asset*) override {
		// Do nothing.
	}
	virtual void undo(class Asset*) override {
		// Do nothing.
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
		case SELECT_ALL:
			SelectAll();

			return Variant(true);
		case SELECT_WORD:
			SelectWordUnderCursor();

			return Variant(true);
		case FOCUS:
			_acquireFocus = true;

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
		int scale,
		bool pending,
		double /* delta */
	) override {
		ImGuiStyle &style = ImGui::GetStyle();

		if (!GetImePositionUpdatedHandler()) {
			SetImePositionUpdatedHandler(
				[scale] (const ImVec2 &pos) -> ImVec2 {
					return ImVec2(pos.x * scale, pos.y * scale);
				}
			);
		}

		const float statusBarHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;

		const float posY = ImGui::GetCursorPosY();
		ImGui::Dummy(ImVec2(GetHeadSize(), 0));
		ImGui::SameLine();
		ImGui::TextUnformatted("Address     Data (HEX)");
		const float toolBarHeight = ImGui::GetCursorPosY() - posY;

		if (_acquireFocus) {
			if (!ws->popupBox()) {
				_acquireFocus = false;
				ImGui::SetNextWindowFocus();
			}
		}

		Render(title, ImVec2(width, height - statusBarHeight - toolBarHeight));

		context(wnd, rnd, ws);

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
			if (ImGui::MenuItem(ws->theme()->menuEdit_Copy())) {
				copy();
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
		ImGui::Dummy(ImVec2(8, 0));
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::Text(
			"%s",
			_status.text.c_str()
		);
		if (actived)
			ImGui::PopStyleColor();
	}
};

EditorBytes* EditorBytes::create(void) {
	EditorBytesImpl* result = new EditorBytesImpl();

	return result;
}

void EditorBytes::destroy(EditorBytes* ptr) {
	EditorBytesImpl* impl = static_cast<EditorBytesImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
