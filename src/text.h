/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __TEXT_H__
#define __TEXT_H__

#include "bitty.h"
#include "mathematics.h"
#include "object.h"
#include <iostream>
#include <map>
#include <set>
#include <vector>

/*
** {===========================================================================
** Text
*/

/**
 * @brief Text object and utilities.
 */
class Text : public virtual Object {
public:
	typedef std::shared_ptr<Text> Ptr;

	typedef std::vector<std::string> Array;
	typedef std::map<std::string, std::string> Dictionary;
	typedef std::set<std::string> Set;

public:
	BITTY_CLASS_TYPE('T', 'E', 'X', 'T')

	/**
	 * @param[out] len
	 */
	virtual const char* text(size_t* len /* nullable */) const = 0;
	virtual void text(const char* txt /* nullable */, size_t len /* = 0 */) = 0;

	static void locale(const char* loc);

	/**
	 * @param[in, out] str
	 */
	static size_t toLowerCase(char* str, size_t len);
	/**
	 * @param[in, out] str
	 */
	static size_t toLowerCase(std::string &str);
	/**
	 * @param[in, out] str
	 */
	static size_t toUpperCase(char* str, size_t len);
	/**
	 * @param[in, out] str
	 */
	static size_t toUpperCase(std::string &str);

	static std::string toString(Int32 val, unsigned short width = 0, char fill = ' ', std::ios::fmtflags flags = std::ios::fmtflags(0), bool fixed = false);
	static std::string toString(UInt32 val, unsigned short width = 0, char fill = ' ', std::ios::fmtflags flags = std::ios::fmtflags(0), bool fixed = false);
	static std::string toString(Int64 val, unsigned short width = 0, char fill = ' ', std::ios::fmtflags flags = std::ios::fmtflags(0), bool fixed = false);
	static std::string toString(UInt64 val, unsigned short width = 0, char fill = ' ', std::ios::fmtflags flags = std::ios::fmtflags(0), bool fixed = false);
	static std::string toString(Single val, unsigned short precision = 6, unsigned short width = 0, char fill = ' ', std::ios::fmtflags flags = std::ios::fmtflags(0));
	static std::string toString(Double val, unsigned short precision = 6, unsigned short width = 0, char fill = ' ', std::ios::fmtflags flags = std::ios::fmtflags(0));
	static std::string toString(bool val, bool yesNo = false);
	static std::string toString(Int32 val, bool fixed);
	static std::string toString(UInt32 val, bool fixed);
	static std::string toString(Int64 val, bool fixed);
	static std::string toString(UInt64 val, bool fixed);
	/**
	 * @param[out] val
	 */
	static bool fromString(const std::string &str, bool &val);
	/**
	 * @param[out] val
	 */
	static bool fromString(const std::string &str, Int32 &val);
	/**
	 * @param[out] val
	 */
	static bool fromString(const std::string &str, UInt32 &val);
	/**
	 * @param[out] val
	 */
	static bool fromString(const std::string &str, Int64 &val);
	/**
	 * @param[out] val
	 */
	static bool fromString(const std::string &str, UInt64 &val);
	/**
	 * @param[out] val
	 */
	static bool fromString(const std::string &str, Single &val);
	/**
	 * @param[out] val
	 */
	static bool fromString(const std::string &str, Double &val);

	static std::string toHex(Int32 val, unsigned short width = sizeof(Int32) * 2, char fill = '0', bool toupper = false);
	static std::string toHex(UInt32 val, unsigned short width = sizeof(UInt32) * 2, char fill = '0', bool toupper = false);
	static std::string toHex(Int64 val, unsigned short width = sizeof(Int64) * 2, char fill = '0', bool toupper = false);
	static std::string toHex(UInt64 val, unsigned short width = sizeof(UInt64) * 2, char fill = '0', bool toupper = false);
	static std::string toHex(Int32 val, bool toupper);
	static std::string toHex(UInt32 val, bool toupper);
	static std::string toHex(Int64 val, bool toupper);
	static std::string toHex(UInt64 val, bool toupper);

	static std::string remove(const std::string &str, const std::string &charsToRemove);
	static std::string trim(const std::string &str, const std::string &delims = " \f\n\r\t\v");
	static std::string replace(const std::string &str, const std::string &from, const std::string &to, bool all = true);
	static Array split(const std::string &str, const std::string &delims, size_t maxSplits = 0);
	static Array tokenise(const std::string &str, const std::string &singleDelims, const std::string &doubleDelims, size_t maxSplits = 0);
	/**
	 * @param[out] txt
	 */
	static bool postfix(const std::string &str, std::string &txt, int &num);

	static size_t indexOf(const std::string &str, char what, size_t start = 0);
	static size_t indexOf(const std::string &str, const std::string &what, size_t start = 0);
	static size_t lastIndexOf(const std::string &str, char what, size_t start = std::string::npos);
	static size_t lastIndexOf(const std::string &str, const std::string &what, size_t start = std::string::npos);

	static bool startsWith(const std::string &str, const std::string &what, bool caseInsensitive);
	static bool endsWith(const std::string &str, const std::string &what, bool caseInsensitive);
	static bool matchWildcard(const std::string &str, const char* wildcard, bool caseInsensitive);
	static size_t count(const std::string &str, const char what);

	static const char* styleOf(const std::string &str);
	/**
	 * @param[in, out] str
	 */
	static void stylish(std::string &str, const char* dst, const char* src = nullptr);

	static std::string cformat(const char* fmt, ...);

	/**
	 * @param[out] endptr
	 */
	static long strtol(char const* str, char** endptr, int base);
	/**
	 * @param[out] endptr
	 */
	static long long strtoll(char const* str, char** endptr, int base);
	/**
	 * @param[out] endptr
	 */
	static unsigned long long strtoull(char const* str, char** endptr, int base);
	/**
	 * @param[out] endptr
	 */
	static double strtod(const char* str, char** endptr);

	static Text* create(void);
	static void destroy(Text* ptr);
};

/* ===========================================================================} */

#endif /* __TEXT_H__ */
