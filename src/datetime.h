/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __DATETIME_H__
#define __DATETIME_H__

#include "bitty.h"
#include <string>

/*
** {===========================================================================
** Date time
*/

/**
 * @brief Date time utilities.
 */
class DateTime {
public:
	/**
	 * @param[out] sec 0-based.
	 * @param[out] mi 0-based.
	 * @param[out] hr 0-based.
	 * @param[out] mday 1-based.
	 * @param[out] mo 0-based.
	 * @param[out] yr since 1900.
	 * @param[out] wday 0-based, days since Sunday.
	 * @param[out] yday 0-based, days since January 1.
	 * @param[out] isdst Daylight savings time flag.
	 * @return The UTC time in seconds elapsed since midnight, Jan. 1, 1970,
	 *   or -1 for error.
	 */
	static long long utc(
		int* sec = nullptr, int* mi = nullptr, int* hr = nullptr,
		int* mday = nullptr, int* mo = nullptr, int* yr = nullptr,
		int* wday = nullptr,
		int* yday = nullptr,
		int* isdst = nullptr
	);
	/**
	 * @param[out] str In "YYYY-MM-DD HH:MM:SS" format.
	 * @return The UTC time in seconds elapsed since midnight, Jan. 1, 1970,
	 *   or -1 for error.
	 */
	static long long utc(std::string &str);

	/**
	 * @param[out] sec 0-based.
	 * @param[out] mi 0-based.
	 * @param[out] hr 0-based.
	 * @param[out] mday 1-based.
	 * @param[out] mo 0-based.
	 * @param[out] yr since 1900.
	 * @param[out] wday 0-based, days since Sunday.
	 * @param[out] yday 0-based, days since January 1.
	 * @param[out] isdst Daylight savings time flag.
	 * @return The local time in seconds elapsed since midnight, Jan. 1, 1970,
	 *   or -1 for error.
	 */
	static long long now(
		int* sec = nullptr, int* mi = nullptr, int* hr = nullptr,
		int* mday = nullptr, int* mo = nullptr, int* yr = nullptr,
		int* wday = nullptr,
		int* yday = nullptr,
		int* isdst = nullptr
	);
	/**
	 * @param[out] str In "YYYY-MM-DD HH:MM:SS" format.
	 * @return The local time in seconds elapsed since midnight, Jan. 1, 1970,
	 *   or -1 for error.
	 */
	static long long now(std::string &str);

	/**
	 * @brief Gets wall clock independent ticks in nanoseconds.
	 */
	static long long ticks(void);

	/**
	 * @brief Converts nanoseconds to milliseconds.
	 */
	static int toMilliseconds(long long t);
	/**
	 * @brief Converts milliseconds to nanoseconds.
	 */
	static long long fromMilliseconds(int t);

	/**
	 * @brief Converts nanoseconds to seconds.
	 */
	static double toSeconds(long long t);
	/**
	 * @brief Converts seconds to nanoseconds.
	 */
	static long long fromSeconds(double t);

	/**
	 * @brief Sleeps for specific milliseconds.
	 */
	static void sleep(int ms);
};

/* ===========================================================================} */

#endif /* __DATETIME_H__ */
