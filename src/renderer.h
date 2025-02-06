/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
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
	ProcedureGuard<class Texture> BITTY_UNIQUE_NAME(__TARGET__)( \
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
	ProcedureGuard<int> BITTY_UNIQUE_NAME(__SCALE__)( \
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
	 * @brief Gets the raw pointer.
	 *
	 * @return `SDL_Renderer*`.
	 */
	virtual void* pointer(void) = 0;

	/**
	 * @brief Opens the renderer for further operation.
	 */
	virtual bool open(class Window* wnd, bool software) = 0;
	/**
	 * @brief Closes the renderer after all operations.
	 */
	virtual bool close(void) = 0;

	/**
	 * @brief Gets the backend driver of the renderer.
	 */
	virtual const char* driver(void) const = 0;

	/**
	 * @brief Gets whether render target is supported by the renderer.
	 */
	virtual bool renderTargetSupported(void) const = 0;

	/**
	 * @brief Gets the maximum texture width supported by the renderer.
	 */
	virtual int maxTextureWidth(void) const = 0;
	/**
	 * @brief Gets the maximum texture height supported by the renderer.
	 */
	virtual int maxTextureHeight(void) const = 0;

	/**
	 * @brief Gets the current width of the renderer.
	 */
	virtual int width(void) const = 0;
	/**
	 * @brief Gets the current height of the renderer.
	 */
	virtual int height(void) const = 0;

	/**
	 * @brief Gets the current scale of the renderer.
	 */
	virtual int scale(void) const = 0;
	/**
	 * @brief Sets the current scale of the renderer.
	 */
	virtual void scale(int val) = 0;

	/**
	 * @brief Gets the current target of the renderer.
	 */
	virtual class Texture* target(void) = 0;
	/**
	 * @brief Sets the current target of the renderer.
	 */
	virtual void target(class Texture* tex /* nullable */) = 0;

	/**
	 * @brief Gets the current blend mode of the renderer.
	 */
	virtual unsigned blend(void) const = 0;
	/**
	 * @brief Sets the current blend mode of the renderer.
	 */
	virtual void blend(unsigned mode) = 0;

	/**
	 * @brief Sets the current clip area of the renderer.
	 */
	virtual void clip(int x, int y, int width, int height) = 0;
	/**
	 * @brief Resets the current clip area of the renderer.
	 */
	virtual void clip(void) = 0;

	/**
	 * @brief Clears the renderer with the specific color.
	 */
	virtual void clear(const Color* col /* nullable */) = 0;

	/**
	 * @brief Renders the specific texture.
	 *   For `STATIC`, `STREAMING`, `TARGET`.
	 */
	virtual void render(
		class Texture* tex,
		const Math::Recti* srcRect /* nullable */, const Math::Recti* dstRect /* nullable */,
		const double* rotAngle /* nullable */, const Math::Vec2f* rotCenter /* nullable */,
		bool hFlip, bool vFlip,
		const Color* color /* nullable */, bool colorChanged, bool alphaChanged
	) = 0;

	/**
	 * @brief Flushes the renderer.
	 */
	virtual void flush(void) = 0;

	static Renderer* create(void);
	static void destroy(Renderer* ptr);
};

/* ===========================================================================} */

#endif /* __RENDERER_H__ */
