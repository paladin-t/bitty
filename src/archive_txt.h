/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2023 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __ARCHIVE_TXT_H__
#define __ARCHIVE_TXT_H__

#include "archive.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef ARCHIVE_ARCHIVE_MEDIA_TYPE
#	define ARCHIVE_ARCHIVE_MEDIA_TYPE "application/vnd.bitty-archive"
#endif /* ARCHIVE_ARCHIVE_MEDIA_TYPE */

/* ===========================================================================} */

/*
** {===========================================================================
** Text-based archive
*/

class Archive* archive_create_txt(void);
void archive_destroy_txt(class Archive* ptr);

/* ===========================================================================} */

#endif /* __ARCHIVE_TXT_H__ */
