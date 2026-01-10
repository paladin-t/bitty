/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __DATABASE_H__
#define __DATABASE_H__

#include "bitty.h"
#include "object.h"

/*
** {===========================================================================
** Database
*/

/**
 * @brief Database object.
 */
class Database : public virtual Object {
public:
	typedef std::shared_ptr<Database> Ptr;

public:
	BITTY_CLASS_TYPE('D', 'T', 'B', 'S')

	static Database* create(void);
	static void destroy(Database* ptr);
};

/* ===========================================================================} */

#endif /* __DATABASE_H__ */
