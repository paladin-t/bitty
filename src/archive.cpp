/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "archive.h"
#include "archive_txt.h"
#include "archive_zip.h"
#include "file_handle.h"

/*
** {===========================================================================
** Archive
*/

Archive::Formats Archive::formatOf(const char* path) {
	Formats result = ZIP;

	File* file = File::create();
	if (file->open(path, Stream::READ)) {
		std::string ln;
		if (file->readLine(ln)) {
			if (ln == ARCHIVE_PACKAGE_MEDIA_HEAD ":" ARCHIVE_ARCHIVE_MEDIA_TYPE ";")
				result = TXT;
		}
		file->close();
	}
	File::destroy(file);

	return result;
}

Archive* Archive::create(Formats type) {
	switch (type) {
	case TXT:
		return archive_create_txt();
	case ZIP:
		return archive_create_zip();
	default:
		assert(false && "Unknown.");

		return nullptr;
	}
}

void Archive::destroy(Archive* ptr) {
	const Formats type = ptr->format();
	switch (type) {
	case TXT:
		archive_destroy_txt(ptr);

		break;
	case ZIP:
		archive_destroy_zip(ptr);

		break;
	default:
		assert(false && "Unknown.");

		break;
	}
}

/* ===========================================================================} */
