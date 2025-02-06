/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __EDITPR_SOUND_H__
#define __EDITPR_SOUND_H__

#include "editable.h"

/*
** {===========================================================================
** Sound editor
*/

class EditorSound : public Editable, public virtual Object {
public:
	BITTY_CLASS_TYPE('S', 'N', 'D', 'E')

	static EditorSound* create(void);
	static void destroy(EditorSound* ptr);
};

/* ===========================================================================} */

#endif /* __EDITPR_SOUND_H__ */
