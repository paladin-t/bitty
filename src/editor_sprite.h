/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2023 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __EDITOR_SPRITE_H__
#define __EDITOR_SPRITE_H__

#include "editor.h"

/*
** {===========================================================================
** Sprite editor
*/

class EditorSprite : public Editor, public virtual Object {
public:
	BITTY_CLASS_TYPE('S', 'P', 'R', 'E')

	static EditorSprite* create(void);
	static void destroy(EditorSprite* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_SPRITE_H__ */
