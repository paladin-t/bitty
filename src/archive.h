/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2023 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __ARCHIVE_H__
#define __ARCHIVE_H__

#include "bitty.h"
#include "stream.h"
#include "text.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef ARCHIVE_PACKAGE_MEDIA_HEAD
#	define ARCHIVE_PACKAGE_MEDIA_HEAD "package"
#endif /* ARCHIVE_PACKAGE_MEDIA_HEAD */

/* ===========================================================================} */

/*
** {===========================================================================
** Archive
*/

/**
 * @brief Archive object.
 */
class Archive : public virtual Object {
public:
	typedef std::shared_ptr<Archive> Ptr;

	enum Formats : unsigned {
		TXT,
		ZIP
	};

public:
	BITTY_CLASS_TYPE('A', 'R', 'C', 'H')

	virtual bool open(const char* path, Stream::Accesses access) = 0;
	virtual bool close(void) = 0;

	virtual Formats format(void) const = 0;

	virtual Stream::Accesses accessibility(void) const = 0;

	virtual const char* password(void) const = 0;
	virtual bool password(const char* pwd /* nullable */) = 0;

	/**
	 * @param[out] entries
	 */
	virtual bool all(Text::Array &entries) const = 0;

	virtual bool exists(const char* nameInArchive) const = 0;
	virtual bool make(const char* nameInArchive) = 0;
	virtual bool removable(void) const = 0;
	virtual bool remove(const char* nameInArchive) = 0;
	virtual bool renamable(void) const = 0;
	virtual bool rename(const char* nameInArchive, const char* newNameInArchive) = 0;

	/**
	 * @param[out] val
	 */
	virtual bool toBytes(class Bytes* val, const char* nameInArchive) const = 0;
	virtual bool fromBytes(const class Bytes* val, const char* nameInArchive) = 0;

	virtual bool toFile(const char* path, const char* nameInArchive) const = 0;
	virtual bool fromFile(const char* path, const char* nameInArchive) = 0;

	virtual bool toDirectory(const char* dir) const = 0;
	virtual bool fromDirectory(const char* dir) = 0;

	static Formats formatOf(const char* path);

	static Archive* create(Formats type);
	static void destroy(Archive* ptr);
};

/* ===========================================================================} */

#endif /* __ARCHIVE_H__ */
