/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __EDITOR_PALETTE_H__
#define __EDITOR_PALETTE_H__

#include "editor.h"

/*
** {===========================================================================
** Palette editor
*/

class EditorPalette : public Editor, public virtual Object {
public:
	BITTY_CLASS_TYPE('P', 'L', 'T', 'E')

	static EditorPalette* create(void);
	static void destroy(EditorPalette* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_PALETTE_H__ */
