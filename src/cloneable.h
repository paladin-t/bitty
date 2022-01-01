/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __CLONEABLE_H__
#define __CLONEABLE_H__

#include "bitty.h"

/*
** {===========================================================================
** Cloneable
*/

/**
 * @brief Cloneable interface.
 */
template<typename T> class Cloneable {
public:
	/**
	 * @param[out] ptr
	 */
	virtual bool clone(T** ptr) const = 0;
};

/* ===========================================================================} */

#endif /* __CLONEABLE_H__ */
