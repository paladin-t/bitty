/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#define HAVE_M_PI
#include "mathematics.h"
#include <chrono>
#define _USE_MATH_DEFINES
#include <math.h>

/*
** {===========================================================================
** Math utilities
*/

namespace Math {

double PI(void) {
	return M_PI;
}

void srand(unsigned seed) {
	std::srand(seed);
}

void srand(void) {
	union {
		long long ticks;
		struct { unsigned low; unsigned high; } parts;
	} s;

	s.ticks = std::chrono::steady_clock::now().time_since_epoch().count();
	std::srand(s.parts.low);
}

int rand(void) {
	return std::rand();
}

}

/* ===========================================================================} */
