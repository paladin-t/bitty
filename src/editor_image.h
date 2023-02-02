/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2023 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __EDITOR_IMAGE_H__
#define __EDITOR_IMAGE_H__

#include "editor.h"

/*
** {===========================================================================
** Image editor
*/

class EditorImage : public Editor, public virtual Object {
public:
	BITTY_CLASS_TYPE('I', 'M', 'G', 'E')

	static EditorImage* create(void);
	static void destroy(EditorImage* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_IMAGE_H__ */
