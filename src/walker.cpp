/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "walker.h"

/*
** {===========================================================================
** Walker
**
** @note See: https://paladin-t.github.io/articles/smooth-tile-based-movement-algorithm-with-sliding.html.
*/

class WalkerImpl : public Walker {
private:
	Math::Vec2i _objSize = Math::Vec2i(BITTY_GRID_DEFAULT_SIZE, BITTY_GRID_DEFAULT_SIZE);
	Math::Vec2i _tileSize = Math::Vec2i(BITTY_GRID_DEFAULT_SIZE, BITTY_GRID_DEFAULT_SIZE);
	Math::Vec2f _offset = Math::Vec2f(0, 0);

public:
	WalkerImpl() {
	}
	virtual ~WalkerImpl() override {
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual Math::Vec2i objectSize(void) const override {
		return _objSize;
	}
	virtual void objectSize(const Math::Vec2i &size) override {
		_objSize = size;
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
		const Math::Vec2f &objPos, const Math::Vec2f &expDir,
		const AccessHandler &access,
		Math::Vec2f &newDir,
		int slidable
	) override {
		if (_objSize.x <= 0 || _objSize.y <= 0)
			return 0;
		if (_tileSize.x <= 0 || _tileSize.y <= 0)
			return 0;

		const Real expDirX = expDir.x, expDirY = expDir.y;
		int n = tend( // Tend straightforward.
			objPos, expDir,
			access,
			newDir,
			slidable,
			&_objSize, &_tileSize, &_offset
		);
		if (!n)
			return n;

		if (!slidable)
			return n;

		if (Math::sign(expDirX) != Math::sign(newDir.x) || Math::sign(expDirY) != Math::sign(newDir.y)) { // The movement has been redirected.
			const Math::Vec2f newExpDir(newDir.x, newDir.y);
			Math::Vec2f newNewDir(0, 0);
			n = tend( // Tend into a new direction.
				objPos, newExpDir,
				access,
				newNewDir,
				slidable,
				&_objSize, &_tileSize, &_offset
			);
			if (!n)
				return n;

			if (Math::sign(newDir.x) != Math::sign(newNewDir.x) || Math::sign(newDir.y) != Math::sign(newNewDir.y)) // Neither passable.
				return 0;
		}

		return n;
	}

private:
	template<typename T = Real> static int tend(
		const Math::Vec2f &objPos, const Math::Vec2f &expDir,
		const AccessHandler &access,
		Math::Vec2f &newDir,
		int slidable,
		const Math::Vec2i* objSize_, const Math::Vec2i* tileSize_, const Math::Vec2f* offset
	) {
		// Prepare.
		typedef T Number;

		constexpr const Number EPSILON = Number(0.000001f);
		constexpr const Number MARGIN = Number(1.001f);

		const Math::Vec2i objSize = objSize_ ? *objSize_ : Math::Vec2i(BITTY_GRID_DEFAULT_SIZE, BITTY_GRID_DEFAULT_SIZE);
		const Math::Vec2i tileSize = tileSize_ ? *tileSize_ : Math::Vec2i(BITTY_GRID_DEFAULT_SIZE, BITTY_GRID_DEFAULT_SIZE);

		BlockingHandler block = nullptr;
		if (access.isLeft()) {
			block = access.left().get();
		} else {
			EvaluationHandler eval = access.right().get();
			block = [eval] (const Math::Vec2i &pos) -> Blocking {
				return Blocking(eval(pos) > 15, NONE);
			};
		}

		if (expDir.x == 0 && expDir.y == 0) {
			newDir.x = newDir.y = 0;

			return 0;
		}

		// Calculate the edges and center position.
		const Number objWidth = Number(objSize.x);
		const Number objHeight = Number(objSize.y);

		const Number objLocalX0 = -objWidth / 2;
		const Number objLocalX1 = objWidth / 2;
		const Number objLocalY0 = -objHeight / 2;
		const Number objLocalY1 = objHeight / 2;

		Number centerX = Number(objPos.x) + objWidth / 2 + Number(expDir.x);
		Number centerY = Number(objPos.y) + objHeight / 2 + Number(expDir.y);
		if (offset) {
			centerX -= Number(offset->x);
			centerY -= Number(offset->y);
		}

		// Resolve.
		Number dirX = Number(expDir.x);
		Number dirY = Number(expDir.y);
		Number dampingX = Number(0);
		Number dampingY = Number(0);

		const Number stepHeight = Number(objHeight - MARGIN * 2);
		const Number stepY = Number(stepHeight / std::ceil(stepHeight / tileSize.y));
		if (dirX > Number(0)) {
			int total = 0;
			int blocked = 0;
			Number diffX = 0;
			const Number frontX = centerX + objLocalX1;
			for (Number j = Number(objLocalY0 + MARGIN); j < objLocalY1; j += stepY) {
				const Number frontY = centerY + j;
				const int frontTileIdxX = (int)std::floor(frontX / tileSize.x);
				const int frontTileIdxY = (int)std::floor(frontY / tileSize.y);
				const Blocking blk = block(Math::Vec2i(frontTileIdxX, frontTileIdxY));
				if (blk.block && !(blk.pass & RIGHT)) {
					const Number diff = frontTileIdxX * tileSize.x - frontX;
					if (diff < diffX)
						diffX = diff;
					dampingX -= Math::sign(j) * std::abs(diff);
					++blocked;
				}
				++total;
			}
			if (diffX < Number(0)) {
				dirX += diffX;
				if (std::abs(dirX) <= EPSILON)
					dirX = Number(0);
			}
			if (dirX < Number(0))
				dirX = Number(0);
			if (Number(blocked) / total * 10 > slidable)
				dampingX = 0;
		} else if (dirX < Number(0)) {
			int total = 0;
			int blocked = 0;
			Number diffX = 0;
			const Number frontX = centerX + objLocalX0;
			for (Number j = Number(objLocalY0 + MARGIN); j < objLocalY1; j += stepY) {
				const Number frontY = centerY + j;
				const int frontTileIdxX = (int)std::floor(frontX / tileSize.x);
				const int frontTileIdxY = (int)std::floor(frontY / tileSize.y);
				const Blocking blk = block(Math::Vec2i(frontTileIdxX, frontTileIdxY));
				if (blk.block && !(blk.pass & LEFT)) {
					const Number diff = frontTileIdxX * tileSize.x + tileSize.x - frontX;
					if (diff > diffX)
						diffX = diff;
					dampingX -= Math::sign(j) * std::abs(diff);
					++blocked;
				}
				++total;
			}
			if (diffX > Number(0)) {
				dirX += diffX;
				if (std::abs(dirX) <= EPSILON)
					dirX = Number(0);
			}
			if (dirX > Number(0))
				dirX = Number(0);
			if (Number(blocked) / total * 10 > slidable)
				dampingX = 0;
		}

		const Number stepWidth = Number(objWidth - MARGIN * 2);
		const Number stepX = Number(stepWidth / std::ceil(stepWidth / tileSize.x));
		if (dirY > Number(0)) {
			int total = 0;
			int blocked = 0;
			Number diffY = 0;
			const Number frontY = centerY + objLocalY1;
			for (Number i = Number(objLocalX0 + MARGIN); i < objLocalX1; i += stepX) {
				const Number frontX = centerX + i;
				const int frontTileIdxX = (int)std::floor(frontX / tileSize.x);
				const int frontTileIdxY = (int)std::floor(frontY / tileSize.y);
				const Blocking blk = block(Math::Vec2i(frontTileIdxX, frontTileIdxY));
				if (blk.block && !(blk.pass & DOWN)) {
					const Number diff = frontTileIdxY * tileSize.y - frontY;
					if (diff < diffY)
						diffY = diff;
					dampingY -= Math::sign(i) * std::abs(diff);
					++blocked;
				}
				++total;
			}
			if (diffY < Number(0)) {
				dirY += diffY;
				if (std::abs(dirY) <= EPSILON)
					dirY = Number(0);
			}
			if (dirY < Number(0))
				dirY = Number(0);
			if (Number(blocked) / total * 10 > slidable)
				dampingY = 0;
		} else if (dirY < Number(0)) {
			int total = 0;
			int blocked = 0;
			Number diffY = 0;
			const Number frontY = centerY + objLocalY0;
			for (Number i = Number(objLocalX0 + MARGIN); i < objLocalX1; i += stepX) {
				const Number frontX = centerX + i;
				const int frontTileIdxX = (int)std::floor(frontX / tileSize.x);
				const int frontTileIdxY = (int)std::floor(frontY / tileSize.y);
				const Blocking blk = block(Math::Vec2i(frontTileIdxX, frontTileIdxY));
				if (blk.block && !(blk.pass & UP)) {
					const Number diff = frontTileIdxY * tileSize.y + tileSize.y - frontY;
					if (diff > diffY)
						diffY = diff;
					dampingY -= Math::sign(i) * std::abs(diff);
					++blocked;
				}
				++total;
			}
			if (diffY > Number(0)) {
				dirY += diffY;
				if (std::abs(dirY) <= EPSILON)
					dirY = Number(0);
			}
			if (dirY > Number(0))
				dirY = Number(0);
			if (Number(blocked) / total * 10 > slidable)
				dampingY = 0;
		}

		// Slide.
		if (slidable) {
			if (dirX == Number(0) && expDir.x != 0 && expDir.y == 0) {
				if (dampingX == Number(0)) {
					dirY = Number(0);
				} else {
					const Number expDirX = Number(expDir.x);
					Number frontX = Number(0);
					if (expDirX > Number(0))
						frontX = centerX + objLocalX1;
					else if (expDirX < Number(0))
						frontX = centerX + objLocalX0;
					if (expDirX != Number(0)) {
						const Number frontY = centerY;
						const int frontTileIdxX = (int)std::floor(frontX / tileSize.x);
						const int frontTileIdxY = (int)std::floor(frontY / tileSize.y);
						const Blocking blk = block(Math::Vec2i(frontTileIdxX, frontTileIdxY));
						if (!blk.block) {
							if (dampingX < Number(0))
								dirY = (frontTileIdxY + 1) * tileSize.y - (frontY + objLocalY1);
							else /* if (dampingX > Number(0)) */
								dirY = frontTileIdxY * tileSize.y - (frontY + objLocalY0);
							if (std::abs(dirY) > std::abs(expDirX))
								dirY = Math::sign(dirY) * std::abs(expDirX);
						}
					}
				}
			}

			if (dirY == Number(0) && expDir.y != 0 && expDir.x == 0) {
				if (dampingY == Number(0)) {
					dirX = Number(0);
				} else {
					const Number expDirY = Number(expDir.y);
					Number frontY = Number(0);
					if (expDirY > Number(0))
						frontY = centerY + objLocalY1;
					else if (expDirY < Number(0))
						frontY = centerY + objLocalY0;
					if (expDirY != Number(0)) {
						const Number frontX = centerX;
						const int frontTileIdxX = (int)std::floor(frontX / tileSize.x);
						const int frontTileIdxY = (int)std::floor(frontY / tileSize.y);
						const Blocking blk = block(Math::Vec2i(frontTileIdxX, frontTileIdxY));
						if (!blk.block) {
							if (dampingY < Number(0))
								dirX = (frontTileIdxX + 1) * tileSize.x - (frontX + objLocalX1);
							else /* if (dampingY > Number(0)) */
								dirX = frontTileIdxX * tileSize.x - (frontX + objLocalX0);
							if (std::abs(dirX) > std::abs(expDirY))
								dirX = Math::sign(dirX) * std::abs(expDirY);
						}
					}
				}
			}
		}

		// Accept.
		newDir = Math::Vec2f((Real)dirX, (Real)dirY);

		return (int)newDir.length();
	}
};

Walker::Blocking::Blocking() {
}

Walker::Blocking::Blocking(bool blk, unsigned pass_) : block(blk), pass(pass_) {
}

Walker* Walker::create(void) {
	WalkerImpl* p = new WalkerImpl();

	return p;
}

void Walker::destroy(Walker* ptr) {
	WalkerImpl* impl = static_cast<WalkerImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
