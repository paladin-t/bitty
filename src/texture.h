/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include "bitty.h"
#include "color.h"
#include "object.h"

/*
** {===========================================================================
** Texture
*/

/**
 * @brief Texture object.
 */
class Texture : public virtual Object {
public:
	typedef std::shared_ptr<Texture> Ptr;

	enum ScaleModes {
		NEAREST,
		LINEAR,
		ANISOTROPIC
	};

	enum Usages {
		STATIC = 0,
		STREAMING = 1,
		TARGET = 2
	};

	enum BlendModes : unsigned {
		NONE = 0x00000000,
		BLEND = 0x00000001,
		ADD = 0x00000002,
		MOD = 0x00000004,
		MUL = 0x00000008,
		INVALID = 0x7fffffff
	};

public:
	BITTY_CLASS_TYPE('T', 'X', 'T', 'R')

	/**
	 * @return `SDL_Texture*`.
	 */
	virtual void* pointer(class Renderer* rnd) = 0;

	virtual Usages usage(void) const = 0;

	virtual ScaleModes scaling(void) const = 0;
	virtual void scaling(ScaleModes scale) = 0;

	virtual BlendModes blend(void) const = 0;
	virtual void blend(BlendModes blend) = 0;

	virtual int paletted(void) const = 0;

	virtual int width(void) const = 0;
	virtual int height(void) const = 0;

	/**
	 * @brief For `STREAMING`, `TARGET`.
	 */
	virtual bool resize(class Renderer* rnd, int width, int height) = 0;

	/**
	 * @brief Thread unsafe, allowed to call from the graphics thread only.
	 */
	virtual bool set(int x, int y, const Color &col) = 0;
	/**
	 * @brief Thread unsafe, allowed to call from the graphics thread only.
	 */
	virtual bool set(int x, int y, int index) = 0;

	/**
	 * @brief Loads a paletted or 32bit true-color texture.
	 */
	virtual bool fromImage(class Renderer* rnd, Usages usg, class Image* img) = 0;

	/**
	 * @brief For `STREAMING`, `TARGET`.
	 */
	virtual int toBytes(class Renderer* rnd, Byte* pixels /* nullable */) = 0;
	/**
	 * @brief Loads a paletted or 32bit true-color texture.
	 */
	virtual bool fromBytes(class Renderer* rnd, Usages usg, const Byte* pixels, int width, int height, int paletted) = 0;

	static Texture* create(void);
	static void destroy(Texture* ptr);
};

/* ===========================================================================} */

#endif /* __TEXTURE_H__ */
