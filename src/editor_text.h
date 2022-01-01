/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __EDITOR_TEXT_H__
#define __EDITOR_TEXT_H__

#include "editable.h"

/*
** {===========================================================================
** Text editor
*/

class EditorText : public Editable, public virtual Object {
public:
	BITTY_CLASS_TYPE('T', 'X', 'T', 'E')

	/**
	 * @param[out] len
	 */
	virtual const char* text(size_t* len) const = 0;
	virtual void text(const char* txt, size_t len = 0) = 0;

	static EditorText* create(void);
	static void destroy(EditorText* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_TEXT_H__ */
