/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "commands_palette.h"
#include "editing.h"
#include "editor_palette.h"
#include "encoding.h"
#include "image.h"
#include "platform.h"
#include "theme.h"
#include "workspace.h"
#include "../lib/jpath/jpath.hpp"
#include <SDL.h>

/*
** {===========================================================================
** Palette editor
*/

class EditorPaletteImpl : public EditorPalette {
private:
	bool _opened = false;

	std::string _name;
	Palette::Ptr _object = nullptr;
	CommandQueue* _commands = nullptr;
	Editing::Data::Checkpoint _checkpoint;

	int _cursor = 0;

	struct Ref : public Editor::Ref {
		Color color = Color(255, 255, 255, 255);
		float real[4] = { 1, 1, 1, 1 };

		void fill(const Color &col) {
			color = col;

			real[0] = color.r / 255.0f;
			real[1] = color.g / 255.0f;
			real[2] = color.b / 255.0f;
			real[3] = color.a / 255.0f;
		}
		void clear(void) {
			color = Color(255, 255, 255, 255);

			real[0] = 1;
			real[1] = 1;
			real[2] = 1;
			real[3] = 1;
		}
	} _ref;

public:
	EditorPaletteImpl() {
		_commands = new CommandQueue();
		_commands->registerFactory<Commands::Palette::Change>();
		_commands->registerFactory<Commands::Palette::Cut>();
		_commands->registerFactory<Commands::Palette::Paste>();
		_commands->registerFactory<Commands::Palette::Delete>();

		_checkpoint.fill();
	}
	virtual ~EditorPaletteImpl() override {
		delete _commands;
		_commands = nullptr;

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

		_object = Object::as<Palette::Ptr>(obj);
		Editing::Data::toCheckpoint(project, _name.c_str(), _checkpoint);

		if (_object) {
			Color col;
			_object->get(0, col);
			_ref.fill(col);
		}

		fprintf(stdout, "Palette editor opened: \"%s\".\n", _name.c_str());
	}
	virtual void close(const class Project* project) override {
		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "Palette editor closed: \"%s\".\n", _name.c_str());

		if (!_checkpoint.empty()) {
			if (hasUnsavedChanges())
				Editing::Data::fromCheckpoint(project, _name.c_str(), _checkpoint);
			_checkpoint.clear();
		}

		_ref.clear();

		_object = nullptr;
		_name.clear();
	}

	virtual void flush(void) const override {
		// Do nothing.
	}

	virtual bool readonly(void) const override {
		return false;
	}
	virtual void readonly(bool) override {
		// Do nothing.
	}

	virtual bool hasUnsavedChanges(void) const override {
		return _commands->hasUnsavedChanges();
	}
	virtual void markChangesSaved(const class Project* project) override {
		Editing::Data::toCheckpoint(project, _name.c_str(), _checkpoint);

		_commands->markChangesSaved();
	}

	virtual void copy(void) override {
		Color col;
		copy(col);
	}
	bool copy(Color &col) {
		if (_cursor < 0 || _cursor >= _object->count())
			return false;

		_object->get(_cursor, col);

		rapidjson::Document doc;
		Jpath::set(doc, doc, col.r, 0);
		Jpath::set(doc, doc, col.g, 1);
		Jpath::set(doc, doc, col.b, 2);
		Jpath::set(doc, doc, col.a, 3);

		std::string buf;
		Json::toString(doc, buf);
		const std::string osstr = Unicode::toOs(buf);

		Platform::clipboardText(osstr.c_str());

		return true;
	}
	virtual void cut(void) override {
		Color old;
		if (!copy(old))
			return;

		Color val;
		if (!del(val))
			return;

		_commands->enqueue<Commands::Palette::Cut>()
			->with(_cursor, old, val)
			->exec(_object);
	}
	virtual bool pastable(void) const override {
		return Platform::hasClipboardText();
	}
	virtual void paste(void) override {
		if (_cursor < 0 || _cursor >= _object->count())
			return;

		Color old;
		_object->get(_cursor, old);

		Color val;
		if (!paste(val))
			return;

		_object->set(_cursor, &val);

		_commands->enqueue<Commands::Palette::Paste>()
			->with(_cursor, old, val)
			->exec(_object);
	}
	bool paste(Color &col) {
		const std::string osstr = Platform::clipboardText();
		const std::string buf = Unicode::fromOs(osstr);

		rapidjson::Document doc;
		if (!Json::fromString(doc, buf.c_str()))
			return false;

		if (!Jpath::get(doc, col.r, 0))
			return false;
		if (!Jpath::get(doc, col.g, 1))
			return false;
		if (!Jpath::get(doc, col.b, 2))
			return false;
		if (!Jpath::get(doc, col.a, 3))
			return false;

		return true;
	}
	virtual void del(void) override {
		Color old;
		_object->get(_cursor, old);

		Color val;
		del(val);

		_commands->enqueue<Commands::Palette::Delete>()
			->with(_cursor, old, val)
			->exec(_object);
	}
	bool del(Color &col) {
		if (_cursor < 0 || _cursor >= _object->count())
			return false;

		col = Color(0, 0, 0, 0);
		_object->set(_cursor, &col);

		return true;
	}
	virtual bool selectable(void) const override {
		return false;
	}

	virtual const char* redoable(void) const override {
		const Command* cmd = _commands->redoable();
		if (!cmd)
			return nullptr;

		return cmd->toString();
	}
	virtual const char* undoable(void) const override {
		const Command* cmd = _commands->undoable();
		if (!cmd)
			return nullptr;

		return cmd->toString();
	}

	virtual void redo(class Asset*) override {
		_commands->redo(_object);
	}
	virtual void undo(class Asset*) override {
		_commands->undo(_object);
	}

	virtual Variant post(unsigned, int, const Variant*) override {
		return Variant(false);
	}

	virtual void update(
		class Window* wnd, class Renderer* rnd,
		class Workspace* ws, const class Project* /* project */, class Executable* /* exec */,
		const char* /* title */,
		float /* x */, float /* y */, float width, float height,
		float /* scaleX */, float /* scaleY */,
		bool pending,
		double /* delta */
	) override {
		ImGuiStyle &style = ImGui::GetStyle();

		shortcuts(wnd, rnd, ws);

		if (!_object)
			return;

		const Splitter splitter = split();

		const float statusBarHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;
		bool statusBarActived = ImGui::IsWindowFocused();

		ImGui::BeginChild("@Pal/Ed", ImVec2(splitter.first, height - statusBarHeight), false, ImGuiWindowFlags_NoNav);
		{
			_object->validate();

			const bool painting = Editing::palette(
				rnd, ws,
				_object.get(),
				splitter.first - style.ScrollbarSize,
				&_ref.color, &_cursor, true,
				ws->theme()->iconTransparent()
			);
			if (painting && ImGui::IsWindowFocused())
				_ref.fill(_ref.color);

			statusBarActived |= ImGui::IsWindowFocused();

			context(wnd, rnd, ws);
		}
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("@Pal/Tls", ImVec2(splitter.second, height - statusBarHeight), true, _ref.windowFlags());
		{
			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

			Color color;
			_object->get(_cursor, color);

			const float spwidth = _ref.windowWidth(splitter.second);
			ImGui::SetNextItemWidth(spwidth);
			ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoSidePreview;
			if (spwidth < ImGui::ColorPickerMinWidthForInput()) {
				flags |= ImGuiColorEditFlags_NoInputs;
				ImGui::ColorPicker4("", _ref.real, flags);

				ImGui::PushID("@Img/Tls/Alf");
				ImGui::SetNextItemWidth(spwidth);
				int alpha = (int)(_ref.real[3] * 255);
				if (ImGui::DragInt("", &alpha, 1, 0, 255, "A: %d"))
					_ref.real[3] = alpha / 255.0f;
				ImGui::PopID();
			} else {
				ImGui::ColorPicker4("", _ref.real, flags);
			}

			const Color current(
				(Byte)(_ref.real[0] * 255.0f),
				(Byte)(_ref.real[1] * 255.0f),
				(Byte)(_ref.real[2] * 255.0f),
				(Byte)(_ref.real[3] * 255.0f)
			);
			if (color != current) {
				ImGui::NewLine();
				ImGui::CentralizeButton(2);
				if (ImGui::Button(ws->theme()->generic_Apply(), ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || (ImGui::IsWindowFocused() && ImGui::IsKeyReleased(SDL_SCANCODE_RETURN))) {
					_commands->enqueue<Commands::Palette::Change>()
						->with(_cursor, color, current)
						->exec(_object);

					_ref.fill(current);
				}
				ImGui::SameLine();
				if (ImGui::Button(ws->theme()->generic_Revert(), ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || (ImGui::IsWindowFocused() && ImGui::IsKeyReleased(SDL_SCANCODE_ESCAPE))) {
					_ref.fill(_ref.color);
				}
			}

			statusBarActived |= ImGui::IsWindowFocused();
		}
		ImGui::EndChild();

		renderStatus(wnd, rnd, ws, width, statusBarHeight, pending, statusBarActived);
	}

	virtual void played(class Renderer* /* rnd */, const class Project* /* project */) override {
		// Do nothing.
	}
	virtual void stopped(class Renderer* /* rnd */, const class Project* /* project */) override {
		// Do nothing.
	}

	virtual void resized(class Renderer* /* rnd */, const class Project* /* project */) override {
		_ref.windowResized();
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

		const Editing::Shortcut home(SDL_SCANCODE_HOME);
		const Editing::Shortcut end(SDL_SCANCODE_END);
		const Editing::Shortcut left(SDL_SCANCODE_LEFT);
		const Editing::Shortcut right(SDL_SCANCODE_RIGHT);
		const Editing::Shortcut up(SDL_SCANCODE_UP);
		const Editing::Shortcut down(SDL_SCANCODE_DOWN);
		const int X_COUNT = (int)std::sqrt(IMAGE_PALETTE_COLOR_COUNT);
		if (home.pressed()) {
			if (_cursor != 0) {
				_cursor = 0;

				_object->get(_cursor, _ref.color);
				_ref.fill(_ref.color);
			}
		} else if (end.pressed()) {
			if (_cursor != _object->count() - 1) {
				_cursor = _object->count() - 1;

				_object->get(_cursor, _ref.color);
				_ref.fill(_ref.color);
			}
		} else if (left.pressed()) {
			if (--_cursor < 0)
				_cursor = _object->count() - 1;

			_object->get(_cursor, _ref.color);
			_ref.fill(_ref.color);
		} else if (right.pressed()) {
			if (++_cursor >= _object->count())
				_cursor = 0;

			_object->get(_cursor, _ref.color);
			_ref.fill(_ref.color);
		} else if (up.pressed()) {
			_cursor -= X_COUNT;
			if (_cursor < 0)
				_cursor = _cursor + _object->count();

			_object->get(_cursor, _ref.color);
			_ref.fill(_ref.color);
		} else if (down.pressed()) {
			_cursor += X_COUNT;
			if (_cursor >= _object->count())
				_cursor = _cursor - _object->count();

			_object->get(_cursor, _ref.color);
			_ref.fill(_ref.color);
		}
	}

	void context(Window* /* wnd */, Renderer* /* rnd */, Workspace* ws) {
		ImGuiStyle &style = ImGui::GetStyle();

		if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
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

			ImGui::EndPopup();
		}
	}

	void renderStatus(Window* /* wnd */, Renderer* /* rnd */, Workspace* ws, float width, float height, bool pending, bool actived) const {
		ImGuiStyle &style = ImGui::GetStyle();

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
			"%s %d",
			ws->theme()->statusItem_Index().c_str(),
			_cursor
		);
		if (actived)
			ImGui::PopStyleColor();
	}
};

EditorPalette* EditorPalette::create(void) {
	EditorPaletteImpl* result = new EditorPaletteImpl();

	return result;
}

void EditorPalette::destroy(EditorPalette* ptr) {
	EditorPaletteImpl* impl = static_cast<EditorPaletteImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
