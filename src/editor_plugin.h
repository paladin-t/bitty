/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __EDITOR_PLUGIN_H__
#define __EDITOR_PLUGIN_H__

#include "editable.h"

/*
** {===========================================================================
** Editor customized by plugin
*/

class EditorPlugin : public Editable, public virtual Object {
public:
	BITTY_CLASS_TYPE('P', 'L', 'G', 'E')

	static EditorPlugin* create(void);
	static void destroy(EditorPlugin* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_PLUGIN_H__ */
