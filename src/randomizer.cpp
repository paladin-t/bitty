/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "randomizer.h"
#include <array>
#include <cfloat>
#include <time.h>

/*
** {===========================================================================
** Macros and constants
*/

#define FLOATATT(n) (DBL_##n)
#define FIGS FLOATATT(MANT_DIG)
#define MATHOP(op) (Double)op

#ifndef RANDOMIZER_TRIM64
#	define RANDOMIZER_TRIM64(x) ((x) & 0xffffffffffffffffu)
#endif /* RANDOMIZER_TRIM64 */
#ifndef RANDOMIZER_I2UINT
#	define RANDOMIZER_I2UINT(x) ((UInt64)RANDOMIZER_TRIM64(x))
#endif /* RANDOMIZER_I2UINT */
#ifndef RANDOMIZER_SHIFT64_FIG
#	define RANDOMIZER_SHIFT64_FIG (64 - FIGS)
#endif /* RANDOMIZER_SHIFT64_FIG */
#ifndef RANDOMIZER_SCALE_FIG
#	define RANDOMIZER_SCALE_FIG (MATHOP(0.5) / ((UInt64)1 << (FIGS - 1)))
#endif /* RANDOMIZER_SCALE_FIG */

/* ===========================================================================} */

/*
** {===========================================================================
** Randomizer
**
** @note See: "./lib/lua/src/lmathlib.c" for the original implementation.
*/

class RandomizerImpl : public Randomizer {
private:
	typedef std::array<UInt64, 4> State;

private:
	State _state;

public:
	RandomizerImpl() {
		const UInt64 first = (UInt64)time(nullptr);
		const UInt64 second = (UInt64)this;
		seed(first, second);
	}
	virtual ~RandomizerImpl() override {
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual Seed seed(UInt64 first, UInt64 second) override {
		_state[0] = (UInt64)first;
		_state[1] = (UInt64)0xff;
		_state[2] = (UInt64)second;
		_state[3] = (UInt64)0;
		for (int i = 0; i < 16; ++i)
			next(_state);

		return Seed(first, second);
	}
	virtual Seed seed(UInt64 first) override {
		return seed(first, 0);
	}
	virtual Seed seed(void) override {
		return seed((UInt64)time(nullptr));
	}

	virtual Int64 next(Int64 low, Int64 up) override {
		const UInt64 val = next(_state);
		if (low > up)
			std::swap(low, up);

		const UInt64 p = project(RANDOMIZER_I2UINT(val), (UInt64)up - (UInt64)low, _state);

		return p + (UInt64)low;
	}
	virtual Int64 next(Int64 up) override {
		const UInt64 val = next(_state);
		Int64 low = 1;
		if (up == 0)
			return RANDOMIZER_I2UINT(val);

		if (low > up)
			std::swap(low, up);

		const UInt64 p = project(RANDOMIZER_I2UINT(val), (UInt64)up - (UInt64)low, _state);

		return p + (UInt64)low;
	}
	virtual Double next(void) override {
		const UInt64 val = next(_state);

		return integerToDouble(val);
	}

private:
	static UInt64 rotateLeft(UInt64 x, int n) {
		return (x << n) | (RANDOMIZER_TRIM64(x) >> (64 - n));
	}
	static Double integerToDouble(UInt64 x) {
		return (Double)(RANDOMIZER_TRIM64(x) >> RANDOMIZER_SHIFT64_FIG) * RANDOMIZER_SCALE_FIG;
	}

	static UInt64 project(UInt64 ran, UInt64 n, State &state) {
		if ((n & (n + 1)) == 0) {
			return ran & n;
		} else {
			UInt64 lim = n;
			lim |= (lim >> 1);
			lim |= (lim >> 2);
			lim |= (lim >> 4);
			lim |= (lim >> 8);
			lim |= (lim >> 16);
			#if (LUA_MAXUNSIGNED >> 31) >= 3
			lim |= (lim >> 32);
			#endif
			assert((lim & (lim + 1)) == 0 && lim >= n && (lim >> 1) < n);
			while ((ran &= lim) > n)
				ran = RANDOMIZER_I2UINT(next(state));

			return ran;
		}
	}
	static UInt64 next(State &state) {
		const UInt64 state0 = state[0];
		const UInt64 state1 = state[1];
		const UInt64 state2 = state[2] ^ state0;
		const UInt64 state3 = state[3] ^ state1;
		const UInt64 res = rotateLeft(state1 * 5, 7) * 9;
		state[0] = state0 ^ state3;
		state[1] = state1 ^ state2;
		state[2] = state2 ^ (state1 << 17);
		state[3] = rotateLeft(state3, 45);

		return res;
	}
};

Randomizer* Randomizer::create(void) {
	RandomizerImpl* p = new RandomizerImpl();

	return p;
}

void Randomizer::destroy(Randomizer* ptr) {
	RandomizerImpl* impl = static_cast<RandomizerImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
