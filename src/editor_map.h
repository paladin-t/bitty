/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __EDITOR_MAP_H__
#define __EDITOR_MAP_H__

#include "editor.h"

/*
** {===========================================================================
** Map editor
*/

class EditorMap : public Editor, public virtual Object {
public:
	BITTY_CLASS_TYPE('M', 'A', 'P', 'E')

	static EditorMap* create(void);
	static void destroy(EditorMap* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_MAP_H__ */
