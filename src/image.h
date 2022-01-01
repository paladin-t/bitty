/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __IMAGE_H__
#define __IMAGE_H__

#include "bitty.h"
#include "palette.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef IMAGE_PALETTE_BITS
#	define IMAGE_PALETTE_BITS 8
#endif /* IMAGE_PALETTE_BITS */
#ifndef IMAGE_PALETTE_COLOR_COUNT
#	define IMAGE_PALETTE_COLOR_COUNT Math::pow(2, IMAGE_PALETTE_BITS)
#endif /* IMAGE_PALETTE_COLOR_COUNT */

#ifndef IMAGE_PALETTED_HEADER
#	define IMAGE_PALETTED_HEADER { 'I', 'M', 'G', 'P' }
#endif /* IMAGE_PALETTED_HEADER */
#ifndef IMAGE_COLORED_HEADER
#	define IMAGE_COLORED_HEADER { 'I', 'M', 'G', 'C' }
#endif /* IMAGE_COLORED_HEADER */

/* ===========================================================================} */

/*
** {===========================================================================
** Image
*/

/**
 * @brief Image resource object.
 */
class Image : public Cloneable<Image>, public virtual Object {
public:
	typedef std::shared_ptr<Image> Ptr;
	typedef std::weak_ptr<Image> WeakPtr;

public:
	BITTY_CLASS_TYPE('I', 'M', 'G', 'A')

	using Cloneable<Image>::clone;
	using Object::clone;

	/**
	 * @return `SDL_Surface*`.
	 */
	virtual void* pointer(void) = 0;
	virtual void pointer(std::nullptr_t) = 0;

	virtual bool blank(void) const = 0;

	virtual const Palette::Ptr palette(void) const = 0;
	virtual void palette(Palette::Ptr val) = 0;
	virtual int paletted(void) const = 0;

	virtual const Byte* pixels(void) const = 0;
	virtual Byte* pixels(void) = 0;

	virtual int width(void) const = 0;
	virtual int height(void) const = 0;

	virtual int channels(void) const = 0;

	virtual bool resize(int width, int height, bool stretch) = 0;

	/**
	 * @param[out] col
	 */
	virtual bool get(int x, int y, Color &col) const = 0;
	virtual bool set(int x, int y, const Color &col) = 0;
	/**
	 * @param[out] index
	 */
	virtual bool get(int x, int y, int &index) const = 0;
	virtual bool set(int x, int y, int index) = 0;

	virtual void weight(int r, int g, int b, int a) = 0;
	virtual bool quantize(const Color* colors, int colorCount, bool p2p) = 0;

	virtual bool blit(Image* dst, int x, int y, int w, int h, int sx, int sy) const = 0;

	virtual bool fromBlank(int width, int height, int paletted) = 0;

	virtual bool fromImage(const Image* src) = 0;

	/**
	 * @param[out] val
	 */
	virtual bool toBytes(class Bytes* val, const char* type) const = 0;
	virtual bool fromBytes(const Byte* val, size_t size) = 0;
	virtual bool fromBytes(const class Bytes* val) = 0;

	/**
	 * @param[out] val
	 * @param[in, out] doc
	 */
	virtual bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const = 0;
	/**
	 * @param[in, out] val
	 */
	virtual bool toJson(rapidjson::Document &val) const = 0;
	virtual bool fromJson(const rapidjson::Value &val) = 0;
	virtual bool fromJson(const rapidjson::Document &val) = 0;

	static Image* create(Palette::Ptr palette /* nullable */);
	static void destroy(Image* ptr);
};

/* ===========================================================================} */

#endif /* __IMAGE_H__ */
