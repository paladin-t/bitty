/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __PROMISE_H__
#define __PROMISE_H__

#include "bitty.h"
#include "object.h"
#include "plus.h"
#include "updatable.h"

/*
** {===========================================================================
** Promise
**
** @note Minimal promise implementation to help handling asynchronous functions.
*/

/**
 * @brief Promise object.
 */
class Promise : public Updatable, public virtual Object {
public:
	typedef std::shared_ptr<Promise> Ptr;
	typedef std::weak_ptr<Promise> WeakPtr;

	enum States {
		PENDING,
		RESOLVED,
		REJECTED
	};

	struct ThenHandler : public Handler<ThenHandler, void, ThenHandler*, const Variant &> {
		using Handler::Handler;
	};
	struct FailHandler : public Handler<FailHandler, void, FailHandler*, const Variant &> {
		using Handler::Handler;
	};
	struct AlwaysHandler : public Handler<AlwaysHandler, void, AlwaysHandler*> {
		using Handler::Handler;
	};

public:
	BITTY_CLASS_TYPE('P', 'R', 'M', 'S')

	virtual States state(void) const = 0;
	virtual Variant value(void) const = 0;
	virtual Variant error(void) const = 0;

	virtual Promise* then(const ThenHandler &cb /* nullable */) = 0;
	virtual Promise* fail(const FailHandler &cb /* nullable */) = 0;
	virtual Promise* always(const AlwaysHandler &cb /* nullable */) = 0;

	virtual void resolve(const Variant &val) = 0;
	virtual void reject(const Variant &val) = 0;

	virtual void clear(void) = 0;

	static Promise* create(void);
	static void destroy(Promise* ptr);
};

/* ===========================================================================} */

#endif /* __PROMISE_H__ */
