/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __PRIMITIVES_H__
#define __PRIMITIVES_H__

#include "bitty.h"
#include "font.h"
#include "input.h"
#include "resources.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef PRIMITIVE_CLIP
#	define PRIMITIVE_CLIP(P, C) \
		ProcedureGuard<Primitives> __CLIP##__LINE__( \
			P, \
			[&] (void) -> Primitives* { \
				P->clip(C.xMin(), C.yMin(), C.width(), C.height()); \
				return P; \
			}, \
			[&] (Primitives*) -> void { \
				P->clip(); \
			} \
		);
#endif /* PRIMITIVE_CLIP */

/* ===========================================================================} */

/*
** {===========================================================================
** Primitives
*/

/**
 * @brief Primitives.
 *
 * @note For multithread build, primitives emitted from code are queued for
 *   the graphics thread to consume.
 */
class Primitives {
public:
	typedef std::function<void(const Variant &)> Function;

public:
	/**
	 * @brief Opens the primitives.
	 */
	virtual bool open(class Window* wnd, class Renderer* rnd, const class Project* project, Resources* res, class Effects* effects) = 0;
	/**
	 * @brief Closes the primitives.
	 */
	virtual bool close(void) = 0;

	/**
	 * @brief Gets the window object.
	 *
	 * @note By the graphics thread.
	 */
	virtual class Window* window(void) = 0;
	/**
	 * @brief Gets the renderer object.
	 *
	 * @note By the graphics thread.
	 */
	virtual class Renderer* renderer(void) = 0;

	/**
	 * @brief Gets the effects object.
	 *
	 * @note By the graphics thread.
	 */
	virtual class Effects* effects(void) = 0;

	/**
	 * @brief Gets the input object.
	 *
	 * @note By the graphics thread.
	 */
	virtual Input* input(void) = 0;

	/**
	 * @brief Gets the command count of the latest commission.
	 *
	 * @note By the graphics thread.
	 */
	virtual unsigned commands(void) const = 0;

	/**
	 * @brief Gets the current render target.
	 *
	 * @return The current render target, `nullptr` for the main canvas.
	 */
	virtual Resources::Texture::Ptr target(void) const = 0;
	/**
	 * @brief Sets the current render target.
	 *
	 * @param[in] tex The specific render target, `nullptr` for the main canvas.
	 */
	virtual void target(Resources::Texture::Ptr tex) = 0;

	/**
	 * @brief Gets whether to clear the canvas automatically.
	 */
	virtual bool autoCls(void) const = 0;
	/**
	 * @brief Sets whether to clear the canvas automatically.
	 */
	virtual void autoCls(bool cls) = 0;

	/**
	 * @brief Clears the canvas.
	 *
	 * @param[in] col The specific color to use, `nullptr` to use the previous value.
	 * @return The previous color value.
	 */
	virtual Color cls(const Color* col /* nullable */) = 0;
	/**
	 * @brief Sets the blend mode of the texture with the specific mode.
	 *
	 * @param[in] tex The texture to set.
	 * @param[in] mode The blend value to set.
	 */
	virtual void blend(Resources::Texture::Ptr tex, unsigned mode) = 0;
	/**
	 * @brief Sets the blend mode of the main canvas with the specific mode.
	 *
	 * @param[in] mode The blend value to set.
	 */
	virtual void blend(unsigned mode) = 0;
	/**
	 * @brief Resets the blend mode of the main canvas to alpha blend.
	 */
	virtual void blend(void) = 0;
	/**
	 * @brief Gets the active camera offset.
	 *
	 * @param[out] x To receive the camera's x offset.
	 * @param[out] y To receive the camera's y offset.
	 * @return `true` for valid offset, `false` for non-offset.
	 */
	virtual bool camera(int* x /* nullable */, int* y /* nullable */) const = 0;
	/**
	 * @brief Sets the active camera offset.
	 *
	 * @param[in] x The camera's x offset.
	 * @param[in] y The camera's y offset.
	 */
	virtual void camera(int x, int y) = 0;
	/**
	 * @brief Resets the active camera offset to none.
	 */
	virtual void camera(void) = 0;
	/**
	 * @brief Gets the active clip area.
	 *
	 * @param[out] x To receive the clip area's x position.
	 * @param[out] y To receive the clip area's y position.
	 * @param[out] width To receive the clip area's width.
	 * @param[out] height To receive the clip area's height.
	 * @return `true` for valid clip, `false` for non-clip.
	 */
	virtual bool clip(int* x /* nullable */, int* y /* nullable */, int* width /* nullable */, int* height /* nullable */) const = 0;
	/**
	 * @brief Sets the active clip area.
	 *
	 * @param[in] x The clip area's x position.
	 * @param[in] y The clip area's y position.
	 * @param[in] width The clip area's width.
	 * @param[in] height The clip area's height.
	 */
	virtual void clip(int x, int y, int width, int height) = 0;
	/**
	 * @brief Resets the active clip area to none.
	 */
	virtual void clip(void) = 0;
	/**
	 * @brief Sets the active color.
	 *
	 * @param[in] col The color to set, `nullptr` for white.
	 * @return The previous color value
	 */
	virtual Color color(const Color* col /* nullable */) = 0;
	/**
	 * @brief Resets the active color to white.
	 *
	 * @return The previous color value
	 */
	virtual Color color(void) = 0;
	/**
	 * @brief Plots a point.
	 *
	 * @param[in] col The color to draw, `nullptr` to use the current active color.
	 */
	virtual void plot(int x, int y, const Color* col /* nullable */) const = 0;
	/**
	 * @brief Draws a line.
	 *
	 * @param[in] col The color to draw, `nullptr` to use the current active color.
	 */
	virtual void line(int x0, int y0, int x1, int y1, const Color* col /* nullable */) const = 0;
	/**
	 * @brief Draws a circle.
	 *
	 * @param[in] col The color to draw, `nullptr` to use the current active color.
	 */
	virtual void circ(int x, int y, int r, bool fill, const Color* col /* nullable */) const = 0;
	/**
	 * @brief Draws an ellipse.
	 *
	 * @param[in] col The color to draw, `nullptr` to use the current active color.
	 */
	virtual void ellipse(int x, int y, int rx, int ry, bool fill, const Color* col /* nullable */) const = 0;
	/**
	 * @brief Draws a pie.
	 *
	 * @param[in] col The color to draw, `nullptr` to use the current active color.
	 */
	virtual void pie(int x, int y, int r, int sa, int ea, bool fill, const Color* col /* nullable */) const = 0;
	/**
	 * @brief Draws a rectangle.
	 *
	 * @param[in] col The color to draw, `nullptr` to use the current active color.
	 */
	virtual void rect(int x0, int y0, int x1, int y1, bool fill, const Color* col /* nullable */, const int* rad /* nullable */) const = 0;
	/**
	 * @brief Draws a triangle.
	 *
	 * @param[in] col The color to draw, `nullptr` to use the current active color.
	 */
	virtual void tri(const Math::Vec2f &p0, const Math::Vec2f &p1, const Math::Vec2f &p2, bool fill, const Color* col /* nullable */) const = 0;
	/**
	 * @brief Draws a triangle, fills with the specific texture.
	 */
	virtual void tri(const Math::Vec2f &p0, const Math::Vec2f &p1, const Math::Vec2f &p2, Resources::Texture::Ptr tex, const Math::Vec2f &uv0, const Math::Vec2f &uv1, const Math::Vec2f &uv2) const = 0;
	/**
	 * @brief Sets the active font.
	 *
	 * @param[in] font The font to set.
	 */
	virtual void font(Font::Ptr font) = 0;
	/**
	 * @brief Resets the active font.
	 */
	virtual void font(void) = 0;
	/**
	 * @brief Measures the size of some text with the specific font.
	 *
	 * @param[in] font The font to use, `nullptr` for the default built-in.
	 */
	virtual Math::Vec2f measure(const char* text, Font::Ptr font /* nullable */, int margin, const float* scale /* nullable */) const = 0;
	/**
	 * @brief Draws a piece of text with the current active font.
	 *
	 * @param[in] col The color to draw, `nullptr` to use the current active color.
	 */
	virtual void text(const char* text, int x, int y, const Color* col /* nullable */, int margin, const float* scale /* nullable */) const = 0;
	/**
	 * @brief Draws a texture.
	 *
	 * @param[in] rotAngle Rotation angle in DEG.
	 */
	virtual void tex(Resources::Texture::Ptr tex /* nullable */, int x, int y, int width, int height, int sx, int sy, int swidth, int sheight, const double* rotAngle /* nullable */, const Math::Vec2f* rotCenter /* nullable */, bool hFlip, bool vFlip, const Color* col /* nullable */) const = 0;
	/**
	 * @brief Draws a sprite.
	 *
	 * @param[in] rotAngle Rotation angle in DEG.
	 */
	virtual void spr(Resources::Sprite::Ptr spr, int x, int y, int width, int height, const double* rotAngle /* nullable */, const Math::Vec2f* rotCenter /* nullable */, double delta, const Color* col /* nullable */) const = 0;
	/**
	 * @brief Plays the specific sprite, asynchronized.
	 */
	virtual void play(Resources::Sprite::Ptr spr, int begin, int end, bool reset, bool loop) const = 0;
	/**
	 * @brief Plays the specific sprite, asynchronized.
	 */
	virtual void play(Resources::Sprite::Ptr spr, const std::string &key, bool reset, bool loop) const = 0;
	/**
	 * @brief Draws a map.
	 */
	virtual void map(Resources::Map::Ptr map, int x, int y, double delta, const Color* col /* nullable */, int scale) const = 0;
	/**
	 * @brief Gets the palette color at the specific index.
	 *
	 * @param[out] col
	 */
	virtual void pget(Resources::Palette::Ptr plt, int idx, Color &col) const = 0;
	/**
	 * @brief Sets the palette color at the specific index.
	 *
	 * @param[in] col
	 */
	virtual void pset(Resources::Palette::Ptr plt, int idx, const Color &col) = 0;
	/**
	 * @brief Gets the map cel at the specific position.
	 *
	 * @param[out] cel
	 */
	virtual void mget(Resources::Map::Ptr map, int x, int y, int &cel) const = 0;
	/**
	 * @brief Sets the map cel at the specific position.
	 *
	 * @param[in] cel
	 */
	virtual void mset(Resources::Map::Ptr map, int x, int y, int cel) = 0;

	/**
	 * @brief Sets the SFX and music volume values.
	 */
	virtual void volume(const Audio::SfxVolume &sfxVol, float musicVol) const = 0;
	/**
	 * @brief Sets the SFX and music volume values.
	 */
	virtual void volume(float sfxVol, float musicVol) const = 0;
	/**
	 * @brief Plays the specific SFX.
	 */
	virtual void play(Resources::Sfx::Ptr sfx, bool loop, const int* fadeInMs /* nullable */, int channel = -1) const = 0;
	/**
	 * @brief Plays the specific music.
	 */
	virtual void play(Resources::Music::Ptr mus, bool loop, const int* fadeInMs /* nullable */, const double* pos = nullptr) const = 0;
	/**
	 * @brief Stops the specific SFX.
	 */
	virtual void stop(Resources::Sfx::Ptr sfx, const int* fadeOutMs /* nullable */) const = 0;
	/**
	 * @brief Stops the specific music.
	 */
	virtual void stop(Resources::Music::Ptr mus, const int* fadeOutMs /* nullable */) const = 0;

	/**
	 * @brief Gets the pressed state of the specific gamepad button.
	 *
	 * @param[in] idx Positive for virtual gamepad, negative for hardware controller.
	 */
	virtual int btn(int btn, int idx) const = 0;
	/**
	 * @brief Gets the released state of the specific gamepad button.
	 *
	 * @param[in] idx Positive for virtual gamepad, negative for hardware controller.
	 */
	virtual int btnp(int btn, int idx) const = 0;
	/**
	 * @brief Rumbles the specific gamepad if possible.
	 *
	 * @param[in] idx Positive for virtual gamepad, negative for hardware controller.
	 */
	virtual void rumble(int idx, int lowHz, int hiHz, unsigned ms) const = 0;
	/**
	 * @brief Gets the pressed state of the specific key.
	 */
	virtual bool key(int key) const = 0;
	/**
	 * @brief Gets the released state of the specific key.
	 */
	virtual bool keyp(int key) const = 0;
	/**
	 * @brief Gets the mouse states.
	 *
	 * @param[out] x
	 * @param[out] y
	 * @param[out] b0
	 * @param[out] b1
	 * @param[out] b2
	 * @param[out] wheelX
	 * @param[out] wheelY
	 */
	virtual bool mouse(int btn, int* x /* nullable */, int* y /* nullable */, bool* b0 /* nullable */, bool* b1 /* nullable */, bool* b2 /* nullable */, int* wheelX /* nullable */, int* wheelY /* nullable */) const = 0;
	/**
	 * @brief Sets the mouse cursor indicator.
	 */
	virtual void cursor(Image::Ptr img /* nullable */, float x, float y) const = 0;
	/**
	 * @brief Schedules with a custom C++ function.
	 */
	virtual void function(Function func, const Variant &arg /* nullable */, bool block) const = 0;

	/**
	 * @brief Begins a new frame.
	 */
	virtual int newFrame(void) = 0;
	/**
	 * @brief Commits all pending commands.
	 */
	virtual int commit(void) = 0;
	/**
	 * @brief Synchronizes and waits until all pending commands are consumed.
	 */
	virtual int sync(void) = 0;

	/**
	 * @brief Loads the specific resource.
	 */
	virtual bool load(const Resources::Asset::Ptr &res) = 0;
	/**
	 * @brief Unloads the specific resource.
	 */
	virtual bool unload(const Resources::Asset::Ptr &res) = 0;

	/**
	 * @brief Disposes the specific object.
	 */
	virtual bool dispose(const Object::Ptr &obj) = 0;
	/**
	 * @brief Collects all unused resources.
	 */
	virtual void collect(void) = 0;

	/**
	 * @brief Gets the texture of the main canvas.
	 *
	 * @note By the graphics thread.
	 */
	virtual Texture::Ptr canvas(void) const = 0;
	/**
	 * @brief Sets the texture of the main canvas.
	 *
	 * @note By the graphics thread.
	 */
	virtual void canvas(Texture::Ptr tex) = 0;

	/**
	 * @brief Gets the mouse cursor indicator.
	 *
	 * @note By the graphics thread.
	 */
	virtual Image::Ptr indicator(float* x /* nullable */, float* y /* nullable */) const = 0;
	/**
	 * @brief Sets the mouse cursor indicator.
	 *
	 * @note By the graphics thread.
	 */
	virtual void indicator(Image::Ptr img /* nullable */, float x, float y) = 0;

	/**
	 * @brief Forbids command synchronizing.
	 *
	 * @note By the graphics thread.
	 */
	virtual void forbid(void) = 0;
	/**
	 * @brief Resets the primitives.
	 *
	 * @note By the graphics thread.
	 */
	virtual void reset(void) = 0;

	/**
	 * @brief Updates the primitives.
	 *
	 * @note By the graphics thread.
	 */
	virtual bool update(const Math::Rectf* clientArea, const Math::Vec2i* canvasSize, int scale, double delta, bool hovering, bool* indicated /* nullable */) = 0;

	static Primitives* create(bool withAudio);
	static void destroy(Primitives* ptr);
};

/* ===========================================================================} */

#endif /* __PRIMITIVES_H__ */
