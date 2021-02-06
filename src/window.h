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
	 * @return `SDL_Window*`.
	 */
	virtual const void* pointer(void) const = 0;
	virtual void* pointer(void) = 0;

	virtual bool open(
		const char* title,
		int displayIndex, int width, int height,
		int minWidth, int minHeight,
		bool borderless,
		bool highDpi
	) = 0;
	virtual bool close(void) = 0;

	virtual const char* title(void) const = 0;
	virtual void title(const char* txt) = 0;

	virtual int displayIndex(void) const = 0;
	virtual void displayIndex(int idx) = 0;

	virtual Math::Vec2i position(void) const = 0;
	virtual void position(const Math::Vec2i &val) = 0;

	virtual Math::Vec2i size(void) const = 0;
	virtual void size(const Math::Vec2i &val) = 0;

	virtual bool resizable(void) const = 0;
	virtual void resizable(bool val) = 0;

	virtual bool maximized(void) const = 0;
	virtual void maximize(void) = 0;
	virtual bool minimized(void) const = 0;
	virtual void minimize(void) = 0;
	virtual void restore(void) const = 0;
	virtual bool fullscreen(void) const = 0;
	virtual void fullscreen(bool val) = 0;

	virtual int width(void) const = 0;
	virtual int height(void) const = 0;

	virtual int scale(void) const = 0;
	virtual void scale(int val) = 0;

	static Window* create(void);
	static void destroy(Window* ptr);
};

/* ===========================================================================} */

#endif /* __WINDOW_H__ */
