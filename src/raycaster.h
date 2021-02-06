/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __RAYCASTER_H__
#define __RAYCASTER_H__

#include "bitty.h"
#include "either.h"
#include "mathematics.h"
#include "object.h"

/*
** {===========================================================================
** Raycaster
*/

/**
 * @brief Raycaster algorithm.
 */
class Raycaster : public virtual Object {
public:
	typedef std::shared_ptr<Raycaster> Ptr;

	typedef std::function<bool(const Math::Vec2i &)> BlockingHandler;
	typedef std::function<int(const Math::Vec2i &)> EvaluationHandler;

	typedef Either<BlockingHandler, EvaluationHandler> AccessHandler;

public:
	BITTY_CLASS_TYPE('R', 'C', 'S', 'T')

	virtual Math::Vec2i tileSize(void) const = 0;
	virtual void tileSize(const Math::Vec2i &size) = 0;

	virtual Math::Vec2f offset(void) const = 0;
	virtual void offset(const Math::Vec2f &offset) = 0;

	/**
	 * @param[out] intersectionPos
	 * @param[out] intersectionIndex
	 */
	virtual int solve(
		const Math::Vec2f &rayPos, const Math::Vec2f &rayDir,
		const AccessHandler &access,
		Math::Vec2f &intersectionPos, Math::Vec2i &intersectionIndex
	) = 0;

	static Raycaster* create(void);
	static void destroy(Raycaster* ptr);
};

/* ===========================================================================} */

#endif /* __RAYCASTER_H__ */
