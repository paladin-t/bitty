/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "database.h"

/*
** {===========================================================================
** Database
*/

class DatabaseImpl : public Database {
public:
	virtual ~DatabaseImpl() override {
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Object** ptr) const override { // Non-clonable.
		if (ptr)
			*ptr = nullptr;

		return false;
	}
};

Database* Database::create(void) {
	DatabaseImpl* result = new DatabaseImpl();

	return result;
}

void Database::destroy(Database* ptr) {
	DatabaseImpl* impl = static_cast<DatabaseImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
