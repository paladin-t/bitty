/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __RANDOMIZER_H__
#define __RANDOMIZER_H__

#include "bitty.h"
#include "mathematics.h"
#include "object.h"

/*
** {===========================================================================
** Randomizer
*/

/**
 * @brief Randomizer algorithm.
 */
class Randomizer : public virtual Object {
public:
	typedef std::shared_ptr<Randomizer> Ptr;

	typedef std::pair<UInt64, UInt64> Seed;

public:
	BITTY_CLASS_TYPE('R', 'A', 'N', 'D')

	virtual Seed seed(UInt64 first, UInt64 second) = 0;
	virtual Seed seed(UInt64 first) = 0;
	virtual Seed seed(void) = 0;

	virtual Int64 next(Int64 low, Int64 up) = 0;
	virtual Int64 next(Int64 up) = 0;
	virtual Double next(void) = 0;

	static Randomizer* create(void);
	static void destroy(Randomizer* ptr);
};

/* ===========================================================================} */

#endif /* __RANDOMIZER_H__ */
