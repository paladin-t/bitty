/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "bitty.h"
#include "color.h"
#include "plus.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef BITTY_RENDER_TARGET
#	define BITTY_RENDER_TARGET(RND, TEX) \
	ProcedureGuard<class Texture> __TARGET##__LINE__( \
		std::bind( \
			[&] (class Renderer* rnd) -> class Texture* { \
				class Texture* result = rnd->target(); \
				rnd->target(TEX); \
				return result; \
			}, \
			(RND) \
		), \
		std::bind( \
			[] (class Renderer* rnd, class Texture* tex) -> void { \
				rnd->target(tex); \
			}, \
			(RND), std::placeholders::_1 \
		) \
	);
#endif /* BITTY_RENDER_TARGET */

#ifndef BITTY_RENDER_SCALE
#	define BITTY_RENDER_SCALE(RND, SCL) \
	ProcedureGuard<int> __SCALE##__LINE__( \
		std::bind( \
			[&] (class Renderer* rnd) -> int* { \
				int* result = (int*)(uintptr_t)rnd->scale(); \
				rnd->scale(SCL); \
				return result; \
			}, \
			(RND) \
		), \
		std::bind( \
			[] (class Renderer* rnd, int* scl) -> void { \
				rnd->scale((int)(uintptr_t)scl); \
			}, \
			(RND), std::placeholders::_1 \
		) \
	);
#endif /* BITTY_RENDER_SCALE */

/* ===========================================================================} */

/*
** {===========================================================================
** Renderer
*/

/**
 * @brief Renderer structure and context.
 */
class Renderer {
public:
	/**
	 * @return `SDL_Renderer*`.
	 */
	virtual const void* pointer(void) const = 0;
	virtual void* pointer(void) = 0;

	virtual bool open(class Window* wnd) = 0;
	virtual bool close(void) = 0;

	virtual const char* driver(void) const = 0;

	virtual bool renderTargetSupported(void) const = 0;

	virtual int maxTextureWidth(void) const = 0;
	virtual int maxTextureHeight(void) const = 0;

	virtual int width(void) const = 0;
	virtual int height(void) const = 0;

	virtual int scale(void) const = 0;
	virtual void scale(int val) = 0;

	virtual class Texture* target(void) = 0;
	virtual void target(class Texture* tex /* nullable */) = 0;

	virtual unsigned blend(void) const = 0;
	virtual void blend(unsigned mode) = 0;

	virtual void clip(int x, int y, int width, int height) = 0;
	virtual void clip(void) = 0;

	virtual void clear(const Color* col /* nullable */) = 0;

	/**
	 * @brief For `STATIC`, `STREAMING`, `TARGET`.
	 */
	virtual void render(class Texture* tex, const Math::Recti* srcRect /* nullable */, const Math::Recti* dstRect /* nullable */, const double* rotAngle /* nullable */, const Math::Vec2f* rotCenter /* nullable */, bool hFlip, bool vFlip) = 0;

	virtual void present(void) = 0;

	static Renderer* create(void);
	static void destroy(Renderer* ptr);
};

/* ===========================================================================} */

#endif /* __RENDERER_H__ */
