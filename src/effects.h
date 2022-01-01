/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __EFFECTS_H__
#define __EFFECTS_H__

#include "bitty.h"
#include "resources.h"

/*
** {===========================================================================
** Effects
*/

/**
 * @brief Special effects.
 */
class Effects {
public:
	/**
	 * @brief Opens the effects for further operation.
	 */
	virtual bool open(class Window* wnd, class Renderer* rnd, class Workspace* ws, bool enabled) = 0;
	/**
	 * @brief Closes the effects after all operations.
	 */
	virtual bool close(void) = 0;

	/**
	 * @brief Gets whether the effects are valid.
	 */
	virtual bool valid(void) const = 0;
	/**
	 * @brief Configures the effects.
	 */
	virtual bool use(class Workspace* ws, const char* material) = 0;

	/**
	 * @brief Injects float uniform data.
	 */
	virtual bool inject(const char* entry, float arg) = 0;
	/**
	 * @brief Injects vec2 uniform data.
	 */
	virtual bool inject(const char* entry, const Math::Vec2f &arg) = 0;
	/**
	 * @brief Injects vec3 uniform data.
	 */
	virtual bool inject(const char* entry, const Math::Vec3f &arg) = 0;
	/**
	 * @brief Injects vec4 uniform data.
	 */
	virtual bool inject(const char* entry, const Math::Vec4f &arg) = 0;
	/**
	 * @brief Injects sampler2D uniform data.
	 */
	virtual bool inject(const char* entry, const Resources::Texture::Ptr &arg) = 0;
	/**
	 * @brief Injects sampler2D uniform data.
	 */
	virtual bool inject(const char* entry, const Image::Ptr &arg) = 0;

	/**
	 * @brief Prepares the effects before rendering new frame.
	 */
	virtual void prepare(class Window* wnd, class Renderer* rnd, class Workspace* ws, double delta) = 0;
	/**
	 * @brief Finishes and presents the effects after rendering a frame.
	 */
	virtual void finish(class Window* wnd, class Renderer* rnd, class Workspace* ws) = 0;

	/**
	 * @brief Callback for render targets reset.
	 */
	virtual void renderTargetsReset(void) = 0;

	static Effects* create(void);
	static void destroy(Effects* ptr);
};

/* ===========================================================================} */

#endif /* __EFFECTS_H__ */
