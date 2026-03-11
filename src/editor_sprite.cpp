/*
** Bitty
**
** An itty bitty 2D game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "commands_sprite.h"
#include "editing.h"
#include "editor_sprite.h"
#include "encoding.h"
#include "image.h"
#include "operations.h"
#include "platform.h"
#include "project.h"
#include "sprite.h"
#include "theme.h"
#include "../lib/jpath/jpath.hpp"
#include <SDL.h>

/*
** {===========================================================================
** Sprite editor
*/

class EditorSpriteImpl : public EditorSprite {
private:
	bool _opened = false;

	std::string _name;
	Sprite::Ptr _object = nullptr;
	Text::Array _animations;
	CommandQueue* _commands = nullptr;
	Editing::Data::Checkpoint _checkpoint;

	Editing::Frame _cursor;
	struct {
		std::string text = "Sprite";
		Editing::Frame cursor;

		void clear(void) {
			text = "Sprite";
			cursor = Editing::Frame();
		}
	} _status;
	std::string _focus;
	Sprite::Ptr _player = nullptr;

	struct Ref : public Editor::Ref {
		std::string name;
		Image::Ptr image = nullptr;
		Texture::Ptr texture = nullptr;
		Editing::Brush cursor;

		bool gridsVisible = false;
		Math::Vec2i gridUnit = Math::Vec2i(0, 0);
		bool transparentBackbroundVisible = true;

		bool fill(Renderer* rnd, const Project* project) {
			if (image && texture)
				return true;

			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj)
				return false;

			Asset* asset = prj->get(name.c_str());
			if (!asset)
				return false;

			asset->prepare(Asset::EDITING, true);
			Object::Ptr obj = asset->object(Asset::EDITING);
			asset->finish(Asset::EDITING, true);

			image = Object::as<Image::Ptr>(obj);
			if (!image)
				return false;

			texture = Texture::Ptr(Texture::create());
			texture->fromImage(rnd, Texture::STREAMING, image.get(), Texture::NEAREST);
			texture->blend(Texture::BLEND);

			return true;
		}
		void clear(const Project*) {
			name.clear();
			image = nullptr;
			texture = nullptr;
			cursor = Editing::Brush();

			gridsVisible = false;
			gridUnit = Math::Vec2i(0, 0);
			transparentBackbroundVisible = true;
		}

		bool getArea(int frameWidth, int frameHeight, Math::Recti &area) const {
			area = Math::Recti(
				cursor.position.x, cursor.position.y,
				cursor.position.x + frameWidth - 1, cursor.position.y + frameHeight - 1
			);

			return area.xMin() >= 0 && area.xMax() < image->width() &&
				area.yMin() >= 0 && area.yMax() < image->height();
		}
	} _ref;
	struct Tools {
		bool focused = false;

		int magnification = 0;
		int viewIndex = 0;

		double interval = SPRITE_DEFAULT_INTERVAL;

		void clear(void) {
			focused = false;

			magnification = 0;
			viewIndex = 0;

			interval = SPRITE_DEFAULT_INTERVAL;
		}
	} _tools;

public:
	EditorSpriteImpl() {
		_commands = new CommandQueue();
		_commands->registerFactory<Commands::Sprite::AddAnimation>();
		_commands->registerFactory<Commands::Sprite::CutAnimation>();
		_commands->registerFactory<Commands::Sprite::PasteAnimation>();
		_commands->registerFactory<Commands::Sprite::DeleteAnimation>();
		_commands->registerFactory<Commands::Sprite::RenameAnimation>();
		_commands->registerFactory<Commands::Sprite::AddFrame>();
		_commands->registerFactory<Commands::Sprite::CutFrame>();
		_commands->registerFactory<Commands::Sprite::PasteFrame>();
		_commands->registerFactory<Commands::Sprite::DeleteFrame>();
		_commands->registerFactory<Commands::Sprite::ChangeArea>();
		_commands->registerFactory<Commands::Sprite::ChangeInterval>();

		_checkpoint.fill();
	}
	virtual ~EditorSpriteImpl() override {
		delete _commands;
		_commands = nullptr;

		close(nullptr);
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual void open(const class Project* project, const char* name, Object::Ptr obj, const char* ref) override {
		if (_opened)
			return;
		_opened = true;

		_name = name;

		_object = Object::as<Sprite::Ptr>(obj);
		Editing::Data::toCheckpoint(project, _name.c_str(), _checkpoint);

		getAnimations();

		_ref.name = ref ? ref : "";
		Math::Vec2i grid;
		if (_object)
			grid = Math::Vec2i(_object->width(), _object->height());
		if (grid.x == 0)
			grid.x = BITTY_SPRITE_DEFAULT_WIDTH;
		if (grid.y == 0)
			grid.y = BITTY_SPRITE_DEFAULT_HEIGHT;
		_ref.cursor.position = Math::Vec2i(0, 0);
		_ref.cursor.size = grid;

		const Editing::Shortcut caps(SDL_SCANCODE_UNKNOWN, false, false, false, false, true);
		const Editing::Shortcut num(SDL_SCANCODE_UNKNOWN, false, false, false, true, false);
		_ref.gridsVisible = caps.pressed();
		_ref.gridUnit = _ref.cursor.size;
		_ref.transparentBackbroundVisible = num.pressed();

		fprintf(stdout, "Sprite editor opened: \"%s\".\n", _name.c_str());
	}
	virtual void close(const class Project* project) override {
		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "Sprite editor closed: \"%s\".\n", _name.c_str());

		if (!_checkpoint.empty()) {
			if (hasUnsavedChanges())
				Editing::Data::fromCheckpoint(project, _name.c_str(), _checkpoint);
			_checkpoint.clear();
		}

		_tools.clear();

		_ref.clear(project);

		_player = nullptr;
		_focus.clear();
		_status.clear();

		_animations.clear();
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
		if (_tools.focused)
			return;

		auto toString = [] (const Sprite::Ptr obj, int index) -> std::string {
			Math::Recti area;
			double interval = 0;
			const char* key = nullptr;
			if (!obj->get(index, nullptr, &area, &interval, &key))
				return "";

			rapidjson::Document doc;
			Jpath::set(doc, doc, area.xMin(), "x");
			Jpath::set(doc, doc, area.yMin(), "y");
			Jpath::set(doc, doc, area.width(), "width");
			Jpath::set(doc, doc, area.height(), "height");
			Jpath::set(doc, doc, interval, "interval");
			Jpath::set(doc, doc, key ? key : "", "key");

			std::string buf;
			Json::toString(doc, buf);

			return buf;
		};

		if (_cursor.empty())
			return;

		const std::string buf = toString(_object, _cursor.index);
		const std::string osstr = Unicode::toOs(buf);

		Platform::clipboardText(osstr.c_str());
	}
	virtual void cut(void) override {
		if (_tools.focused)
			return;

		_player = nullptr;

		copy();

		const Sprite::Range range = _object->rangeOf(_cursor.animation);
		const int beginIdx = std::get<0>(range);
		const int endIdx = std::get<1>(range);
		if (beginIdx >= 0 && beginIdx == endIdx) {
			Commands::Sprite::CutAnimation* cmd = _commands->enqueue<Commands::Sprite::CutAnimation>();
			cmd->with(_cursor.animation.c_str())
				->exec(_object, _ref.texture);

			_cursor = Editing::Frame();
			cmd->cursor(_cursor);

			getAnimations();
		} else {
			Commands::Sprite::CutFrame* cmd = _commands->enqueue<Commands::Sprite::CutFrame>();
			cmd->with(_cursor.index, _cursor.animation.empty())
				->exec(_object, _ref.texture);

			_cursor = Editing::Frame();
			cmd->cursor(_cursor);
		}
	}
	virtual bool pastable(void) const override {
		return Platform::hasClipboardText();
	}
	virtual void paste(void) override {
		if (_tools.focused)
			return;

		auto fromString = [] (const Sprite::Ptr obj, const std::string &buf, Math::Recti &area, double &interval, std::string &key) -> bool {
			rapidjson::Document doc;
			if (!Json::fromString(doc, buf.c_str()))
				return false;

			int x = -1, y = -1, width = -1, height = -1;
			if (!Jpath::get(doc, x, "x"))
				return false;
			if (!Jpath::get(doc, y, "y"))
				return false;
			if (!Jpath::get(doc, width, "width"))
				return false;
			if (!Jpath::get(doc, height, "height"))
				return false;
			if (!Jpath::get(doc, interval, "interval"))
				return false;
			if (!Jpath::get(doc, key, "key"))
				return false;

			area = Math::Recti::byXYWH(x, y, width, height);

			return true;
		};

		_player = nullptr;

		const std::string osstr = Platform::clipboardText();
		const std::string buf = Unicode::fromOs(osstr);
		Math::Recti area;
		double interval = 0;
		std::string key;
		if (!fromString(_object, buf, area, interval, key))
			return;

		if (area.width() != _object->width() || area.height() != _object->height())
			return;

		if (_cursor.empty()) {
			std::string name = key;
			int postfix = 1;
			while (hasAnimation(name)) {
				name = key;
				name += "_";
				name += Text::toString(postfix++);
			}

			Commands::Sprite::PasteAnimation* cmd = _commands->enqueue<Commands::Sprite::PasteAnimation>();
			cmd->with(name.c_str(), area, interval)
				->exec(_object, _ref.texture);

			cmd->cursor(_cursor);

			getAnimations();

			_status.cursor = Editing::Frame();
		} else {
			Commands::Sprite::PasteFrame* cmd = _commands->enqueue<Commands::Sprite::PasteFrame>();
			cmd->with(_cursor.index, area, interval, false)
				->exec(_object, _ref.texture);

			cmd->cursor(_cursor);

			_status.cursor = Editing::Frame();
		}
	}
	virtual void del(void) override {
		if (_tools.focused)
			return;

		_player = nullptr;

		const Sprite::Range range = _object->rangeOf(_cursor.animation);
		const int beginIdx = std::get<0>(range);
		const int endIdx = std::get<1>(range);
		if (beginIdx >= 0 && beginIdx == endIdx) {
			Commands::Sprite::DeleteAnimation* cmd = _commands->enqueue<Commands::Sprite::DeleteAnimation>();
			cmd->with(_cursor.animation.c_str())
				->exec(_object, _ref.texture);

			_cursor = Editing::Frame();
			cmd->cursor(_cursor);

			getAnimations();
		} else {
			Commands::Sprite::DeleteFrame* cmd = _commands->enqueue<Commands::Sprite::DeleteFrame>();
			cmd->with(_cursor.index, _cursor.animation.empty())
				->exec(_object, _ref.texture);

			_cursor = Editing::Frame();
			cmd->cursor(_cursor);
		}
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
		_player = nullptr;

		bool animChanged = false;
		if (_commands->redo(_object, _ref.texture, Variant((void*)&animChanged))) {
			if (animChanged)
				getAnimations();

			_cursor = getLatestCursor();
			_status.cursor = Editing::Frame();

			_focus = _cursor.animation;
		}
	}
	virtual void undo(class Asset*) override {
		_player = nullptr;

		bool animChanged = false;
		if (_commands->undo(_object, _ref.texture, Variant((void*)&animChanged))) {
			if (animChanged)
				getAnimations();

			_cursor = getLatestCursor();
			_status.cursor = Editing::Frame();

			_focus = _cursor.animation;
		}
	}

	virtual Variant post(unsigned, int, const Variant*) override {
		return Variant(false);
	}

	virtual void update(
		class Window* wnd, class Renderer* rnd,
		class Workspace* ws, const class Project* project, class Executable* /* exec */,
		const char* /* title */,
		float /* x */, float /* y */, float width, float height,
		int /* scale */,
		bool pending,
		double delta
	) override {
		ImGuiIO &io = ImGui::GetIO();
		ImGuiStyle &style = ImGui::GetStyle();

		shortcuts(wnd, rnd, ws);

		if (!_object)
			return;
		if (_object->count() > 0) {
			Texture::Ptr ref = nullptr;
			if (!_object->get(0, &ref, nullptr, nullptr, nullptr) || !ref)
				return;
		}

		const Splitter splitter = split();

		const float statusBarHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;
		bool statusBarActived = ImGui::IsWindowFocused();

		const float posY = ImGui::GetCursorPosY();
		ImGui::PushID("@Spr/Ctrl");
		{
			VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2(3, 3));

			const ImVec2 buttonSize(13 * io.FontGlobalScale, 13 * io.FontGlobalScale);
			const bool selected = !_cursor.empty() && _cursor.index >= 0 && _cursor.index < _object->count();
			float spwidth = splitter.first;
			if (selected)
				spwidth -= (buttonSize.x + style.FramePadding.x * 2) * 3;

			const float xPos = ImGui::GetCursorPosX();
			ImGui::Dummy(ImVec2(8, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(ws->theme()->dialogItem_View());
			ImGui::SameLine();
			const char* items[] = {
				ws->theme()->generic_List().c_str(),
				ws->theme()->generic_Tab().c_str()
			};
			ImGui::SetNextItemWidth(spwidth - (ImGui::GetCursorPosX() - xPos));
			const bool changed = ImGui::Combo(
				"",
				&_tools.viewIndex,
				[] (void* data, int idx, const char** outText) -> bool {
					const char** items = (const char**)data;
					*outText = items[idx];

					return true;
				},
				&items,
				BITTY_COUNTOF(items)
			);
			if (changed)
				_focus.clear();

			bool firstPlay = false;
			if (selected) {
				ImGui::SameLine();
				if (ImGui::ImageButton(ws->theme()->slicePlus()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor), false, ws->theme()->tooltipEditing_InsertFrame().c_str())) {
					const Editing::Frame frame = _cursor;
					frameInserted(rnd, ws, false, frame);
				}

				ImGui::SameLine();
				if (ImGui::ImageButton(ws->theme()->sliceMinus()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor), false, ws->theme()->tooltipEditing_DeleteFrame().c_str())) {
					const Editing::Frame frame = _cursor;
					frameRemoved(rnd, ws, frame);
				}

				ImGui::SameLine();
				if (_player) {
					if (ImGui::ImageButton(ws->theme()->sliceStop()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor))) {
						_player = nullptr;
					}
				} else {
					if (ImGui::ImageButton(ws->theme()->slicePlay()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor))) {
						_tools.viewIndex = 1;
						Sprite* spr = nullptr;
						_object->clone(&spr);
						_player = Sprite::Ptr(spr);
						_player->play(_cursor.animation, true, true, nullptr);
						firstPlay = true;
					}
				}
			}

			if (!firstPlay && _player && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle))) {
				_player = nullptr;
			}
		}
		ImGui::PopID();
		const float toolBarHeight = ImGui::GetCursorPosY() - posY;

		ImGui::BeginChild("@Spr/Pat", ImVec2(splitter.first, height - statusBarHeight - toolBarHeight), false, ImGuiWindowFlags_NoNav);
		{
			if (_cursor.empty() && !_animations.empty()) {
				_cursor.animation = _animations.front();
				_cursor.index = 0;

				refreshStatus(wnd, rnd, ws, _cursor);
			}

			bool painting = false;
			if (_tools.viewIndex == 0) {
				painting = Editing::sprite(
					rnd,
					ws,
					_object.get(),
					ImGui::GetContentRegionAvail().x, (float)(_tools.magnification + 1),
					&_cursor, true,
					std::bind(&EditorSpriteImpl::animationAdded, this, rnd, ws),
					std::bind(&EditorSpriteImpl::animationRemoved, this, rnd, ws, std::placeholders::_1),
					std::bind(&EditorSpriteImpl::animationRenamed, this, rnd, ws, std::placeholders::_1),
					std::bind(&EditorSpriteImpl::frameInserted, this, rnd, ws, true, std::placeholders::_1),
					std::bind(&EditorSpriteImpl::frameRemoved, this, rnd, ws, std::placeholders::_1),
					std::bind(&EditorSpriteImpl::frameIntervalChanged, this, rnd, ws, std::placeholders::_1)
				);
			} else {
				if (_player && _player->update(delta, nullptr)) {
					int idx = -1;
					Math::Recti area;
					double interval = 0;
					_player->current(&idx, nullptr, &area, &interval, nullptr);
					_cursor.index = idx;
					_ref.cursor.fromRect(area);
					_tools.interval = interval;
				}

				painting = Editing::sprite(
					rnd,
					ws,
					_object.get(),
					ImGui::GetContentRegionAvail().x, (float)(_tools.magnification + 1),
					&_cursor, true,
					_animations,
					&_focus,
					_player.get(),
					std::bind(&EditorSpriteImpl::animationAdded, this, rnd, ws),
					std::bind(&EditorSpriteImpl::animationRemoved, this, rnd, ws, std::placeholders::_1),
					std::bind(&EditorSpriteImpl::animationRenamed, this, rnd, ws, std::placeholders::_1),
					std::bind(&EditorSpriteImpl::frameInserted, this, rnd, ws, true, std::placeholders::_1),
					std::bind(&EditorSpriteImpl::frameRemoved, this, rnd, ws, std::placeholders::_1),
					std::bind(&EditorSpriteImpl::frameIntervalChanged, this, rnd, ws, std::placeholders::_1)
				);
			}
			if (painting && ImGui::IsWindowFocused()) {
				Math::Recti area;
				if (_object->get(_cursor.index, nullptr, &area, &_tools.interval, nullptr))
					_ref.cursor.fromRect(area);
			}
			refreshStatus(wnd, rnd, ws, _cursor);

			statusBarActived |= ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

			context(wnd, rnd, ws);
		}
		ImGui::EndChild();

		ImGui::SameLine();
		const float posX = ImGui::GetCursorPosX();
		ImGui::SetCursorPos(ImVec2(posX, posY));
		ImGui::BeginChild("@Spr/Tls", ImVec2(splitter.second, height - statusBarHeight), true, _ref.windowFlags());
		{
			const bool refed = _ref.fill(rnd, project);

			ImGui::PushID("@Spr/Tls/Ref");
			{
				const float curPosX = ImGui::GetCursorPosX();
				ImGui::AlignTextToFramePadding();
				ImGui::Dummy(ImVec2(4, 0));
				ImGui::SameLine();
				if (refed)
					ImGui::TextUnformatted(ws->theme()->dialogItem_Ref());
				else
					ImGui::TextUnformatted("? ");
				ImGui::SameLine();
				ImGui::SetNextItemWidth(splitter.second - (ImGui::GetCursorPosX() - curPosX) - style.ChildBorderSize * 3);
				ImGui::InputText("", (char*)_ref.name.c_str(), _ref.name.length(), ImGuiInputTextFlags_ReadOnly);
			}
			ImGui::PopID();

			const float spwidth = _ref.windowWidth(splitter.second);
			ImGui::BeginChild("@Spr/Tls/Img", ImVec2(spwidth, spwidth), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoNav);
			{
				if (_ref.image && _ref.texture) {
					float refWidth = spwidth - style.ScrollbarSize - style.ChildBorderSize * 4;
					if (_ref.image->width() >= refWidth) {
						refWidth = (float)_ref.image->width();
					} else {
						if (_ref.image->width() > _ref.image->height())
							refWidth *= (float)_ref.image->width() / _ref.image->height();
					}
					Editing::Brush cursor = _ref.cursor;
					bool painting = Editing::image(
						rnd,
						ws,
						_ref.image.get(), _ref.texture.get(),
						refWidth,
						&cursor, true,
						nullptr,
						nullptr,
						&_ref.gridUnit, _ref.gridsVisible || ws->settings()->editorAlwaysShowGrids,
						_ref.transparentBackbroundVisible || ws->settings()->editorAlwaysShowTransparentBackground,
						ImGuiMouseButton_Left
					);
					if (painting)
						_ref.cursor = cursor;
					if (painting)
						showRefStatus(wnd, rnd, ws, _ref.cursor);
				}

				statusBarActived |= ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
			}
			ImGui::EndChild();

			do {
				const bool selected = !_cursor.empty() && _cursor.index >= 0 && _cursor.index < _object->count();
				if (!selected)
					break;

				Math::Recti area;
				double interval = 0;
				if (!_object->get(_cursor.index, nullptr, &area, &interval, nullptr))
					break;

				ImGui::NewLine();
				Editing::Tools::tickable(rnd, ws, &_tools.interval, -1.0f);

				VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

				if (_ref.cursor == area && _tools.interval == interval)
					break;

				ImGui::NewLine();
				ImGui::CentralizeButton(2);
				if (ImGui::Button(ws->theme()->generic_Apply(), ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || (ImGui::IsWindowFocused() && ImGui::IsKeyReleased(SDL_SCANCODE_RETURN))) {
					if (_ref.cursor != area) {
						area = _ref.cursor.toRect();

						Commands::Sprite::ChangeArea* cmd = _commands->enqueue<Commands::Sprite::ChangeArea>();
						cmd->with(_cursor.index, area)
							->exec(_object, _ref.texture);

						cmd->cursor(_cursor);
					}
					if (_tools.interval != interval) {
						interval = _tools.interval;

						Commands::Sprite::ChangeInterval* cmd = _commands->enqueue<Commands::Sprite::ChangeInterval>();
						cmd->with(_cursor.index, interval)
							->exec(_object, _ref.texture);

						cmd->cursor(_cursor);
					}
				}
				ImGui::SameLine();
				if (ImGui::Button(ws->theme()->generic_Cancel(), ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || (ImGui::IsWindowFocused() && ImGui::IsKeyReleased(SDL_SCANCODE_ESCAPE))) {
					if (_ref.cursor != area)
						_ref.cursor.fromRect(area);
					if (_tools.interval != interval)
						_tools.interval = interval;
				}
			} while (false);

			ImGui::NewLine();
			Editing::Tools::magnifiable(rnd, ws, &_tools.magnification, -1.0f, ws->canUseShortcuts());

			_tools.focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
			statusBarActived |= ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
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
		_player = nullptr;

		_ref.image = nullptr;
		_ref.texture = nullptr;
	}
	virtual void gainFocus(class Renderer* rnd, const class Project* project) override {
		_ref.fill(rnd, project);

		if (_object) {
			const int n = _object->count();
			for (int i = 0; i < n; ++i) {
				Math::Recti area;
				double interval = 0;
				const char* key = nullptr;
				_object->get(i, nullptr, &area, &interval, &key);
				_object->set(i, _ref.texture, &area, &interval, key);
			}
		}
	}

private:
	void shortcuts(Window* /* wnd */, Renderer* /* rnd */, Workspace* ws) {
		const Editing::Shortcut caps(SDL_SCANCODE_UNKNOWN, false, false, false, false, true);
		const Editing::Shortcut num(SDL_SCANCODE_UNKNOWN, false, false, false, true, false);
		_ref.gridsVisible = caps.pressed();
		_ref.transparentBackbroundVisible = num.pressed();

		if (!ws->canUseShortcuts()) {
			return;
		}

		const Editing::Shortcut shift(SDL_SCANCODE_UNKNOWN, false, true, false);
		if (shift.pressed(false)) {
			_ref.gridsVisible = true;
		} else if (shift.released()) {
			_ref.gridsVisible = caps.pressed();
		}

		const Editing::Shortcut esc(SDL_SCANCODE_ESCAPE);
		if (esc.pressed())
			_player = nullptr;
	}

	void context(Window* /* wnd */, Renderer* /* rnd */, Workspace* ws) {
		ImGuiStyle &style = ImGui::GetStyle();

		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
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

	void refreshStatus(Window* /* wnd */, Renderer* /* rnd */, Workspace* ws, const Editing::Frame &cursor) {
		if (_status.cursor == cursor)
			return;

		_status.cursor = cursor;

		Math::Recti area;
		if (_object->get(_status.cursor.index, nullptr, &area, &_tools.interval, nullptr))
			_ref.cursor.fromRect(area);

		_status.text = ws->theme()->statusItem_Index();
		_status.text += " ";
		_status.text += Text::toString(_status.cursor.index);

		_status.text += "  ";
		_status.text += ws->theme()->statusItem_Area();
		_status.text += " ";
		_status.text += Text::toString(area.xMin());
		_status.text += ", ";
		_status.text += Text::toString(area.yMin());
		_status.text += "; ";
		_status.text += Text::toString(area.width());
		_status.text += "x";
		_status.text += Text::toString(area.height());
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
		ImGui::TextUnformatted(_status.text);
		if (actived)
			ImGui::PopStyleColor();
	}
	void showRefStatus(Window* /* wnd */, Renderer* /* rnd */, Workspace* ws, const Editing::Brush &cursor) {
		ImGuiStyle &style = ImGui::GetStyle();

		if (!_object || !_ref.image)
			return;
		if (cursor.position.x == -1 || cursor.position.y == -1)
			return;

		const Int w = _ref.image->width() / _object->width();
		const Int x = cursor.position.x / _object->width();
		const Int y = cursor.position.y / _object->height();
		const Int idx = x + y * w;

		std::string text;

		text = ws->theme()->statusItem_Pos();
		text += " ";
		text += Text::toString(x);
		text += ", ";
		text += Text::toString(y);

		text += "  ";

		text += ws->theme()->statusItem_Index();
		text += " ";
		text += Text::toString(idx);

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		ImGui::SetTooltip(text);
	}

	void getAnimations(void) {
		_animations.clear();

		if (!_object)
			return;

		for (int i = 0; i < _object->count(); ++i) {
			const char* key = nullptr;
			_object->get(i, nullptr, nullptr, nullptr, &key);
			if (key && *key)
				_animations.push_back(key);
			else if (i == 0)
				_animations.push_back("");
		}
	}
	bool hasAnimation(const std::string &name) const {
		return std::find(_animations.begin(), _animations.end(), name) != _animations.end();
	}

	bool getRefArea(Renderer* rnd, Workspace* ws, Math::Recti &area) const {
		if (!_ref.getArea(_object->width(), _object->height(), area)) {
			Operations::popupMessage(rnd, ws, ws->theme()->dialogPrompt_InvalidArea().c_str());

			return false;
		}

		return true;
	}

	Editing::Frame getLatestCursor(void) const {
		Editing::Frame result;
		const int idx = _commands->cursor() - 1;
		Command* cmd = _commands->at(idx);
		if (idx >= 0 && cmd) {
			Commands::Sprite::Cursorial* cursor = Command::as<Commands::Sprite::Cursorial>(cmd);
			result = cursor->cursor();
		}

		return result;
	}

	void animationAdded(Renderer* rnd, Workspace* ws) {
		if (_object->count() >= BITTY_SPRITE_FRAME_MAX_COUNT) {
			Operations::popupMessage(rnd, ws, ws->theme()->dialogPrompt_CannotAddMoreFrame().c_str());

			return;
		}

		Math::Recti area;
		if (!getRefArea(rnd, ws, area))
			return;

		std::string default_ = EDITING_NONAME;
		int postfix = 1;
		while (hasAnimation(default_)) {
			default_ = EDITING_NONAME;
			default_ += "_";
			default_ += Text::toString(postfix++);
		}

		Operations::popupInput(rnd, ws, ws->theme()->dialogItem_InputAnimationName().c_str(), default_.c_str())
			.then(
				[this, rnd, ws, area] (const char* name) -> void {
					std::string key = name;
					key = Text::trim(key);

					if (hasAnimation(key.c_str())) {
						Operations::popupMessage(rnd, ws, ws->theme()->dialogPrompt_AlreadyExists().c_str());

						return;
					}

					Commands::Sprite::AddAnimation* cmd = _commands->enqueue<Commands::Sprite::AddAnimation>();
					cmd->with(key.c_str(), area, _tools.interval)
						->exec(_object, _ref.texture);

					_cursor = Editing::Frame(cmd->animation(), cmd->index());
					cmd->cursor(_cursor);

					getAnimations();
				}
			);
	}
	void animationRemoved(Renderer*, Workspace*, const char* anim) {
		Commands::Sprite::DeleteAnimation* cmd = _commands->enqueue<Commands::Sprite::DeleteAnimation>();
		cmd->with(anim)
			->exec(_object, _ref.texture);

		_cursor = Editing::Frame();
		cmd->cursor(_cursor);

		getAnimations();
	}
	void animationRenamed(Renderer* rnd, Workspace* ws, const char* anim) {
		Operations::popupInput(rnd, ws, ws->theme()->dialogItem_InputAnimationName().c_str(), anim)
			.then(
				[this, rnd, ws, anim] (const char* name) -> void {
					std::string key = name;
					key = Text::trim(key);

					if (hasAnimation(key.c_str())) {
						Operations::popupMessage(rnd, ws, ws->theme()->dialogPrompt_AlreadyExists().c_str());

						return;
					}

					Commands::Sprite::RenameAnimation* cmd = _commands->enqueue<Commands::Sprite::RenameAnimation>();
					cmd->with(anim, key.c_str())
						->exec(_object, _ref.texture);

					cmd->cursor(_cursor);

					getAnimations();
				}
			);
	}
	void frameInserted(Renderer* rnd, Workspace* ws, bool append, const Editing::Frame &frame) {
		if (_object->count() >= BITTY_SPRITE_FRAME_MAX_COUNT) {
			Operations::popupMessage(rnd, ws, ws->theme()->dialogPrompt_CannotAddMoreFrame().c_str());

			return;
		}

		Math::Recti area;
		if (!getRefArea(rnd, ws, area))
			return;

		std::string key;
		if (!append) {
			const int i = frame.index;
			const char* key_ = nullptr;
			_object->get(i, nullptr, nullptr, nullptr, &key_);
			if (key_ && *key_)
				key = key_;
		}

		Commands::Sprite::AddFrame* cmd = _commands->enqueue<Commands::Sprite::AddFrame>();
		cmd->with(frame.index, area, _tools.interval, append)
			->exec(_object, _ref.texture);

		_cursor = Editing::Frame(key, cmd->index());
		cmd->cursor(_cursor);
	}
	void frameRemoved(Renderer*, Workspace*, const Editing::Frame &frame) {
		const Sprite::Range range = _object->rangeOf(_cursor.animation);
		const int beginIdx = std::get<0>(range);
		const int endIdx = std::get<1>(range);
		if (beginIdx >= 0 && beginIdx == endIdx) {
			Commands::Sprite::DeleteAnimation* cmd = _commands->enqueue<Commands::Sprite::DeleteAnimation>();
			cmd->with(frame.animation.c_str())
				->exec(_object, _ref.texture);

			_cursor = Editing::Frame();
			cmd->cursor(_cursor);

			getAnimations();
		} else {
			Commands::Sprite::DeleteFrame* cmd = _commands->enqueue<Commands::Sprite::DeleteFrame>();
			cmd->with(frame.index, _cursor.animation.empty())
				->exec(_object, _ref.texture);

			_cursor = Editing::Frame();
			cmd->cursor(_cursor);
		}
	}
	void frameIntervalChanged(Renderer* rnd, Workspace* ws, const Editing::Frame &frame) {
		double interval = 0;
		if (!_object->get(frame.index, nullptr, nullptr, &interval, nullptr))
			return;

		const std::string default_ = Text::toString(interval);

		Operations::popupInput(rnd, ws, ws->theme()->dialogItem_InputInterval().c_str(), default_.c_str(), ImGuiInputTextFlags_CharsScientific)
			.then(
				[this, frame, interval] (const char* intervalStr) -> void {
					double interval_ = 0;
					if (!Text::fromString(intervalStr, interval_))
						return;

					if (interval == interval_)
						return;

					Commands::Sprite::ChangeInterval* cmd = _commands->enqueue<Commands::Sprite::ChangeInterval>();
					cmd->with(frame.index, interval_)
						->exec(_object, _ref.texture);

					cmd->cursor(_cursor);

					_tools.interval = interval_;
				}
			);
	}
};

EditorSprite* EditorSprite::create(void) {
	EditorSpriteImpl* result = new EditorSpriteImpl();

	return result;
}

void EditorSprite::destroy(EditorSprite* ptr) {
	EditorSpriteImpl* impl = static_cast<EditorSpriteImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
