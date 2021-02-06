/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __COLLECTIBLE_H__
#define __COLLECTIBLE_H__

#include "bitty.h"

/*
** {===========================================================================
** Collectible
*/

/**
 * @brief Collectible interface.
 */
class Collectible {
public:
	virtual int collect(void) = 0;
	virtual int cleanup(void) = 0;
};

/* ===========================================================================} */

#endif /* __COLLECTIBLE_H__ */
