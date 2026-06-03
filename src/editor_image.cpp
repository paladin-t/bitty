/*
** Bitty
**
** An itty bitty 2D game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "commands_image.h"
#include "editing.h"
#include "editor_image.h"
#include "encoding.h"
#include "image.h"
#include "platform.h"
#include "theme.h"
#include "workspace.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../lib/imgui/imgui_internal.h"
#include "../lib/jpath/jpath.hpp"
#include <SDL.h>

/*
** {===========================================================================
** Macros and constants
*/

constexpr const int MAGNIFICATIONS[] = {
	1, 4, 8, 16
};

/* ===========================================================================} */

/*
** {===========================================================================
** Image editor
*/

namespace Bitty {

class EditorImageImpl : public EditorImage {
private:
	struct Pixels {
		Color colored[EDITING_ITEM_COUNT_PER_LINE * 2];
	};

	typedef std::function<bool(const Math::Vec2i &, Editing::Dot &)> PixelGetter;
	typedef std::function<bool(const Math::Vec2i &, const Editing::Dot &)> PixelSetter;

	typedef std::function<int(const Math::Recti* /* nullable */, Editing::Dots &)> PixelsGetter;
	typedef std::function<int(const Math::Recti* /* nullable */, const Editing::Dots &)> PixelsSetter;

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
	Image::Ptr _object = nullptr;
	Texture::Ptr _texture = nullptr;
	CommandQueue* _commands = nullptr;
	Editing::Data::Checkpoint _checkpoint;

	Editing::Brush _cursor;
	struct {
		Math::Vec2i initial = Math::Vec2i(-1, -1);
		Editing::Brush brush;

		ImVec2 mouse = ImVec2(-1, -1);
		int idle = 0;

		void clear(void) {
			initial = Math::Vec2i(-1, -1);
			brush = Editing::Brush();

			mouse = ImVec2(-1, -1);
			idle = 0;
		}

		int area(Math::Recti &area) const {
			if (brush.empty())
				return 0;

			area = Math::Recti::byXYWH(brush.position.x, brush.position.y, brush.size.x, brush.size.y);

			return area.width() * area.height();
		}
	} _selection;
	struct {
		std::string text = "Image";
		Editing::Brush cursor;

		void clear(void) {
			text = "Image";
			cursor = Editing::Brush();
		}
	} _status;
	Editing::Painting _painting;
	struct {
		PixelGetter getPixel = nullptr;
		PixelSetter setPixel = nullptr;
		PixelsGetter getPixels = nullptr;
		PixelsSetter setPixels = nullptr;

		bool skipFrame = false;

		bool empty(void) const {
			return !getPixel || !setPixel || !getPixels || !setPixels;
		}
		void fill(PixelGetter getPixel_, PixelSetter setPixel_, PixelsGetter getPixels_, PixelsSetter setPixels_) {
			getPixel = getPixel_;
			setPixel = setPixel_;
			getPixels = getPixels_;
			setPixels = setPixels_;

			skipFrame = false;
		}
		void clear(void) {
			getPixel = nullptr;
			setPixel = nullptr;
			getPixels = nullptr;
			setPixels = nullptr;
		}
	} _binding;
	struct {
		Image::Ptr blank = nullptr;
		Texture::Ptr texture = nullptr;

		bool empty(void) const {
			return !texture;
		}
		void clear(void) {
			blank = nullptr;
			texture = nullptr;
		}
	} _overlay;

	struct Ref : public Editor::Ref {
		std::string name;
		int palette = -1;
		float real[4] = { 1, 1, 1, 1 };
		Pixels* latest = nullptr;

		void clear(void) {
			name.clear();
			palette = -1;
			for (int i = 0; i < BITTY_COUNTOF(real); ++i)
				real[i] = 1.0f;
			if (latest) {
				delete latest;
				latest = nullptr;
			}
		}
	} _ref;
	struct Tools {
		bool focused = false;

		Editing::Tools::Tools painting = Editing::Tools::PENCIL;
		int magnification = 0;
		int weighting = 0;

		bool gridsVisible = false;
		Math::Vec2i gridUnit = Math::Vec2i(0, 0);
		bool transparentBackbroundVisible = true;

		ImVec2 mousePos = ImVec2(-1, -1);
		ImVec2 mouseDiff = ImVec2(0, 0);
		int mouseActionButton = ImGuiMouseButton_Left;

		PostHandler post = nullptr;
		Editing::Tools::Tools postType = Editing::Tools::PENCIL;

		void clear(void) {
			focused = false;

			painting = Editing::Tools::PENCIL;
			magnification = 0;
			weighting = 0;

			gridsVisible = false;
			gridUnit = Math::Vec2i(0, 0);
			transparentBackbroundVisible = true;

			mousePos = ImVec2(-1, -1);
			mouseDiff = ImVec2(0, 0);
			mouseActionButton = ImGuiMouseButton_Left;

			post = nullptr;
			postType = Editing::Tools::PENCIL;
		}
	} _tools;
	Processor _processors[Editing::Tools::COUNT] = {
		Processor{ // Move.
			std::bind(&EditorImageImpl::moveToolDown, this, std::placeholders::_1),
			std::bind(&EditorImageImpl::moveToolMove, this, std::placeholders::_1),
			std::bind(&EditorImageImpl::moveToolUp, this, std::placeholders::_1),
			std::bind(&EditorImageImpl::moveToolHover, this, std::placeholders::_1)
		},
		Processor{ // Eyedropper.
			std::bind(&EditorImageImpl::eyedropperToolDown, this, std::placeholders::_1),
			std::bind(&EditorImageImpl::eyedropperToolMove, this, std::placeholders::_1),
			nullptr,
			nullptr
		},
		Processor{ // Pencil.
			std::bind(&EditorImageImpl::paintToolDown<Commands::Image::Pencil>, this, std::placeholders::_1),
			std::bind(&EditorImageImpl::paintToolMove<Commands::Image::Pencil, false>, this, std::placeholders::_1),
			std::bind(&EditorImageImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		},
		Processor{ // Paintbucket.
			nullptr,
			nullptr,
			std::bind(&EditorImageImpl::paintbucketToolUp, this, std::placeholders::_1),
			nullptr
		},
		Processor{ // Lasso.
			std::bind(&EditorImageImpl::lassoToolDown, this, std::placeholders::_1),
			std::bind(&EditorImageImpl::lassoToolMove, this, std::placeholders::_1),
			std::bind(&EditorImageImpl::lassoToolUp, this, std::placeholders::_1),
			nullptr
		},
		Processor{ // Line.
			std::bind(&EditorImageImpl::paintToolDown<Commands::Image::Line>, this, std::placeholders::_1),
			std::bind(&EditorImageImpl::paintToolMove<Commands::Image::Line>, this, std::placeholders::_1),
			std::bind(&EditorImageImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		},
		Processor{ // Box.
			std::bind(&EditorImageImpl::paintToolDown<Commands::Image::Box>, this, std::placeholders::_1),
			std::bind(&EditorImageImpl::paintToolMove<Commands::Image::Box>, this, std::placeholders::_1),
			std::bind(&EditorImageImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		},
		Processor{ // Box fill.
			std::bind(&EditorImageImpl::paintToolDown<Commands::Image::BoxFill>, this, std::placeholders::_1),
			std::bind(&EditorImageImpl::paintToolMove<Commands::Image::BoxFill>, this, std::placeholders::_1),
			std::bind(&EditorImageImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		},
		Processor{ // Ellipse.
			std::bind(&EditorImageImpl::paintToolDown<Commands::Image::Ellipse>, this, std::placeholders::_1),
			std::bind(&EditorImageImpl::paintToolMove<Commands::Image::Ellipse>, this, std::placeholders::_1),
			std::bind(&EditorImageImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		},
		Processor{ // Ellipse fill.
			std::bind(&EditorImageImpl::paintToolDown<Commands::Image::EllipseFill>, this, std::placeholders::_1),
			std::bind(&EditorImageImpl::paintToolMove<Commands::Image::EllipseFill>, this, std::placeholders::_1),
			std::bind(&EditorImageImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		},
		Processor{ // Pasting (stamp).
			nullptr,
			nullptr,
			nullptr,
			nullptr
		}
	};

public:
	EditorImageImpl() {
		_commands = new CommandQueue();
		_commands->registerFactory<Commands::Image::Pencil>();
		_commands->registerFactory<Commands::Image::Line>();
		_commands->registerFactory<Commands::Image::Box>();
		_commands->registerFactory<Commands::Image::BoxFill>();
		_commands->registerFactory<Commands::Image::Ellipse>();
		_commands->registerFactory<Commands::Image::EllipseFill>();
		_commands->registerFactory<Commands::Image::Fill>();
		_commands->registerFactory<Commands::Image::Replace>();
		_commands->registerFactory<Commands::Image::Rotate>();
		_commands->registerFactory<Commands::Image::Flip>();
		_commands->registerFactory<Commands::Image::Cut>();
		_commands->registerFactory<Commands::Image::Paste>();
		_commands->registerFactory<Commands::Image::Delete>();
		_commands->registerFactory<Commands::Image::Resize>();

		_checkpoint.fill();
	}
	virtual ~EditorImageImpl() override {
		close(nullptr);

		delete _commands;
		_commands = nullptr;
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual void open(const class Project* project, const char* name, Object::Ptr obj, const char* ref) override {
		if (_opened)
			return;
		_opened = true;

		_name = name;

		_object = Object::as<Image::Ptr>(obj);
		Editing::Data::toCheckpoint(project, _name.c_str(), _checkpoint);

		_ref.name = ref ? ref : "";

		const Editing::Shortcut caps(SDL_SCANCODE_UNKNOWN, false, false, false, false, true);
		const Editing::Shortcut num(SDL_SCANCODE_UNKNOWN, false, false, false, true, false);
		_tools.gridsVisible = caps.pressed();
		_tools.gridUnit = Math::Vec2i(BITTY_GRID_DEFAULT_SIZE, BITTY_GRID_DEFAULT_SIZE);
		_tools.transparentBackbroundVisible = num.pressed();

		fprintf(stdout, "Image editor opened: \"%s\".\n", _name.c_str());
	}
	virtual void close(const class Project* project) override {
		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "Image editor closed: \"%s\".\n", _name.c_str());

		if (!_checkpoint.empty()) {
			if (hasUnsavedChanges())
				Editing::Data::fromCheckpoint(project, _name.c_str(), _checkpoint);
			_checkpoint.clear();
		}

		_tools.clear();

		_ref.clear();

		_overlay.clear();
		_binding.clear();
		_painting.clear();
		_status.clear();
		_selection.clear();

		_texture = nullptr;
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

		auto toString = [] (const Image::Ptr obj, const Math::Recti &area, const Editing::Dots &dots) -> std::string {
			rapidjson::Document doc;
			Jpath::set(doc, doc, area.width(), "width");
			Jpath::set(doc, doc, area.height(), "height");
			Jpath::set(doc, doc, obj->paletted() ? 8 : 0, "depth");
			int k = 0;
			for (int j = area.yMin(); j <= area.yMax(); ++j) {
				for (int i = area.xMin(); i <= area.xMax(); ++i) {
					if (obj->paletted())
						Jpath::set(doc, doc, dots.indexed[k], "data", k);
					else
						Jpath::set(doc, doc, dots.colored[k].toRGBA(), "data", k);

					++k;
				}
			}

			std::string buf;
			Json::toString(doc, buf);

			return buf;
		};

		if (!_binding.getPixels || !_binding.setPixels)
			return;

		Math::Recti sel;
		const Math::Recti* selPtr = nullptr;
		int size = _selection.area(sel);
		if (size == 0)
			size = _object->width() * _object->height();
		else
			selPtr = &sel;

		if (_object->paletted()) {
			std::vector<int> vec(size, 0);
			Editing::Dots dots(&vec.front());
			_binding.getPixels(selPtr, dots);

			const std::string buf = toString(_object, sel, dots);
			const std::string osstr = Unicode::toOs(buf);

			Platform::clipboardText(osstr.c_str());
		} else {
			std::vector<Color> vec(size, Color());
			Editing::Dots dots(&vec.front());
			_binding.getPixels(selPtr, dots);

			const std::string buf = toString(_object, sel, dots);
			const std::string osstr = Unicode::toOs(buf);

			Platform::clipboardText(osstr.c_str());
		}
	}
	virtual void cut(void) override {
		if (_tools.focused)
			return;

		if (!_binding.getPixel || !_binding.setPixel)
			return;

		copy();

		Math::Recti sel;
		const int size = area(&sel);
		if (size == 0)
			return;

		_commands->enqueue<Commands::Image::Cut>()
			->with(_binding.getPixel, _binding.setPixel)
			->with(sel)
			->exec(_object);
	}
	virtual bool pastable(void) const override {
		return Platform::hasClipboardText();
	}
	virtual void paste(void) override {
		if (_tools.focused)
			return;

		auto fromString = [] (const Image::Ptr obj, const std::string &buf, Math::Recti &area, Editing::Dot::Array &dots) -> bool {
			rapidjson::Document doc;
			if (!Json::fromString(doc, buf.c_str()))
				return false;

			int width = -1, height = -1;
			int depth = -1;
			if (!Jpath::get(doc, width, "width"))
				return false;
			if (!Jpath::get(doc, height, "height"))
				return false;
			if (!Jpath::get(doc, depth, "depth"))
				return false;
			area = Math::Recti::byXYWH(0, 0, width, height);
			if (obj->paletted() && depth != 8)
				return false;
			if (!obj->paletted() && depth != 0)
				return false;
			int k = 0;
			for (int j = area.yMin(); j <= area.yMax(); ++j) {
				for (int i = area.xMin(); i <= area.xMax(); ++i) {
					if (obj->paletted()) {
						int idx = -1;
						if (!Jpath::get(doc, idx, "data", k))
							return false;

						Editing::Dot dot;
						dot.indexed = idx;
						dots.push_back(dot);
					} else {
						UInt32 col = 0;
						if (!Jpath::get(doc, col, "data", k))
							return false;

						Editing::Dot dot;
						dot.colored.fromRGBA(col);
						dots.push_back(dot);
					}

					++k;
				}
			}

			return true;
		};

		const std::string osstr = Platform::clipboardText();
		const std::string buf = Unicode::fromOs(osstr);
		Math::Recti area;
		Editing::Dot::Array dots;
		if (!fromString(_object, buf, area, dots))
			return;

		const Editing::Tools::Tools prevTool = _tools.painting;

		_tools.painting = Editing::Tools::STAMP;

		_processors[Editing::Tools::STAMP] = Processor{
			nullptr,
			nullptr,
			std::bind(&EditorImageImpl::stampToolUp_Paste, this, std::placeholders::_1, area, dots, prevTool),
			std::bind(&EditorImageImpl::stampToolHover_Paste, this, std::placeholders::_1, area, dots)
		};

		destroyOverlay();
	}
	virtual void del(void) override {
		if (_tools.focused)
			return;

		if (!_binding.getPixel || !_binding.setPixel)
			return;

		Math::Recti sel;
		const int size = area(&sel);
		if (size == 0)
			return;

		_commands->enqueue<Commands::Image::Delete>()
			->with(_binding.getPixel, _binding.setPixel)
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
		_commands->redo(_object, &_texture);

		if (width != _object->width() || height != _object->height())
			_selection.clear();
	}
	virtual void undo(class Asset*) override {
		const int width = _object->width();
		const int height = _object->height();
		_commands->undo(_object, &_texture);

		if (width != _object->width() || height != _object->height())
			_selection.clear();
	}

	virtual Variant post(unsigned msg, int argc, const Variant* argv) override {
		switch (msg) {
		case MAGNIFICATION_CHANGED: {
				const bool fineZooming = unpack<bool>(argc, argv, 0, false);

				if (fineZooming) {
					_tools.magnification = Math::clamp(MAGNIFICATIONS[_tools.magnification], MAGNIFICATIONS[0], MAGNIFICATIONS[BITTY_COUNTOF(MAGNIFICATIONS) - 1]);
				} else {
					int idx = 0;
					for (int i = 0; i < (int)BITTY_COUNTOF(MAGNIFICATIONS); ++i) {
						if (_tools.magnification >= MAGNIFICATIONS[i])
							idx = i;
						else
							break;
					}
					_tools.magnification = Math::clamp(idx, 0, (int)BITTY_COUNTOF(MAGNIFICATIONS) - 1);
				}
			}

			return Variant(true);
		case RESIZE: {
				if (_object->paletted())
					return Variant(false);

				const Variant::Int width = unpack<Variant::Int>(argc, argv, 0, 0);
				const Variant::Int height = unpack<Variant::Int>(argc, argv, 1, 0);
				if (width == _object->width() && height == _object->height())
					return Variant(true);

				_commands->enqueue<Commands::Image::Resize>()
					->with(Math::Vec2i(width, height))
					->exec(_object, &_texture);

				_selection.clear();
			}

			return Variant(true);
		case RESIZE_GRID: {
				const Variant::Int width = unpack<Variant::Int>(argc, argv, 0, 0);
				const Variant::Int height = unpack<Variant::Int>(argc, argv, 1, 0);

				_tools.gridUnit = Math::Vec2i(width, height);
			}

			return Variant(true);
		case SELECT_ALL:
			_selection.brush.position = Math::Vec2i(0, 0);
			_selection.brush.size = Math::Vec2i(_object->width(), _object->height());

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
		const char* /* title */,
		float /* x */, float /* y */, float width, float height,
		int /* scale */,
		bool pending,
		double /* delta */
	) override {
		ImGuiStyle &style = ImGui::GetStyle();

		if (_binding.empty()) {
			_binding.fill(
				std::bind(&EditorImageImpl::getPixel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorImageImpl::setPixel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorImageImpl::getPixels, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorImageImpl::setPixels, this, rnd, std::placeholders::_1, std::placeholders::_2)
			);
		}

		const bool allowMouseOperations = shortcuts(wnd, rnd, ws);

		if (!_object)
			return;
		if (_object->paletted()) {
			Palette::Ptr ref = _object->palette();
			if (!ref)
				return;
		}

		const Splitter splitter = split();

		const float statusBarHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;
		bool statusBarActived = ImGui::IsWindowFocused();

		ImGui::BeginChild("@Img/Pat", ImVec2(splitter.first, height - statusBarHeight), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoNav);
		{
			if (!_texture || !_texture->pointer(rnd)) {
				_texture = Texture::Ptr(Texture::create());
				_texture->fromImage(rnd, Texture::STREAMING, _object.get(), Texture::NEAREST);
				_texture->blend(Texture::BLEND);
			}

			if (_texture) {
				Editing::Brush cursor = _cursor;
				const bool withCursor = _tools.painting != Editing::Tools::MOVE;

				if (ws->settings()->editorFineZoomingEnabled) {
					_tools.magnification = Math::clamp(_tools.magnification, MAGNIFICATIONS[0], MAGNIFICATIONS[BITTY_COUNTOF(MAGNIFICATIONS) - 1]);
				} else {
					_tools.magnification = Math::clamp(_tools.magnification, 0, (int)BITTY_COUNTOF(MAGNIFICATIONS) - 1);
				}
				const int scale = ws->settings()->editorFineZoomingEnabled ? _tools.magnification : MAGNIFICATIONS[_tools.magnification];
				_painting.set(
					Editing::image(
						rnd,
						ws,
						_object.get(), _texture.get(),
						(float)_object->width() * scale,
						withCursor ? &cursor : nullptr, false,
						_selection.brush.empty() ? nullptr : &_selection.brush,
						_overlay.texture ? _overlay.texture.get() : nullptr,
						&_tools.gridUnit, _tools.gridsVisible || ws->settings()->editorAlwaysShowGrids,
						_tools.transparentBackbroundVisible || ws->settings()->editorAlwaysShowTransparentBackground,
						_tools.mouseActionButton
					)
				);
				if (
					(_painting && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) ||
					_tools.painting == Editing::Tools::STAMP
				) {
					_cursor = cursor;
				}
				refreshStatus(wnd, rnd, ws, cursor);
			} else {
				_painting.set(false);
			}

			if (allowMouseOperations) {
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
			}

			if (_binding.skipFrame) {
				_binding.skipFrame = false;
				ws->skipFrame();
			}

			statusBarActived |= ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

			context(wnd, rnd, ws);
		}
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("@Img/Tls", ImVec2(splitter.second, height - statusBarHeight), true, _ref.windowFlags());
		{
			if (_object->paletted()) {
				ImGui::PushID("@Img/Tls/Ref");
				{
					const float curPosX = ImGui::GetCursorPosX();
					ImGui::AlignTextToFramePadding();
					ImGui::Dummy(ImVec2(4, 0));
					ImGui::SameLine();
					if (_object->palette())
						ImGui::TextUnformatted(ws->theme()->dialogItem_Ref());
					else
						ImGui::TextUnformatted("? ");
					ImGui::SameLine();
					ImGui::SetNextItemWidth(splitter.second - (ImGui::GetCursorPosX() - curPosX) - style.ChildBorderSize * 3);
					ImGui::InputText("", (char*)_ref.name.c_str(), _ref.name.length(), ImGuiInputTextFlags_ReadOnly);
				}
				ImGui::PopID();
			}

			const float spwidth = _ref.windowWidth(splitter.second);
			if (_object->paletted()) {
				Palette::Ptr palette = _object->palette();

				Editing::palette(rnd, ws, palette.get(), spwidth, nullptr, &_ref.palette, true, ws->theme()->iconTransparent());
			} else {
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
			}

			if (!_ref.latest) {
				if (!_object->paletted()) {
					_ref.latest = new Pixels();
					const Color white(255, 255, 255, 255);
					_ref.latest->colored[0] = Color(255, 0, 0, 255);
					_ref.latest->colored[1] = Color(0, 255, 0, 255);
					_ref.latest->colored[2] = Color(255, 255, 0, 255);
					_ref.latest->colored[3] = Color(0, 0, 255, 255);
					_ref.latest->colored[4] = Color(255, 0, 255, 255);
					_ref.latest->colored[5] = Color(0, 255, 255, 255);
					_ref.latest->colored[6] = Color(0, 0, 0, 255);
					_ref.latest->colored[7] = Color(255, 255, 255, 255);
					_ref.latest->colored[8] = Color(128, 128, 128, 255);
					_ref.latest->colored[9] = Color(255, 255, 255, 0);
				}
			}

			if (_ref.latest) {
				ImGui::NewLine();
				int coloring = -1;
				if (Editing::Tools::colorable(rnd, ws, _ref.latest->colored, &coloring, spwidth, ws->canUseShortcuts())) {
					const Color col = _ref.latest->colored[coloring];
					_ref.real[0] = Math::clamp(col.r / 255.0f, 0.0f, 1.0f);
					_ref.real[1] = Math::clamp(col.g / 255.0f, 0.0f, 1.0f);
					_ref.real[2] = Math::clamp(col.b / 255.0f, 0.0f, 1.0f);
					_ref.real[3] = Math::clamp(col.a / 255.0f, 0.0f, 1.0f);
				}
			}

			ImGui::NewLine();
			if (Editing::Tools::paintable(rnd, ws, &_tools.painting, spwidth, ws->canUseShortcuts(), 0xffffffff & ~(1 << Editing::Tools::STAMP))) {
				destroyOverlay();
			}

			ImGui::NewLine();
			if (ws->settings()->editorFineZoomingEnabled) {
				Editing::Tools::magnifiable(
					rnd, ws,
					&_tools.magnification,
					MAGNIFICATIONS[0], MAGNIFICATIONS[BITTY_COUNTOF(MAGNIFICATIONS) - 1],
					spwidth, true,
					nullptr
				);
			} else {
				Editing::Tools::magnifiable(rnd, ws, &_tools.magnification, spwidth, ws->canUseShortcuts());
			}

			switch (_tools.painting) {
			case Editing::Tools::PENCIL: // Fall through.
			case Editing::Tools::LINE: // Fall through.
			case Editing::Tools::BOX: // Fall through.
			case Editing::Tools::BOX_FILL: // Fall through.
			case Editing::Tools::ELLIPSE: // Fall through.
			case Editing::Tools::ELLIPSE_FILL:
				ImGui::NewLine();
				Editing::Tools::weighable(rnd, ws, &_tools.weighting, spwidth, ws->canUseShortcuts());

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
				if (Editing::Tools::flippable(rnd, ws, &flipping, spwidth, ws->canUseShortcuts(), mask)) {
					flip(flipping);
				}
			}

			_tools.focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows); // Ignore shortcuts when the window is not focused.
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
		if (_object)
			_object->pointer(nullptr);
		_texture = nullptr;

		_selection.clear();
	}
	virtual void gainFocus(class Renderer* /* rnd */, const class Project* /* project */) override {
		// Do nothing.
	}

private:
	bool shortcuts(Window* /* wnd */, Renderer* /* rnd */, Workspace* ws) {
		const Editing::Shortcut caps(SDL_SCANCODE_UNKNOWN, false, false, false, false, true);
		const Editing::Shortcut num(SDL_SCANCODE_UNKNOWN, false, false, false, true, false);
		_tools.gridsVisible = caps.pressed();
		_tools.transparentBackbroundVisible = num.pressed();

		if (!ws->canUseShortcuts()) {
			if (_tools.post) {
				_tools.post();
				_tools.post = nullptr;
			}

			return true;
		}

		const Editing::Shortcut shift(SDL_SCANCODE_UNKNOWN, false, true, false);
		const Editing::Shortcut alt(SDL_SCANCODE_UNKNOWN, false, false, true);
		if (shift.pressed(false)) {
			_tools.gridsVisible = true;

			if (!_tools.post) {
				const Editing::Tools::Tools prevTool = _tools.painting;

				_tools.painting = Editing::Tools::MOVE;

				_tools.post = [&, prevTool] (void) -> void {
					_tools.painting = prevTool;
				};
				_tools.postType = Editing::Tools::MOVE;
			}

			return true;
		} else if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
			if (!_tools.post) {
				const Editing::Tools::Tools prevTool = _tools.painting;

				_tools.painting = Editing::Tools::MOVE;

				_tools.mouseActionButton = ImGuiMouseButton_Middle;

				_tools.post = [&, prevTool] (void) -> void {
					if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
						_tools.painting = prevTool;
						_tools.mouseActionButton = ImGuiMouseButton_Left;
					}
				};
				_tools.postType = Editing::Tools::MOVE;
			}

			return true;
		} else if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle)) {
			if (_tools.post && _tools.postType == Editing::Tools::MOVE) {
				_tools.post();
				_tools.post = nullptr;

				return false;
			}
		} else if (shift.released()) {
			_tools.gridsVisible = caps.pressed();

			if (_tools.post && _tools.postType == Editing::Tools::MOVE) {
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
				_tools.postType = Editing::Tools::EYEDROPPER;
			}

			return true;
		} else if (alt.released()) {
			if (_tools.post && _tools.postType == Editing::Tools::EYEDROPPER) {
				_tools.post();
				_tools.post = nullptr;
			}
		}

		const Editing::Shortcut esc(SDL_SCANCODE_ESCAPE);
		if (esc.pressed())
			_selection.clear();

		return true;
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

	void refreshStatus(Window* /* wnd */, Renderer* /* rnd */, Workspace* ws, const Editing::Brush &cursor) {
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

	void flip(Editing::Tools::RotationsAndFlippings f) {
		if (!_binding.getPixel || !_binding.setPixel)
			return;

		Math::Recti frame;
		const int size = area(&frame);
		if (size == 0)
			return;

		switch (f) {
		case Editing::Tools::ROTATE_CLOCKWISE:
			_commands->enqueue<Commands::Image::Rotate>()
				->with(_binding.getPixel, _binding.setPixel)
				->with(1)
				->with(frame)
				->exec(_object);

			break;
		case Editing::Tools::ROTATE_ANTICLOCKWISE:
			_commands->enqueue<Commands::Image::Rotate>()
				->with(_binding.getPixel, _binding.setPixel)
				->with(-1)
				->with(frame)
				->exec(_object);

			break;
		case Editing::Tools::HALF_TURN:
			_commands->enqueue<Commands::Image::Rotate>()
				->with(_binding.getPixel, _binding.setPixel)
				->with(2)
				->with(frame)
				->exec(_object);

			break;
		case Editing::Tools::FLIP_VERTICALLY:
			_commands->enqueue<Commands::Image::Flip>()
				->with(_binding.getPixel, _binding.setPixel)
				->with(1)
				->with(frame)
				->exec(_object);

			break;
		case Editing::Tools::FLIP_HORIZONTALLY:
			_commands->enqueue<Commands::Image::Flip>()
				->with(_binding.getPixel, _binding.setPixel)
				->with(0)
				->with(frame)
				->exec(_object);

			break;
		default: // Do nothing.
			break;
		}
	}

	void createOverlay(Renderer* rnd) {
		_overlay.blank = Image::Ptr(Image::create(nullptr));
		_overlay.blank->fromBlank(_object->width(), _object->height(), 0);
		_overlay.texture = Texture::Ptr(Texture::create());
		_overlay.texture->fromImage(rnd, Texture::STREAMING, _overlay.blank.get(), Texture::NEAREST);
		_overlay.texture->blend(Texture::BLEND);
	}
	void destroyOverlay(void) {
		_overlay.clear();
	}
	void clearOverlay(Renderer* rnd) {
		_overlay.texture->fromImage(rnd, Texture::STREAMING, _overlay.blank.get(), Texture::NEAREST);
		_overlay.texture->blend(Texture::BLEND);
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
		_tools.mouseActionButton = ImGuiMouseButton_Left;
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
		if (_object->paletted()) {
			int idx = -1;
			_object->get(_cursor.position.x, _cursor.position.y, idx);
			_ref.palette = idx;
		} else {
			Color col;
			_object->get(_cursor.position.x, _cursor.position.y, col);
			_ref.real[0] = Math::clamp(col.r / 255.0f, 0.0f, 1.0f);
			_ref.real[1] = Math::clamp(col.g / 255.0f, 0.0f, 1.0f);
			_ref.real[2] = Math::clamp(col.b / 255.0f, 0.0f, 1.0f);
			_ref.real[3] = Math::clamp(col.a / 255.0f, 0.0f, 1.0f);
		}
	}
	void eyedropperToolMove(Renderer*) {
		if (_object->paletted()) {
			int idx = -1;
			_object->get(_cursor.position.x, _cursor.position.y, idx);
			_ref.palette = idx;
		} else {
			Color col;
			_object->get(_cursor.position.x, _cursor.position.y, col);
			_ref.real[0] = Math::clamp(col.r / 255.0f, 0.0f, 1.0f);
			_ref.real[1] = Math::clamp(col.g / 255.0f, 0.0f, 1.0f);
			_ref.real[2] = Math::clamp(col.b / 255.0f, 0.0f, 1.0f);
			_ref.real[3] = Math::clamp(col.a / 255.0f, 0.0f, 1.0f);
		}
	}

	template<typename T> void paintbucketToolUp_(Renderer* rnd, const Math::Recti &sel) {
		T* cmd = _commands->enqueue<T>();
		cmd->with(
			std::bind(&EditorImageImpl::getPixel, this, rnd, std::placeholders::_1, std::placeholders::_2),
			std::bind(&EditorImageImpl::setPixel, this, rnd, std::placeholders::_1, std::placeholders::_2)
		);

		if (_object->paletted()) {
			Color col;
			Palette::Ptr plt = _object->palette();
			plt->get(_ref.palette, col);

			cmd->with(
				Math::Vec2i(_object->width(), _object->height()),
				_selection.brush.empty() ? nullptr : &sel,
				_cursor.position, _ref.palette
			);
		} else {
			const Color col(
				(Byte)(_ref.real[0] * 255),
				(Byte)(_ref.real[1] * 255),
				(Byte)(_ref.real[2] * 255),
				(Byte)(_ref.real[3] * 255)
			);

			cmd->with(
				Math::Vec2i(_object->width(), _object->height()),
				_selection.brush.empty() ? nullptr : &sel,
				_cursor.position, col
			);
		}

		cmd->exec(_object);
	}
	void paintbucketToolUp(Renderer* rnd) {
		Math::Recti sel;
		if (_selection.area(sel) && !Math::intersects(sel, _cursor.position))
			_selection.clear();

		const Editing::Shortcut ctrl(SDL_SCANCODE_UNKNOWN, true, false, false);

		if (ctrl.pressed())
			paintbucketToolUp_<Commands::Image::Replace>(rnd, sel);
		else
			paintbucketToolUp_<Commands::Image::Fill>(rnd, sel);
	}

	void lassoToolDown(Renderer*) {
		_selection.mouse = ImGui::GetMousePos();

		_selection.initial = _cursor.position;
		_selection.brush.position = _cursor.position;
		_selection.brush.size = Math::Vec2i(1, 1);
	}
	void lassoToolMove(Renderer*) {
		const Math::Vec2i diff = _cursor.position - _selection.initial + Math::Vec2i(1, 1);

		const Math::Recti rect = Math::Recti::byXYWH(_selection.initial.x, _selection.initial.y, diff.x, diff.y);
		_selection.brush.position = Math::Vec2i(rect.xMin(), rect.yMin());
		_selection.brush.size = Math::Vec2i(rect.width(), rect.height());
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

	void stampToolUp_Paste(Renderer*, const Math::Recti &area, const Editing::Dot::Array &dots, Editing::Tools::Tools prevTool) {
		if (!_binding.getPixel || !_binding.setPixel)
			return;

		const int xOff = -area.width() / 2;
		const int yOff = -area.height() / 2;
		const int x = _cursor.position.x + area.xMin() + xOff;
		const int y = _cursor.position.y + area.yMin() + yOff;

		_commands->enqueue<Commands::Image::Paste>()
			->with(_binding.getPixel, _binding.setPixel)
			->with(Math::Recti::byXYWH(x, y, area.width(), area.height()), dots)
			->exec(_object);

		destroyOverlay();

		_tools.painting = prevTool;
	}
	void stampToolHover_Paste(Renderer* rnd, const Math::Recti &area, const Editing::Dot::Array &dots) {
		bool reload = _painting.moved();

		if (_overlay.empty()) {
			createOverlay(rnd);

			reload = true;
		}

		if (reload) {
			clearOverlay(rnd);

			const int xOff = -area.width() / 2;
			const int yOff = -area.height() / 2;
			int k = 0;
			for (int j = area.yMin(); j <= area.yMax(); ++j) {
				const int y = _cursor.position.y + j + yOff;
				for (int i = area.xMin(); i <= area.xMax(); ++i) {
					const int x = _cursor.position.x + i + xOff;

					if (_object->paletted()) {
						Color col;
						Palette::Ptr plt = _object->palette();
						plt->get(dots[k].indexed, col);

						_overlay.texture->set(x, y, col);
					} else {
						const Color &col = dots[k].colored;

						_overlay.texture->set(x, y, col);
					}

					++k;
				}
			}
		}
	}

	template<typename T> void paintToolDown(Renderer* rnd) {
		_selection.clear();

		_commands->enqueue<T>()
			->with(
				std::bind(&EditorImageImpl::getPixel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorImageImpl::setPixel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				_tools.weighting + 1
			)
			->exec(_object);

		createOverlay(rnd);

		paintToolMove<T>(rnd);
	}
	template<typename T, bool Clear = true> void paintToolMove(Renderer* rnd) {
		if (_commands->empty())
			return;

		if (Clear)
			clearOverlay(rnd);

		Command* back = _commands->back();
		BITTY_ASSERT(back->type() == T::TYPE());
		T* cmd = Command::as<T>(back);
		if (_object->paletted()) {
			Color col;
			Palette::Ptr plt = _object->palette();
			plt->get(_ref.palette, col);

			cmd->with(
				Math::Vec2i(_object->width(), _object->height()),
				_cursor.position, _ref.palette,
				[&] (int x, int y) -> void {
					Color col_ = col;
					if (col_.a == 0) {
						if (((x % 2) + (y % 2)) % 2)
							col_ = Color(121, 121, 121, 255);
						else
							col_ = Color(221, 221, 221, 255);
					}
					_overlay.texture->set(x, y, col_);
				}
			);
		} else {
			const Color col(
				(Byte)(_ref.real[0] * 255),
				(Byte)(_ref.real[1] * 255),
				(Byte)(_ref.real[2] * 255),
				(Byte)(_ref.real[3] * 255)
			);

			cmd->with(
				Math::Vec2i(_object->width(), _object->height()),
				_cursor.position, col,
				[&] (int x, int y) -> void {
					Color col_ = col;
					if (col_.a == 0) {
						if (((x % 2) + (y % 2)) % 2)
							col_ = Color(121, 121, 121, 255);
						else
							col_ = Color(221, 221, 221, 255);
					}
					_overlay.texture->set(x, y, col_);
				}
			);
		}
	}
	void paintToolUp(Renderer*) {
		if (!_commands->empty())
			_commands->back()->redo(_object, &_texture);

		destroyOverlay();
	}

	int getPixels(Renderer*, const Math::Recti* area /* nullable */, Editing::Dots &dots) const {
		int result = 0;

		const Math::Recti area_ = area ? *area : Math::Recti::byXYWH(0, 0, _object->width(), _object->height());
		int k = 0;
		for (int j = area_.yMin(); j <= area_.yMax(); ++j) {
			for (int i = area_.xMin(); i <= area_.xMax(); ++i) {
				if (_object->paletted()) {
					int idx = -1;
					_object->get(i, j, idx);
					dots.indexed[k] = idx;
				} else {
					Color col;
					_object->get(i, j, col);
					dots.colored[k] = col;
				}
				++k;
				++result;
			}
		}

		return result;
	}
	int setPixels(Renderer* rnd, const Math::Recti* area /* nullable */, const Editing::Dots &dots) {
		int result = 0;

		bool reload = _texture->usage() != Texture::STREAMING;
		const Math::Recti area_ = area ? *area : Math::Recti::byXYWH(0, 0, _object->width(), _object->height());
		int k = 0;
		for (int j = area_.yMin(); j <= area_.yMax(); ++j) {
			for (int i = area_.xMin(); i <= area_.xMax(); ++i) {
				if (_object->paletted()) {
					const int idx = dots.indexed[k];
					_object->set(i, j, idx);

					if (_texture->usage() == Texture::STREAMING) {
						if (!_texture->set(i, j, idx))
							reload = true;
					}
				} else {
					const Color &col = dots.colored[k];
					_object->set(i, j, col);

					if (_texture->usage() == Texture::STREAMING) {
						if (!_texture->set(i, j, col))
							reload = true;
					}
				}
				++k;
				++result;
			}
		}

		if (reload) {
			_texture = Texture::Ptr(Texture::create());
			_texture->fromImage(rnd, Texture::STREAMING, _object.get(), Texture::NEAREST);

			_texture->blend(Texture::BLEND);
		}

		return result;
	}
	bool getPixel(Renderer*, const Math::Vec2i &pos, Editing::Dot &dot) const {
		if (_object->paletted())
			return _object->get(pos.x, pos.y, dot.indexed);
		else
			return _object->get(pos.x, pos.y, dot.colored);
	}
	bool setPixel(Renderer* rnd, const Math::Vec2i &pos, const Editing::Dot &dot) {
		if (_object->paletted()) {
			if (!_object->set(pos.x, pos.y, dot.indexed))
				return false;

			if (_texture->usage() == Texture::STREAMING) {
				if (!_texture->set(pos.x, pos.y, dot.indexed))
					return false;
			} else {
				_texture = Texture::Ptr(Texture::create());
				if (!_texture->fromImage(rnd, Texture::STREAMING, _object.get(), Texture::NEAREST))
					return false;

				_texture->blend(Texture::BLEND);

				_binding.skipFrame = true;
			}
		} else {
			if (!_object->set(pos.x, pos.y, dot.colored))
				return false;

			if (_texture->usage() == Texture::STREAMING) {
				if (!_texture->set(pos.x, pos.y, dot.colored))
					return false;
			} else {
				_texture = Texture::Ptr(Texture::create());
				if (!_texture->fromImage(rnd, Texture::STREAMING, _object.get(), Texture::NEAREST))
					return false;

				_texture->blend(Texture::BLEND);

				_binding.skipFrame = true;
			}
		}

		return true;
	}
};

EditorImage* EditorImage::create(void) {
	EditorImageImpl* result = new EditorImageImpl();

	return result;
}

void EditorImage::destroy(EditorImage* ptr) {
	EditorImageImpl* impl = static_cast<EditorImageImpl*>(ptr);
	delete impl;
}

}

/* ===========================================================================} */
