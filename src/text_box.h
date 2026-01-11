/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __TEXT_BOX_H__
#define __TEXT_BOX_H__

#include "bitty.h"
#include "editable.h"

/*
** {===========================================================================
** Text box
*/

/**
 * @brief Text box object.
 */
class TextBox : public Editable, public virtual Object {
public:
	typedef std::shared_ptr<TextBox> Ptr;

public:
	BITTY_CLASS_TYPE('T', 'X', 'T', 'B')

	virtual const char* text(size_t* len) const = 0;
	virtual void text(const char* txt, size_t len = 0) = 0;

	virtual void update(
		class Window* wnd, class Renderer* rnd,
		class Workspace* ws,
		float x, float y, float width, float height
	) = 0;

	static TextBox* create(void);
	static void destroy(TextBox* ptr);
};

/* ===========================================================================} */

#endif /* __TEXT_BOX_H__ */
