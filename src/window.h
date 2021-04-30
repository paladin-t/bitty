/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __WINDOW_H__
#define __WINDOW_H__

#include "bitty.h"
#include "mathematics.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef WINDOW_MIN_WIDTH
#	define WINDOW_MIN_WIDTH 720
#endif /* WINDOW_MIN_WIDTH */
#ifndef WINDOW_MIN_HEIGHT
#	define WINDOW_MIN_HEIGHT 480
#endif /* WINDOW_MIN_HEIGHT */

#ifndef WINDOW_DEFAULT_WIDTH
#	define WINDOW_DEFAULT_WIDTH 900
#endif /* WINDOW_DEFAULT_WIDTH */
#ifndef WINDOW_DEFAULT_HEIGHT
#	define WINDOW_DEFAULT_HEIGHT 600
#endif /* WINDOW_DEFAULT_HEIGHT */

#ifndef WINDOW_LAZY_TOGGLE_FULLSCREEN
#	if BITTY_EFFECTS_ENABLED
#		define WINDOW_LAZY_TOGGLE_FULLSCREEN 1
#	else /* BITTY_EFFECTS_ENABLED */
#		define WINDOW_LAZY_TOGGLE_FULLSCREEN 0
#	endif /* BITTY_EFFECTS_ENABLED */
#endif /* WINDOW_LAZY_TOGGLE_FULLSCREEN */

/* ===========================================================================} */

/*
** {===========================================================================
** Window
*/

/**
 * @brief Window structure and context.
 */
class Window {
public:
	/**
	 * @brief Gets the raw pointer.
	 *
	 * @return `SDL_Window*`.
	 */
	virtual void* pointer(void) = 0;

	/**
	 * @brief Opens the window for further operation.
	 */
	virtual bool open(
		const char* title,
		int displayIndex, int width, int height,
		int minWidth, int minHeight, bool borderless,
		bool highDpi, bool opengl
	) = 0;
	/**
	 * @brief Closes the window after all operations.
	 */
	virtual bool close(void) = 0;

	/**
	 * @brief Gets the title of the window.
	 */
	virtual const char* title(void) const = 0;
	/**
	 * @brief Sets the title of the window.
	 */
	virtual void title(const char* txt) = 0;

	/**
	 * @brief Gets the current display index of the window.
	 */
	virtual int displayIndex(void) const = 0;
	/**
	 * @brief Sets the current display index of the window.
	 */
	virtual void displayIndex(int idx) = 0;

	/**
	 * @brief Centralize of the window.
	 */
	virtual void centralize(void) = 0;

	/**
	 * @brief Gets the current position of the window.
	 */
	virtual Math::Vec2i position(void) const = 0;
	/**
	 * @brief Sets the current position of the window.
	 */
	virtual void position(const Math::Vec2i &val) = 0;

	/**
	 * @brief Gets the current size of the window.
	 */
	virtual Math::Vec2i size(void) const = 0;
	/**
	 * @brief Sets the current size of the window.
	 */
	virtual void size(const Math::Vec2i &val) = 0;

	/**
	 * @brief Gets the minimum size of the window.
	 */
	virtual Math::Vec2i minimumSize(void) const = 0;
	/**
	 * @brief Sets the minimum size of the window.
	 */
	virtual void minimumSize(const Math::Vec2i &val) = 0;
	/**
	 * @brief Gets the maximum size of the window.
	 */
	virtual Math::Vec2i maximumSize(void) const = 0;
	/**
	 * @brief Sets the maximum size of the window.
	 */
	virtual void maximumSize(const Math::Vec2i &val) = 0;

	/**
	 * @brief Gets whether the window is bordered.
	 */
	virtual bool bordered(void) const = 0;
	/**
	 * @brief Sets whether the window is bordered.
	 */
	virtual void bordered(bool val) = 0;

	/**
	 * @brief Gets whether the window is resizable.
	 */
	virtual bool resizable(void) const = 0;
	/**
	 * @brief Sets whether the window is resizable.
	 */
	virtual void resizable(bool val) = 0;

	/**
	 * @brief Gets whether the window is maximized.
	 */
	virtual bool maximized(void) const = 0;
	/**
	 * @brief Sets whether the window is maximized.
	 */
	virtual void maximize(void) = 0;
	/**
	 * @brief Gets whether the window is minimized.
	 */
	virtual bool minimized(void) const = 0;
	/**
	 * @brief Sets whether the window is minimized.
	 */
	virtual void minimize(void) = 0;
	/**
	 * @brief Restores the window.
	 */
	virtual void restore(void) const = 0;
	/**
	 * @brief Gets whether the window is in fullscreen mode.
	 */
	virtual bool fullscreen(void) const = 0;
	/**
	 * @brief Sets whether the window is in fullscreen mode.
	 */
	virtual void fullscreen(bool val) = 0;

	/**
	 * @brief Gets the width of the window.
	 */
	virtual int width(void) const = 0;
	/**
	 * @brief Gets the height of the window.
	 */
	virtual int height(void) const = 0;

	/**
	 * @brief Gets the scale of the window.
	 */
	virtual int scale(void) const = 0;
	/**
	 * @brief Sets the scale of the window.
	 */
	virtual void scale(int val) = 0;

	/**
	 * @brief Updates the window.
	 */
	virtual void update(void) = 0;

	static Window* create(void);
	static void destroy(Window* ptr);
};

/* ===========================================================================} */

#endif /* __WINDOW_H__ */
