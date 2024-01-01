/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2024 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __ARCHIVE_ZIP_H__
#define __ARCHIVE_ZIP_H__

#include "archive.h"

/*
** {===========================================================================
** ZIP archive
*/

class Archive* archive_create_zip(void);
void archive_destroy_zip(class Archive* ptr);

/* ===========================================================================} */

#endif /* __ARCHIVE_ZIP_H__ */
