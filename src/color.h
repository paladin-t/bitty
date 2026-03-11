/*
** Bitty
**
** An itty bitty 2D game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __COLOR_H__
#define __COLOR_H__

#include "bitty.h"
#include "mathematics.h"
#include <string>

/*
** {===========================================================================
** Color
*/

/**
 * @brief Color structure.
 *
 * @note Conversion to `UInt32` follows little-endian, vice versa.
 */
struct Color {
	Byte r = 255;
	Byte g = 255;
	Byte b = 255;
	Byte a = 255;

	Color();
	Color(Byte r_, Byte g_, Byte b_);
	Color(Byte r_, Byte g_, Byte b_, Byte a_);
	Color(const Color &other);

	Color &operator = (const Color &other);
	Color operator - (void) const;
	Color operator + (const Color &other) const;
	Color operator - (const Color &other) const;
	Color operator * (const Color &other) const;
	Color operator * (Real other) const;
	Color &operator += (const Color &other);
	Color &operator -= (const Color &other);
	Color &operator *= (const Color &other);
	Color &operator *= (Real other);

	bool operator == (const Color &other) const;
	bool operator != (const Color &other) const;
	bool operator < (const Color &other) const;
	bool operator > (const Color &other) const;

	int compare(const Color &other) const;
	bool equals(const Color &other) const;

	int toGray(void) const;

	/**
	 * @brief 0xAABBGGRR as little-endian.
	 */
	UInt32 toRGBA(void) const;
	/**
	 * @brief 0xBBGGRRAA as little-endian.
	 */
	UInt32 toARGB(void) const;
	/**
	 * @brief Extracts HSV from color.
	 *
	 * @param[out] h
	 * @param[out] s
	 * @param[out] v
	 */
	void toHSV(float* h /* nullable */, float* s /* nullable */, float* v /* nullable */, Byte* a = nullptr) const;

	void fromRGBA(UInt32 rgba);
	void fromARGB(UInt32 argb);
	void fromHSV(float h, float s, float v, Byte a = 255);

	std::string toString(void) const;
	bool fromString(const std::string &str);
};

/* ===========================================================================} */

#endif /* __COLOR_H__ */
