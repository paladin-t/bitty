/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2023 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __BYTES_H__
#define __BYTES_H__

#include "bitty.h"
#include "object.h"
#include "stream.h"

/*
** {===========================================================================
** Bytes
*/

/**
 * @brief Bytes streaming object.
 */
class Bytes : public Stream, public virtual Object {
public:
	typedef std::shared_ptr<Bytes> Ptr;

public:
	BITTY_CLASS_TYPE('B', 'Y', 'T', 'E')

	virtual const Byte* pointer(void) const = 0;
	virtual Byte* pointer(void) = 0;

	virtual const Byte &get(size_t index) const = 0;
	virtual void set(size_t index, Byte val) = 0;

	virtual Bytes* resize(size_t size) = 0;
	virtual void clear(void) = 0;

	static Bytes* create(void);
	static void destroy(Bytes* ptr);
};

/* ===========================================================================} */

#endif /* __BYTES_H__ */
