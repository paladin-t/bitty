/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "commands_map.h"
#include "editing.h"
#include "editor_map.h"
#include "encoding.h"
#include "image.h"
#include "map.h"
#include "platform.h"
#include "project.h"
#include "theme.h"
#include "workspace.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../lib/imgui/imgui_internal.h"
#include "../lib/jpath/jpath.hpp"
#include <SDL.h>

/*
** {===========================================================================
** Map editor
*/

class EditorMapImpl : public EditorMap {
private:
	typedef std::function<bool(const Math::Vec2i &, Editing::Dot &)> CelGetter;
	typedef std::function<bool(const Math::Vec2i &, const Editing::Dot &)> CelSetter;

	typedef std::function<int(const Math::Recti* /* nullable */, Editing::Dots &)> CelsGetter;
	typedef std::function<int(const Math::Recti* /* nullable */, const Editing::Dots &)> CelsSetter;

	typedef std::function<void(void)> PostHandler;

	struct Processor {
		typedef std::function<void(Renderer*)> Handler;

		Handler down;
		Handler move;
		Handler up;
		Handler hover;
	};

private:
	bool _opened = false;

	std::string _name;
	Map::Ptr _object = nullptr;
	CommandQueue* _commands = nullptr;
	Editing::Data::Checkpoint _checkpoint;

	Math::Vec2i _tileSize = Math::Vec2i(BITTY_MAP_TILE_DEFAULT_SIZE, BITTY_MAP_TILE_DEFAULT_SIZE);
	Editing::Tiler _cursor;
	struct {
		Math::Vec2i initial = Math::Vec2i(-1, -1);
		Editing::Tiler tiler;

		ImVec2 mouse = ImVec2(-1, -1);
		int idle = 0;

		void clear(void) {
			initial = Math::Vec2i(-1, -1);
			tiler = Editing::Tiler();

			mouse = ImVec2(-1, -1);
			idle = 0;
		}

		int area(Math::Recti &area) const {
			if (tiler.empty())
				return 0;

			area = Math::Recti::byXYWH(tiler.position.x, tiler.position.y, tiler.size.x, tiler.size.y);

			return area.width() * area.height();
		}
	} _selection;
	struct {
		std::string text = "Map";
		Editing::Tiler cursor;

		void clear(void) {
			text = "Map";
			cursor = Editing::Tiler();
		}
	} _status;
	Editing::Painting _painting;
	struct {
		CelGetter getCel = nullptr;
		CelSetter setCel = nullptr;
		CelsGetter getCels = nullptr;
		CelsSetter setCels = nullptr;

		bool empty(void) const {
			return !getCel || !setCel || !getCels || !setCels;
		}
		void fill(CelGetter getCel_, CelSetter setCel_, CelsGetter getCels_, CelsSetter setCels_) {
			getCel = getCel_;
			setCel = setCel_;
			getCels = getCels_;
			setCels = setCels_;
		}
		void clear(void) {
			getCel = nullptr;
			setCel = nullptr;
			getCels = nullptr;
			setCels = nullptr;
		}
	} _binding;
	Editing::MapCelGetter _overlay = nullptr;

	struct Ref : public Editor::Ref {
		std::string name;
		Image::Ptr image = nullptr;
		Texture::Ptr texture = nullptr;
		Editing::Brush cursor;
		Editing::Brush stamp;

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
			stamp = Editing::Brush();

			gridsVisible = false;
			gridUnit = Math::Vec2i(0, 0);
			transparentBackbroundVisible = true;
		}
	} _ref;
	struct Tools {
		Editing::Tools::Tools painting = Editing::Tools::PENCIL;
		bool scaled = false;
		int magnification = 0;
		int weighting = 0;

		bool gridsVisible = false;
		Math::Vec2i gridUnit = Math::Vec2i(0, 0);
		bool transparentBackbroundVisible = true;

		ImVec2 mousePos = ImVec2(-1, -1);
		ImVec2 mouseDiff = ImVec2(0, 0);

		PostHandler post = nullptr;

		void clear(void) {
			painting = Editing::Tools::PENCIL;
			scaled = false;
			magnification = 0;
			weighting = 0;

			gridsVisible = false;
			gridUnit = Math::Vec2i(0, 0);
			transparentBackbroundVisible = true;

			mousePos = ImVec2(-1, -1);
			mouseDiff = ImVec2(0, 0);

			post = nullptr;
		}
	} _tools;
	Processor _processors[Editing::Tools::COUNT] = {
		Processor{ // Move.
			std::bind(&EditorMapImpl::moveToolDown, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::moveToolMove, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::moveToolUp, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::moveToolHover, this, std::placeholders::_1)
		},
		Processor{ // Eyedropper.
			std::bind(&EditorMapImpl::eyedropperToolDown, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::eyedropperToolMove, this, std::placeholders::_1),
			nullptr,
			nullptr
		},
		Processor{ // Pencil.
			std::bind(&EditorMapImpl::paintToolDown<Commands::Map::Pencil>, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::paintToolMove<Commands::Map::Pencil>, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		},
		Processor{ // Paintbucket.
			nullptr,
			nullptr,
			std::bind(&EditorMapImpl::paintbucketToolUp, this, std::placeholders::_1),
			nullptr
		},
		Processor{ // Lasso.
			std::bind(&EditorMapImpl::lassoToolDown, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::lassoToolMove, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::lassoToolUp, this, std::placeholders::_1),
			nullptr
		},
		Processor{ // Line.
			std::bind(&EditorMapImpl::paintToolDown<Commands::Map::Line>, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::paintToolMove<Commands::Map::Line>, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		},
		Processor{ // Box.
			std::bind(&EditorMapImpl::paintToolDown<Commands::Map::Box>, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::paintToolMove<Commands::Map::Box>, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		},
		Processor{ // Box fill.
			std::bind(&EditorMapImpl::paintToolDown<Commands::Map::BoxFill>, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::paintToolMove<Commands::Map::BoxFill>, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		},
		Processor{ // Ellipse.
			std::bind(&EditorMapImpl::paintToolDown<Commands::Map::Ellipse>, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::paintToolMove<Commands::Map::Ellipse>, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		},
		Processor{ // Ellipse fill.
			std::bind(&EditorMapImpl::paintToolDown<Commands::Map::EllipseFill>, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::paintToolMove<Commands::Map::EllipseFill>, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		},
		Processor{ // Stamp, pasting.
			std::bind(&EditorMapImpl::stampToolDown, this, std::placeholders::_1),
			nullptr,
			std::bind(&EditorMapImpl::stampToolUp, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::stampToolHover, this, std::placeholders::_1)
		}
	};

public:
	EditorMapImpl() {
		_commands = new CommandQueue();
		_commands->registerFactory<Commands::Map::Pencil>();
		_commands->registerFactory<Commands::Map::Line>();
		_commands->registerFactory<Commands::Map::Box>();
		_commands->registerFactory<Commands::Map::BoxFill>();
		_commands->registerFactory<Commands::Map::Ellipse>();
		_commands->registerFactory<Commands::Map::EllipseFill>();
		_commands->registerFactory<Commands::Map::Fill>();
		_commands->registerFactory<Commands::Map::Replace>();
		_commands->registerFactory<Commands::Map::Stamp>();
		_commands->registerFactory<Commands::Map::Rotate>();
		_commands->registerFactory<Commands::Map::Flip>();
		_commands->registerFactory<Commands::Map::Cut>();
		_commands->registerFactory<Commands::Map::Paste>();
		_commands->registerFactory<Commands::Map::Delete>();
		_commands->registerFactory<Commands::Map::Resize>();

		_checkpoint.fill();
	}
	virtual ~EditorMapImpl() override {
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

		_object = Object::as<Map::Ptr>(obj);
		Map::Tiles tiles;
		if (_object && _object->tiles(tiles) && tiles.texture) {
			_tileSize = tiles.size();

			if (_tileSize.x == 0)
				_tileSize.x = BITTY_MAP_TILE_DEFAULT_SIZE;
			if (_tileSize.y == 0)
				_tileSize.y = BITTY_MAP_TILE_DEFAULT_SIZE;
		}
		Editing::Data::toCheckpoint(project, _name.c_str(), _checkpoint);

		_ref.name = ref ? ref : "";
		_ref.cursor.position = Math::Vec2i(0, 0);
		_ref.cursor.size = _tileSize;
		_ref.stamp.position = Math::Vec2i(0, 0);
		_ref.stamp.size = _tileSize;

		const Editing::Shortcut caps(SDL_SCANCODE_UNKNOWN, false, false, false, false, true);
		const Editing::Shortcut num(SDL_SCANCODE_UNKNOWN, false, false, false, true, false);
		_ref.gridsVisible = caps.pressed();
		_ref.gridUnit = _ref.cursor.size;
		_ref.transparentBackbroundVisible = num.pressed();

		_tools.gridsVisible = caps.pressed();
		_tools.gridUnit = Math::Vec2i(BITTY_GRID_DEFAULT_SIZE, BITTY_GRID_DEFAULT_SIZE);
		_tools.transparentBackbroundVisible = num.pressed();

		fprintf(stdout, "Map editor opened: \"%s\".\n", _name.c_str());
	}
	virtual void close(const class Project* project) override {
		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "Map editor closed: \"%s\".\n", _name.c_str());

		if (!_checkpoint.empty()) {
			if (hasUnsavedChanges())
				Editing::Data::fromCheckpoint(project, _name.c_str(), _checkpoint);
			_checkpoint.clear();
		}

		_tools.clear();

		_ref.clear(project);

		_overlay = nullptr;
		_binding.clear();
		_painting.clear();
		_status.clear();
		_selection.clear();
		_tileSize = Math::Vec2i(BITTY_MAP_TILE_DEFAULT_SIZE, BITTY_MAP_TILE_DEFAULT_SIZE);

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
		auto toString = [] (const Math::Recti &area, const Editing::Dots &dots) -> std::string {
			rapidjson::Document doc;
			Jpath::set(doc, doc, area.width(), "width");
			Jpath::set(doc, doc, area.height(), "height");
			int k = 0;
			for (int j = area.yMin(); j <= area.yMax(); ++j) {
				for (int i = area.xMin(); i <= area.xMax(); ++i) {
					Jpath::set(doc, doc, dots.indexed[k], "data", k);

					++k;
				}
			}

			std::string buf;
			Json::toString(doc, buf);

			return buf;
		};

		if (!_binding.getCels || !_binding.setCels)
			return;

		Math::Recti sel;
		const Math::Recti* selPtr = nullptr;
		int size = _selection.area(sel);
		if (size == 0)
			size = _object->width() * _object->height();
		else
			selPtr = &sel;

		std::vector<int> vec(size, 0);
		Editing::Dots dots(&vec.front());
		_binding.getCels(selPtr, dots);

		const std::string buf = toString(sel, dots);
		const std::string osstr = Unicode::toOs(buf);

		Platform::clipboardText(osstr.c_str());
	}
	virtual void cut(void) override {
		if (!_binding.getCel || !_binding.setCel)
			return;

		copy();

		Math::Recti sel;
		const int size = area(&sel);
		if (size == 0)
			return;

		_commands->enqueue<Commands::Map::Cut>()
			->with(_binding.getCel, _binding.setCel)
			->with(sel)
			->exec(_object);
	}
	virtual bool pastable(void) const override {
		return Platform::hasClipboardText();
	}
	virtual void paste(void) override {
		auto fromString = [] (const std::string &buf, Math::Recti &area, Editing::Dot::Array &dots) -> bool {
			rapidjson::Document doc;
			if (!Json::fromString(doc, buf.c_str()))
				return false;

			int width = -1, height = -1;
			if (!Jpath::get(doc, width, "width"))
				return false;
			if (!Jpath::get(doc, height, "height"))
				return false;
			area = Math::Recti::byXYWH(0, 0, width, height);
			int k = 0;
			for (int j = area.yMin(); j <= area.yMax(); ++j) {
				for (int i = area.xMin(); i <= area.xMax(); ++i) {
					int idx = Map::INVALID();
					if (!Jpath::get(doc, idx, "data", k))
						return false;

					Editing::Dot dot;
					dot.indexed = idx;
					dots.push_back(dot);

					++k;
				}
			}

			return true;
		};

		const std::string osstr = Platform::clipboardText();
		const std::string buf = Unicode::fromOs(osstr);
		Math::Recti area;
		Editing::Dot::Array dots;
		if (!fromString(buf, area, dots))
			return;

		const Editing::Tools::Tools prevTool = _tools.painting;

		_tools.painting = Editing::Tools::STAMP;

		_processors[Editing::Tools::STAMP] = Processor{
			nullptr,
			nullptr,
			std::bind(&EditorMapImpl::stampToolUp_Paste, this, std::placeholders::_1, area, dots, prevTool),
			std::bind(&EditorMapImpl::stampToolHover_Paste, this, std::placeholders::_1, area, dots)
		};

		destroyOverlay();
	}
	virtual void del(void) override {
		if (!_binding.getCel || !_binding.setCel)
			return;

		Math::Recti sel;
		const int size = area(&sel);
		if (size == 0)
			return;

		_commands->enqueue<Commands::Map::Delete>()
			->with(_binding.getCel, _binding.setCel)
			->with(sel)
			->exec(_object);
	}
	int area(Math::Recti* area /* nullable */) const {
		Math::Recti sel;
		int size = _selection.area(sel);
		if (size == 0) {
			sel = Math::Recti::byXYWH(0, 0, _object->width(), _object->height());
			size = _object->width() * _object->height();
		}
		if (area)
			*area = sel;

		return size;
	}
	virtual bool selectable(void) const override {
		return true;
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
		const int width = _object->width();
		const int height = _object->height();
		_commands->redo(_object);

		if (width != _object->width() || height != _object->height())
			_selection.clear();
	}
	virtual void undo(class Asset*) override {
		const int width = _object->width();
		const int height = _object->height();
		_commands->undo(_object);

		if (width != _object->width() || height != _object->height())
			_selection.clear();
	}

	virtual Variant post(unsigned msg, int argc, const Variant* argv) override {
		switch (msg) {
		case RECALCULATE: {
				Map::Tiles tiles;
				if (_object && _object->tiles(tiles) && tiles.texture) {
					_tileSize = tiles.size();

					if (_tileSize.x == 0)
						_tileSize.x = BITTY_MAP_TILE_DEFAULT_SIZE;
					if (_tileSize.y == 0)
						_tileSize.y = BITTY_MAP_TILE_DEFAULT_SIZE;
				}

				_ref.cursor.position = Math::Vec2i(0, 0);
				_ref.cursor.size = _tileSize;
				_ref.stamp.position = Math::Vec2i(0, 0);
				_ref.stamp.size = _tileSize;
			}

			return Variant(true);
		case RESIZE: {
				const Variant::Int width = unpack<Variant::Int>(argc, argv, 0, 0);
				const Variant::Int height = unpack<Variant::Int>(argc, argv, 1, 0);
				if (width == _object->width() && height == _object->height())
					return Variant(true);

				_commands->enqueue<Commands::Map::Resize>()
					->with(Math::Vec2i(width, height))
					->exec(_object);

				_selection.clear();
			}

			return Variant(true);
		case SELECT_ALL:
			_selection.tiler.position = Math::Vec2i(0, 0);
			_selection.tiler.size = Math::Vec2i(_object->width(), _object->height());

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
		const char* /* title */,
		float /* x */, float /* y */, float width, float height,
		float /* scaleX */, float /* scaleY */,
		bool pending,
		double /* delta */
	) override {
		ImGuiStyle &style = ImGui::GetStyle();

		if (_binding.empty()) {
			_binding.fill(
				std::bind(&EditorMapImpl::getCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorMapImpl::setCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorMapImpl::getCels, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorMapImpl::setCels, this, rnd, std::placeholders::_1, std::placeholders::_2)
			);
		}

		shortcuts(wnd, rnd, ws);

		if (!_object)
			return;
		Map::Tiles tiles;
		if (!_object->tiles(tiles) || !tiles.texture)
			return;

		const Splitter splitter = split();

		const float statusBarHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;
		bool statusBarActived = ImGui::IsWindowFocused();

		ImGuiWindowFlags flags = ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoNav;
		if (_tools.scaled) {
			flags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
			flags |= ImGuiWindowFlags_AlwaysHorizontalScrollbar;
		}
		ImGui::BeginChild("@Map/Pat", ImVec2(splitter.first, height - statusBarHeight), false, flags);
		{
			if (_cursor.empty()) {
				_cursor.position = Math::Vec2i(0, 0);
				_cursor.size = Math::Vec2i(1, 1);
			}

			Editing::Tiler cursor = _cursor;
			const bool withCursor = _tools.painting != Editing::Tools::MOVE;

			const ImVec2 content = ImGui::GetContentRegionAvail();
			float width_ = (float)(_object->width() * _tileSize.x);
			if (content.x / _tileSize.x > 1600 || content.y / _tileSize.y > 1600) {
				width_ *= 32;
				_tools.scaled = true;
			} else if (content.x / _tileSize.x > 800 || content.y / _tileSize.y > 800) {
				width_ *= 16;
				_tools.scaled = true;
			} else if (content.x / _tileSize.x > 400 || content.y / _tileSize.y > 400) {
				width_ *= 8;
				_tools.scaled = true;
			} else if (content.x / _tileSize.x > 200 || content.y / _tileSize.y > 200) {
				width_ *= 4;
				_tools.scaled = true;
			} else if (content.x / _tileSize.x > 100 || content.y / _tileSize.y > 100) {
				width_ *= 2;
				_tools.scaled = true;
			}
			width_ *= (float)(_tools.magnification + 1);

			_painting.set(
				Editing::map(
					rnd,
					ws,
					_object.get(), _tileSize,
					width_,
					withCursor ? &cursor : nullptr, false,
					_selection.tiler.empty() ? nullptr : &_selection.tiler,
					(_tools.gridsVisible || ws->settings()->editorAlwaysShowGrids) ? &_tools.gridUnit : nullptr,
					_tools.transparentBackbroundVisible || ws->settings()->editorAlwaysShowTransparentBackground,
					_overlay
				)
			);
			if (
				(_painting && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) ||
				_tools.painting == Editing::Tools::STAMP
			) {
				_cursor = cursor;
			}
			refreshStatus(wnd, rnd, ws, cursor);

			if (_painting.down()) {
				if (_processors[_tools.painting].down)
					_processors[_tools.painting].down(rnd);
			}
			if (_painting && _painting.moved()) {
				if (_processors[_tools.painting].move)
					_processors[_tools.painting].move(rnd);
			}
			if (_painting.up()) {
				if (_processors[_tools.painting].up)
					_processors[_tools.painting].up(rnd);
			}
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
				if (_processors[_tools.painting].hover)
					_processors[_tools.painting].hover(rnd);
			}

			statusBarActived |= ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

			context(wnd, rnd, ws);
		}
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("@Map/Tls", ImVec2(splitter.second, height - statusBarHeight), true, _ref.windowFlags());
		{
			const bool refed = _ref.fill(rnd, project);

			ImGui::PushID("@Map/Tls/Ref");
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
			ImGui::BeginChild("@Map/Tls/Img", ImVec2(spwidth, spwidth), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoNav);
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
					bool painting = false;
					if (_tools.painting == Editing::Tools::STAMP) {
						painting = Editing::image(
							rnd,
							ws,
							_ref.image.get(), _ref.texture.get(),
							refWidth,
							&cursor, true, std::move(_ref.stamp),
							nullptr,
							nullptr,
							&_tools.gridUnit, _tools.gridsVisible || ws->settings()->editorAlwaysShowGrids,
							_ref.transparentBackbroundVisible || ws->settings()->editorAlwaysShowTransparentBackground
						);
					} else {
						painting = Editing::image(
							rnd,
							ws,
							_ref.image.get(), _ref.texture.get(),
							refWidth,
							&cursor, true,
							nullptr,
							nullptr,
							&_tools.gridUnit, _tools.gridsVisible || ws->settings()->editorAlwaysShowGrids,
							_ref.transparentBackbroundVisible || ws->settings()->editorAlwaysShowTransparentBackground
						);
					}
					if (painting || _tools.painting == Editing::Tools::STAMP)
						_ref.cursor = cursor;
					if (painting)
						showRefStatus(wnd, rnd, ws, _ref.cursor);
				}

				statusBarActived |= ImGui::IsWindowFocused();
			}
			ImGui::EndChild();

			ImGui::NewLine();
			if (Editing::Tools::paintable(rnd, ws, &_tools.painting, -1.0f, ws->canUseShortcuts())) {
				if (_tools.painting == Editing::Tools::STAMP) {
					_processors[Editing::Tools::STAMP] = Processor{
						std::bind(&EditorMapImpl::stampToolDown, this, std::placeholders::_1),
						nullptr,
						std::bind(&EditorMapImpl::stampToolUp, this, std::placeholders::_1),
						std::bind(&EditorMapImpl::stampToolHover, this, std::placeholders::_1)
					};
				} else {
					_ref.cursor.size = _tileSize;
				}

				destroyOverlay();
			}

			ImGui::NewLine();
			Editing::Tools::magnifiable(rnd, ws, &_tools.magnification, -1.0f, ws->canUseShortcuts());

			switch (_tools.painting) {
			case Editing::Tools::PENCIL: // Fall through.
			case Editing::Tools::LINE: // Fall through.
			case Editing::Tools::BOX: // Fall through.
			case Editing::Tools::BOX_FILL: // Fall through.
			case Editing::Tools::ELLIPSE: // Fall through.
			case Editing::Tools::ELLIPSE_FILL:
				ImGui::NewLine();
				Editing::Tools::weighable(rnd, ws, &_tools.weighting, -1.0f, ws->canUseShortcuts());

				break;
			default: // Do nothing.
				break;
			}

			if (!_tools.post) {
				ImGui::NewLine();
				Editing::Tools::RotationsAndFlippings flipping = Editing::Tools::INVALID;
				unsigned mask = 0xffffffff;
				Math::Recti frame;
				area(&frame);
				if (frame.width() == 1 && frame.height() == 1) {
					mask &= ~(1 << Editing::Tools::ROTATE_CLOCKWISE);
					mask &= ~(1 << Editing::Tools::ROTATE_ANTICLOCKWISE);
					mask &= ~(1 << Editing::Tools::HALF_TURN);
					mask &= ~(1 << Editing::Tools::FLIP_VERTICALLY);
					mask &= ~(1 << Editing::Tools::FLIP_HORIZONTALLY);
				} else if (frame.width() != frame.height()) {
					mask &= ~(1 << Editing::Tools::ROTATE_CLOCKWISE);
					mask &= ~(1 << Editing::Tools::ROTATE_ANTICLOCKWISE);
				}
				if (Editing::Tools::flippable(rnd, ws, &flipping, -1.0f, ws->canUseShortcuts(), mask)) {
					flip(flipping);
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
		_selection.clear();

		_ref.image = nullptr;
		_ref.texture = nullptr;
	}
	virtual void gainFocus(class Renderer* rnd, const class Project* project) override {
		_ref.fill(rnd, project);

		if (_object) {
			Map::Tiles tiles;
			_object->tiles(tiles);
			tiles.texture = _ref.texture;
			_object->tiles(&tiles);
		}
	}

private:
	void shortcuts(Window* /* wnd */, Renderer* /* rnd */, Workspace* ws) {
		const Editing::Shortcut caps(SDL_SCANCODE_UNKNOWN, false, false, false, false, true);
		const Editing::Shortcut num(SDL_SCANCODE_UNKNOWN, false, false, false, true, false);
		_ref.gridsVisible = caps.pressed();
		_ref.transparentBackbroundVisible = num.pressed();
		_tools.gridsVisible = caps.pressed();
		_tools.transparentBackbroundVisible = num.pressed();

		if (!ws->canUseShortcuts()) {
			if (_tools.post) {
				_tools.post();
				_tools.post = nullptr;
			}

			return;
		}

		const Editing::Shortcut shift(SDL_SCANCODE_UNKNOWN, false, true, false);
		const Editing::Shortcut alt(SDL_SCANCODE_UNKNOWN, false, false, true);
		if (shift.pressed(false)) {
			_ref.gridsVisible = true;
			_tools.gridsVisible = true;

			if (!_tools.post) {
				const Editing::Tools::Tools prevTool = _tools.painting;

				_tools.painting = Editing::Tools::MOVE;

				_tools.post = [&, prevTool] (void) -> void {
					_tools.painting = prevTool;
				};
			}

			return;
		} else if (shift.released()) {
			_ref.gridsVisible = caps.pressed();
			_tools.gridsVisible = caps.pressed();

			if (_tools.post) {
				_tools.post();
				_tools.post = nullptr;
			}
		}
		if (alt.pressed(false)) {
			if (!_tools.post) {
				const Editing::Tools::Tools prevTool = _tools.painting;

				_tools.painting = Editing::Tools::EYEDROPPER;

				_tools.post = [&, prevTool] (void) -> void {
					_tools.painting = prevTool;
				};
			}

			return;
		} else if (alt.released()) {
			if (_tools.post) {
				_tools.post();
				_tools.post = nullptr;
			}
		}

		const Editing::Shortcut esc(SDL_SCANCODE_ESCAPE);
		if (esc.pressed())
			_selection.clear();
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
			ImGui::Separator();
			if (ImGui::MenuItem(ws->theme()->menuEdit_SelectAll())) {
				post(Editable::SELECT_ALL);
			}

			ImGui::EndPopup();
		}
	}

	void refreshStatus(Window* /* wnd */, Renderer* /* rnd */, Workspace* ws, const Editing::Tiler &cursor) {
		if (_status.cursor == cursor)
			return;

		_status.cursor = cursor;

		if (_status.cursor.position.x == -1 || _status.cursor.position.y == -1) {
			_status.text = ws->theme()->statusItem_Pos();
			_status.text += " ";
			_status.text += ws->theme()->generic_None();
		} else {
			_status.text = ws->theme()->statusItem_Pos();
			_status.text += " ";
			_status.text += Text::toString(_status.cursor.position.x);
			_status.text += ", ";
			_status.text += Text::toString(_status.cursor.position.y);

			_status.text += "  ";

			_status.text += ws->theme()->statusItem_Index();
			_status.text += " ";
			const int idx = _object->get(_status.cursor.position.x, _status.cursor.position.y);
			_status.text += Text::toString(idx);
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
		ImGui::TextUnformatted(_status.text);
		if (actived)
			ImGui::PopStyleColor();
	}
	void showRefStatus(Window* /* wnd */, Renderer* /* rnd */, Workspace* ws, const Editing::Brush &cursor) {
		ImGuiStyle &style = ImGui::GetStyle();

		if (!_ref.image)
			return;
		if (cursor.position.x == -1 || cursor.position.y == -1)
			return;

		const Int w = _ref.image->width() / _tileSize.x;
		const Int x = cursor.position.x / _tileSize.x;
		const Int y = cursor.position.y / _tileSize.y;
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

	void flip(Editing::Tools::RotationsAndFlippings f) {
		if (!_binding.getCel || !_binding.setCel)
			return;

		Math::Recti frame;
		const int size = area(&frame);
		if (size == 0)
			return;

		switch (f) {
		case Editing::Tools::ROTATE_CLOCKWISE:
			_commands->enqueue<Commands::Map::Rotate>()
				->with(_binding.getCel, _binding.setCel)
				->with(1)
				->with(frame)
				->exec(_object);

			break;
		case Editing::Tools::ROTATE_ANTICLOCKWISE:
			_commands->enqueue<Commands::Map::Rotate>()
				->with(_binding.getCel, _binding.setCel)
				->with(-1)
				->with(frame)
				->exec(_object);

			break;
		case Editing::Tools::HALF_TURN:
			_commands->enqueue<Commands::Map::Rotate>()
				->with(_binding.getCel, _binding.setCel)
				->with(2)
				->with(frame)
				->exec(_object);

			break;
		case Editing::Tools::FLIP_VERTICALLY:
			_commands->enqueue<Commands::Map::Flip>()
				->with(_binding.getCel, _binding.setCel)
				->with(1)
				->with(frame)
				->exec(_object);

			break;
		case Editing::Tools::FLIP_HORIZONTALLY:
			_commands->enqueue<Commands::Map::Flip>()
				->with(_binding.getCel, _binding.setCel)
				->with(0)
				->with(frame)
				->exec(_object);

			break;
		default: // Do nothing.
			break;
		}
	}

	void createOverlay(void) {
		_overlay = std::bind(&EditorMapImpl::getOverlayCel, this, std::placeholders::_1);
	}
	void destroyOverlay(void) {
		if (_overlay)
			_overlay = nullptr;
	}

	void moveToolDown(Renderer*) {
		_tools.mousePos = ImGui::GetMousePos();
	}
	void moveToolMove(Renderer*) {
		const ImVec2 pos = ImGui::GetMousePos();
		_tools.mouseDiff = pos - _tools.mousePos;
		_tools.mousePos = pos;
	}
	void moveToolUp(Renderer*) {
		_tools.mouseDiff = ImVec2(0, 0);
	}
	void moveToolHover(Renderer*) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);

		if (_tools.mouseDiff.x != 0) {
			ImGui::SetScrollX(ImGui::GetScrollX() - _tools.mouseDiff.x);
			_tools.mouseDiff.x = 0;
		}
		if (_tools.mouseDiff.y != 0) {
			ImGui::SetScrollY(ImGui::GetScrollY() - _tools.mouseDiff.y);
			_tools.mouseDiff.y = 0;
		}
	}

	void eyedropperToolDown(Renderer*) {
		const int idx = _object->get(_cursor.position.x, _cursor.position.y);
		if (idx == Map::INVALID())
			return;

		if (_tileSize.x <= 0)
			return;

		const std::div_t div = std::div(idx, (int)(_ref.image->width() / _tileSize.x));
		_ref.cursor.position = Math::Vec2i(div.rem * _tileSize.x, div.quot * _tileSize.y);
		_ref.cursor.size = _tileSize;
	}
	void eyedropperToolMove(Renderer*) {
		const int idx = _object->get(_cursor.position.x, _cursor.position.y);
		if (idx == Map::INVALID())
			return;

		if (_tileSize.x <= 0)
			return;

		const std::div_t div = std::div(idx, (int)(_ref.image->width() / _tileSize.x));
		_ref.cursor.position = Math::Vec2i(div.rem * _tileSize.x, div.quot * _tileSize.y);
		_ref.cursor.size = _tileSize;
	}

	template<typename T> void paintbucketToolUp_(Renderer* rnd, const Math::Recti &sel) {
		T* cmd = _commands->enqueue<T>();
		cmd->with(
			std::bind(&EditorMapImpl::getCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
			std::bind(&EditorMapImpl::setCel, this, rnd, std::placeholders::_1, std::placeholders::_2)
		);

		const int idx = _ref.cursor.unit().x + _ref.cursor.unit().y * (_ref.image->width() / _tileSize.x);

		cmd->with(
			Math::Vec2i(_object->width(), _object->height()),
			_selection.tiler.empty() ? nullptr : &sel,
			_cursor.position, idx
		);

		cmd->exec(_object);
	}
	void paintbucketToolUp(Renderer* rnd) {
		Math::Recti sel;
		if (_selection.area(sel) && !Math::intersects(sel, _cursor.position))
			_selection.clear();

		const Editing::Shortcut ctrl(SDL_SCANCODE_UNKNOWN, true, false, false);

		if (ctrl.pressed())
			paintbucketToolUp_<Commands::Map::Replace>(rnd, sel);
		else
			paintbucketToolUp_<Commands::Map::Fill>(rnd, sel);
	}

	void lassoToolDown(Renderer*) {
		_selection.mouse = ImGui::GetMousePos();

		_selection.initial = _cursor.position;
		_selection.tiler.position = _cursor.position;
		_selection.tiler.size = Math::Vec2i(1, 1);
	}
	void lassoToolMove(Renderer*) {
		const Math::Vec2i diff = _cursor.position - _selection.initial + Math::Vec2i(1, 1);

		const Math::Recti rect = Math::Recti::byXYWH(_selection.initial.x, _selection.initial.y, diff.x, diff.y);
		_selection.tiler.position = Math::Vec2i(rect.xMin(), rect.yMin());
		_selection.tiler.size = Math::Vec2i(rect.width(), rect.height());
	}
	void lassoToolUp(Renderer*) {
		const ImVec2 diff = ImGui::GetMousePos() - _selection.mouse;

		if (std::abs(diff.x) < Math::EPSILON<float>() && std::abs(diff.y) < Math::EPSILON<float>()) {
			if (_selection.idle == 0) {
				_selection.clear();
				++_selection.idle;
			} else {
				_selection.idle = 0;
			}
		}
	}

	void stampToolDown(Renderer*) {
		_selection.clear();
	}
	void stampToolUp(Renderer*) {
		if (!_binding.getCel || !_binding.setCel)
			return;

		const Math::Recti cursor = Math::Recti::byXYWH(
			_ref.cursor.position.x / _tileSize.x, _ref.cursor.position.y / _tileSize.y,
			_ref.cursor.size.x / _tileSize.x, _ref.cursor.size.y / _tileSize.y
		);
		const Math::Recti area = Math::Recti::byXYWH(0, 0, cursor.width(), cursor.height());
		Editing::Dot::Array dots;
		for (int j = cursor.yMin(); j <= cursor.yMax(); ++j) {
			for (int i = cursor.xMin(); i <= cursor.xMax(); ++i) {
				const int idx = i + j * (_ref.image->width() / _tileSize.x);
				Editing::Dot dot;
				dot.indexed = idx;
				dots.push_back(dot);
			}
		}

		const int xOff = -area.width() / 2;
		const int yOff = -area.height() / 2;
		const int x = _cursor.position.x + area.xMin() + xOff;
		const int y = _cursor.position.y + area.yMin() + yOff;

		_commands->enqueue<Commands::Map::Stamp>()
			->with(_binding.getCel, _binding.setCel)
			->with(Math::Recti::byXYWH(x, y, area.width(), area.height()), dots)
			->exec(_object);

		destroyOverlay();
	}
	void stampToolHover(Renderer*) {
		if (_overlay && !_painting.moved())
			return;

		const Math::Recti cursor = Math::Recti::byXYWH(
			_ref.cursor.position.x / _tileSize.x, _ref.cursor.position.y / _tileSize.y,
			_ref.cursor.size.x / _tileSize.x, _ref.cursor.size.y / _tileSize.y
		);
		const Math::Recti area = Math::Recti::byXYWH(0, 0, cursor.width(), cursor.height());
		Editing::Dot::Array dots;
		for (int j = cursor.yMin(); j <= cursor.yMax(); ++j) {
			for (int i = cursor.xMin(); i <= cursor.xMax(); ++i) {
				const int idx = i + j * (_ref.image->width() / _tileSize.x);
				Editing::Dot dot;
				dot.indexed = idx;
				dots.push_back(dot);
			}
		}

		const int xOff = -area.width() / 2;
		const int yOff = -area.height() / 2;
		const int x = _cursor.position.x + area.xMin() + xOff;
		const int y = _cursor.position.y + area.yMin() + yOff;

		_overlay = std::bind(
			[] (const Math::Recti area, const Editing::Dot::Array dots, const Math::Vec2i offset, const Math::Vec2i &pos) -> int {
				const Math::Vec2i position = pos - offset;
				if (!Math::intersects(area, position))
					return Map::INVALID();

				return dots[position.x + position.y * area.width()].indexed;
			},
			area, dots, Math::Vec2i(x, y), std::placeholders::_1
		);
	}

	void stampToolUp_Paste(Renderer*, const Math::Recti &area, const Editing::Dot::Array &dots, Editing::Tools::Tools prevTool) {
		if (!_binding.getCel || !_binding.setCel)
			return;

		const int xOff = -area.width() / 2;
		const int yOff = -area.height() / 2;
		const int x = _cursor.position.x + area.xMin() + xOff;
		const int y = _cursor.position.y + area.yMin() + yOff;

		_commands->enqueue<Commands::Map::Paste>()
			->with(_binding.getCel, _binding.setCel)
			->with(Math::Recti::byXYWH(x, y, area.width(), area.height()), dots)
			->exec(_object);

		_processors[Editing::Tools::STAMP] = Processor{
			nullptr,
			nullptr,
			std::bind(&EditorMapImpl::stampToolUp, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::stampToolHover, this, std::placeholders::_1)
		};

		destroyOverlay();

		_tools.painting = prevTool;
	}
	void stampToolHover_Paste(Renderer*, const Math::Recti &area, const Editing::Dot::Array &dots) {
		if (_overlay && !_painting.moved())
			return;

		const int xOff = -area.width() / 2;
		const int yOff = -area.height() / 2;
		const int x = _cursor.position.x + area.xMin() + xOff;
		const int y = _cursor.position.y + area.yMin() + yOff;

		_overlay = std::bind(
			[] (const Math::Recti area, const Editing::Dot::Array dots, const Math::Vec2i offset, const Math::Vec2i &pos) -> int {
				const Math::Vec2i position = pos - offset;
				if (!Math::intersects(area, position))
					return Map::INVALID();

				return dots[position.x + position.y * area.width()].indexed;
			},
			area, dots, Math::Vec2i(x, y), std::placeholders::_1
		);
	}

	template<typename T> void paintToolDown(Renderer* rnd) {
		_selection.clear();

		_commands->enqueue<T>()
			->with(
				std::bind(&EditorMapImpl::getCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorMapImpl::setCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				_tools.weighting + 1
			)
			->exec(_object);

		createOverlay();

		paintToolMove<T>(rnd);
	}
	template<typename T> void paintToolMove(Renderer*) {
		if (_commands->empty())
			return;

		Command* back = _commands->back();
		assert(back->type() == T::TYPE());
		T* cmd = Command::as<T>(back);
		const int idx = _ref.cursor.unit().x + _ref.cursor.unit().y * (_ref.image->width() / _tileSize.x);

		cmd->with(
			Math::Vec2i(_object->width(), _object->height()),
			_cursor.position, idx,
			nullptr
		);
	}
	void paintToolUp(Renderer*) {
		if (!_commands->empty())
			_commands->back()->redo(_object);

		destroyOverlay();
	}

	int getOverlayCel(const Math::Vec2i &pos) const {
		Command* back = _commands->back();
		switch (back->type()) {
		case Commands::Map::Pencil::TYPE(): // Fall through.
		case Commands::Map::Line::TYPE(): // Fall through.
		case Commands::Map::Box::TYPE(): // Fall through.
		case Commands::Map::BoxFill::TYPE(): // Fall through.
		case Commands::Map::Ellipse::TYPE(): // Fall through.
		case Commands::Map::EllipseFill::TYPE(): {
				Commands::Paintable::Paint* cmd = Command::as<Commands::Paintable::Paint>(back);
				const Editing::Point key(pos);
				Editing::Point::Set::iterator it = cmd->points().find(key);
				if (it == cmd->points().end())
					return Map::INVALID();

				return it->dot.indexed;
			}
		default:
			return Map::INVALID();
		}
	}

	int getCels(Renderer*, const Math::Recti* area /* nullable */, Editing::Dots &dots) const {
		int result = 0;

		const Math::Recti area_ = area ? *area : Math::Recti::byXYWH(0, 0, _object->width(), _object->height());
		int k = 0;
		for (int j = area_.yMin(); j <= area_.yMax(); ++j) {
			for (int i = area_.xMin(); i <= area_.xMax(); ++i) {
				const int idx = _object->get(i, j);
				dots.indexed[k] = idx;
				++k;
				++result;
			}
		}

		return result;
	}
	int setCels(Renderer*, const Math::Recti* area /* nullable */, const Editing::Dots &dots) {
		int result = 0;

		const Math::Recti area_ = area ? *area : Math::Recti::byXYWH(0, 0, _object->width(), _object->height());
		int k = 0;
		for (int j = area_.yMin(); j <= area_.yMax(); ++j) {
			for (int i = area_.xMin(); i <= area_.xMax(); ++i) {
				const int idx = dots.indexed[k];
				_object->set(i, j, idx);
				++k;
				++result;
			}
		}

		return result;
	}
	bool getCel(Renderer*, const Math::Vec2i &pos, Editing::Dot &dot) const {
		const int idx = _object->get(pos.x, pos.y);
		if (idx == Map::INVALID())
			return false;

		dot.indexed = idx;

		return true;
	}
	bool setCel(Renderer*, const Math::Vec2i &pos, const Editing::Dot &dot) {
		return _object->set(pos.x, pos.y, dot.indexed);
	}
};

EditorMap* EditorMap::create(void) {
	EditorMapImpl* result = new EditorMapImpl();

	return result;
}

void EditorMap::destroy(EditorMap* ptr) {
	EditorMapImpl* impl = static_cast<EditorMapImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
