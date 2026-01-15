/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "color.h"
#include "text.h"

/*
** {===========================================================================
** Color
*/

Color::Color() {
}

Color::Color(Byte r_, Byte g_, Byte b_) : r(r_), g(g_), b(b_), a(255) {
}

Color::Color(Byte r_, Byte g_, Byte b_, Byte a_) : r(r_), g(g_), b(b_), a(a_) {
}

Color::Color(const Color &other) {
	r = other.r;
	g = other.g;
	b = other.b;
	a = other.a;
}

Color &Color::operator = (const Color &other) {
	r = other.r;
	g = other.g;
	b = other.b;
	a = other.a;

	return *this;
}

Color Color::operator - (void) const {
	return Color(
		(Byte)Math::clamp(255 - r, 0, 255),
		(Byte)Math::clamp(255 - g, 0, 255),
		(Byte)Math::clamp(255 - b, 0, 255),
		a
	);
}

Color Color::operator + (const Color &other) const {
	return Color(
		(Byte)Math::clamp(r + other.r, 0, 255),
		(Byte)Math::clamp(g + other.g, 0, 255),
		(Byte)Math::clamp(b + other.b, 0, 255),
		(Byte)Math::clamp(a + other.a, 0, 255)
	);
}

Color Color::operator - (const Color &other) const {
	return Color(
		(Byte)Math::clamp(r - other.r, 0, 255),
		(Byte)Math::clamp(g - other.g, 0, 255),
		(Byte)Math::clamp(b - other.b, 0, 255),
		(Byte)Math::clamp(a - other.a, 0, 255)
	);
}

Color Color::operator * (const Color &other) const {
	return Color(
		(Byte)Math::clamp(r * (other.r / 255.0f), 0.0f, 255.0f),
		(Byte)Math::clamp(g * (other.g / 255.0f), 0.0f, 255.0f),
		(Byte)Math::clamp(b * (other.b / 255.0f), 0.0f, 255.0f),
		(Byte)Math::clamp(a * (other.a / 255.0f), 0.0f, 255.0f)
	);
}

Color Color::operator * (Real other) const {
	return Color(
		(Byte)Math::clamp(r * other, (Real)0.0f, (Real)255.0f),
		(Byte)Math::clamp(g * other, (Real)0.0f, (Real)255.0f),
		(Byte)Math::clamp(b * other, (Real)0.0f, (Real)255.0f),
		(Byte)Math::clamp(a * other, (Real)0.0f, (Real)255.0f)
	);
}

Color &Color::operator += (const Color &other) {
	r = (Byte)Math::clamp(r + other.r, 0, 255);
	g = (Byte)Math::clamp(g + other.g, 0, 255);
	b = (Byte)Math::clamp(b + other.b, 0, 255);
	a = (Byte)Math::clamp(a + other.a, 0, 255);

	return *this;
}

Color &Color::operator -= (const Color &other) {
	r = (Byte)Math::clamp(r - other.r, 0, 255);
	g = (Byte)Math::clamp(g - other.g, 0, 255);
	b = (Byte)Math::clamp(b - other.b, 0, 255);
	a = (Byte)Math::clamp(a - other.a, 0, 255);

	return *this;
}

Color &Color::operator *= (const Color &other) {
	r = (Byte)Math::clamp(r * (other.r / 255.0f), 0.0f, 255.0f);
	g = (Byte)Math::clamp(g * (other.g / 255.0f), 0.0f, 255.0f);
	b = (Byte)Math::clamp(b * (other.b / 255.0f), 0.0f, 255.0f);
	a = (Byte)Math::clamp(a * (other.a / 255.0f), 0.0f, 255.0f);

	return *this;
}

Color &Color::operator *= (Real other) {
	r = (Byte)Math::clamp(r * other, (Real)0.0f, (Real)255.0f);
	g = (Byte)Math::clamp(g * other, (Real)0.0f, (Real)255.0f);
	b = (Byte)Math::clamp(b * other, (Real)0.0f, (Real)255.0f);
	a = (Byte)Math::clamp(a * other, (Real)0.0f, (Real)255.0f);

	return *this;
}

bool Color::operator == (const Color &other) const {
	return r == other.r && g == other.g && b == other.b && a == other.a;
}

bool Color::operator != (const Color &other) const {
	return r != other.r || g != other.g || b != other.b || a != other.a;
}

UInt32 Color::toRGBA(void) const {
	return (r << 0) | (g << 8) | (b << 16) | (a << 24);
}

UInt32 Color::toARGB(void) const {
	return (a << 0) | (r << 8) | (g << 16) | (b << 24);
}

void Color::fromRGBA(UInt32 rgba) {
	r = (Byte)(rgba & 0x000000ff);
	g = (Byte)((rgba & 0x0000ff00) >> 8);
	b = (Byte)((rgba & 0x00ff0000) >> 16);
	a = (Byte)((rgba & 0xff000000) >> 24);
}

void Color::fromARGB(UInt32 argb) {
	a = (Byte)(argb & 0x000000ff);
	r = (Byte)((argb & 0x0000ff00) >> 8);
	g = (Byte)((argb & 0x00ff0000) >> 16);
	b = (Byte)((argb & 0xff000000) >> 24);
}

std::string Color::toString(void) const {
	std::string ret = "#";
	ret += Text::toHex(r, 2, '0', false);
	ret += Text::toHex(g, 2, '0', false);
	ret += Text::toHex(b, 2, '0', false);
	ret += Text::toHex(a, 2, '0', false);

	return ret;
}

bool Color::fromString(const std::string &str) {
	std::string hex = str;
	if (hex[0] != '#')
		return false;

	hex = hex.substr(1);

	if (hex.length() == 3 || hex.length() == 4) {
		std::string fullHex;
		for (char c : hex) {
			fullHex += c;
			fullHex += c;
		}
		hex = fullHex;
	}

	if (hex.length() != 6 && hex.length() != 8)
		return false;

	UInt8 b0 = 0;
	UInt8 b1 = 0;
	UInt8 b2 = 0;
	UInt8 b3 = 0;
	UInt8 b4 = 0;
	UInt8 b5 = 0;
	UInt8 b6 = 0;
	UInt8 b7 = 0;
	if (
		!Text::fromHexCharacter(hex[0], b0) ||
		!Text::fromHexCharacter(hex[1], b1) ||
		!Text::fromHexCharacter(hex[2], b2) ||
		!Text::fromHexCharacter(hex[3], b3) ||
		!Text::fromHexCharacter(hex[4], b4) ||
		!Text::fromHexCharacter(hex[5], b5) ||
		!Text::fromHexCharacter(hex[6], b6) ||
		!Text::fromHexCharacter(hex[7], b7)
	) {
		return false;
	}

	r = b0 * 16 + b1;
	g = b2 * 16 + b3;
	b = b4 * 16 + b5;
	a = (hex.length() == 8) ?
		b6 * 16 + b7 : 255;

	return true;
}

/* ===========================================================================} */
