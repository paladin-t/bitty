/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2023 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "datetime.h"
#include "text.h"
#include <thread>
#if defined BITTY_CP_VC
#	include <Windows.h>
#endif /* BITTY_CP_VC */

/*
** {===========================================================================
** Utilities
*/

#if defined BITTY_CP_VC
static long long datetimeTicks(void) {
	static LONGLONG start = 0; // Shared.
	LARGE_INTEGER li;
	LONGLONG freq = 0;
	long long ret = 0;

	::QueryPerformanceFrequency(&li);
	freq = li.QuadPart;
	::QueryPerformanceCounter(&li);
	if (start == 0)
		start = li.QuadPart;
	if (freq == 10000000)
		ret = (li.QuadPart - start) * 100;
	else
		ret = (long long)((double)(li.QuadPart - start) / (freq / 1000000000.0));

	return ret;
}

static void datetimeSleep(int ms) {
	::Sleep((DWORD)ms);
}
#else /* BITTY_CP_VC */
static long long datetimeTicks(void) {
	static long long start = 0; // Shared.
	const long long now = std::chrono::steady_clock::now().time_since_epoch().count();
	if (start == 0)
		start = now;
	long long ret = 0;
	if (now >= start)
		ret = now - start;

	return ret;
}

static void datetimeSleep(int ms) {
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
#endif /* BITTY_CP_VC */

/* ===========================================================================} */

/*
** {===========================================================================
** Date time
*/

long long DateTime::utc(int* sec, int* mi, int* hr, int* mday, int* mo, int* yr, int* wday, int* yday, int* isdst) {
	time_t ct;
	struct tm* timeinfo = nullptr;
	const long long ret = (long long)time(&ct);
	timeinfo = gmtime(&ct);
	if (sec)
		*sec = timeinfo->tm_sec;
	if (mi)
		*mi = timeinfo->tm_min;
	if (hr)
		*hr = timeinfo->tm_hour;
	if (mday)
		*mday = timeinfo->tm_mday;
	if (mo)
		*mo = timeinfo->tm_mon;
	if (yr)
		*yr = timeinfo->tm_year;
	if (wday)
		*wday = timeinfo->tm_wday;
	if (yday)
		*yday = timeinfo->tm_yday;
	if (isdst)
		*isdst = timeinfo->tm_isdst;

	return ret;
}

long long DateTime::utc(std::string &str) {
	int sec = 0, mi = 0, hr = 0;
	int mday = 0, mo = 0, yr = 0;
	int wday = 0, yday = 0, isdst = 0;
	const long long ticks = DateTime::utc(
		&sec, &mi, &hr,
		&mday, &mo, &yr,
		&wday, &yday, &isdst
	);
	str = Text::toString(yr + 1900);
	str += '-';
	str += Text::toString(mo + 1, 2u, '0', std::ios::fmtflags(0), true);
	str += '-';
	str += Text::toString(mday, 2u, '0', std::ios::fmtflags(0), true);
	str += ' ';
	str += Text::toString(hr, 2u, '0', std::ios::fmtflags(0), true);
	str += ':';
	str += Text::toString(mi, 2u, '0', std::ios::fmtflags(0), true);
	str += ':';
	str += Text::toString(sec, 2u, '0', std::ios::fmtflags(0), true);

	return ticks;
}

long long DateTime::now(int* sec, int* mi, int* hr, int* mday, int* mo, int* yr, int* wday, int* yday, int* isdst) {
	time_t ct;
	struct tm* timeinfo = nullptr;
	const long long ret = (long long)time(&ct);
	timeinfo = localtime(&ct);
	if (sec)
		*sec = timeinfo->tm_sec;
	if (mi)
		*mi = timeinfo->tm_min;
	if (hr)
		*hr = timeinfo->tm_hour;
	if (mday)
		*mday = timeinfo->tm_mday;
	if (mo)
		*mo = timeinfo->tm_mon;
	if (yr)
		*yr = timeinfo->tm_year;
	if (wday)
		*wday = timeinfo->tm_wday;
	if (yday)
		*yday = timeinfo->tm_yday;
	if (isdst)
		*isdst = timeinfo->tm_isdst;

	return ret;
}

long long DateTime::now(std::string &str) {
	int sec = 0, mi = 0, hr = 0;
	int mday = 0, mo = 0, yr = 0;
	int wday = 0, yday = 0, isdst = 0;
	const long long ticks = DateTime::now(
		&sec, &mi, &hr,
		&mday, &mo, &yr,
		&wday, &yday, &isdst
	);
	str = Text::toString(yr + 1900);
	str += '-';
	str += Text::toString(mo + 1, 2u, '0', std::ios::fmtflags(0), true);
	str += '-';
	str += Text::toString(mday, 2u, '0', std::ios::fmtflags(0), true);
	str += ' ';
	str += Text::toString(hr, 2u, '0', std::ios::fmtflags(0), true);
	str += ':';
	str += Text::toString(mi, 2u, '0', std::ios::fmtflags(0), true);
	str += ':';
	str += Text::toString(sec, 2u, '0', std::ios::fmtflags(0), true);

	return ticks;
}

long long DateTime::ticks(void) {
	return datetimeTicks();
}

int DateTime::toMilliseconds(long long t) {
	return (int)(t / 1000000);
}

long long DateTime::fromMilliseconds(int t) {
	return (long long)t * 1000000ull;
}

double DateTime::toSeconds(long long t) {
	return t / 1000000000.0;
}

long long DateTime::fromSeconds(double t) {
	return (long long)(t * 1000000000.0);
}

void DateTime::sleep(int ms) {
	datetimeSleep(ms);
}

/* ===========================================================================} */
