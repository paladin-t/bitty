/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2023 Tony Wang, all rights reserved
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

class EditorFont : public Editable, public virtual Object {
public:
	BITTY_CLASS_TYPE('F', 'N', 'T', 'E')

	static EditorFont* create(void);
	static void destroy(EditorFont* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_FONT_H__ */
