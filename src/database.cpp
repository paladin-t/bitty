/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "bytes.h"
#include "database.h"
#include "../lib/sqlite3pp/src/sqlite3pp.h"

/*
** {===========================================================================
** Database
*/

class DatabaseImpl : public Database {
private:
	enum class BlobTypes {
		STRING,
		LIST
	};

private:
	bool _opened = false;
	std::string _path;
	sqlite3pp::database _db;
	BlobTypes _blobType = BlobTypes::STRING;

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
			return false;

		_path = path;

		int flags = 0;
		switch (access) {
		case Stream::Accesses::WRITE: // Fall through.
		case Stream::Accesses::READ_WRITE:
			flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

			break;
		case Stream::Accesses::APPEND:
			flags = SQLITE_OPEN_READWRITE;

			break;
		case Stream::Accesses::READ: // Fall through.
		default:
			if (access != Stream::Accesses::READ)
				fprintf(stderr, "Unsupported access mode %d, fall to \"%s\".\n", (int)access, "READ");

			flags = SQLITE_OPEN_READONLY;

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
			return false;

		_db.disconnect();
		_path.clear();
		_opened = false;

		return true;
	}

	virtual bool option(const std::string &key, const Variant &val) override {
		if (key == "blob_as") {
			const std::string val_ = (std::string)val;
			if (val_ == "string") {
				_blobType = BlobTypes::STRING;

				return true;
			} else if (val_ == "list") {
				_blobType = BlobTypes::LIST;

				return true;
			}

			return false;
		}

		return false;
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
							const int n = row.column_bytes(i);
							const void* ptr = row.get<const void*>(i);
							Variant val = nullptr;
							try {
								if (_blobType == BlobTypes::STRING) {
									std::string str;
									str.assign((const char*)ptr, (size_t)n);
									val = str;
								} else /* if (_blobType == BlobTypes::LIST) */ {
									IList::Ptr lst(List::create());
									for (int j = 0; j < n; ++j) {
										const Byte byte = ((const Byte*)ptr)[j];
										lst->add(Variant((Int)byte));
									}
									val = (Object::Ptr)lst;
								}
							} catch (const std::bad_alloc &) {
								val = std::string("Cannot retrieve BLOB: no enough memory.");
							} catch (const std::exception &ex) {
								val = std::string("Cannot retrieve BLOB: ") + ex.what() + std::string(".");
							} catch (...) {
								val = std::string("Cannot retrieve BLOB: unknown error.");
							}
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
		} catch (sqlite3pp::database_error &ex) {
			ret = ex.what();

			return false; // Failed.
		}

		return true; // Succeeded.
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
		} catch (sqlite3pp::database_error &ex) {
			ret = ex.what();

			return false; // Failed.
		}

		return true; // Succeeded.
	}

	virtual long long lastInsertedRowId(void) const override {
		if (!_opened)
			return false;

		try {
			return _db.last_insert_rowid();
		} catch (sqlite3pp::database_error &) {
			// Do nothing.
		}

		return -1;
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
