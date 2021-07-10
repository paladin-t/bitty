/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "application.h"
#include "bytes.h"
#include "datetime.h"
#include "effects.h"
#include "encoding.h"
#include "file_handle.h"
#include "filesystem.h"
#include "luaxx.h"
#include "platform.h"
#include "primitives.h"
#include "project.h"
#include "renderer.h"
#include "scripting.h"
#include "window.h"
#include "workspace.h"
#include "../lib/curl/include/curl/curl.h"
#include "../lib/imgui_sdl/imgui_sdl.h"
#include "../lib/mongoose/mongoose.h"
#include "../lib/sdl_gfx/SDL2_gfxPrimitives.h"
#include "../lib/zlib/zlib.h"
#include <SDL_mixer.h>
#if defined BITTY_OS_WIN
#	include <SDL_syswm.h>
#endif /* BITTY_OS_WIN */

/*
** {===========================================================================
** Macros and constants
*/

#ifndef APPLICATION_ICON_FILE
#	define APPLICATION_ICON_FILE "../icon.png"
#endif /* APPLICATION_ICON_FILE */

#ifndef APPLICATION_IDLE_FRAME_RATE
#	define APPLICATION_IDLE_FRAME_RATE 15
#endif /* APPLICATION_IDLE_FRAME_RATE*/
#ifndef APPLICATION_INPUTING_FRAME_RATE
#	define APPLICATION_INPUTING_FRAME_RATE 30
#endif /* APPLICATION_INPUTING_FRAME_RATE*/

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

static Text::Dictionary applicationParseArgs(int argc, const char* argv[]) {
	Text::Dictionary result;

	if (argc == 0 || !argv)
		return result;

	int i = 0;
	while (i < argc) {
		const char* arg = argv[i];
		if (*arg == '-') {
			std::string key, val;
			key = arg + 1;
			if (i + 1 < argc) {
				const char* data = argv[i + 1];
				if (*data != '-') {
					val = data;
					++i;
				}
			}
			Text::toLowerCase(key);
			result[key] = val;
		} else if (*arg == '\0') {
			// Do nothing.
		} else {
			std::string val = arg;
			if (val.front() == '"' && val.back() == '"') {
				val.erase(val.begin());
				val.pop_back();
			}
			result[""] = val;
		}
		++i;
	}

	return result;
}

/* ===========================================================================} */

/*
** {===========================================================================
** Application
*/

class Application {
private:
	struct Context {
		unsigned expectedFrameRate = BITTY_ACTIVE_FRAME_RATE;
		unsigned updatedFrameCount = 0;
		double updatedSeconds = 0;
		unsigned fps = 0;

		double delta = 0.0;

		char* clipboardTextData = nullptr;

		bool mouseCursorIndicated = false;
		SDL_Cursor* mouseCursors[ImGuiMouseCursor_COUNT] = { };
		bool mousePressed[3] = { false, false, false };
		ImVec2 mousePosition;
		bool mouseCanUseGlobalState = true;
	};

private:
	bool _opened = false;

	Window* _window = nullptr;
	Renderer* _renderer = nullptr;
	Effects* _effects = nullptr;
	long long _stamp = 0;

	Project* _project = nullptr;
	Resources* _resources = nullptr;
	Primitives* _primitives = nullptr;
	Executable* _executable = nullptr;
	Workspace* _workspace = nullptr;

	Context _context;

public:
	Application(Workspace* workspace) : _workspace(workspace) {
		help();
		versions();
		paths();

		_project = new Project();

		_resources = Resources::create();

		_primitives = Primitives::create(true);

		_executable = Scripting::create(Executable::LUA);
	}
	~Application() {
		delete _workspace;
		_workspace = nullptr;

		Scripting::destroy(_executable);
		_executable = nullptr;

		Primitives::destroy(_primitives);
		_primitives = nullptr;

		Resources::destroy(_resources);
		_resources = nullptr;

		delete _project;
		_project = nullptr;
	}

	bool open(const Text::Dictionary &options) {
		// Prepare.
		if (_opened)
			return false;
		_opened = true;

		// Initialize the window and renderer.
		const bool borderless = options.find(WORKSPACE_OPTION_WINDOW_BORDERLESS_KEY) != options.end();
		int scale = 1;
		if (options.find(WORKSPACE_OPTION_RENDERER_X2_KEY) != options.end())
			scale = 2;
		else if (options.find(WORKSPACE_OPTION_RENDERER_X3_KEY) != options.end())
			scale = 3;
		const bool highDpi = options.find(WORKSPACE_OPTION_WINDOW_HIGH_DPI_DISABLED_KEY) == options.end();
#if BITTY_EFFECTS_ENABLED
		const bool opengl = true;
#	if defined BITTY_OS_WIN
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
#	elif defined BITTY_OS_MAC
		SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "opengl", SDL_HINT_OVERRIDE);
		SDL_SetHintWithPriority(SDL_HINT_RENDER_OPENGL_SHADERS, "1", SDL_HINT_OVERRIDE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
#	elif defined BITTY_OS_LINUX
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
#	endif /* Platform macro. */
#else /* BITTY_EFFECTS_ENABLED */
		const bool opengl = false;
#endif /* BITTY_EFFECTS_ENABLED */
		_window = Window::create();
		_window->open(
			BITTY_TITLE " v" BITTY_VERSION_STRING,
			0, WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT,
			WINDOW_MIN_WIDTH * scale, WINDOW_MIN_HEIGHT * scale, borderless,
			highDpi, opengl
		);

		_renderer = Renderer::create();
		_renderer->open(_window);

		int wndScale = 1;
		if (highDpi) {
			const int wndW = _window->width();
			const int rndW = _renderer->width();
			if (wndW > 0 && rndW > wndW) { // Only happens on MacOS with retina display.
				wndScale *= rndW / wndW;
				if (wndScale <= 0)
					wndScale = 1;
			}
		}

		if (scale != 1 || wndScale != 1)
			_renderer->scale(scale * wndScale); // Scale the renderer.
		if (wndScale != 1)
			_window->scale(wndScale); // Scale the window (on high-DPI monitor).

		// Create the effects.
		_effects = Effects::create();

		// Initialize the icon.
		if (Path::existsFile(APPLICATION_ICON_FILE)) {
			File::Ptr file(File::create());
			if (file->open(APPLICATION_ICON_FILE, Stream::READ)) {
				Bytes::Ptr bytes(Bytes::create());
				file->readBytes(bytes.get());
				file->close();

				Image::Ptr img(Image::create(nullptr));
				if (img->fromBytes(bytes.get())) {
					SDL_Window* wnd = (SDL_Window*)_window->pointer();
					SDL_Surface* sur = (SDL_Surface*)img->pointer();
					SDL_SetWindowIcon(wnd, sur);
				}
			}
		}

		// Initialize the randomizer.
		Math::srand();

		// Initialize the timestamp.
		_stamp = DateTime::ticks();

		// Initialize the GUI system.
		openImGui();

		// Initialize the project.
		_project->open(_renderer);

		// Initialize the resources module.
		_resources->open();

		// Initialize the primitives module.
		_primitives->open(_window, _renderer, _project, _resources, _effects);

		// Initialize the executable module.
		_executable->open(_workspace, _project, nullptr, _primitives);
		if (options.find(WORKSPACE_OPTION_EXECUTABLE_TIMEOUT_DISABLED_KEY) != options.end())
			_executable->timeout(-1);
#if defined BITTY_OS_HTML
		_executable->timeout(-1); // Timeout of invoking is always disabled for HTML.
#endif /* BITTY_OS_HTML */

		// Initialize the workspace.
		_workspace->load(_window, _renderer, _project, _primitives);
		_workspace->open(_window, _renderer, _project, _executable, _primitives, options);

		// Initialize the effects.
		_effects->open(_window, _renderer, _workspace, options.find(WORKSPACE_OPTION_RENDERER_EFFECTS_DISABLED_KEY) == options.end());

		// Finish.
		return true;
	}
	bool close(void) {
		// Prepare.
		if (!_opened)
			return false;
		_opened = false;

		// Dispose the primitive.
		primitivePurge();

		// Dispose the workspace.
		_workspace->close(_window, _renderer, _project, _executable);
		_workspace->save(_window, _renderer, _project, _primitives);

		// Dispose the executable module.
		_executable->close();

		// Dispose the primitives module.
		_primitives->close();

		// Dispose the resources module.
		_resources->close();

		// Dispose the project.
		_project->close();

		// Dispose the GUI system.
		closeImGui();

		// Dispose the timestamp.
		_stamp = 0;

		// Dispose the effects.
		if (_effects) {
			_effects->close();
			Effects::destroy(_effects);
			_effects = nullptr;
		}

		// Dispose the window and renderer.
		if (_renderer) {
			_renderer->close();
			Renderer::destroy(_renderer);
			_renderer = nullptr;
		}
		if (_window) {
			_window->close();
			Window::destroy(_window);
			_window = nullptr;
		}

		// Finish.
		return true;
	}

	bool update(void) {
		_context.updatedSeconds += _context.delta;
		if (++_context.updatedFrameCount >= _context.expectedFrameRate * 3) { // 3 seconds.
			if (_context.updatedSeconds > 0)
				_context.fps = (unsigned)(_context.updatedFrameCount / _context.updatedSeconds);
			else
				_context.fps = 0;
			_context.expectedFrameRate = APPLICATION_IDLE_FRAME_RATE;
			_context.updatedFrameCount = 0;
			_context.updatedSeconds = 0;
		}

		const long long begin = DateTime::ticks();
		_context.delta = begin >= _stamp ? DateTime::toSeconds(begin - _stamp) : 0;
		_stamp = begin;

		const bool alive = updateImGui(_context.delta, _context.mouseCursorIndicated);

		const Color cls(0x2e, 0x32, 0x38, 0xff);
		_effects->prepare(_window, _renderer, _workspace, _context.delta);
		_renderer->clip(0, 0, _renderer->width(), _renderer->height());
		_renderer->clear(&cls);
		{
			ImGui::NewFrame();

			_context.mouseCursorIndicated = false;
			const unsigned fps = _workspace->update(
				_window, _renderer,
				_project, _executable,
				_primitives,
				_context.delta, _context.fps, alive,
				&_context.mouseCursorIndicated
			);
			requestFrameRate(fps);

			ImGui::Render();

			ImGuiSDL::Render(ImGui::GetDrawData());
		}
		_effects->finish(_window, _renderer, _workspace);
		_window->update();

		const long long end = DateTime::ticks();
		const long long diff = end >= begin ? end - begin : 0;
		const double elapsed = DateTime::toSeconds(diff);
		const double expected = 1.0 / _context.expectedFrameRate;
		const double rest = expected - elapsed;
		if (rest > 0)
			DateTime::sleep((int)(rest * 1000));

		return alive;
	}

private:
	void openImGui(void) {
		SDL_Window* wnd = (SDL_Window*)_window->pointer();
		SDL_Renderer* rnd = (SDL_Renderer*)_renderer->pointer();

		ImGui::CreateContext();
		ImGuiSDL::Initialize(rnd, WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT);

		ImGuiIO &io = ImGui::GetIO();

		io.IniFilename = nullptr;

		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
		io.BackendFlags |= ImGuiBackendFlags_HasGamepad | ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos;

		io.KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
		io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
		io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
		io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
		io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
		io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
		io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
		io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
		io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
		io.KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
		io.KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
		io.KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
		io.KeyMap[ImGuiKey_Space] = SDL_SCANCODE_SPACE;
		io.KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
		io.KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
		io.KeyMap[ImGuiKey_KeyPadEnter] = SDL_SCANCODE_KP_ENTER;
		io.KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
		io.KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
		io.KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
		io.KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
		io.KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
		io.KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;

		io.SetClipboardTextFn = [] (void* userdata, const char* text) -> void {
			Context* data = (Context*)userdata;
			(void)data;
			SDL_SetClipboardText(text);
		};
		io.GetClipboardTextFn = [] (void* userdata) -> const char* {
			Context* data = (Context*)userdata;
			if (data->clipboardTextData)
				SDL_free(data->clipboardTextData);
			data->clipboardTextData = SDL_GetClipboardText();

			return data->clipboardTextData;
		};
		io.ClipboardUserData = &_context;

		_context.mouseCursors[ImGuiMouseCursor_Arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
		_context.mouseCursors[ImGuiMouseCursor_TextInput] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
		_context.mouseCursors[ImGuiMouseCursor_ResizeAll] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
		_context.mouseCursors[ImGuiMouseCursor_ResizeNS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
		_context.mouseCursors[ImGuiMouseCursor_ResizeEW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
		_context.mouseCursors[ImGuiMouseCursor_ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
		_context.mouseCursors[ImGuiMouseCursor_ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
		_context.mouseCursors[ImGuiMouseCursor_Hand] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
		_context.mouseCursors[ImGuiMouseCursor_NotAllowed] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);

		_context.mouseCanUseGlobalState = !!strncmp(SDL_GetCurrentVideoDriver(), "wayland", 7);

#if defined BITTY_OS_WIN
		SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);
		SDL_GetWindowWMInfo(wnd, &wmInfo);
		io.ImeWindowHandle = wmInfo.info.win.window;
#else /* BITTY_OS_WIN */
		(void)wnd;

		Platform::openInput();
		io.ImeSetInputScreenPosFn = Platform::inputScreenPosition;
#endif /* BITTY_OS_WIN */

		fprintf(stdout, "ImGui opened.\n");
	}
	void closeImGui(void) {
		ImGuiIO &io = ImGui::GetIO();

#if defined BITTY_OS_WIN
		(void)io;
#else /* BITTY_OS_WIN */
		Platform::closeInput();
		io.ImeSetInputScreenPosFn = nullptr;
#endif /* BITTY_OS_WIN */

		if (_context.clipboardTextData)
			SDL_free(_context.clipboardTextData);
		_context.clipboardTextData = nullptr;

		for (ImGuiMouseCursor i = 0; i < ImGuiMouseCursor_COUNT; ++i)
			SDL_FreeCursor(_context.mouseCursors[i]);
		memset(_context.mouseCursors, 0, sizeof(_context.mouseCursors));

		ImGuiSDL::Deinitialize();
		ImGui::DestroyContext();

		fprintf(stdout, "ImGui closed.\n");
	}

	bool updateImGui(double delta, bool mouseCursorIndicated) {
		SDL_Window* wnd = (SDL_Window*)_window->pointer();

		ImGuiIO &io = ImGui::GetIO();

		SDL_Event evt;
		bool alive = true;
		bool reset = false;
		while (SDL_PollEvent(&evt)) {
			switch (evt.type) {
			case SDL_QUIT:
				fprintf(stdout, "SDL: SDL_QUIT.\n");

				alive = !_workspace->quit(
					_window, _renderer,
					_project,
					_executable,
					_primitives
				);

				break;
			case SDL_WINDOWEVENT:
				switch (evt.window.event) {
				case SDL_WINDOWEVENT_RESIZED:
					fprintf(stdout, "SDL: SDL_WINDOWEVENT_RESIZED.\n");

					{
						int w = 0, h = 0;
						SDL_GetWindowSize(wnd, &w, &h);
						_workspace->resized(_window, _renderer, _project, Math::Vec2i(w, h));
					}

					break;
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					fprintf(stdout, "SDL: SDL_WINDOWEVENT_SIZE_CHANGED.\n");

					reset = true;

					break;
				case SDL_WINDOWEVENT_MOVED:
					break;
				case SDL_WINDOWEVENT_MAXIMIZED:
					fprintf(stdout, "SDL: SDL_WINDOWEVENT_MAXIMIZED.\n");

					_workspace->maximized(_window, _renderer);

					break;
				case SDL_WINDOWEVENT_RESTORED:
					fprintf(stdout, "SDL: SDL_WINDOWEVENT_RESTORED.\n");

					_workspace->restored(_window, _renderer);

					reset = true;

					break;
				case SDL_WINDOWEVENT_FOCUS_GAINED:
					fprintf(stdout, "SDL: SDL_WINDOWEVENT_FOCUS_GAINED.\n");

					_workspace->focusGained(_window, _renderer, _project, _executable, _primitives);

					break;
				case SDL_WINDOWEVENT_FOCUS_LOST:
					fprintf(stdout, "SDL: SDL_WINDOWEVENT_FOCUS_LOST.\n");

					_workspace->focusLost(_window, _renderer, _project, _executable, _primitives);

					break;
				}

				break;
			case SDL_RENDER_TARGETS_RESET:
				fprintf(stdout, "SDL: SDL_RENDER_TARGETS_RESET.\n");

				_workspace->renderTargetsReset(_window, _renderer, _project, _executable, _primitives);

				reset = true;

				break;
			case SDL_RENDER_DEVICE_RESET:
				fprintf(stdout, "SDL: SDL_RENDER_DEVICE_RESET.\n");

				break;
			case SDL_MOUSEWHEEL:
				if (evt.wheel.x > 0)
					io.MouseWheelH += 1;
				if (evt.wheel.x < 0)
					io.MouseWheelH -= 1;
				if (evt.wheel.y > 0)
					io.MouseWheel += 1;
				if (evt.wheel.y < 0)
					io.MouseWheel -= 1;

				requestFrameRate(APPLICATION_INPUTING_FRAME_RATE);

				break;
			case SDL_MOUSEBUTTONDOWN:
				if (evt.button.button == SDL_BUTTON_LEFT)
					_context.mousePressed[0] = true;
				if (evt.button.button == SDL_BUTTON_RIGHT)
					_context.mousePressed[1] = true;
				if (evt.button.button == SDL_BUTTON_MIDDLE)
					_context.mousePressed[2] = true;

				requestFrameRate(APPLICATION_INPUTING_FRAME_RATE);

				break;
			case SDL_KEYDOWN: // Fall through.
			case SDL_KEYUP: {
					const SDL_Keymod mod = SDL_GetModState();
					int key = evt.key.keysym.scancode;
					if (!(mod & KMOD_NUM)) {
						switch (key) {
						case SDL_SCANCODE_KP_1:
							key = SDL_SCANCODE_END;

							break;
						case SDL_SCANCODE_KP_3:
							key = SDL_SCANCODE_PAGEDOWN;

							break;
						case SDL_SCANCODE_KP_7:
							key = SDL_SCANCODE_HOME;

							break;
						case SDL_SCANCODE_KP_9:
							key = SDL_SCANCODE_PAGEUP;

							break;
						case SDL_SCANCODE_KP_PERIOD:
							key = SDL_SCANCODE_DELETE;

							break;
						default:
							// Do nothing.

							break;
						}
					}
					assert(key >= 0 && key < BITTY_COUNTOF(io.KeysDown));
					io.KeysDown[key] = evt.type == SDL_KEYDOWN;
					io.KeyShift = !!(mod & KMOD_SHIFT);
					if (!!(mod & KMOD_LCTRL) && !(mod & KMOD_RCTRL) && !(mod & KMOD_LALT) && !!(mod & KMOD_RALT)) {
						// FIXME: this is not a perfect solution, just works, to avoid some
						// unexpected key events, eg. RAlt+A on Poland keyboard also triggers
						// LCtrl.
						io.KeyCtrl = false;
					} else {
						io.KeyCtrl = !!(mod & KMOD_CTRL);
					}
					io.KeyAlt = !!(mod & KMOD_ALT);
#if defined BITTY_OS_WIN
					io.KeySuper = false;
#else /* BITTY_OS_WIN */
					io.KeySuper = !!(mod & KMOD_GUI);
#endif /* BITTY_OS_WIN */

					requestFrameRate(APPLICATION_INPUTING_FRAME_RATE);
				}

				break;
			case SDL_TEXTINPUT:
				io.AddInputCharactersUTF8(evt.text.text);

				requestFrameRate(APPLICATION_INPUTING_FRAME_RATE);

				break;
			default: // Do nothing.
				break;
			}
		}
		if (reset) {
			ImGuiSDL::Reset();

			_resources->resetRenderTargets();

			_effects->renderTargetsReset();
		}

		io.DeltaTime = (float)delta;

		do {
			const int wndW = _window->width(), wndH = _window->height();
			const int displayW = _renderer->width(), displayH = _renderer->height();
			io.DisplaySize = ImVec2((float)displayW, (float)displayH);
			if (wndW > 0 && wndH > 0)
				io.DisplayFramebufferScale = ImVec2((float)displayW / wndW, (float)displayH / wndH);
		} while (false);

		do {
			if (io.WantSetMousePos)
				SDL_WarpMouseInWindow(wnd, (int)io.MousePos.x, (int)io.MousePos.y);
			else
				io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

			int mouseX = 0, mouseY = 0;
			const Uint32 mouseButtons = SDL_GetMouseState(&mouseX, &mouseY);
			io.MouseDown[0] = _context.mousePressed[0] || !!(mouseButtons & SDL_BUTTON(SDL_BUTTON_LEFT)); // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
			io.MouseDown[1] = _context.mousePressed[1] || !!(mouseButtons & SDL_BUTTON(SDL_BUTTON_RIGHT));
			io.MouseDown[2] = _context.mousePressed[2] || !!(mouseButtons & SDL_BUTTON(SDL_BUTTON_MIDDLE));
			_context.mousePressed[0] = _context.mousePressed[1] = _context.mousePressed[2] = false;

#if SDL_VERSION_ATLEAST(2, 0, 4) && (defined BITTY_OS_WIN || defined BITTY_OS_MAC || defined BITTY_OS_LINUX)
			SDL_Window* focusedWindow = SDL_GetKeyboardFocus();
			if (wnd == focusedWindow) {
				if (_context.mouseCanUseGlobalState) {
					// `SDL_GetMouseState` gives mouse position seemingly based on the last window entered/focused(?)
					// The creation of a new windows at runtime and `SDL_CaptureMouse` both seems to severely mess up with that, so we retrieve that position globally.
					// Won't use this workaround when on Wayland, as there is no global mouse position.
					int wndX = 0, wndY = 0;
					SDL_GetWindowPosition(focusedWindow, &wndX, &wndY);
					SDL_GetGlobalMouseState(&mouseX, &mouseY);
					mouseX -= wndX;
					mouseY -= wndY;
				}
				const int scale = _renderer->scale() / _window->scale();
				ImVec2 pos((float)mouseX, (float)mouseY);
				if (scale != 1) {
					pos.x /= scale;
					pos.y /= scale;
				}
				if (_context.mousePosition.x != pos.x || _context.mousePosition.y != pos.y) {
					_context.mousePosition = pos;

					requestFrameRate(APPLICATION_INPUTING_FRAME_RATE);
				}
				io.MousePos = pos;
			}

			// `SDL_CaptureMouse` let the OS know e.g. that our ImGui drag outside the SDL window boundaries shouldn't e.g. trigger the OS window resize cursor.
			// The function is only supported from SDL 2.0.4 (released Jan 2016).
			const bool anyMouseButtonDown = ImGui::IsAnyMouseDown();
			SDL_CaptureMouse(anyMouseButtonDown ? SDL_TRUE : SDL_FALSE);
#else /* Platform macro. */
			if (SDL_GetWindowFlags(wnd) & SDL_WINDOW_INPUT_FOCUS)
				io.MousePos = ImVec2((float)mouseX, (float)mouseY);
#endif /* Platform macro. */
		} while (false);

		do {
			if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
				break;

			// Skip setting cursor if it's already indicated.
			if (mouseCursorIndicated && _workspace->canvasHovering())
				break;

			ImGuiMouseCursor imguiCursor = ImGui::GetMouseCursor();
			if (io.MouseDrawCursor || imguiCursor == ImGuiMouseCursor_None) {
				// Hide OS mouse cursor if imgui is drawing it or if it wants no cursor.
				SDL_ShowCursor(SDL_FALSE);
			} else {
				// Show OS mouse cursor.
				SDL_SetCursor(_context.mouseCursors[imguiCursor] ? _context.mouseCursors[imguiCursor] : _context.mouseCursors[ImGuiMouseCursor_Arrow]);
				SDL_ShowCursor(SDL_TRUE);
			}
		} while (false);

		do {
			memset(io.NavInputs, 0, sizeof(io.NavInputs));
			if (!(io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad))
				break;

			SDL_GameController* gameController = SDL_GameControllerOpen(0);
			if (!gameController) {
				io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;

				break;
			}

#define MAP_BUTTON(NAV_NO, BUTTON_NO) { io.NavInputs[NAV_NO] = SDL_GameControllerGetButton(gameController, BUTTON_NO) ? 1.0f : 0.0f; }
#define MAP_ANALOG(NAV_NO, AXIS_NO, V0, V1) { float vn = (float)(SDL_GameControllerGetAxis(gameController, AXIS_NO) - V0) / (float)(V1 - V0); if (vn > 1.0f) vn = 1.0f; if (vn > 0.0f && io.NavInputs[NAV_NO] < vn) io.NavInputs[NAV_NO] = vn; }
			const int thumbDeadZone = 8000; // "SDL_gamecontroller.h" suggests using this value.
			MAP_BUTTON(ImGuiNavInput_Activate, SDL_CONTROLLER_BUTTON_A);              // A/Cross.
			MAP_BUTTON(ImGuiNavInput_Cancel, SDL_CONTROLLER_BUTTON_B);                // B/Circle.
			MAP_BUTTON(ImGuiNavInput_Menu, SDL_CONTROLLER_BUTTON_X);                  // X/Square.
			MAP_BUTTON(ImGuiNavInput_Input, SDL_CONTROLLER_BUTTON_Y);                 // Y/Triangle.
			MAP_BUTTON(ImGuiNavInput_DpadLeft, SDL_CONTROLLER_BUTTON_DPAD_LEFT);      // D-Pad left.
			MAP_BUTTON(ImGuiNavInput_DpadRight, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);    // D-Pad right.
			MAP_BUTTON(ImGuiNavInput_DpadUp, SDL_CONTROLLER_BUTTON_DPAD_UP);          // D-Pad up.
			MAP_BUTTON(ImGuiNavInput_DpadDown, SDL_CONTROLLER_BUTTON_DPAD_DOWN);      // D-Pad down.
			MAP_BUTTON(ImGuiNavInput_FocusPrev, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);  // L1/LB.
			MAP_BUTTON(ImGuiNavInput_FocusNext, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER); // R1/RB.
			MAP_BUTTON(ImGuiNavInput_TweakSlow, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);  // L1/LB.
			MAP_BUTTON(ImGuiNavInput_TweakFast, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER); // R1/RB.
			MAP_ANALOG(ImGuiNavInput_LStickLeft, SDL_CONTROLLER_AXIS_LEFTX, -thumbDeadZone, -32768);
			MAP_ANALOG(ImGuiNavInput_LStickRight, SDL_CONTROLLER_AXIS_LEFTX, +thumbDeadZone, +32767);
			MAP_ANALOG(ImGuiNavInput_LStickUp, SDL_CONTROLLER_AXIS_LEFTY, -thumbDeadZone, -32767);
			MAP_ANALOG(ImGuiNavInput_LStickDown, SDL_CONTROLLER_AXIS_LEFTY, +thumbDeadZone, +32767);

			io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
#undef MAP_BUTTON
#undef MAP_ANALOG
		} while (false);

		return alive;
	}

	void requestFrameRate(unsigned fps) {
		if (fps > _context.expectedFrameRate) {
			_context.expectedFrameRate = fps;
			_context.updatedFrameCount = 0;
		}
	}

	static void help(void) {
#if defined BITTY_OS_WIN || defined BITTY_OS_MAC || defined BITTY_OS_LINUX
#	if defined BITTY_OS_WIN
#		define _BITTY_EXE "bitty.exe"
#	elif defined BITTY_OS_MAC
#		define _BITTY_EXE "bitty"
#	elif defined BITTY_OS_LINUX
#		define _BITTY_EXE "bitty"
#	endif /* Platform macro. */

		fprintf(
			stdout,
			"Usage: " _BITTY_EXE
			" [-" WORKSPACE_OPTION_APPLICATION_CWD_KEY " \"PATH\"]"
#if defined BITTY_OS_WIN
			" [-" WORKSPACE_OPTION_APPLICATION_CONSOLE_ENABLED_KEY "]"
#endif /* BITTY_OS_WIN */
			" [-" WORKSPACE_OPTION_WINDOW_BORDERLESS_KEY "]"
			" [-" WORKSPACE_OPTION_WINDOW_SIZE_KEY " MxN]"
			" [-" WORKSPACE_OPTION_WINDOW_HIGH_DPI_DISABLED_KEY " ]"
			" [-" WORKSPACE_OPTION_RENDERER_X2_KEY "]"
			" [-" WORKSPACE_OPTION_RENDERER_X3_KEY "]"
#if BITTY_EFFECTS_ENABLED
			" [-" WORKSPACE_OPTION_RENDERER_EFFECTS_DISABLED_KEY "]"
#endif /* BITTY_EFFECTS_ENABLED */
			" [-" WORKSPACE_OPTION_PLUGIN_DISABLED_KEY "]"
			" [-" WORKSPACE_OPTION_EXECUTABLE_TIMEOUT_DISABLED_KEY "]"
			" [> log.txt]\n"
		);
		fprintf(stdout, "  -" WORKSPACE_OPTION_APPLICATION_CWD_KEY             " \"PATH\" Specify the working directory.\n");
#if defined BITTY_OS_WIN
		fprintf(stdout, "  -" WORKSPACE_OPTION_APPLICATION_CONSOLE_ENABLED_KEY "        Enable console window.\n");
#endif /* BITTY_OS_WIN */
		fprintf(stdout, "  -" WORKSPACE_OPTION_WINDOW_BORDERLESS_KEY           "        Run with borderless window.\n");
		fprintf(stdout, "  -" WORKSPACE_OPTION_WINDOW_SIZE_KEY                 " MxN    Specify window size.\n");
		fprintf(stdout, "  -" WORKSPACE_OPTION_WINDOW_HIGH_DPI_DISABLED_KEY                 "        Disable high-DPI.\n");
		fprintf(stdout, "  -" WORKSPACE_OPTION_RENDERER_X2_KEY                 "       Set renderer scale to x2.\n");
		fprintf(stdout, "  -" WORKSPACE_OPTION_RENDERER_X3_KEY                 "       Set renderer scale to x3.\n");
#if BITTY_EFFECTS_ENABLED
		fprintf(stdout, "  -" WORKSPACE_OPTION_RENDERER_EFFECTS_DISABLED_KEY             "        Disable effects.\n");
#endif /* BITTY_EFFECTS_ENABLED */
		fprintf(stdout, "  -" WORKSPACE_OPTION_PLUGIN_DISABLED_KEY             "        Disable plugins.\n");
		fprintf(stdout, "  -" WORKSPACE_OPTION_EXECUTABLE_TIMEOUT_DISABLED_KEY "        Disable invoking timeout.\n");
		fprintf(stdout, "\n");

#	undef _BITTY_EXE
#endif /* Platform macro. */
	}
	static void versions(void) {
		fprintf(stdout, BITTY_NAME " v" BITTY_VERSION_STRING " - " BITTY_OS ", with %s, " BITTY_CP "\n", Platform::isLittleEndian() ? "little-endian" : "big-endian");
		fprintf(stdout, "\n");
		fprintf(stdout, "      Lua v" LUA_VERSION_MAJOR "." LUA_VERSION_MINOR "." LUA_VERSION_RELEASE "\n");
		fprintf(stdout, "      SDL v%d.%d.%d\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
		fprintf(stdout, "SDL mixer v%d.%d.%d\n", SDL_MIXER_MAJOR_VERSION, SDL_MIXER_MINOR_VERSION, SDL_MIXER_PATCHLEVEL);
		fprintf(stdout, "    ImGui v" IMGUI_VERSION "\n");
#if !defined BITTY_OS_HTML
		fprintf(stdout, " Mongoose v" MG_VERSION "\n");
		fprintf(stdout, "     cURL v" LIBCURL_VERSION "\n");
#endif /* BITTY_OS_HTML */
		fprintf(stdout, "RapidJSON v" RAPIDJSON_VERSION_STRING "\n");
		fprintf(stdout, "     zlib v" ZLIB_VERSION "\n");
		fprintf(stdout, "\n");
	}
	static void paths(void) {
		const std::string exeFile = Unicode::toOs(Path::executableFile());
		const std::string currentDir = Unicode::toOs(Path::currentDirectory());
		const std::string docDir = Unicode::toOs(Path::documentDirectory());
		const std::string writableDir = Unicode::toOs(Path::writableDirectory());

		fprintf(stdout, "   Executable file: \"%s\".\n", exeFile.c_str());
		fprintf(stdout, " Current directory: \"%s\".\n", currentDir.c_str());
		fprintf(stdout, "Document directory: \"%s\".\n", docDir.c_str());
		fprintf(stdout, "Writable directory: \"%s\".\n", writableDir.c_str());
		fprintf(stdout, "\n");
	}
};

class Application* createApplication(class Workspace* workspace, int argc, const char* argv[]) {
	// Prepare.
	Text::Dictionary options = applicationParseArgs(argc, argv);

	// Initialize locale.
	Platform::locale("");
	Text::locale("C");
	fprintf(stdout, "\n");

	// Initialize working directory.
	Text::Dictionary::const_iterator cwdOpt = options.find(WORKSPACE_OPTION_APPLICATION_CWD_KEY);
	if (cwdOpt == options.end()) {
#if defined BITTY_OS_WIN || defined BITTY_OS_MAC || defined BITTY_OS_LINUX
		std::string path = Path::executableFile();
		Path::split(path, nullptr, nullptr, &path);
		Path::currentDirectory(path.c_str());
#endif /* Platform macro. */
	} else {
		std::string path = cwdOpt->second;
		path = Unicode::fromOs(path);
		if (path.size() >= 2 && path.front() == '\"' && path.back() == '\"') {
			path.erase(path.begin());
			path.pop_back();
		}
		Path::uniform(path);
		Path::currentDirectory(path.c_str());
	}

	// Initialize the SDL library.
#if defined BITTY_OS_HTML
	if (SDL_Init(SDL_INIT_EVERYTHING & ~SDL_INIT_HAPTIC) < 0)
		fprintf(stderr, "Unable to open SDL: %s\n", SDL_GetError());
	if (Mix_Init(MIX_INIT_FLAC | MIX_INIT_MOD | MIX_INIT_MP3 | MIX_INIT_OGG | MIX_INIT_MID) < 0)
		fprintf(stderr, "Unable to open SDL mixer: %s\n", SDL_GetError());
	if (Mix_OpenAudioDevice(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096, nullptr, SDL_AUDIO_ALLOW_ANY_CHANGE))
		fprintf(stderr, "Unable to open audio: %s\n", SDL_GetError());
#else /* BITTY_OS_HTML */
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
		fprintf(stderr, "Unable to open SDL: %s\n", SDL_GetError());
	if (Mix_Init(MIX_INIT_FLAC | MIX_INIT_MOD | MIX_INIT_MP3 | MIX_INIT_OGG | MIX_INIT_MID | MIX_INIT_OPUS) < 0)
		fprintf(stderr, "Unable to open SDL mixer: %s\n", SDL_GetError());
	if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 512) < 0)
		fprintf(stderr, "Unable to open audio: %s\n", SDL_GetError());
#endif /* BITTY_OS_HTML */

	// Create an application instance.
	Application* result = new Application(workspace);
	result->open(options);

	// Finish.
	return result;
}

void destroyApplication(class Application* app) {
	// Destroy the application instance.
	app->close();
	delete app;

	// Dispose the SDL library.
	Mix_CloseAudio();
	Mix_Quit();
	SDL_Quit();
}

bool updateApplication(class Application* app) {
	// Update the application.
	const bool result = app->update();

	return result;
}

/* ===========================================================================} */
