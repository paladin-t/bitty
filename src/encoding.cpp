/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2024 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "bytes.h"
#include "encoding.h"
#include "../lib/b64/b64.h"
#include "../lib/lz4/lib/lz4.h"
#if ENCODING_STRING_CONVERTER == ENCODING_STRING_CONVERTER_WINAPI
#	include <Windows.h>
#elif ENCODING_STRING_CONVERTER == ENCODING_STRING_CONVERTER_CUSTOM
	// Do nothing.
#elif ENCODING_STRING_CONVERTER == ENCODING_STRING_CONVERTER_CODECVT
#	include <codecvt> // Deprecated in C++17, use `ENCODING_STRING_CONVERTER_CUSTOM`
	                  // instead if it's not available.
#	include <locale>
#endif /* ENCODING_STRING_CONVERTER */

/*
** {===========================================================================
** Utilities
*/

#if ENCODING_STRING_CONVERTER == ENCODING_STRING_CONVERTER_WINAPI
static int encodingBytesToWchar(const char* sz, wchar_t** out, size_t size) {
	int result = ::MultiByteToWideChar(CP_UTF8, 0, sz, -1, 0, 0);
	if ((int)size < result)
		*out = new wchar_t[result];
	::MultiByteToWideChar(CP_UTF8, 0, sz, -1, *out, result);

	return result;
}

static int encodingWcharToBytesAnsi(const wchar_t* sz, char** out, size_t size) {
	int result = ::WideCharToMultiByte(CP_ACP, 0, sz, -1, 0, 0, 0, 0);
	if ((int)size < result)
		*out = new char[result];
	::WideCharToMultiByte(CP_ACP, 0, sz, -1, *out, result, 0, 0);

	return result;
}

static int encodingBytesAnsiToWchar(const char* sz, wchar_t** out, size_t size) {
	int result = ::MultiByteToWideChar(CP_ACP, 0, sz, -1, 0, 0);
	if ((int)size < result)
		*out = new wchar_t[result];
	::MultiByteToWideChar(CP_ACP, 0, sz, -1, *out, result);

	return result;
}

static int encodingWcharToBytes(const wchar_t* sz, char** out, size_t size) {
	int result = ::WideCharToMultiByte(CP_UTF8, 0, sz, -1, 0, 0, 0, 0);
	if ((int)size < result)
		*out = new char[result];
	::WideCharToMultiByte(CP_UTF8, 0, sz, -1, *out, result, 0, 0);

	return result;
}
#endif /* ENCODING_STRING_CONVERTER */

#if ENCODING_STRING_CONVERTER == ENCODING_STRING_CONVERTER_CUSTOM
static int encodingCharToUtf8(char* buf, int buf_size, unsigned int c) {
	if (c < 0x80) {
		buf[0] = (char)c;

		return 1;
	}
	if (c < 0x800) {
		if (buf_size < 2)
			return 0;
		buf[0] = (char)(0xc0 + (c >> 6));
		buf[1] = (char)(0x80 + (c & 0x3f));

		return 2;
	}
	if (c >= 0xdc00 && c < 0xe000) {
		return 0;
	}
	if (c >= 0xd800 && c < 0xdc00) {
		if (buf_size < 4)
			return 0;
		buf[0] = (char)(0xf0 + (c >> 18));
		buf[1] = (char)(0x80 + ((c >> 12) & 0x3f));
		buf[2] = (char)(0x80 + ((c >> 6) & 0x3f));
		buf[3] = (char)(0x80 + ((c) & 0x3f));

		return 4;
	} /*else if (c < 0x10000)*/ {
		if (buf_size < 3)
			return 0;
		buf[0] = (char)(0xe0 + (c >> 12));
		buf[1] = (char)(0x80 + ((c >> 6) & 0x3f));
		buf[2] = (char)(0x80 + ((c) & 0x3f));

		return 3;
	}
}

static int encodingCharFromUtf8(unsigned int* out_char, const char* in_text, const char* in_text_end) {
	unsigned int c = (unsigned int)-1;
	const unsigned char* str = (const unsigned char*)in_text;
	if (!(*str & 0x80)) {
		c = (unsigned int)(*str++);
		*out_char = c;

		return 1;
	}
	if ((*str & 0xe0) == 0xc0) {
		*out_char = 0xfffd; // Will be invalid but not end of string.
		if (in_text_end && in_text_end - (const char*)str < 2)
			return 1;
		if (*str < 0xc2)
			return 2;
		c = (unsigned int)((*str++ & 0x1f) << 6);
		if ((*str & 0xc0) != 0x80)
			return 2;
		c += (*str++ & 0x3f);
		*out_char = c;

		return 2;
	}
	if ((*str & 0xf0) == 0xe0) {
		*out_char = 0xfffd; // Will be invalid but not end of string.
		if (in_text_end && in_text_end - (const char*)str < 3)
			return 1;
		if (*str == 0xe0 && (str[1] < 0xa0 || str[1] > 0xbf))
			return 3;
		if (*str == 0xed && str[1] > 0x9f) // str[1] < 0x80 is checked below.
			return 3;
		c = (unsigned int)((*str++ & 0x0f) << 12);
		if ((*str & 0xc0) != 0x80)
			return 3;
		c += (unsigned int)((*str++ & 0x3f) << 6);
		if ((*str & 0xc0) != 0x80)
			return 3;
		c += (*str++ & 0x3f);
		*out_char = c;

		return 3;
	}
	if ((*str & 0xf8) == 0xf0) {
		*out_char = 0xfffd; // Will be invalid but not end of string.
		if (in_text_end && in_text_end - (const char*)str < 4)
			return 1;
		if (*str > 0xf4)
			return 4;
		if (*str == 0xf0 && (str[1] < 0x90 || str[1] > 0xbf))
			return 4;
		if (*str == 0xf4 && str[1] > 0x8f) // str[1] < 0x80 is checked below.
			return 4;
		c = (unsigned int)((*str++ & 0x07) << 18);
		if ((*str & 0xc0) != 0x80)
			return 4;
		c += (unsigned int)((*str++ & 0x3f) << 12);
		if ((*str & 0xc0) != 0x80)
			return 4;
		c += (unsigned int)((*str++ & 0x3f) << 6);
		if ((*str & 0xc0) != 0x80)
			return 4;
		c += (*str++ & 0x3f);
		// UTF8 encodings of values used in surrogate pairs are invalid.
		if ((c & 0xfffff800) == 0xd800)
			return 4;
		*out_char = c;

		return 4;
	}
	*out_char = 0;

	return 0;
}

static int encodingStrToUtf8(char* buf, int buf_size, const wchar_t* in_text, const wchar_t* in_text_end) {
	char* buf_out = buf;
	const char* buf_end = buf + buf_size;
	while (buf_out < buf_end - 1 && (!in_text_end || in_text < in_text_end) && *in_text) {
		unsigned int c = (unsigned int)(*in_text++);
		if (c < 0x80)
			*buf_out++ = (char)c;
		else
			buf_out += encodingCharToUtf8(buf_out, (int)(buf_end - buf_out - 1), c);
	}
	*buf_out = 0;

	return (int)(buf_out - buf);
}

static int encodingStrFromUtf8(wchar_t* buf, int buf_size, const char* in_text, const char* in_text_end, const char** in_text_remaining) {
	wchar_t* buf_out = buf;
	wchar_t* buf_end = buf + buf_size;
	while (buf_out < buf_end - 1 && (!in_text_end || in_text < in_text_end) && *in_text) {
		unsigned int c;
		in_text += encodingCharFromUtf8(&c, in_text, in_text_end);
		if (c == 0)
			break;
		if (c < 0x10000) // FIXME: losing characters that don't fit in 2 bytes.
			*buf_out++ = (wchar_t)c;
	}
	*buf_out = 0;
	if (in_text_remaining)
		*in_text_remaining = in_text;

	return (int)(buf_out - buf);
}
#endif /* ENCODING_STRING_CONVERTER */

/* ===========================================================================} */

/*
** {===========================================================================
** Unicode
*/

std::string Unicode::fromOs(const char* str) {
#if defined BITTY_OS_WIN
	std::string result;
	char strb[16];
	char* strp = strb;
	wchar_t wstr[16];
	wchar_t* wstrp = wstr;
	encodingBytesAnsiToWchar(str, &wstrp, BITTY_COUNTOF(wstr));
	encodingWcharToBytes(wstrp, &strp, BITTY_COUNTOF(strb));
	result = strp;
	if (wstrp != wstr)
		delete wstrp;
	if (strp != strb)
		delete strp;

	return result;
#else /* BITTY_OS_WIN */
	return str;
#endif /* BITTY_OS_WIN */
}

std::string Unicode::fromOs(const std::string &str) {
	return fromOs(str.c_str());
}

std::string Unicode::toOs(const char* str) {
#if defined BITTY_OS_WIN
	std::string result;
	char strb[16];
	char* strp = strb;
	wchar_t wstr[16];
	wchar_t* wstrp = wstr;
	encodingBytesToWchar(str, &wstrp, BITTY_COUNTOF(wstr));
	encodingWcharToBytesAnsi(wstrp, &strp, BITTY_COUNTOF(strb));
	result = strp;
	if (wstrp != wstr)
		delete wstrp;
	if (strp != strb)
		delete strp;

	return result;
#else /* BITTY_OS_WIN */
	return str;
#endif /* BITTY_OS_WIN */
}

std::string Unicode::toOs(const std::string &str) {
	return toOs(str.c_str());
}

std::string Unicode::fromWide(const wchar_t* str) {
	if (!str || !(*str))
		return std::string();

#if ENCODING_STRING_CONVERTER == ENCODING_STRING_CONVERTER_WINAPI
	std::string result;
	char strb[16];
	char* strp = strb;
	encodingWcharToBytes(str, &strp, BITTY_COUNTOF(strb));
	result = strp;
	if (strp != strb)
		delete strp;

	return result;
#elif ENCODING_STRING_CONVERTER == ENCODING_STRING_CONVERTER_CUSTOM
	std::wstring src = str;
	const int l = (int)(src.length() * 4);
	char* strp = new char [l];
	memset(strp, 0, l);
	encodingStrToUtf8(strp, l, src.c_str(), nullptr);
	std::string result = strp;
	delete [] strp;

	return result;
#elif ENCODING_STRING_CONVERTER == ENCODING_STRING_CONVERTER_CODECVT
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convert;
	std::string result = convert.to_bytes(str);

	return result;
#endif /* ENCODING_STRING_CONVERTER */
}

std::string Unicode::fromWide(const std::wstring &str) {
	return fromWide(str.c_str());
}

std::wstring Unicode::toWide(const char* str) {
	if (!str || !(*str))
		return std::wstring();

#if ENCODING_STRING_CONVERTER == ENCODING_STRING_CONVERTER_WINAPI
	std::wstring result;
	wchar_t wstr[16];
	wchar_t* wstrp = wstr;
	encodingBytesToWchar(str, &wstrp, BITTY_COUNTOF(wstr));
	result = wstrp;
	if (wstrp != wstr)
		delete wstrp;

	return result;
#elif ENCODING_STRING_CONVERTER == ENCODING_STRING_CONVERTER_CUSTOM
	std::string src = str;
	const int l = (int)(src.length() * 4);
	wchar_t* strp = new wchar_t [l];
	memset(strp, 0, l);
	encodingStrFromUtf8(strp, l, src.c_str(), nullptr, nullptr);
	std::wstring result = strp;
	delete [] strp;

	return result;
#elif ENCODING_STRING_CONVERTER == ENCODING_STRING_CONVERTER_CODECVT
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convert;
	std::wstring result = convert.from_bytes(str);

	return result;
#endif /* ENCODING_STRING_CONVERTER */
}

std::wstring Unicode::toWide(const std::string &str) {
	return toWide(str.c_str());
}

bool Unicode::isAscii(const char* str) {
	if (!str || !*str)
		return true;

	while (*str) {
		if (!isprint(*str))
			return false;

		++str;
	}

	return true;
}

bool Unicode::isUtf8(const char* str) {
	if (!str || !*str)
		return false;

	while (*str) {
		int n = expectUtf8(str);
		if (!n)
			break;
		if (n > 1)
			return true;

		str += n;
	}

	return false;
}

int Unicode::expectUtf8(const char* ch) {
#define _TAKE(__ch, __c, __r) do { __c = *__ch++; __r++; } while (0)
#define _COPY(__ch, __c, __r, __cp) do { _TAKE(__ch, __c, __r); __cp = (__cp << 6) | ((unsigned char)__c & 0x3fu); } while (0)
#define _TRANS(__m, __cp, __g) do { __cp &= ((__g[(unsigned char)c] & __m) != 0); } while (0)
#define _TAIL(__ch, __c, __r, __cp, __g) do { _COPY(__ch, __c, __r, __cp); _TRANS(0x70, __cp, __g); } while (0)

	static constexpr const unsigned char RANGE[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
		0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
		0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
		0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
		8, 8, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		10, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 3, 3, 11, 6, 6, 6, 5, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
	};

	int result = 0;
	unsigned codepoint = 0;
	unsigned char type = 0;
	char c = 0;

	if (!ch)
		return 0;

	_TAKE(ch, c, result);
	if (!(c & 0x80)) {
		codepoint = (unsigned char)c;

		return 1;
	}

	type = RANGE[(unsigned char)c];
	codepoint = (0xff >> type) & (unsigned char)c;

	switch (type) {
	case 2: _TAIL(ch, c, result, codepoint, RANGE); return result;
	case 3: _TAIL(ch, c, result, codepoint, RANGE); _TAIL(ch, c, result, codepoint, RANGE); return result;
	case 4: _COPY(ch, c, result, codepoint); _TRANS(0x50, codepoint, RANGE); _TAIL(ch, c, result, codepoint, RANGE); return result;
	case 5: _COPY(ch, c, result, codepoint); _TRANS(0x10, codepoint, RANGE); _TAIL(ch, c, result, codepoint, RANGE); _TAIL(ch, c, result, codepoint, RANGE); return result;
	case 6: _TAIL(ch, c, result, codepoint, RANGE); _TAIL(ch, c, result, codepoint, RANGE); _TAIL(ch, c, result, codepoint, RANGE); return result;
	case 10: _COPY(ch, c, result, codepoint); _TRANS(0x20, codepoint, RANGE); _TAIL(ch, c, result, codepoint, RANGE); return result;
	case 11: _COPY(ch, c, result, codepoint); _TRANS(0x60, codepoint, RANGE); _TAIL(ch, c, result, codepoint, RANGE); _TAIL(ch, c, result, codepoint, RANGE); return result;
	default: return 0;
	}

#undef _TAKE
#undef _COPY
#undef _TRANS
#undef _TAIL
}

unsigned Unicode::takeUtf8(const char* ch, int n) {
	union { unsigned ui; char ch[4]; } u;
	u.ui = 0;
	for (int i = 0; i < n; ++i)
		u.ch[i] = ch[i];
	for (int i = n; i < 4; ++i)
		u.ch[i] = '\0';

	return u.ui;
}

/* ===========================================================================} */

/*
** {===========================================================================
** Base64
*/

#ifndef b64_free
	// Use this function instead of `b64_realloc`, due to the behaviour
	// underneath the `b64_realloc` (`realloc`) is implementation dependent in
	// the C99/C++11 standards.
#	define b64_free(ptr) free(ptr)
#endif /* b64_free */

bool Base64::toBytes(class Bytes* val, const std::string &str) {
	size_t len = 0;
	Byte* tmp = b64_decode_ex(str.c_str(), str.length(), &len);
	if (!tmp)
		return false;

	val->writeBytes(tmp, len);
	b64_free((void*)tmp);

	return true;
}

bool Base64::fromBytes(std::string &val, const class Bytes* buf) {
	val.clear();

	char* tmp = b64_encode(buf->pointer(), buf->count());
	if (!tmp)
		return false;

	val = tmp;
	b64_free((void*)tmp);

	return true;
}

/* ===========================================================================} */

/*
** {===========================================================================
** LZ4
*/

bool Lz4::toBytes(class Bytes* val, const class Bytes* src) {
	if (!val || !src)
		return false;

	val->clear();
	if (src->empty())
		return true;

	constexpr const int ONEK = 1024;

	int n = (int)src->count();
	if (n <= 8 * ONEK)
		n *= 8;
	else if (n <= 16 * ONEK)
		n *= 4;
	else if (n <= 32 * ONEK)
		n *= 2;
	else if (n <= 64 * ONEK)
		n = 64 * ONEK;
	val->resize((size_t)n);
	int ret = 0;
	do {
		ret = LZ4_decompress_safe(
			(const char*)src->pointer(), (char*)val->pointer(),
			(int)src->count(), (int)val->count()
		);
		if (ret < 0) {
			n *= 2;
			val->resize((size_t)n);
		}
	} while (ret < 0);
	if (!ret)
		return false;

	val->resize((size_t)ret);

	return true;
}

bool Lz4::fromBytes(class Bytes* val, const class Bytes* src) {
	if (!val || !src)
		return false;

	val->clear();
	if (src->empty())
		return true;

	int n = LZ4_compressBound((int)src->count());
	val->resize((size_t)n);
	n = LZ4_compress_default(
		(const char*)src->pointer(), (char*)val->pointer(),
		(int)src->count(), (int)val->count()
	);
	if (!n)
		return false;

	val->resize((size_t)n);

	return true;
}

/* ===========================================================================} */
