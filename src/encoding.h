/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __ENCODING_H__
#define __ENCODING_H__

#include "bitty.h"
#include <string>

/*
** {===========================================================================
** Unicode
*/

/**
 * @brief Unicode utilities.
 */
class Unicode {
public:
	static std::string fromOs(const char* str);
	static std::string fromOs(const std::string &str);
	static std::string toOs(const char* str);
	static std::string toOs(const std::string &str);

	static std::string fromWide(const wchar_t* str);
	static std::string fromWide(const std::wstring &str);
	static std::wstring toWide(const char* str);
	static std::wstring toWide(const std::string &str);

	static bool isAscii(const char* str);
	static bool isUtf8(const char* str);

	static int expectUtf8(const char* ch);
	static unsigned takeUtf8(const char* ch, int n);
};

/* ===========================================================================} */

/*
** {===========================================================================
** Base64
*/

/**
 * @brief Base64 utilities.
 */
class Base64 {
public:
	/**
	 * @param[out] val
	 */
	static bool toBytes(class Bytes* val, const std::string &str);
	/**
	 * @param[out] val
	 */
	static bool fromBytes(std::string &val, const class Bytes* buf);
};

/* ===========================================================================} */

/*
** {===========================================================================
** LZ4
*/

/**
 * @brief LZ4 utilities.
 */
class Lz4 {
public:
	/**
	 * @param[out] val
	 */
	static bool toBytes(class Bytes* val, const class Bytes* src);
	/**
	 * @param[out] val
	 */
	static bool fromBytes(class Bytes* val, const class Bytes* src);
};

/* ===========================================================================} */

#endif /* __ENCODING_H__ */
