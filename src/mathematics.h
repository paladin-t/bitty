/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __MATHEMATICS_H__
#define __MATHEMATICS_H__

#include "bitty.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <list>
#include <stdint.h>

/*
** {===========================================================================
** Numeric types
*/

typedef unsigned char Byte;

typedef int Int;

typedef double Real;

typedef int8_t Int8;

typedef uint8_t UInt8;

typedef int16_t Int16;

typedef uint16_t UInt16;

typedef int32_t Int32;

typedef uint32_t UInt32;

typedef int64_t Int64;

typedef uint64_t UInt64;

typedef float Single;

typedef double Double;

/* ===========================================================================} */

/*
** {===========================================================================
** Simple structures
*/

namespace Math {

template<typename T, typename R = Real> struct Vec2 {
	typedef std::list<Vec2> List;

	typedef T ValueType;

	T x = 0;
	T y = 0;

	Vec2() {
	}
	Vec2(T x_, T y_) : x(x_), y(y_) {
	}
	Vec2(const Vec2 &other) {
		x = other.x;
		y = other.y;
	}

	Vec2 &operator = (const Vec2 &other) {
		x = other.x;
		y = other.y;

		return *this;
	}
	Vec2 operator - (void) const {
		return Vec2(-x, -y);
	}
	Vec2 operator + (const Vec2 &other) const {
		return Vec2(x + other.x, y + other.y);
	}
	Vec2 operator - (const Vec2 &other) const {
		return Vec2(x - other.x, y - other.y);
	}
	Vec2 operator * (const Vec2 &other) const {
		return Vec2(x * other.x, y * other.y);
	}
	Vec2 operator * (T other) const {
		return Vec2(x * other, y * other);
	}
	Vec2 &operator += (const Vec2 &other) {
		x += other.x;
		y += other.y;

		return *this;
	}
	Vec2 &operator -= (const Vec2 &other) {
		x -= other.x;
		y -= other.y;

		return *this;
	}
	Vec2 &operator *= (const Vec2 &other) {
		x *= other.x;
		y *= other.y;

		return *this;
	}
	Vec2 &operator *= (T other) {
		x *= other;
		y *= other;

		return *this;
	}
	bool operator == (const Vec2 &other) const {
		return equals(other);
	}
	bool operator != (const Vec2 &other) const {
		return !equals(other);
	}
	bool operator < (const Vec2 &other) const {
		return compare(other) < 0;
	}

	int compare(const Vec2 &other) const {
		if (y < other.y)
			return -1;
		else if (y > other.y)
			return 1;

		if (x < other.x)
			return -1;
		else if (x > other.x)
			return 1;

		return 0;
	}
	bool equals(const Vec2 &other) const {
		return x == other.x && y == other.y;
	}

	R normalize(void) {
		const R length = std::sqrt((R)(x * x + y * y));
		const R invLength = 1.0 / length;

		x = (T)(x * invLength);
		y = (T)(y * invLength);

		return length;
	}
	Vec2 normalized(void) const {
		const R length = std::sqrt((R)(x * x + y * y));
		const R invLength = 1 / length;

		return Vec2((T)(x * invLength), (T)(y * invLength));
	}

	R length(void) const {
		return std::sqrt((R)(x * x + y * y));
	}
	R lengthSquared(void) const {
		return (R)(x * x + y * y);
	}

	T distanceTo(const Vec2 &other) const {
		return std::sqrt((x - other.x) * (x - other.x) + (y - other.y) * (y - other.y));
	}
	T squaredDistanceTo(const Vec2 &other) const {
		return (x - other.x) * (x - other.x) + (y - other.y) * (y - other.y);
	}
	T hamiltonDistanceTo(const Vec2 &other) const {
		return std::abs(x - other.x) + std::abs(y - other.y);
	}

	R dot(const Vec2 &other) const {
		return (R)(x * other.x + y * other.y);
	}
	Vec2 cross(R other) const {
		return Vec2((T)(x * other), (T)(x * -other));
	}
	R cross(const Vec2 &other) const {
		return (R)(x * other.y - y * other.x);
	}

	R angle(void) const {
		return std::atan2((R)y, (R)x);
	}
	R angleTo(const Vec2 &other) const {
		return std::atan2((R)other.y, (R)other.x) - std::atan2((R)y, (R)x);
	}

	Vec2 rotated(R angle) const {
		const R len = std::sqrt(x * x + y * y);
		R ang = std::atan2(y, x);
		ang += angle;

		return Vec2((T)(std::cos(ang) * len), (T)(std::sin(ang) * len));
	}
	Vec2 rotated(R angle, const Vec2 &pivot) const {
		const Vec2 diff(x - pivot.x, y - pivot.y);
		const R len = std::sqrt(diff.x * diff.x + diff.y * diff.y);
		R ang = std::atan2(diff.y, diff.x);
		ang += angle;

		return Vec2((T)(pivot.x + std::cos(ang) * len), (T)(pivot.y + std::sin(ang) * len));
	}
};

template<typename T, typename R = Real> struct Vec3 {
	typedef T ValueType;

	T x = 0;
	T y = 0;
	T z = 0;

	Vec3() {
	}
	Vec3(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {
	}
	Vec3(const Vec3 &other) {
		x = other.x;
		y = other.y;
		z = other.z;
	}

	Vec3 &operator = (const Vec3 &other) {
		x = other.x;
		y = other.y;
		z = other.z;

		return *this;
	}
	Vec3 operator - (void) const {
		return Vec3(-x, -y, -z);
	}
	Vec3 operator + (const Vec3 &other) const {
		return Vec3(x + other.x, y + other.y, z + other.z);
	}
	Vec3 operator - (const Vec3 &other) const {
		return Vec3(x - other.x, y - other.y, z - other.z);
	}
	Vec3 operator * (const Vec3 &other) const {
		return Vec3(x * other.x, y * other.y, z * other.z);
	}
	Vec3 operator * (T other) const {
		return Vec3(x * other, y * other, z * other);
	}
	Vec3 &operator += (const Vec3 &other) {
		x += other.x;
		y += other.y;
		z += other.z;

		return *this;
	}
	Vec3 &operator -= (const Vec3 &other) {
		x -= other.x;
		y -= other.y;
		z -= other.z;

		return *this;
	}
	Vec3 &operator *= (const Vec3 &other) {
		x *= other.x;
		y *= other.y;
		z *= other.z;

		return *this;
	}
	Vec3 &operator *= (T other) {
		x *= other;
		y *= other;
		z *= other;

		return *this;
	}
	bool operator == (const Vec3 &other) const {
		return equals(other);
	}
	bool operator != (const Vec3 &other) const {
		return !equals(other);
	}
	bool operator < (const Vec3 &other) const {
		return compare(other) < 0;
	}

	int compare(const Vec3 &other) const {
		if (z < other.z)
			return -1;
		else if (z > other.z)
			return 1;

		if (y < other.y)
			return -1;
		else if (y > other.y)
			return 1;

		if (x < other.x)
			return -1;
		else if (x > other.x)
			return 1;

		return 0;
	}
	bool equals(const Vec3 &other) const {
		return x == other.x && y == other.y && z == other.z;
	}

	R normalize(void) {
		const R length = std::sqrt((R)(x * x + y * y + z * z));
		const R invLength = 1.0 / length;

		x = (T)(x * invLength);
		y = (T)(y * invLength);
		z = (T)(z * invLength);

		return length;
	}
	Vec3 normalized(void) const {
		const R length = std::sqrt((R)(x * x + y * y + z * z));
		const R invLength = 1.0 / length;

		return Vec3((T)(x * invLength), (T)(y * invLength), (T)(z * invLength));
	}

	R length(void) const {
		return std::sqrt((R)(x * x + y * y + z * z));
	}
	R lengthSquared(void) const {
		return (R)(x * x + y * y + z * z);
	}

	R dot(const Vec3 &other) const {
		return (R)(x * other.x + y * other.y + z * other.z);
	}
	Vec3 cross(const Vec3 &other) const {
		return Vec3(
			(T)(y * other.z - z * other.y),
			(T)(z * x - x * other.z),
			(T)(x * other.y - y * x)
		);
	}
};

template<typename T> struct Vec4 {
	typedef T ValueType;

	T x = 0;
	T y = 0;
	T z = 0;
	T w = 0;

	Vec4() {
	}
	Vec4(T x_, T y_, T z_, T w_) : x(x_), y(y_), z(z_), w(w_) {
	}
	Vec4(const Vec4 &other) {
		x = other.x;
		y = other.y;
		z = other.z;
		w = other.w;
	}

	Vec4 &operator = (const Vec4 &other) {
		x = other.x;
		y = other.y;
		z = other.z;
		w = other.w;

		return *this;
	}
	Vec4 operator - (void) const {
		return Vec4(-x, -y, -z, -w);
	}
	Vec4 operator + (const Vec4 &other) const {
		return Vec4(x + other.x, y + other.y, z + other.z, w + other.w);
	}
	Vec4 operator - (const Vec4 &other) const {
		return Vec4(x - other.x, y - other.y, z - other.z, w - other.w);
	}
	Vec4 operator * (const Vec4 &other) const {
		return Vec4(x * other.x, y * other.y, z * other.z, w * other.w);
	}
	Vec4 operator * (T other) const {
		return Vec4(x * other, y * other, z * other, w * other);
	}
	Vec4 &operator += (const Vec4 &other) {
		x += other.x;
		y += other.y;
		z += other.z;
		w += other.w;

		return *this;
	}
	Vec4 &operator -= (const Vec4 &other) {
		x -= other.x;
		y -= other.y;
		z -= other.z;
		w -= other.w;

		return *this;
	}
	Vec4 &operator *= (const Vec4 &other) {
		x *= other.x;
		y *= other.y;
		z *= other.z;
		w *= other.w;

		return *this;
	}
	Vec4 &operator *= (T other) {
		x *= other;
		y *= other;
		z *= other;
		w *= other;

		return *this;
	}
	bool operator == (const Vec4 &other) const {
		return equals(other);
	}
	bool operator != (const Vec4 &other) const {
		return !equals(other);
	}
	bool operator < (const Vec4 &other) const {
		return compare(other) < 0;
	}

	int compare(const Vec4 &other) const {
		if (w < other.w)
			return -1;
		else if (w > other.w)
			return 1;

		if (z < other.z)
			return -1;
		else if (z > other.z)
			return 1;

		if (y < other.y)
			return -1;
		else if (y > other.y)
			return 1;

		if (x < other.x)
			return -1;
		else if (x > other.x)
			return 1;

		return 0;
	}
	bool equals(const Vec4 &other) const {
		return x == other.x && y == other.y && z == other.z && w == other.w;
	}
};

template<typename T, int S> struct Rect {
	typedef T ValueType;

	T x0 = 0;
	T y0 = 0;
	T x1 = 0;
	T y1 = 0;

	Rect() {
	}
	Rect(T x0_, T y0_, T x1_, T y1_) : x0(x0_), y0(y0_), x1(x1_), y1(y1_) {
	}
	Rect(const Rect &other) {
		x0 = other.x0;
		y0 = other.y0;
		x1 = other.x1;
		y1 = other.y1;
	}

	Rect<ValueType, S> operator + (const Vec2<T> &other) const {
		Rect<ValueType, S> result;
		result.x0 = std::min(x0, x1);
		result.y0 = std::min(y0, y1);
		result.x1 = std::max(x0, x1);
		result.y1 = std::max(y0, y1);
		if (other.x < result.x0)
			result.x0 = other.x;
		if (other.x > result.x1)
			result.x1 = other.x;
		if (other.y < result.y0)
			result.y0 = other.y;
		if (other.y > result.y1)
			result.y1 = other.y;

		return result;
	}

	Rect &operator = (const Rect &other) {
		x0 = other.x0;
		y0 = other.y0;
		x1 = other.x1;
		y1 = other.y1;

		return *this;
	}
	bool operator == (const Rect &other) const {
		return equals(other);
	}
	bool operator != (const Rect &other) const {
		return !equals(other);
	}
	bool operator < (const Rect &other) const {
		return compare(other) < 0;
	}

	int compare(const Rect &other) const {
		if (y1 < other.y1)
			return -1;
		else if (y1 > other.y1)
			return 1;

		if (x1 < other.x1)
			return -1;
		else if (x1 > other.x1)
			return 1;

		if (y0 < other.y0)
			return -1;
		else if (y0 > other.y0)
			return 1;

		if (x0 < other.x0)
			return -1;
		else if (x0 > other.x0)
			return 1;

		return 0;
	}
	bool equals(const Rect &other) const {
		return x0 == other.x0 && y0 == other.y0 && x1 == other.x1 && y1 == other.y1;
	}

	T xMin(void) const {
		return std::min(x0, x1);
	}
	T yMin(void) const {
		return std::min(y0, y1);
	}
	T xMax(void) const {
		return std::max(x0, x1);
	}
	T yMax(void) const {
		return std::max(y0, y1);
	}

	T width(void) const {
		return std::abs(x1 - x0) + S;
	}
	T height(void) const {
		return std::abs(y1 - y0) + S;
	}

	static Rect byXYWH(T x, T y, T w, T h) {
		return Rect(x, y, x + w - S, y + h - S);
	}
};

template<typename T> struct Rot {
	typedef T ValueType;

	T s = 0;
	T c = 1;

	Rot() {
	}
	Rot(T angle) {
		s = std::sin(angle);
		c = std::cos(angle);
	}
	Rot(T s_, T c_) : s(s_), c(c_) {
	}
	Rot(const Rot &other) {
		s = other.s;
		c = other.c;
	}

	Rot &operator = (const Rot &other) {
		s = other.s;
		c = other.c;

		return *this;
	}
	Rot operator * (const Rot &other) const {
		return Rot(
			s * other.c + c * other.s,
			c * other.c - s * other.s
		);
	}
	Vec2<T> operator * (const Vec2<T> &other) const {
		return Vec2<T>(
			c * other.x - s * other.y,
			s * other.x + c * other.y
		);
	}
	Rot &operator *= (const Rot &other) {
		T s_ = s, c_ = c;
		s = s_ * other.c + c_ * other.s;
		c = c_ * other.c - s_ * other.s;

		return *this;
	}
	bool operator == (const Rot &other) const {
		return s == other.s && c == other.c;
	}
	bool operator != (const Rot &other) const {
		return s != other.s || c != other.c;
	}

	T angle(void) const {
		return std::atan2(s, c);
	}
	void angle(T angle) {
		s = std::sin(angle);
		c = std::cos(angle);
	}

	static Rot IDENTITY(void) {
		return Rot(0, 1);
	}
};

typedef Vec2<Real> Vec2f;

typedef Vec3<Real> Vec3f;

typedef Vec4<Real> Vec4f;

typedef Rect<Real, 0> Rectf;

typedef Vec2<Int> Vec2i;

typedef Vec3<Int> Vec3i;

typedef Vec4<Int> Vec4i;

typedef Rect<Int, 1> Recti;

typedef Rot<Real> Rotf;

}

/* ===========================================================================} */

/*
** {===========================================================================
** Complex structures
*/

namespace Math {

template<typename T> struct Line {
	typedef T PointType;

	T pointA;
	T pointB;

	Line() {
	}
	Line(T pA, T pB) : pointA(pA), pointB(pB) {
	}
	Line(const Line &other) {
		pointA = other.pointA;
		pointB = other.pointB;
	}

	Line &operator = (const Line &other) {
		pointA = other.pointA;
		pointB = other.pointB;

		return *this;
	}
};

template<typename T, typename R = Real> struct Circle {
	typedef T PointType;
	typedef R RadiusType;

	T center;
	R radius;

	Circle() {
	}
	Circle(T c, R r) : center(c), radius(r) {
	}
	Circle(const Circle &other) {
		center = other.center;
		radius = other.radius;
	}

	Circle &operator = (const Circle &other) {
		center = other.center;
		radius = other.radius;

		return *this;
	}
};

}

/* ===========================================================================} */

/*
** {===========================================================================
** Math utilities
*/

namespace Math {

double PI(void);
template<typename T> inline T EPSILON(void) {
	return std::numeric_limits<T>::epsilon();
}

template<typename T> inline constexpr T pow(T base, T exponent) {
	return exponent == 0 ? 1 : base * pow(base, exponent - 1);
}

template<typename T> inline int sign(T v) {
	if (v < 0)
		return -1;
	else if (v > 0)
		return 1;

	return 0;
}

template<typename T> inline T radToDeg(T v) {
	return (T)((v / PI()) * 180.0);
}
template<typename T> inline T degToRad(T v) {
	return (T)((v / 180.0) * PI());
}

template<typename T> inline T clamp(T v, T lo, T hi) {
	if (v < lo)
		v = lo;
	if (v > hi)
		v = hi;

	return v;
}
template<typename T> inline T lerp(T lo, T hi, float f) {
	T diff = hi - lo;

	return lo + diff * f;
}

/**
 * @brief Evaluates a start code and a generic component to a hash code.
 *
 * @param[in] start The start code.
 * @param[in] val The component value.
 * @return Hash code.
 */
template<typename T> inline size_t hash(size_t start, const T &val) {
	std::hash<T> hasher;
	const size_t result = start ^ hasher(val) + 0x9e3779b9 + (start << 6) + (start >> 2);

	return result;
}
/**
 * @brief Evaluates a start code and a number of generic components to a hash code.
 *
 * @param[in] start The start code.
 * @param[in] car The first component value.
 * @param[in] cdr The rest component value.
 * @return Hash code.
 */
template<typename Car, typename ...Cdr> inline size_t hash(size_t start, const Car &car, const Cdr &...cdr) {
	const size_t result = hash(hash(start, car), cdr...);

	return result;
}

void srand(unsigned seed);
void srand(void);
int rand(void);

}

/* ===========================================================================} */

/*
** {===========================================================================
** Intersections
*/

namespace Math {

// Point-point.
template<typename T> inline bool intersects(const Vec2<T> &point0, const Vec2<T> &point1, T epsilon) {
	return std::abs(point0.x - point1.x) <= epsilon &&
		std::abs(point0.y - point1.y) <= epsilon;
}
template<typename T> inline bool intersects(const Vec2<T> &point0, const Vec2<T> &point1) {
	return intersects(point0, point1, EPSILON<T>());
}
// Point-line.
template<typename T> inline bool intersects(const Vec2<T> &point, const Line<Vec2<T> > &line, T epsilon) {
	const T minX = std::min(line.pointA.x, line.pointB.x);
	const T minY = std::min(line.pointA.y, line.pointB.y);
	const T maxX = std::max(line.pointA.x, line.pointB.x);
	const T maxY = std::max(line.pointA.y, line.pointB.y);
	if (point.x < minX || point.x > maxX || point.y < minY || point.y > maxY)
		return false;

	const Real M = (line.pointB.y - line.pointA.y) / (line.pointB.x - line.pointA.x);
	const Real B = line.pointA.y - line.pointA.x * M;

	return std::abs(point.y - (point.x * M + B)) <= epsilon;
}
template<typename T> inline bool intersects(const Vec2<T> &point, const Line<Vec2<T> > &line) {
	return intersects(point, line, EPSILON<T>());
}
// Point-circle.
template<typename T> inline bool intersects(const Vec2<T> &point, const Circle<Vec2<T> > &circ) {
	const Line<Vec2<T> > line(point, circ.center);

	return line.pointA.squaredDistanceTo(line.pointB) <= circ.radius * circ.radius;
}
// Point-AABB.
template<typename T, int S> inline bool intersects(const Vec2<T> &point, const Rect<T, S> &rect) {
	return (point.x >= rect.xMin() && point.x <= rect.xMax()) && (point.y >= rect.yMin() && point.y <= rect.yMax());
}
// Line-line.
template<typename T> inline bool intersects(const Line<Vec2<T> > &line0, const Line<Vec2<T> > &line1) {
	auto orientation = [] (const Vec2<T> &p, const Vec2<T> &q, const Vec2<T> &r) -> int {
		const T val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);

		return sign(val);
	};
	auto onSegment = [] (const Vec2<T> &p, const Vec2<T> &q, const Vec2<T> &r) -> bool {
		if (q.x <= std::max(p.x, r.x) && q.x >= std::min(p.x, r.x) && q.y <= std::max(p.y, r.y) && q.y >= std::min(p.y, r.y))
			return true;

		return false;
	};

	const int o1 = orientation(line0.pointA, line0.pointB, line1.pointA);
	const int o2 = orientation(line0.pointA, line0.pointB, line1.pointB);
	const int o3 = orientation(line1.pointA, line1.pointB, line0.pointA);
	const int o4 = orientation(line1.pointA, line1.pointB, line0.pointB);
	if (o1 != o2 && o3 != o4)
		return true;
	if (o1 == 0 && onSegment(line0.pointA, line1.pointA, line0.pointB))
		return true;
	if (o2 == 0 && onSegment(line0.pointA, line1.pointB, line0.pointB))
		return true;
	if (o3 == 0 && onSegment(line1.pointA, line0.pointA, line1.pointB))
		return true;
	if (o4 == 0 && onSegment(line1.pointA, line0.pointB, line1.pointB))
		return true;

	return false;
}
// Line-circie.
template<typename T> inline bool intersects(const Line<Vec2<T> > &line, const Circle<Vec2<T> > &circ) {
	const Vec2<T> d = line.pointB - line.pointA;
	const T A = d.lengthSquared();
	const T B = 2 * (d.x * (line.pointA.x - circ.center.x) + d.y * (line.pointA.y - circ.center.y));
	const T C = (line.pointA.x - circ.center.x) * (line.pointA.x - circ.center.x) +
		(line.pointA.y - circ.center.y) * (line.pointA.y - circ.center.y) -
		circ.radius * circ.radius;
	const T det = B * B - 4 * A * C;

	if (A <= EPSILON<T>() || det < 0) {
		return false;
	} else if (det == 0) {
		const Real t = -B / (2 * A);
		// const Vec2<T> p0(line.pointA.x + d.x * t, line.pointA.y + d.y * t);

		return t >= 0 && t <= 1;
	} else {
		const Real t0 = (Real)((-B + std::sqrt(det)) / (2 * A));
		const Real t1 = (Real)((-B - std::sqrt(det)) / (2 * A));
		// const Vec2<T> p0(line.pointA.x + d.x * t0, line.pointA.y + d.y * t0);
		// const Vec2<T> p1(line.pointA.x + d.x * t1, line.pointA.y + d.y * t1);

		return (t0 >= 0 && t0 <= 1) || (t1 >= 0 && t1 <= 1);
	}
}
// Line-AABB.
template<typename T, int S> inline bool intersects(const Line<Vec2<T> > &line, const Rect<T, S> &rect) {
	if (intersects(line.pointA, rect) || intersects(line.pointB, rect))
		return true;
	
	Vec2<T> norm = (line.pointB - line.pointA).normalized();
	norm.x = (norm.x != 0) ? 1 / norm.x : 0;
	norm.y = (norm.y != 0) ? 1 / norm.y : 0;
	const Vec2<T> min = (Vec2<T>(rect.xMin(), rect.yMin()) - line.pointA) * norm;
	const Vec2<T> max = (Vec2<T>(rect.xMax(), rect.yMax()) - line.pointA) * norm;

	const T tmin = std::max(std::min(min.x, max.x), std::min(min.y, max.y));
	const T tmax = std::min(std::max(min.x, max.x), std::max(min.y, max.y));
	if (tmax < 0 || tmin > tmax)
		return false;

	const T t = (tmin < 0) ? tmax : tmin;

	return t > 0 && t * t < line.pointA.squaredDistanceTo(line.pointB);
}
// Circle-circle.
template<typename T> inline bool intersects(const Circle<Vec2<T> > &circ0, const Circle<Vec2<T> > &circ1) {
	const Line<Vec2<T> > line(circ0.center, circ1.center);

	return line.pointA.squaredDistanceTo(line.pointB) <= (circ0.radius + circ1.radius) * (circ0.radius + circ1.radius);
}
// Circle-AABB.
template<typename T, int S> inline bool intersects(const Circle<Vec2<T> > &circ, const Rect<T, S> &rect) {
	const Vec2<T> min(rect.xMin(), rect.yMin());
	const Vec2<T> max(rect.xMax(), rect.yMax());
	const Vec2<T> closest(
		(circ.center.x < min.x) ? min.x :
		(circ.center.x > max.x) ? max.x :
			circ.center.x,
		(circ.center.y < min.y) ? min.y :
		(circ.center.y > max.y) ? max.y :
			circ.center.y
	);
	const Line<Vec2<T> > line(circ.center, closest);

	return line.pointA.squaredDistanceTo(line.pointB) <= circ.radius * circ.radius;
}
// AABB-AABB.
template<typename T, int S> inline bool intersects(const Rect<T, S> &rect0, const Rect<T, S> &rect1) {
	const Vec2<Real> halfsize1(
		rect1.width() * 0.5f, rect1.height() * 0.5f
	);
	const Vec2<Real> center1(
		rect1.xMin() + halfsize1.x, rect1.yMin() + halfsize1.y
	);
	const Rect<Real, 0> rect0m = Rect<Real, 0>::byXYWH(
		rect0.xMin() - halfsize1.x, rect0.yMin() - halfsize1.y,
		rect0.width() + rect1.width(), rect0.height() + rect1.height()
	);

	return intersects(center1, rect0m);
}

// Aliases.
template<typename T, int S> inline bool intersects(const Rect<T, S> &rect, const Vec2<T> &point) {
	return intersects<T, S>(point, rect);
}

}

/* ===========================================================================} */

#endif /* __MATHEMATICS_H__ */
