/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2024 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __EDITOR_BYTES_H__
#define __EDITOR_BYTES_H__

#include "editable.h"

/*
** {===========================================================================
** Bytes editor
*/

class EditorBytes : public Editable, public virtual Object {
public:
	BITTY_CLASS_TYPE('B', 'Y', 'T', 'E')

	static EditorBytes* create(void);
	static void destroy(EditorBytes* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_BYTES_H__ */
