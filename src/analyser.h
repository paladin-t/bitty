/*
** Bitty
**
** An itty bitty 2D game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __ANALYSER_H__
#define __ANALYSER_H__

#include "bitty.h"
#include "mathematics.h"
#include <complex>
#include <vector>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef ANALYSER_COLUMN_COUNT
#	define ANALYSER_COLUMN_COUNT 24
#endif /* ANALYSER_COLUMN_COUNT */

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

typedef std::complex<float> Complex;

/* ===========================================================================} */

/*
** {===========================================================================
** Analyser
*/

/**
 * @brief Sound analyser.
 */
class Analyser {
public:
	struct Spec {
		int frequency = 0;
		UInt16 format = 0;
		int channels = 0;
	};

public:
	static Spec deviceSpec(void);

	/**
	 * @param[out] complexes
	 */
	static void analyse(const Spec &spec, void* stream, int len, Complex* complexes, int size);
	/**
	 * @param[out] complexes
	 */
	static void analyse(const Spec &spec, void* stream, int len, Complex complexes[ANALYSER_COLUMN_COUNT]);
	/**
	 * @param[out] columns
	 */
	static void analyse(const Spec &spec, void* stream, int len, float* columns, int size);
	/**
	 * @param[out] columns
	 */
	static void analyse(const Spec &spec, void* stream, int len, float columns[ANALYSER_COLUMN_COUNT]);
};

/* ===========================================================================} */

#endif /* __ANALYSER_H__ */
