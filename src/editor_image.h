/*
** Bitty
**
** An itty bitty 2D game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
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

namespace Bitty {

class EditorImage : public Editor, public virtual Object {
public:
	BITTY_CLASS_TYPE('I', 'M', 'G', 'E')

	static EditorImage* create(void);
	static void destroy(EditorImage* ptr);
};

}

/* ===========================================================================} */

#endif /* __EDITOR_IMAGE_H__ */
