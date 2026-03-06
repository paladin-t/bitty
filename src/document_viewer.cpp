/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "encoding.h"
#include "hacks.h"
#include "platform.h"
#include "renderer.h"
#include "document_viewer.h"
#include "theme.h"
#include "window.h"
#include "workspace.h"
#include "../lib/imgui/imgui_internal.h"
#include "../lib/imgui_sdl/imgui_sdl.h"
#include "../lib/jpath/jpath.hpp"
#include <SDL.h>
#if defined BITTY_OS_WIN
#	include <SDL_syswm.h>
#endif /* BITTY_OS_WIN */

/*
** {===========================================================================
** Macros and constants
*/

#ifndef DOCUMENT_VIEWER_FONT_RANGES_DEFAULT_NAME
#	define DOCUMENT_VIEWER_FONT_RANGES_DEFAULT_NAME "default"
#endif /* DOCUMENT_VIEWER_FONT_RANGES_DEFAULT_NAME */
#ifndef DOCUMENT_VIEWER_FONT_RANGES_CHINESE_NAME
#	define DOCUMENT_VIEWER_FONT_RANGES_CHINESE_NAME "chinese"
#endif /* DOCUMENT_VIEWER_FONT_RANGES_CHINESE_NAME */
#ifndef DOCUMENT_VIEWER_FONT_RANGES_JAPANESE_NAME
#	define DOCUMENT_VIEWER_FONT_RANGES_JAPANESE_NAME "japanese"
#endif /* DOCUMENT_VIEWER_FONT_RANGES_JAPANESE_NAME */
#ifndef DOCUMENT_VIEWER_FONT_RANGES_KOREAN_NAME
#	define DOCUMENT_VIEWER_FONT_RANGES_KOREAN_NAME "korean"
#endif /* DOCUMENT_VIEWER_FONT_RANGES_KOREAN_NAME */
#ifndef DOCUMENT_VIEWER_FONT_RANGES_CYRILLIC_NAME
#	define DOCUMENT_VIEWER_FONT_RANGES_CYRILLIC_NAME "cyrillic"
#endif /* DOCUMENT_VIEWER_FONT_RANGES_CYRILLIC_NAME */
#ifndef DOCUMENT_VIEWER_FONT_RANGES_THAI_NAME
#	define DOCUMENT_VIEWER_FONT_RANGES_THAI_NAME "thai"
#endif /* DOCUMENT_VIEWER_FONT_RANGES_THAI_NAME */
#ifndef DOCUMENT_VIEWER_FONT_RANGES_VIETNAMESE_NAME
#	define DOCUMENT_VIEWER_FONT_RANGES_VIETNAMESE_NAME "vietnamese"
#endif /* DOCUMENT_VIEWER_FONT_RANGES_VIETNAMESE_NAME */
#ifndef DOCUMENT_VIEWER_FONT_RANGES_POLISH_NAME
#	define DOCUMENT_VIEWER_FONT_RANGES_POLISH_NAME "polish"
#endif /* DOCUMENT_VIEWER_FONT_RANGES_POLISH_NAME */

/* ===========================================================================} */

/*
** {===========================================================================
** Document viewer
*/

class DocumentViewerImpl : public DocumentViewer {
private:
	bool _opened = false;

	std::string _id;

	mutable RecursiveMutex _lock;
	struct Options {
		bool clearBeforeBaking = false;
		bool affectedByCamera = true;
		Math::Vec2i cameraPosition;

		bool toRefreshStyleColor = false;
		ImVec4 styleColors[ImGuiCol_COUNT];
		bool styleColorDirty[ImGuiCol_COUNT];
		float styleVarScrollbarSize = 14.0f;

		bool toRefreshFont = false;
		rapidjson::Document fontConfig;
		FontData fontData;
		ImVector<ImWchar> customFontRanges;

		Options() {
			memset(styleColorDirty, 0, sizeof(styleColorDirty));
		}
	} _options;                                            // By the graphics thread.

	ImGuiContext* _context = nullptr;                      // By the graphics thread.
	ImGuiSDL::Device* _device = nullptr;                   // By the graphics thread.
	Texture* _texture = nullptr;                           // By the graphics thread.
	ImFont* _font = nullptr;                               // By the graphics thread.
	ImGuiMouseCursor _mouseCursor = ImGuiMouseCursor_None; // By the graphics thread.

public:
	DocumentViewerImpl() {
	}
	virtual ~DocumentViewerImpl() override {
		assert(!_opened && "Not closed");
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Object** ptr) const override { // Non-clonable.
		if (ptr)
			*ptr = nullptr;

		return false;
	}

	virtual void open(const char* name) override {
		LockGuard<decltype(_lock)> guard(_lock);

		if (_opened)
			return;
		_opened = true;

		_id = name;

		fprintf(stdout, "Document viewer opened: \"%s\".\n", _id.c_str());
	}
	virtual void close(void) override {
		LockGuard<decltype(_lock)> guard(_lock);

		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "Document viewer closed: \"%s\".\n", _id.c_str());

		_id.clear();

		if (_context) {
			ImGuiContext* mainContext = ImGui::GetCurrentContext();
			ImGui::SetCurrentContext(_context);
			ImGuiSDL::Device* mainDevice = ImGuiSDL::GetCurrentDevice();
			ImGuiSDL::SetCurrentDevice(_device);
			{
				ImGuiIO &io = ImGui::GetIO();
				if (io.Fonts->TexID) {
					ImGuiSDLHack::Texture* texture = static_cast<ImGuiSDLHack::Texture*>(io.Fonts->TexID);
					delete texture;
					io.Fonts->TexID = nullptr;
				}
				io.Fonts->Clear();
				_font = nullptr;

				ImGuiSDL::Deinitialize();

				_device = nullptr;
			}
			ImGuiSDL::SetCurrentDevice(mainDevice);
			ImGui::SetCurrentContext(mainContext);

			ImGui::DestroyContext(_context);
			_context = nullptr;
		}

		if (_texture) {
			Texture::destroy(_texture);
			_texture = nullptr;
		}
	}

	virtual void lock(void) override {
		_lock.lock();
	}
	virtual void unlock(void) override {
		_lock.unlock();
	}
	virtual bool tryLock(void) override {
		return _lock.tryLock();
	}

	virtual bool option(const std::string &key, const Variant &val) override {
		if (key == "clear_before_baking") {
			const bool val_ = (bool)val;
			_options.clearBeforeBaking = val_;

			return true;
		} else if (key == "affected_by_camera") {
			const bool val_ = (bool)val;
			_options.affectedByCamera = val_;

			return true;
		}

		if (key == "style_text") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			_options.toRefreshStyleColor = true;
			_options.styleColors[ImGuiCol_Text] = ImVec4(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
			_options.styleColorDirty[ImGuiCol_Text] = true;

			return true;
		} else if (key == "style_popup_background") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			_options.toRefreshStyleColor = true;
			_options.styleColors[ImGuiCol_PopupBg] = ImVec4(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
			_options.styleColorDirty[ImGuiCol_PopupBg] = true;

			return true;
		} else if (key == "style_border") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			_options.toRefreshStyleColor = true;
			_options.styleColors[ImGuiCol_Border] = ImVec4(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
			_options.styleColorDirty[ImGuiCol_Border] = true;

			return true;
		} else if (key == "style_scrollbar_background") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			_options.toRefreshStyleColor = true;
			_options.styleColors[ImGuiCol_ScrollbarBg] = ImVec4(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
			_options.styleColorDirty[ImGuiCol_ScrollbarBg] = true;

			return true;
		} else if (key == "style_scrollbar_grab") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			_options.toRefreshStyleColor = true;
			_options.styleColors[ImGuiCol_ScrollbarGrab] = ImVec4(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
			_options.styleColorDirty[ImGuiCol_ScrollbarGrab] = true;

			return true;
		} else if (key == "style_scrollbar_grab_hovered") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			_options.toRefreshStyleColor = true;
			_options.styleColors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
			_options.styleColorDirty[ImGuiCol_ScrollbarGrabHovered] = true;

			return true;
		} else if (key == "style_scrollbar_grab_active") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			_options.toRefreshStyleColor = true;
			_options.styleColors[ImGuiCol_ScrollbarGrabActive] = ImVec4(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
			_options.styleColorDirty[ImGuiCol_ScrollbarGrabActive] = true;

			return true;
		} else if (key == "style_header") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			_options.toRefreshStyleColor = true;
			_options.styleColors[ImGuiCol_Header] = ImVec4(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
			_options.styleColorDirty[ImGuiCol_Header] = true;

			return true;
		} else if (key == "style_header_hovered") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			_options.toRefreshStyleColor = true;
			_options.styleColors[ImGuiCol_HeaderHovered] = ImVec4(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
			_options.styleColorDirty[ImGuiCol_HeaderHovered] = true;

			return true;
		} else if (key == "style_header_active") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			_options.toRefreshStyleColor = true;
			_options.styleColors[ImGuiCol_HeaderActive] = ImVec4(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
			_options.styleColorDirty[ImGuiCol_HeaderActive] = true;

			return true;
		} else if (key == "style_separator") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			_options.toRefreshStyleColor = true;
			_options.styleColors[ImGuiCol_Separator] = ImVec4(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
			_options.styleColorDirty[ImGuiCol_Separator] = true;

			return true;
		}

		if (key == "style_scrollbar_size") {
			const float val_ = (float)(Double)val;

			_options.styleVarScrollbarSize = std::max(val_, 0.0f);

			return true;
		}

		return false;
	}

	virtual bool useFont(const Json::Ptr &json, const FontData &fontData) override {
		if (!json)
			return false;

		if (!json->toJson(_options.fontConfig))
			return false;

		_options.toRefreshFont = true;
		_options.fontData = fontData;

		return true;
	}

	virtual bool location(float &v) const override {
		LockGuard<decltype(_lock)> guard(_lock);

		// TODO

		return true;
	}
	virtual void locate(float v) override {
		LockGuard<decltype(_lock)> guard(_lock);

		// TODO
	}

	virtual const char* text(size_t* len) const override {
		LockGuard<decltype(_lock)> guard(_lock);

		// TODO

		return nullptr;
	}
	virtual void text(const char* txt, size_t /* len */) override {
		LockGuard<decltype(_lock)> guard(_lock);

		// TODO
	}

	virtual void update(
		class Window* wnd, class Renderer* rnd,
		class Workspace* ws, const class Project* /* project */, class Executable* /* exec */,
		const char* title,
		float x, float y, float width, float height,
		int /* scale */,
		bool /* pending */,
		double /* delta */
	) override {
		ImGui::SetCursorPos(ImVec2(x, y));

		if (_font && _font->IsLoaded()) {
			ImGui::PushFont(_font);
			//SetFont(_font);
		}
		{
			LockGuard<decltype(_lock)> guard(_lock);

			/*if (_toColorize) {
				_toColorize = false;
				Colorize();
			}*/

			/*if (_acquireFocus) {
				if (!ws->popupBox()) {
					_acquireFocus = false;
					EditorFocused = true;
					ImGui::SetNextWindowFocus();
				}
			}*/

			//_isFocused = IsEditorFocused();

			//Render(title, ImVec2(width, height));
		}
		if (_font && _font->IsLoaded()) {
			//SetFont(nullptr);
			ImGui::PopFont();
		}
	}

	virtual void bake(
		class Window* wnd, class Renderer* rnd,
		class Workspace* ws,
		float x, float y, float width, float height
	) override {
		// Prepare.
		if (!_opened)
			return;

		const int scale = rnd->scale() / wnd->scale();
		const Math::Rectf clientArea = ws->canvasClientArea();
		const Math::Vec2i canvasSize = ws->canvasSize();

		// Touch the context and texture.
		ImGuiContext* currentContext = context(wnd, rnd, (int)width, (int)height);
		Texture* targetTexture = texture(wnd, rnd, (int)width, (int)height);

		// Reserve the main context.
		const ImGuiIO &mainIo = ImGui::GetIO();

		ImGuiContext* mainContext = ImGui::GetCurrentContext();
		ImGuiSDL::Device* mainDevice = ImGuiSDL::GetCurrentDevice();
		Texture* mainTarget = rnd->target();

		// Switch to the local context.
		ImGui::SetCurrentContext(currentContext);

		ImGuiSDL::SetCurrentDevice(_device);

		rnd->target(targetTexture);

		// Initialize the local context.
		ImGuiIO &io = ImGui::GetIO();

		do {
			io.DeltaTime               = mainIo.DeltaTime;
			io.DisplaySize             = ImVec2(width, height);
			io.DisplayFramebufferScale = mainIo.DisplayFramebufferScale;

			io.KeyCtrl                 = mainIo.KeyCtrl;
			io.KeyShift                = mainIo.KeyShift;
			io.KeyAlt                  = mainIo.KeyAlt;
			io.KeySuper                = mainIo.KeySuper;
			memcpy(io.KeysDown,          mainIo.KeysDown,  sizeof(mainIo.KeysDown));

			io.MouseWheel              = mainIo.MouseWheel;
			io.MouseWheelH             = mainIo.MouseWheelH;
			memcpy(io.MouseDown,         mainIo.MouseDown, sizeof(mainIo.MouseDown));
		} while (false);

		refreshFonts(wnd, rnd, io);

		// Prepare for rendering.
		if (_options.clearBeforeBaking) {
			const Color cls(0x2e, 0x32, 0x38, 0xff);
			rnd->clip(0, 0, rnd->width(), rnd->height());
			rnd->clear(&cls);
		}

		// Render the edit area.
		{
			ImGui::NewFrame();

			ImGuiStyle &style = ImGui::GetStyle();

			if (_options.toRefreshStyleColor) {
				_options.toRefreshStyleColor = false;
				for (int i = 0; i < ImGuiCol_COUNT; ++i) {
					if (_options.styleColorDirty[i]) {
						_options.styleColorDirty[i] = false;
						style.Colors[i] = _options.styleColors[i];
					}
				}
			}

			io.MousePos                = mainIo.MousePos;

			do {
				Math::Vec2i pos((Int)io.MousePos.x, (Int)io.MousePos.y);
				fromScreenPosition(pos, clientArea, canvasSize, 1); // Translate mouse position from screen space to local space.
				pos -= Math::Vec2i((Int)x, (Int)y);
				if (_options.affectedByCamera) {
					pos.x += _options.cameraPosition.x;
					pos.y += _options.cameraPosition.y;
				}
				io.MousePos            = ImVec2((float)pos.x, (float)pos.y);
			} while (false);

			do {
				VariableGuard<decltype(style.WindowBorderSize)> guardBorderSize(&style.WindowBorderSize, style.WindowBorderSize, 0.0f);
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());
				VariableGuard<decltype(style.ScrollbarSize)> guardScrollbarSize(&style.ScrollbarSize, style.ScrollbarSize, _options.styleVarScrollbarSize);

				const ImGuiWindowFlags flags =
					ImGuiWindowFlags_NoTitleBar |
					ImGuiWindowFlags_NoResize |
					ImGuiWindowFlags_NoMove |
					ImGuiWindowFlags_NoScrollbar |
					ImGuiWindowFlags_NoCollapse |
					ImGuiWindowFlags_NoSavedSettings |
					ImGuiWindowFlags_NoBringToFrontOnFocus |
					ImGuiWindowFlags_NoNav;
				ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
				ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
				if (ImGui::Begin(_id.c_str(), nullptr, flags)) {
					update(
						wnd, rnd,
						ws, nullptr, nullptr,
						_id.c_str(),
						0, 0, width, height,
						scale,
						false,
						0.0
					);

					ImGui::End();
				}
			} while (false);

			ImGui::Render();

			ImGuiSDL::Render(ImGui::GetDrawData());
		}

		// Finish rendering.
		if (_options.clearBeforeBaking) {
			rnd->clip();
		}

		rnd->target(mainTarget);

		// Save the mouse cursor state for later populating.
		_mouseCursor = ImGui::GetMouseCursor();

		// Switch to the main context.
		ImGuiSDL::SetCurrentDevice(mainDevice);

		ImGui::SetCurrentContext(mainContext);
	}
	virtual void render(
		class Window* wnd, class Renderer* rnd,
		class Workspace* /* ws */,
		float x, float y, float width, float height
	) override {
		// Prepare.
		ImGuiContext* context = ImGui::GetCurrentContext();

		if (!_opened)
			return;

		Texture* tex = texture(wnd, rnd, (int)width, (int)height);

		// Render the baked texture to the target.
		const Math::Recti dstRect = Math::Recti::byXYWH((int)x, (int)y, (int)width, (int)height);
		rnd->render(
			tex,
			nullptr, &dstRect,
			nullptr, nullptr,
			false, false,
			nullptr, false, false
		);

		// Populate the mouse cursor state.
		if (_mouseCursor != ImGuiMouseCursor_None && _mouseCursor != ImGuiMouseCursor_Arrow)
			ImGui::SetMouseCursor(_mouseCursor);
	}

	virtual void translate(int &x0, int &y0, int &x1, int &y1, int camX, int camY) override {
		if (_options.affectedByCamera) {
			_options.cameraPosition = Math::Vec2i(camX, camY);
			x0 -= camX;
			y0 -= camY;
			x1 -= camX;
			y1 -= camY;
		}
	}

private:
	ImGuiContext* context(Window* wnd, Renderer* rnd, int width, int height) {
		// Prepare.
		if (_context)
			return _context;

		const ImGuiIO &mainIo = ImGui::GetIO();

		// Create a new ImGui context.
		_context = ImGui::CreateContext();

		// Switch to the new context.
		ImGuiContext* mainContext = ImGui::GetCurrentContext();
		ImGui::SetCurrentContext(_context);
		ImGuiSDL::Device* mainDevice = ImGuiSDL::GetCurrentDevice();

		// Setup the new context.
		{
			ImGuiStyle &style = ImGui::GetStyle();
			ImGuiIO &io = ImGui::GetIO();

			style.ScrollbarRounding                      = 0;
			style.TabRounding                            = 0;

			style.Colors[ImGuiCol_Text]                  = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
			style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
			style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
			style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			style.Colors[ImGuiCol_PopupBg]               = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
			style.Colors[ImGuiCol_Border]                = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
			style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
			style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
			style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
			style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
			style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
			style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
			style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
			style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
			style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
			style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
			style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
			style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
			style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
			style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.46f, 0.54f, 0.80f, 0.60f);
			style.Colors[ImGuiCol_Button]                = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
			style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
			style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
			style.Colors[ImGuiCol_Header]                = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
			style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
			style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
			style.Colors[ImGuiCol_Separator]             = ImVec4(0.39f, 0.39f, 0.39f, 0.62f);
			style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
			style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
			style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.80f, 0.80f, 0.80f, 0.56f);
			style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
			style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
			style.Colors[ImGuiCol_Tab]                   = ImLerp(style.Colors[ImGuiCol_Header],    style.Colors[ImGuiCol_TitleBgActive], 0.90f);
			style.Colors[ImGuiCol_TabHovered]            = ImVec4(0.00f, 0.48f, 0.80f, 1.00f);
			style.Colors[ImGuiCol_TabActive]             = ImVec4(0.00f, 0.42f, 0.74f, 1.00f);
			style.Colors[ImGuiCol_TabUnfocused]          = ImLerp(style.Colors[ImGuiCol_Tab],       style.Colors[ImGuiCol_TitleBg], 0.80f);
			style.Colors[ImGuiCol_TabUnfocusedActive]    = ImLerp(style.Colors[ImGuiCol_TabActive], style.Colors[ImGuiCol_TitleBg], 0.40f);
			style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
			style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
			style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
			style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.45f, 0.00f, 1.00f);
			style.Colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.78f, 0.87f, 0.98f, 1.00f);
			style.Colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.57f, 0.57f, 0.64f, 1.00f);
			style.Colors[ImGuiCol_TableBorderLight]      = ImVec4(0.68f, 0.68f, 0.74f, 1.00f);
			style.Colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			style.Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(0.30f, 0.30f, 0.30f, 0.09f);
			style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
			style.Colors[ImGuiCol_DragDropTarget]        = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
			style.Colors[ImGuiCol_NavHighlight]          = style.Colors[ImGuiCol_HeaderHovered];
			style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
			style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
			style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

			io.IniFilename                               = nullptr;
			io.ConfigFlags                               = mainIo.ConfigFlags;
			io.BackendFlags                              = mainIo.BackendFlags;

			io.SetClipboardTextFn                        = mainIo.SetClipboardTextFn;
			io.GetClipboardTextFn                        = mainIo.GetClipboardTextFn;
			io.ClipboardUserData                         = mainIo.ClipboardUserData;

			memcpy(io.KeyMap,                              mainIo.KeyMap, sizeof(mainIo.KeyMap));
			io.KeyRepeatDelay                            = mainIo.KeyRepeatDelay;
			io.KeyRepeatRate                             = mainIo.KeyRepeatRate;

			io.MouseDoubleClickTime                      = mainIo.MouseDoubleClickTime;
			io.MouseDoubleClickMaxDist                   = mainIo.MouseDoubleClickMaxDist;
			io.MouseDragThreshold                        = mainIo.MouseDragThreshold;

#if defined BITTY_OS_WIN
			SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

			SDL_SysWMinfo wmInfo;
			SDL_VERSION(&wmInfo.version);
			SDL_GetWindowWMInfo((SDL_Window*)wnd->pointer(), &wmInfo);

			const HWND hwnd = wmInfo.info.win.window;
			io.ImeWindowHandle = hwnd;
#else /* BITTY_OS_WIN */
			(void)wnd;

			io.ImeSetInputScreenPosFn = Platform::inputScreenPosition;
#endif /* BITTY_OS_WIN */

			ImGuiSDL::Initialize((SDL_Renderer*)rnd->pointer(), width, height);
			_device = ImGuiSDL::GetCurrentDevice();
		}

		// Switch to the main context.
		ImGuiSDL::SetCurrentDevice(mainDevice);
		ImGui::SetCurrentContext(mainContext);

		// Return the new created context.
		return _context;
	}
	Texture* texture(Window* /* wnd */, Renderer* rnd, int width, int height) {
		// Invalidate the texture if the desired size has been changed.
		if (_texture && (_texture->width() != width || _texture->height() != height)) {
			Texture::destroy(_texture);
			_texture = nullptr;
		}

		// Reuse the cached texture if possible.
		if (_texture)
			return _texture;

		// Create a new texture.
		_texture = Texture::create();
		Byte* pixels = new Byte[width * height * sizeof(Color)];
		memset(pixels, 0, width * height * sizeof(Color));
		_texture->fromBytes(rnd, Texture::TARGET, pixels, width, height, 0, Texture::NEAREST);
		_texture->blend(Texture::BLEND);
		delete [] pixels;

		// Return the new created texture.
		return _texture;
	}
	bool refreshFonts(Window* /* wnd */, Renderer* rnd, ImGuiIO &io) {
		auto rebuild = [] (Renderer* rnd, ImGuiIO &io) -> void {
			if (io.Fonts->TexID) {
				ImGuiSDLHack::Texture* texture = static_cast<ImGuiSDLHack::Texture*>(io.Fonts->TexID);
				delete texture;
				io.Fonts->TexID = nullptr;
			}

			unsigned char* pixels = nullptr;
			int width = 0, height = 0;
			io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
			ImGuiSDLHack::Texture* texture = new ImGuiSDLHack::Texture(rnd, pixels, width, height);
			io.Fonts->TexID = (void*)texture;
		};

		if (!_options.toRefreshFont) {
			if (!_font) {
				io.Fonts->Clear();
				_font = io.Fonts->AddFontDefault();

				rebuild(rnd, io);
			}

			return true;
		}

		const rapidjson::Value* fonts = nullptr;
		Jpath::get(_options.fontConfig, fonts, "fonts");
		if (fonts && fonts->IsArray()) {
			for (int i = 0; i < (int)fonts->Capacity(); ++i) {
				std::string operation = "merge";
				std::string usage = "generic";
				std::string path;
				float size = 0;
				std::string ranges;
				Math::Vec2i oversample(0, 0);
				Math::Vec2f glyphOffset(0, 0);

				Jpath::get(*fonts, operation, i, "operation");
				Jpath::get(*fonts, usage, i, "usage"); // Trivial for `DocumentViewer` font.
				Jpath::get(*fonts, path, i, "path");
				Jpath::get(*fonts, size, i, "size");
				Jpath::get(*fonts, ranges, i, "ranges");
				Jpath::get(*fonts, oversample.x, i, "oversample", 0);
				Jpath::get(*fonts, oversample.y, i, "oversample", 1);
				Jpath::get(*fonts, glyphOffset.x, i, "glyph_offset", 0);
				Jpath::get(*fonts, glyphOffset.y, i, "glyph_offset", 1);

				size = Math::clamp(size, 4.0f, 96.0f);
				oversample.x = Math::clamp(oversample.x, (Int)1, (Int)8);
				oversample.y = Math::clamp(oversample.y, (Int)1, (Int)8);
				glyphOffset.x = Math::clamp(glyphOffset.x, (Real)-96, (Real)96);
				glyphOffset.y = Math::clamp(glyphOffset.y, (Real)-96, (Real)96);

				const ImWchar* glyphRanges = io.Fonts->GetGlyphRangesDefault();
				if (ranges == DOCUMENT_VIEWER_FONT_RANGES_DEFAULT_NAME) {
					glyphRanges = io.Fonts->GetGlyphRangesDefault();
				} else if (ranges == DOCUMENT_VIEWER_FONT_RANGES_CHINESE_NAME) {
					glyphRanges = io.Fonts->GetGlyphRangesChineseFull();
				} else if (ranges == DOCUMENT_VIEWER_FONT_RANGES_JAPANESE_NAME) {
					glyphRanges = io.Fonts->GetGlyphRangesJapanese();
				} else if (ranges == DOCUMENT_VIEWER_FONT_RANGES_KOREAN_NAME) {
					glyphRanges = io.Fonts->GetGlyphRangesKorean();
				} else if (ranges == DOCUMENT_VIEWER_FONT_RANGES_CYRILLIC_NAME) {
					glyphRanges = io.Fonts->GetGlyphRangesCyrillic();
				} else if (ranges == DOCUMENT_VIEWER_FONT_RANGES_THAI_NAME) {
					glyphRanges = io.Fonts->GetGlyphRangesThai();
				} else if (ranges == DOCUMENT_VIEWER_FONT_RANGES_VIETNAMESE_NAME) {
					glyphRanges = io.Fonts->GetGlyphRangesVietnamese();
				} else if (ranges == DOCUMENT_VIEWER_FONT_RANGES_POLISH_NAME) {
					static constexpr const ImWchar RANGES_POLISH[] = {
						0x0020, 0x00ff, // Basic Latin + Latin supplement.
						0x0100, 0x017f, // Polish alphabet.
						0
					};
					glyphRanges = RANGES_POLISH;
				} else if (!ranges.empty()) {
					const std::wstring wstr = Unicode::toWide(ranges);
					if (!wstr.empty() && wstr.size() % 2 == 0) {
						if (!_options.customFontRanges.empty())
							_options.customFontRanges.pop_back();
						for (int j = 0; j < (int)wstr.length(); j += 2) {
							const wchar_t ch0 = wstr[j];
							const wchar_t ch1 = wstr[j + 1];
							if (ch0 > ch1) {
								_options.customFontRanges.clear();

								break;
							}
							_options.customFontRanges.push_back(ch0);
							_options.customFontRanges.push_back(ch1);
						}
						if (!_options.customFontRanges.empty()) {
							if (_options.customFontRanges.back() != 0)
								_options.customFontRanges.push_back(0);
							glyphRanges = &_options.customFontRanges.front();
						}
					}
				}

				ImFontConfig fontCfg;
				fontCfg.FontDataOwnedByAtlas = false;
				fontCfg.OversampleH = (int)oversample.x; fontCfg.OversampleV = (int)oversample.y;
				fontCfg.GlyphOffset = ImVec2((float)glyphOffset.x, (float)glyphOffset.y);
				fontCfg.MergeMode = operation == "merge";
				if (operation == "set" || operation == "merge") {
					const bool setDefault = operation == "set" /* && usage == "generic" */ && glyphRanges == io.Fonts->GetGlyphRangesDefault();
					if (setDefault)
						io.Fonts->Clear();

					do {
						FontData::iterator it = _options.fontData.find(path);
						if (it == _options.fontData.end())
							break;

						const Bytes::Ptr &bytes = it->second;
						if (!bytes)
							break;

						ImFont* font = io.Fonts->AddFontFromMemoryTTF(
							(void*)bytes->pointer(), (int)bytes->count(),
							size,
							&fontCfg, glyphRanges
						);
						if (setDefault && !font)
							io.Fonts->AddFontDefault();
						// if (usage == "code")
						_font = font;
					} while (false);
				} else if (operation == "clear") {
					io.Fonts->Clear();
					io.Fonts->AddFontDefault();
				}
			}
		}

		rebuild(rnd, io);

		_options.toRefreshFont = false;
		_options.fontData.clear();

		return true;
	}

	static bool fromScreenPosition(Math::Vec2i &pos, const Math::Rectf &clientArea, const Math::Vec2i &canvasSize, int scale) {
		const double dstW = (double)canvasSize.x;
		const double dstH = (double)canvasSize.y;
		double fx = 0, fy = 0;
		if (scale == 1) {
			fx = (double)(pos.x - clientArea.xMin()) / clientArea.width() * dstW;
			fy = (double)(pos.y - clientArea.yMin()) / clientArea.height() * dstH;
		} else {
			fx = (double)(pos.x / (double)scale - clientArea.xMin()) / clientArea.width() * dstW;
			fy = (double)(pos.y / (double)scale - clientArea.yMin()) / clientArea.height() * dstH;
		}
		if (fx >= 0 && fx < canvasSize.x && fy >= 0 && fy < canvasSize.y) {
			pos.x = (int)fx;
			pos.y = (int)fy;

			return true;
		}
		pos.x = -1;
		pos.y = -1;

		return false;
	}
	static bool toScreenPosition(Math::Vec2i &pos, const Math::Rectf &clientArea, const Math::Vec2i &canvasSize, int scale) {
		const double dstW = (double)canvasSize.x;
		const double dstH = (double)canvasSize.y;
		double fx = pos.x, fy = pos.y;
		if (scale == 1) {
			fx = fx / dstW * clientArea.width() + clientArea.xMin();
			fy = fy / dstH * clientArea.height() + clientArea.yMin();
		} else {
			fx = (fx / dstW * clientArea.width() + clientArea.xMin() * (double)scale);
			fy = (fy / dstH * clientArea.height() + clientArea.yMin() * (double)scale);
		}
		pos = Math::Vec2i((int)fx, (int)fy);

		return true;
	}
};

bool DocumentViewer::parseFont(const Json::Ptr &json, FontData &fontData, FontResolver resolveFont) {
	fontData.clear();

	if (!json)
		return false;

	rapidjson::Document doc;
	if (!json->toJson(doc))
		return false;

	const rapidjson::Value* fonts = nullptr;
	Jpath::get(doc, fonts, "fonts");
	if (!fonts || !fonts->IsArray())
		return false;

	for (int i = 0; i < (int)fonts->Capacity(); ++i) {
		std::string path;

		Jpath::get(*fonts, path, i, "path");
		if (path.empty())
			continue;

		Bytes::Ptr bytes = resolveFont(path);
		if (!bytes)
			continue;

		fontData[path] = bytes;
	}

	return true;
}

DocumentViewer* DocumentViewer::create(void) {
	DocumentViewerImpl* result = new DocumentViewerImpl();

	return result;
}

void DocumentViewer::destroy(DocumentViewer* ptr) {
	DocumentViewerImpl* impl = static_cast<DocumentViewerImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
