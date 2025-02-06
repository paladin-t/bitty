/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __EDITOR_JSON_H__
#define __EDITOR_JSON_H__

#include "editable.h"

/*
** {===========================================================================
** JSON editor
*/

class EditorJson : public Editable, public virtual Object {
public:
	BITTY_CLASS_TYPE('J', 'S', 'N', 'E')

	/**
	 * @param[out] len
	 */
	virtual const char* text(size_t* len) const = 0;
	virtual void text(const char* txt, size_t len = 0) = 0;

	static EditorJson* create(void);
	static void destroy(EditorJson* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_JSON_H__ */
