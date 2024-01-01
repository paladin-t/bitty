/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2024 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __PATHFINDER_H__
#define __PATHFINDER_H__

#include "bitty.h"
#include "mathematics.h"
#include "object.h"

/*
** {===========================================================================
** Pathfinder
*/

/**
 * @brief Pathfinder algorithm.
 */
class Pathfinder : public virtual Object {
public:
	typedef std::shared_ptr<Pathfinder> Ptr;

	typedef std::function<float(const Math::Vec2i &)> EvaluationHandler;

public:
	BITTY_CLASS_TYPE('P', 'T', 'H', 'R')

	virtual float diagonalCost(void) const = 0;
	virtual void diagonalCost(float cost) = 0;

	/**
	 * @param[out] cost
	 */
	virtual bool get(const Math::Vec2i &pos, float* cost /* nullable */) const = 0;
	virtual bool set(const Math::Vec2i &pos, float cost) = 0;

	virtual void clear(void) = 0;

	/**
	 * @param[out] path
	 * @param[out] cost
	 */
	virtual int solve(
		const Math::Vec2i &begin, const Math::Vec2i &end,
		EvaluationHandler eval /* nullable */,
		Math::Vec2i::List &path, float* cost /* nullable */
	) = 0;

	static Pathfinder* create(int w, int n, int e, int s);
	static void destroy(Pathfinder* ptr);
};

/* ===========================================================================} */

#endif /* __PATHFINDER_H__ */
