/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "raycaster.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef RAYCASTER_MAX_LENGTH
#	define RAYCASTER_MAX_LENGTH (BITTY_GRID_DEFAULT_SIZE * 256)
#endif /* RAYCASTER_MAX_LENGTH */

/* ===========================================================================} */

/*
** {===========================================================================
** Raycaster
**
** @note See: https://lodev.org/cgtutor/raycasting.html.
*/

class RaycasterImpl : public Raycaster {
private:
	Math::Vec2i _tileSize = Math::Vec2i(BITTY_GRID_DEFAULT_SIZE, BITTY_GRID_DEFAULT_SIZE);
	Math::Vec2f _offset = Math::Vec2f(0, 0);

public:
	RaycasterImpl() {
	}
	virtual ~RaycasterImpl() override {
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual Math::Vec2i tileSize(void) const override {
		return _tileSize;
	}
	virtual void tileSize(const Math::Vec2i &size) override {
		_tileSize = size;
	}

	virtual Math::Vec2f offset(void) const override {
		return _offset;
	}
	virtual void offset(const Math::Vec2f &offset) override {
		_offset = offset;
	}

	virtual int solve(
		const Math::Vec2f &rayPos, const Math::Vec2f &rayDir,
		const AccessHandler &access,
		Math::Vec2f &intersectionPos, Math::Vec2i &intersectionIndex
	) override {
		// Prepare.
		if (_tileSize.x <= 0 || _tileSize.y <= 0)
			return 0;
		if (rayDir == Math::Vec2f(0, 0))
			return 0;

		BlockingHandler block = nullptr;
		if (access.isLeft()) {
			block = access.left().get();
		} else {
			EvaluationHandler eval = access.right().get();
			block = [eval] (const Math::Vec2i &pos) -> bool {
				return eval(pos) > 15;
			};
		}

		// Calculate ray position, index and direction.
		Math::Vec2f dir = rayDir;
		Real len = dir.normalize();
		len = std::min(len, (Real)RAYCASTER_MAX_LENGTH);
		const int steps = (int)(len / std::min(_tileSize.x, _tileSize.y)) * 2;

		const Math::Vec2f pos(
			rayPos.x - _offset.x,
			rayPos.y - _offset.y
		);
		const Math::Vec2f indexf(
			pos.x / _tileSize.x,
			pos.y / _tileSize.y
		);
		Math::Vec2i index(
			(Int)indexf.x,
			(Int)indexf.y
		);
		const Math::Vec2f deltaDst(
			std::abs(1 / dir.x) * _tileSize.x,
			std::abs(1 / dir.y) * _tileSize.y
		);

		// Calculate step and initial side distance.
		Math::Vec2i step;
		Math::Vec2f sideDst;
		if (dir.x < 0) {
			step.x = -1;
			sideDst.x = (indexf.x - index.x) * deltaDst.x;
		} else {
			step.x = 1;
			sideDst.x = (index.x + 1 - indexf.x) * deltaDst.x;
		}
		if (dir.y < 0) {
			step.y = -1;
			sideDst.y = (indexf.y - index.y) * deltaDst.y;
		} else {
			step.y = 1;
			sideDst.y = (index.y + 1 - indexf.y) * deltaDst.y;
		}

		// Perform DDA algorithm.
		int hit = 0;
		int side = 0;
		Math::Vec2i stepped;
		while (hit == 0 && (stepped.x + stepped.y < steps)) {
			if (sideDst.x < sideDst.y) {
				sideDst.x += deltaDst.x;
				index.x += step.x;
				side = 0;
				++stepped.x;
			} else {
				sideDst.y += deltaDst.y;
				index.y += step.y;
				side = 1;
				++stepped.y;
			}
			const bool blk = block(Math::Vec2i(index.x, index.y));
			if (blk)
				hit = 1;
		}

		// Calculate collision position and index.
		Real dist = 0.0f;
		if (_tileSize.x == _tileSize.y) {
			const Int tileSize = _tileSize.x;
			if (side == 0)
				dist = (index.x - indexf.x + (1 - step.x) / 2) / dir.x;
			else
				dist = (index.y - indexf.y + (1 - step.y) / 2) / dir.y;
			dist *= tileSize;
			intersectionPos = pos + dir * dist;
			if (dir.x > 0) // Correct it.
				--intersectionPos.x;
			if (dir.y > 0)
				--intersectionPos.y;
			intersectionIndex = index;
		} else {
			const Math::Vec2f centerPos((index.x + 0.5f) * _tileSize.x, (index.y + 0.5f) * _tileSize.y);
			dist = (centerPos - pos).length();
			intersectionPos = pos + dir * dist;
			intersectionIndex = index;
		}
		if (dist >= len) {
			dist = len;
			hit = 0;
		}

		return hit;
	}
};

Raycaster* Raycaster::create(void) {
	RaycasterImpl* p = new RaycasterImpl();

	return p;
}

void Raycaster::destroy(Raycaster* ptr) {
	RaycasterImpl* impl = static_cast<RaycasterImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
