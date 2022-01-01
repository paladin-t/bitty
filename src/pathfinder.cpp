/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "pathfinder.h"
#include "../lib/micropather/micropather.h"

/*
** {===========================================================================
** Pathfinder
*/

template<typename T> class PathfinderImpl : public Pathfinder, public micropather::Graph {
private:
	typedef T Number;

	typedef union Node {
		struct { Number x; Number y; } point;
		void* pointer;
	} Node;

	static_assert(sizeof(Number) * 2 == sizeof(void*), "Wrong size.");
	static_assert(sizeof(Node) == sizeof(void*), "Wrong size.");

private:
	int _west = 0;
	int _north = 0;
	int _east = 0;
	int _south = 0;
	micropather::MicroPather* _pather = nullptr;

	float _diagonalCost = 1.414f;

	float* _matrix = nullptr;

	EvaluationHandler _evaluator = nullptr;

public:
	PathfinderImpl(int w, int n, int e, int s) : _west(w), _north(n), _east(e), _south(s) {
		if (_east < _west)
			std::swap(_west, _east);
		if (_south < _north)
			std::swap(_north, _south);

		_pather = new micropather::MicroPather(this, 1024);
	}
	virtual ~PathfinderImpl() override {
		delete _pather;

		if (_matrix) {
			delete [] _matrix;
			_matrix = nullptr;
		}
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual float LeastCostEstimate(void* nodeStart, void* nodeEnd) override {
		int bx = 0, by = 0, ex = 0, ey = 0;
		fromNode(nodeStart, &bx, &by);
		fromNode(nodeEnd, &ex, &ey);
		int dx = bx - ex;
		int dy = by - ey;

		return (float)sqrt((double)(dx * dx) + (double)(dy * dy));
	}
	virtual void AdjacentCost(void* node, MP_VECTOR<micropather::StateCost>* adjacent) override {
		// Prepare.
		int x = 0, y = 0;
		const int dx[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
		const int dy[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
		const float cost[8] = { 1, _diagonalCost, 1, _diagonalCost, 1, _diagonalCost, 1, _diagonalCost };
		fromNode(node, &x, &y);

		// For eight directions.
		for (int i = 0; i < 8; ++i) {
			// Get the position.
			int nx = x + dx[i];
			int ny = y + dy[i];

			// Check bounds.
			if (nx < _west || nx > _east || ny < _north || ny > _south) {
				micropather::StateCost nodeCost = { toNode(nx, ny), FLT_MAX };
				adjacent->push_back(nodeCost);

				continue;
			}

			// Acquire passable.
			float pass = 0;
			if (_evaluator) {
				pass = _evaluator(Math::Vec2i(nx, ny));
			} else {
				if (!get(Math::Vec2i(nx, ny), &pass))
					pass = 1;
			}

			// Make cost.
			if (pass > -1e-5) {
				if (pass < 0)
					pass = 0;
				if (cost[i] < -1e-5) {
					micropather::StateCost nodeCost = { toNode(nx, ny), FLT_MAX };
					adjacent->push_back(nodeCost);
				} else {
					micropather::StateCost nodeCost = { toNode(nx, ny), (float)(cost[i] * pass) };
					adjacent->push_back(nodeCost);
				}
			} else {
				micropather::StateCost nodeCost = { toNode(nx, ny), FLT_MAX };
				adjacent->push_back(nodeCost);
			}
		}
	}
	virtual void PrintStateInfo(void* node) override {
		int x, y;
		fromNode(node, &x, &y);
		fprintf(stdout, "At (%d, %d).\n", x, y);
	}

	virtual float diagonalCost(void) const override {
		return _diagonalCost;
	}
	virtual void diagonalCost(float cost) override {
		_diagonalCost = cost;
	}

	virtual bool get(const Math::Vec2i &pos, float* cost) const override {
		if (cost)
			*cost = 0;
		if (_matrix) {
			int i = index((int)pos.x, (int)pos.y);
			float c = _matrix[i];
			if (cost)
				*cost = c;

			return true;
		}

		return false;
	}
	virtual bool set(const Math::Vec2i &pos, float cost) override {
		if (!_matrix) {
			_matrix = new float[width() * height()];
			for (int i = _west; i <= _east; ++i) {
				for (int j = _north; j <= _south; ++j) {
					_matrix[index(i, j)] = 1;
				}
			}
		}
		int i = index((int)pos.x, (int)pos.y);
		if (i == -1)
			return false;

		_matrix[i] = cost;

		return true;
	}

	virtual void clear(void) override {
		_pather->Reset();
		if (_matrix) {
			delete [] _matrix;
			_matrix = nullptr;
		}
	}

	virtual int solve(
		const Math::Vec2i &begin, const Math::Vec2i &end,
		EvaluationHandler eval,
		Math::Vec2i::List &path, float* cost
	) override {
		const int bx = (int)begin.x;
		const int by = (int)begin.y;
		const int ex = (int)end.x;
		const int ey = (int)end.y;

		_evaluator = eval;
		micropather::MPVector<void*> ret;
		float tmpcost = 0;
		int result = _pather->Solve(toNode(bx, by), toNode(ex, ey), &ret, &tmpcost);
		if (cost)
			*cost = tmpcost;
		_evaluator = nullptr;

		for (unsigned i = 0; i < ret.size(); ++i) {
			void* node = ret[i];
			int x, y;
			PathfinderImpl::fromNode(node, &x, &y);
			path.push_back(Math::Vec2i(x, y));
		}

		return result;
	}

private:
	int width(void) const {
		return _east - _west + 1;
	}
	int height(void) const {
		return _south - _north + 1;
	}

	int index(int x, int y) const {
		if (x < _west || x > _east || y < _north || y > _south)
			return -1;

		x -= _west;
		y -= _north;
		int r = x + y * width();
		assert(r >= 0 && r < width() * height());

		return r;
	}

	static void fromNode(void* node, int* x, int* y) {
		Node n;
		n.pointer = node;
		if (x)
			*x = n.point.x;
		if (y)
			*y = n.point.y;
	}
	static void* toNode(int x, int y) {
		Node n;
		n.point.x = (Number)x;
		n.point.y = (Number)y;

		return n.pointer;
	}
};

#if INTPTR_MAX == INT32_MAX
	typedef PathfinderImpl<short> PathfinderImpl_;
#elif INTPTR_MAX == INT64_MAX
	typedef PathfinderImpl<int> PathfinderImpl_;
#endif /* INTPTR_MAX */

Pathfinder* Pathfinder::create(int w, int n, int e, int s) {
	PathfinderImpl_* p = new PathfinderImpl_(w, n, e, s);

	return p;
}

void Pathfinder::destroy(Pathfinder* ptr) {
	PathfinderImpl_* impl = static_cast<PathfinderImpl_*>(ptr);
	delete impl;
}

/* ===========================================================================} */
