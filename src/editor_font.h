/*
** Bitty
**
** An itty bitty 2D game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __EDITOR_FONT_H__
#define __EDITOR_FONT_H__

#include "editable.h"

/*
** {===========================================================================
** Font editor
*/

namespace Bitty {

class EditorFont : public Editable, public virtual Object {
public:
	BITTY_CLASS_TYPE('F', 'N', 'T', 'E')

	static EditorFont* create(void);
	static void destroy(EditorFont* ptr);
};

}

/* ===========================================================================} */

#endif /* __EDITOR_FONT_H__ */
