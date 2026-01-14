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
#include "stream.h"

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

	/**
	 * @brief Gets the raw pointer.
	 *
	 * @return `sqlite3pp::database*`.
	 */
	virtual void* pointer(void) = 0;

	virtual bool open(const char* path, Stream::Accesses access) = 0;
	virtual bool close(void) = 0;

	virtual bool option(const std::string &key, const Variant &val) = 0;

	virtual bool query(Variant &ret, const char* sql) = 0;
	virtual bool exec(Variant &ret, const char* sql) = 0;

	static Database* create(void);
	static void destroy(Database* ptr);
};

/* ===========================================================================} */

#endif /* __DATABASE_H__ */
