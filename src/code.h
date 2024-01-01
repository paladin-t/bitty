/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2024 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __CODE_H__
#define __CODE_H__

#include "bitty.h"
#include "object.h"

/*
** {===========================================================================
** Code
*/

/**
 * @brief Code resource object.
 */
class Code : public virtual Object {
public:
	typedef std::shared_ptr<Code> Ptr;

public:
	BITTY_CLASS_TYPE('C', 'O', 'D', 'A')

	/**
	 * @param[out] len
	 */
	virtual const char* text(size_t* len /* nullable */) const = 0;
	virtual void text(const char* txt /* nullable */, size_t len /* = 0 */) = 0;

	static Code* create(void);
	static void destroy(Code* ptr);
};

/* ===========================================================================} */

#endif /* __CODE_H__ */
