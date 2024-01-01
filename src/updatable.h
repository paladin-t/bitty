/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2024 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __UPDATABLE_H__
#define __UPDATABLE_H__

#include "bitty.h"

/*
** {===========================================================================
** Updatable
*/

/**
 * @brief Updatable interface.
 */
class Updatable {
public:
	virtual bool update(double delta) = 0;
};

/* ===========================================================================} */

#endif /* __UPDATABLE_H__ */
