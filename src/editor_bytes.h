/*
** Bitty
**
** An itty bitty 2D game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
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

namespace Bitty {

class EditorBytes : public Editable, public virtual Object {
public:
	BITTY_CLASS_TYPE('B', 'Y', 'T', 'E')

	static EditorBytes* create(void);
	static void destroy(EditorBytes* ptr);
};

}

/* ===========================================================================} */

#endif /* __EDITOR_BYTES_H__ */
