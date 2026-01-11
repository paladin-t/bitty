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
#include "../lib/sqlite3pp/src/sqlite3pp.h"

/*
** {===========================================================================
** Database
*/

class DatabaseImpl : public Database {
private:
	sqlite3pp::database _db;
	std::string _path;
	bool _opened = false;

public:
	DatabaseImpl() {
	}
	virtual ~DatabaseImpl() override {
		close();
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Object** ptr) const override { // Non-clonable.
		if (ptr)
			*ptr = nullptr;

		return false;
	}

	virtual void* pointer(void) override {
		return &_db;
	}

	virtual bool open(const char* path, Stream::Accesses access) override {
		if (_opened)
			return true;

		_path = path;

		int flags = 0;
		switch (access) {
		case Stream::Accesses::WRITE: // Fall through.
		case Stream::Accesses::APPEND: // Fall through.
			fprintf(stderr, "Unsupported access mode %s, fall to %s.\n", access == Stream::Accesses::WRITE ? "WRITE" : "APPEND", "READ_WRITE");

			// Fall through.
		case Stream::Accesses::READ_WRITE:
			flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

			break;
		case Stream::Accesses::READ: // Fall through.
		default:
			if (access != Stream::Accesses::READ)
				fprintf(stderr, "Unsupported access mode %d, fall to %s.\n", (int)access, "READ");

			flags = SQLITE_OPEN_READONLY | SQLITE_OPEN_CREATE;

			break;
		}
		if (_db.connect(_path.c_str(), flags) == SQLITE_OK) {
			_opened = true;

			return true;
		}

		return false;
	}
	virtual bool close(void) override {
		if (!_opened)
			return true;

		_db.disconnect();
		_path.clear();
		_opened = false;

		return true;
	}

	virtual bool query(Variant &ret, const char* sql) override {
		if (!_opened)
			return false;

		if (!sql)
			return false;

		try {
			sqlite3pp::query query(_db, sql);

			IList::Ptr lstRows(List::create());
			for (auto row : query) {
				IList::Ptr lstCols(List::create());
				for (int i = 0; i < row.data_count(); ++i) {
					const int type = row.column_type(i);
					switch (type) {
					case SQLITE_TEXT: {
							const char* txt = row.get<const char*>(i);
							const Variant val = txt ? txt : "";
							lstCols->add(val);
						}

						break;
					case SQLITE_INTEGER: {
							const Variant::Int int_ = row.get<Variant::Int>(i);
							const Variant val = int_;
							lstCols->add(val);
						}

						break;
					case SQLITE_FLOAT: {
							const double dbl = row.get<double>(i);
							const Variant val = dbl;
							lstCols->add(val);
						}

						break;
					case SQLITE_BLOB: {
							const Variant val = nullptr;
							lstCols->add(val);
						}

						break;
					case SQLITE_NULL: {
							const Variant val = nullptr;
							lstCols->add(val);
						}

						break;
					}
				}
				lstRows->add((Object::Ptr)lstCols);
			}
			ret = (Object::Ptr)lstRows;

			return true;
		} catch (sqlite3pp::database_error &ex) {
			ret = ex.what();
		}

		return false;
	}
	virtual bool exec(Variant &ret, const char* sql) override {
		if (!_opened)
			return false;

		if (!sql)
			return false;

		try {
			sqlite3pp::command cmd(_db, sql);
			const int result = cmd.execute();

			ret = result;

			return true;
		} catch (sqlite3pp::database_error &ex) {
			ret = ex.what();
		}

		return true;
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
