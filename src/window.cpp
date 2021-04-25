/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "window.h"
#include <SDL.h>

/*
** {===========================================================================
** Window
*/

class WindowImpl : public Window {
private:
	SDL_Window* _window = nullptr;
	int _scale = 1;

	bool _resizable = true;
#if WINDOW_LAZY_TOGGLE_FULLSCREEN
	bool _lazySetFullscreen = false;
	bool _lazyFullscreenValue = false;
#endif /* WINDOW_LAZY_TOGGLE_FULLSCREEN */

public:
	virtual ~WindowImpl() {
	}

	virtual void* pointer(void) override {
		return _window;
	}

	virtual bool open(
		const char* title,
		int displayIndex, int width, int height,
		int minWidth, int minHeight, bool borderless,
		bool highDpi, bool opengl
	) override {
		if (_window)
			return false;

#if defined BITTY_OS_HTML
		Uint32 flags = SDL_WINDOW_RESIZABLE;
		if (borderless)
			flags |= SDL_WINDOW_BORDERLESS;
		if (opengl)
			flags |= SDL_WINDOW_OPENGL;
#else /* BITTY_OS_HTML */
		Uint32 flags = SDL_WINDOW_RESIZABLE;
		if (borderless)
			flags |= SDL_WINDOW_BORDERLESS;
		if (highDpi)
			flags |= SDL_WINDOW_ALLOW_HIGHDPI;
		if (opengl)
			flags |= SDL_WINDOW_OPENGL;
#endif /* BITTY_OS_HTML */
		_window = SDL_CreateWindow(
			title,
			SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex), SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex),
			width, height,
			flags
		);
		SDL_SetWindowMinimumSize(_window, minWidth, minHeight);

		fprintf(stdout, "Window opened.\n");

		return true;
	}
	virtual bool close(void) override {
		if (!_window)
			return false;

		SDL_DestroyWindow(_window);
		_window = nullptr;

		fprintf(stdout, "Window closed.\n");

		return true;
	}

	virtual const char* title(void) const override {
		if (!_window)
			return nullptr;

		return SDL_GetWindowTitle(_window);
	}
	virtual void title(const char* txt) override {
		if (!_window)
			return;

		SDL_SetWindowTitle(_window, txt);
	}

	virtual int displayIndex(void) const override {
		return SDL_GetWindowDisplayIndex(_window);
	}
	virtual void displayIndex(int idx) override {
		SDL_SetWindowPosition(_window, SDL_WINDOWPOS_CENTERED_DISPLAY(idx), SDL_WINDOWPOS_CENTERED_DISPLAY(idx));
	}

	virtual void centralize(void) override {
		const int idx = SDL_GetWindowDisplayIndex(_window);
		SDL_SetWindowPosition(_window, SDL_WINDOWPOS_CENTERED_DISPLAY(idx), SDL_WINDOWPOS_CENTERED_DISPLAY(idx));
	}

	virtual Math::Vec2i position(void) const override {
		int x = 0, y = 0;
		SDL_GetWindowPosition(_window, &x, &y);

		return Math::Vec2i(x, y);
	}
	virtual void position(const Math::Vec2i &val) override {
		SDL_SetWindowPosition(_window, val.x, val.y);
	}

	virtual Math::Vec2i size(void) const override {
		int w = 0, h = 0;
		SDL_GetWindowSize(_window, &w, &h);

		return Math::Vec2i(w, h);
	}
	virtual void size(const Math::Vec2i &val) override {
		const int idx = SDL_GetWindowDisplayIndex(_window);
		SDL_Rect bound;
		SDL_GetDisplayUsableBounds(idx, &bound);

		SDL_SetWindowSize(_window, std::min((int)val.x, bound.w), std::min((int)val.y, bound.h));
	}

	virtual Math::Vec2i minimumSize(void) const override {
		int w = 0, h = 0;
		SDL_GetWindowMinimumSize(_window, &w, &h);

		return Math::Vec2i(w, h);
	}
	virtual void minimumSize(const Math::Vec2i &val) override {
		SDL_SetWindowMinimumSize(_window, (int)val.x, (int)val.y);
	}
	virtual Math::Vec2i maximumSize(void) const override {
		int w = 0, h = 0;
		SDL_GetWindowMaximumSize(_window, &w, &h);

		return Math::Vec2i(w, h);
	}
	virtual void maximumSize(const Math::Vec2i &val) override {
		SDL_SetWindowMaximumSize(_window, (int)val.x, (int)val.y);
	}

	virtual bool resizable(void) const override {
		return _resizable;
	}
	virtual void resizable(bool val) override {
		_resizable = val;

		SDL_SetWindowResizable(_window, val ? SDL_TRUE : SDL_FALSE);
	}

	virtual bool maximized(void) const override {
		const Uint32 flags = SDL_GetWindowFlags(_window);

		return !!(flags & SDL_WINDOW_MAXIMIZED);
	}
	virtual void maximize(void) override {
		SDL_MaximizeWindow(_window);
	}
	virtual bool minimized(void) const override {
		const Uint32 flags = SDL_GetWindowFlags(_window);

		return !!(flags & SDL_WINDOW_MINIMIZED);
	}
	virtual void minimize(void) override {
		SDL_MinimizeWindow(_window);
	}
	virtual void restore(void) const override {
		SDL_RestoreWindow(_window);
	}
	virtual bool fullscreen(void) const override {
		const Uint32 flags = SDL_GetWindowFlags(_window);

		return !!(flags & SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
	virtual void fullscreen(bool val) override {
#if WINDOW_LAZY_TOGGLE_FULLSCREEN
		_lazySetFullscreen = true;
		_lazyFullscreenValue = val;
#else /* WINDOW_LAZY_TOGGLE_FULLSCREEN */
		if (val) {
			Uint32 flags = SDL_GetWindowFlags(_window);
			flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
			SDL_SetWindowFullscreen(_window, flags);
		} else {
			Uint32 flags = SDL_GetWindowFlags(_window);
			flags &= ~SDL_WINDOW_FULLSCREEN_DESKTOP;
			SDL_SetWindowFullscreen(_window, flags);
		}
#endif /* WINDOW_LAZY_TOGGLE_FULLSCREEN */
	}

	virtual int width(void) const override {
		int width = 0, height = 0;
		SDL_GetWindowSize(_window, &width, &height);

		return width;
	}
	virtual int height(void) const override {
		int width = 0, height = 0;
		SDL_GetWindowSize(_window, &width, &height);

		return height;
	}

	virtual int scale(void) const override {
		return _scale;
	}
	virtual void scale(int val) override {
		val = std::max(val, 1);
		if (_scale == val)
			return;

		_scale = val;
	}

	virtual void update(void) override {
#if WINDOW_LAZY_TOGGLE_FULLSCREEN
		if (_lazySetFullscreen) {
			_lazySetFullscreen = false;
			if (_lazyFullscreenValue) {
				Uint32 flags = SDL_GetWindowFlags(_window);
				flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
				SDL_SetWindowFullscreen(_window, flags);
			} else {
				Uint32 flags = SDL_GetWindowFlags(_window);
				flags &= ~SDL_WINDOW_FULLSCREEN_DESKTOP;
				SDL_SetWindowFullscreen(_window, flags);
			}
		}
#endif /* WINDOW_LAZY_TOGGLE_FULLSCREEN */
	}
};

Window* Window::create(void) {
	WindowImpl* result = new WindowImpl();

	return result;
}

void Window::destroy(Window* ptr) {
	WindowImpl* impl = static_cast<WindowImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
