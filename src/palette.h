/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __PALETTE_H__
#define __PALETTE_H__

#include "bitty.h"
#include "cloneable.h"
#include "color.h"
#include "json.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef PALETTE_DEFAULT_COLORS
#	define PALETTE_DEFAULT_COLORS { \
		Color(0, 0, 0, 0), Color(29, 43, 83, 255), Color(126, 37, 83, 255), Color(0, 135, 81, 255), \
		Color(171, 82, 54, 255), Color(95,87,79,255), Color(194, 195, 199, 255), Color(255, 241, 232, 255), \
		Color(255, 0, 77, 255), Color(255, 163, 0, 255), Color(255, 236, 39, 255), Color(0, 228, 54, 255), \
		Color(41, 173, 255, 255), Color(131, 118, 156, 255), Color(255, 119, 168, 255), Color(255, 204, 170, 255), \
		Color(247, 9, 9, 255), Color(247, 104, 9, 255), Color(247, 175, 9, 255), Color(223, 247, 9, 255), \
		Color(128, 247, 9, 255), Color(32, 247, 9, 255), Color(9, 247, 80, 255), Color(9, 247, 151, 255), \
		Color(9, 247, 247, 255), Color(9, 151, 247, 255), Color(9, 80, 247, 255), Color(32, 9, 247, 255), \
		Color(128, 9, 247, 255), Color(223, 9, 247, 255), Color(247, 9, 175, 255), Color(247, 9, 104, 255), \
		Color(238, 17, 17, 255), Color(238, 105, 17, 255), Color(238, 172, 17, 255), Color(216, 238, 17, 255), \
		Color(128, 238, 17, 255), Color(39, 238, 17, 255), Color(17, 238, 83, 255), Color(17, 238, 150, 255), \
		Color(17, 238, 238, 255), Color(17, 150, 238, 255), Color(17, 83, 238, 255), Color(39, 17, 238, 255), \
		Color(128, 17, 238, 255), Color(216, 17, 238, 255), Color(238, 17, 172, 255), Color(238, 17, 105, 255), \
		Color(234, 21, 21, 255), Color(234, 106, 21, 255), Color(234, 170, 21, 255), Color(213, 234, 21, 255), \
		Color(128, 234, 21, 255), Color(43, 234, 21, 255), Color(21, 234, 85, 255), Color(21, 234, 149, 255), \
		Color(21, 234, 234, 255), Color(21, 149, 234, 255), Color(21, 85, 234, 255), Color(43, 21, 234, 255), \
		Color(128, 21, 234, 255), Color(213, 21, 234, 255), Color(234, 21, 170, 255), Color(234, 21, 106, 255), \
		Color(225, 30, 30, 255), Color(225, 108, 30, 255), Color(225, 167, 30, 255), Color(206, 225, 30, 255), \
		Color(128, 225, 30, 255), Color(49, 225, 30, 255), Color(30, 225, 88, 255), Color(30, 225, 147, 255), \
		Color(30, 225, 225, 255), Color(30, 147, 225, 255), Color(30, 88, 225, 255), Color(49, 30, 225, 255), \
		Color(128, 30, 225, 255), Color(206, 30, 225, 255), Color(225, 30, 167, 255), Color(225, 30, 108, 255), \
		Color(217, 38, 38, 255), Color(217, 109, 38, 255), Color(217, 164, 38, 255), Color(199, 217, 38, 255), \
		Color(128, 217, 38, 255), Color(56, 217, 38, 255), Color(38, 217, 91, 255), Color(38, 217, 146, 255), \
		Color(38, 217, 217, 255), Color(38, 146, 217, 255), Color(38, 91, 217, 255), Color(56, 38, 217, 255), \
		Color(128, 38, 217, 255), Color(199, 38, 217, 255), Color(217, 38, 164, 255), Color(217, 38, 109, 255), \
		Color(208, 47, 47, 255), Color(208, 112, 47, 255), Color(208, 159, 47, 255), Color(192, 208, 47, 255), \
		Color(128, 208, 47, 255), Color(63, 208, 47, 255), Color(47, 208, 96, 255), Color(47, 208, 143, 255), \
		Color(47, 208, 208, 255), Color(47, 143, 208, 255), Color(47, 96, 208, 255), Color(63, 47, 208, 255), \
		Color(128, 47, 208, 255), Color(192, 47, 208, 255), Color(208, 47, 159, 255), Color(208, 47, 112, 255), \
		Color(200, 55, 55, 255), Color(200, 113, 55, 255), Color(200, 156, 55, 255), Color(185, 200, 55, 255), \
		Color(128, 200, 55, 255), Color(70, 200, 55, 255), Color(55, 200, 99, 255), Color(55, 200, 142, 255), \
		Color(55, 200, 200, 255), Color(55, 142, 200, 255), Color(55, 99, 200, 255), Color(70, 55, 200, 255), \
		Color(128, 55, 200, 255), Color(185, 55, 200, 255), Color(200, 55, 156, 255), Color(200, 55, 113, 255), \
		Color(191, 64, 64, 255), Color(191, 115, 64, 255), Color(191, 153, 64, 255), Color(179, 191, 64, 255), \
		Color(128, 191, 64, 255), Color(77, 191, 64, 255), Color(64, 191, 102, 255), Color(64, 191, 140, 255), \
		Color(64, 191, 191, 255), Color(64, 140, 191, 255), Color(64, 102, 191, 255), Color(77, 64, 191, 255), \
		Color(128, 64, 191, 255), Color(179, 64, 191, 255), Color(191, 64, 153, 255), Color(191, 64, 115, 255), \
		Color(183, 72, 72, 255), Color(183, 117, 72, 255), Color(183, 150, 72, 255), Color(172, 183, 72, 255), \
		Color(128, 183, 72, 255), Color(83, 183, 72, 255), Color(72, 183, 105, 255), Color(72, 183, 138, 255), \
		Color(72, 183, 183, 255), Color(72, 138, 183, 255), Color(72, 105, 183, 255), Color(83, 72, 183, 255), \
		Color(128, 72, 183, 255), Color(172, 72, 183, 255), Color(183, 72, 150, 255), Color(183, 72, 117, 255), \
		Color(174, 81, 81, 255), Color(174, 118, 81, 255), Color(174, 147, 81, 255), Color(165, 174, 81, 255), \
		Color(128, 174, 81, 255), Color(90, 174, 81, 255), Color(81, 174, 108, 255), Color(81, 174, 137, 255), \
		Color(81, 174, 174, 255), Color(81, 137, 174, 255), Color(81, 108, 174, 255), Color(90, 81, 174, 255), \
		Color(128, 81, 174, 255), Color(165, 81, 174, 255), Color(174, 81, 147, 255), Color(174, 81, 118, 255), \
		Color(170, 85, 85, 255), Color(170, 119, 85, 255), Color(170, 145, 85, 255), Color(162, 170, 85, 255), \
		Color(128, 170, 85, 255), Color(94, 170, 85, 255), Color(85, 170, 111, 255), Color(85, 170, 136, 255), \
		Color(85, 170, 170, 255), Color(85, 136, 170, 255), Color(85, 111, 170, 255), Color(94, 85, 170, 255), \
		Color(128, 85, 170, 255), Color(162, 85, 170, 255), Color(170, 85, 145, 255), Color(170, 85, 119, 255), \
		Color(162, 94, 94, 255), Color(162, 121, 94, 255), Color(162, 141, 94, 255), Color(155, 162, 94, 255), \
		Color(128, 162, 94, 255), Color(100, 162, 94, 255), Color(94, 162, 114, 255), Color(94, 162, 134, 255), \
		Color(94, 162, 162, 255), Color(94, 134, 162, 255), Color(94, 114, 162, 255), Color(100, 94, 162, 255), \
		Color(128, 94, 162, 255), Color(155, 94, 162, 255), Color(162, 94, 141, 255), Color(162, 94, 121, 255), \
		Color(153, 102, 102, 255), Color(153, 122, 102, 255), Color(153, 138, 102, 255), Color(148, 153, 102, 255), \
		Color(128, 153, 102, 255), Color(107, 153, 102, 255), Color(102, 153, 117, 255), Color(102, 153, 133, 255), \
		Color(102, 153, 153, 255), Color(102, 133, 153, 255), Color(102, 117, 153, 255), Color(107, 102, 153, 255), \
		Color(128, 102, 153, 255), Color(148, 102, 153, 255), Color(153, 102, 138, 255), Color(153, 102, 122, 255), \
		Color(145, 111, 111, 255), Color(145, 124, 111, 255), Color(145, 134, 111, 255), Color(141, 145, 111, 255), \
		Color(128, 145, 111, 255), Color(114, 145, 111, 255), Color(111, 145, 121, 255), Color(111, 145, 131, 255), \
		Color(111, 145, 145, 255), Color(111, 131, 145, 255), Color(111, 121, 145, 255), Color(114, 111, 145, 255), \
		Color(128, 111, 145, 255), Color(141, 111, 145, 255), Color(145, 111, 134, 255), Color(145, 111, 124, 255), \
		Color(136, 119, 119, 255), Color(136, 125, 119, 255), Color(136, 131, 119, 255), Color(134, 136, 119, 255), \
		Color(128, 136, 119, 255), Color(121, 136, 119, 255), Color(119, 136, 124, 255), Color(119, 136, 130, 255), \
		Color(119, 136, 136, 255), Color(119, 130, 136, 255), Color(119, 124, 136, 255), Color(121, 119, 136, 255), \
		Color(128, 119, 136, 255), Color(134, 119, 136, 255), Color(136, 119, 131, 255), Color(136, 119, 125, 255) \
	}
#endif /* PALETTE_DEFAULT_COLORS */

/* ===========================================================================} */

/*
** {===========================================================================
** Palette
*/

/**
 * @brief Palette resource object.
 */
class Palette : public Cloneable<Palette>, public virtual Object {
public:
	typedef std::shared_ptr<Palette> Ptr;

public:
	BITTY_CLASS_TYPE('P', 'L', 'T', 'A')

	virtual bool clone(Palette** ptr, bool graphical) const = 0;
	using Cloneable<Palette>::clone;
	using Object::clone;

	/**
	 * @return `SDL_Palette*`.
	 */
	virtual void* pointer(void) = 0;

	virtual bool validate(void) = 0;

	virtual int count(void) const = 0;

	/**
	 * @param[out] col
	 */
	virtual const Color* get(int index, Color &col) const = 0;
	virtual bool set(int index, const Color* col) = 0;

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

	static Palette* create(int count);
	static void destroy(Palette* ptr);
};

/* ===========================================================================} */

#endif /* __PALETTE_H__ */
