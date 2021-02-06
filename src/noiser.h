/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __NOISER_H__
#define __NOISER_H__

#include "bitty.h"
#include "mathematics.h"
#include "object.h"

/*
** {===========================================================================
** Noiser
*/

/**
 * @brief Noiser algorithm.
 */
class Noiser : public virtual Object {
public:
	typedef std::shared_ptr<Noiser> Ptr;

public:
	BITTY_CLASS_TYPE('N', 'O', 'I', 'S')

	virtual bool option(const std::string &key, const Variant &val) = 0;

	virtual void seed(int seed) = 0;

	virtual Real get(const Math::Vec2f &pos) = 0;
	virtual Real get(const Math::Vec3f &pos) = 0;

	virtual void domainWarp(Math::Vec2f &pos) = 0;
	virtual void domainWarp(Math::Vec3f &pos) = 0;

	static Noiser* create(void);
	static void destroy(Noiser* ptr);
};

/* ===========================================================================} */

#endif /* __NOISER_H__ */
