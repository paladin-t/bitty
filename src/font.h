/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __FONT_H__
#define __FONT_H__

#include "bitty.h"
#include "color.h"
#include "object.h"

/*
** {===========================================================================
** Font
*/

/**
 * @brief Font resource object.
 */
class Font : public virtual Object {
public:
	typedef unsigned Codepoint;

public:
	typedef std::shared_ptr<Font> Ptr;

public:
	BITTY_CLASS_TYPE('F', 'N', 'T', 'A')

	virtual void* pointer(void) = 0;

	virtual bool measure(Codepoint cp, int* width /* nullable */, int* height /* nullable */) = 0;

	virtual bool render(
		Codepoint cp,
		class Bytes* out /* nullable */,
		const Color* color,
		int* width /* nullable */, int* height /* nullable */
	) = 0;

	virtual bool fromFont(const Font* font) = 0;

	virtual bool fromImage(const class Image* src, int width, int height, int permeation) = 0;

	virtual bool fromBytes(const Byte* data, size_t len, int size, int permeation) = 0;

	static Font* create(void);
	static void destroy(Font* ptr);
};

/* ===========================================================================} */

#endif /* __FONT_H__ */
