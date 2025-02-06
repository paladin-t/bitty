/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __WALKER_H__
#define __WALKER_H__

#include "bitty.h"
#include "either.h"
#include "mathematics.h"
#include "object.h"

/*
** {===========================================================================
** Walker
*/

/**
 * @brief Walker algorithm.
 */
class Walker : public virtual Object {
public:
	typedef std::shared_ptr<Walker> Ptr;

	enum Directions : unsigned short {
		NONE = 0,
		LEFT = 1 << 0,
		RIGHT = 1 << 1,
		UP = 1 << 2,
		DOWN = 1 << 3
	};

	struct Blocking {
		bool block = false;
		unsigned pass = 0;

		Blocking();
		Blocking(bool blk, unsigned pass);
	};

	typedef std::function<Blocking(const Math::Vec2i &)> BlockingHandler;
	typedef std::function<int(const Math::Vec2i &)> EvaluationHandler;

	typedef Either<BlockingHandler, EvaluationHandler> AccessHandler;

public:
	BITTY_CLASS_TYPE('W', 'L', 'K', 'R')

	virtual Math::Vec2i objectSize(void) const = 0;
	virtual void objectSize(const Math::Vec2i &size) = 0;

	virtual Math::Vec2i tileSize(void) const = 0;
	virtual void tileSize(const Math::Vec2i &size) = 0;

	virtual Math::Vec2f offset(void) const = 0;
	virtual void offset(const Math::Vec2f &offset) = 0;

	/**
	 * @param[out] newDir
	 */
	virtual int solve(
		const Math::Vec2f &objPos, const Math::Vec2f &expDir,
		const AccessHandler &access,
		Math::Vec2f &newDir,
		int slidable
	) = 0;

	static Walker* create(void);
	static void destroy(Walker* ptr);
};

/* ===========================================================================} */

#endif /* __WALKER_H__ */
