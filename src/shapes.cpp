/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "mathematics.h"
#include "shapes.h"
#include <queue>

/*
** {===========================================================================
** Shapes
*/

namespace Shapes {

int line(int x0, int y0, int x1, int y1, Plotter plot) {
	int result = 0;
	int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = (dx > dy ? dx : -dy) / 2, e2;
	for ( ; ; ) {
		++result;
		plot(x0, y0);
		if (x0 == x1 && y0 == y1)
			break;
		e2 = err;
		if (e2 > -dx) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dy) {
			err += dx;
			y0 += sy;
		}
	}

	return result;
}

int box(int x0, int y0, int x1, int y1, Plotter plot) {
	int result = 0;
	for (int i = std::min(x0, x1); i <= std::max(x0, x1); ++i) {
		result += 2;
		plot(i, y0);
		plot(i, y1);
	}
	for (int j = std::min(y0, y1) + 1; j <= std::max(y0, y1) - 1; ++j) {
		result += 2;
		plot(x0, j);
		plot(x1, j);
	}

	return result;
}

int boxFill(int x0, int y0, int x1, int y1, Plotter plot) {
	int result = 0;
	for (int j = std::min(y0, y1); j <= std::max(y0, y1); ++j) {
		for (int i = std::min(x0, x1); i <= std::max(x0, x1); ++i) {
			++result;
			plot(i, j);
		}
	}

	return result;
}

int ellipse(int x, int y, int rx, int ry, Plotter plot) {
	int result = 0;
	int ix, iy;
	int h, i, j, k;
	int oh, oi, oj, ok;
	int xmh, xph, ypk, ymk;
	int xmi, xpi, ymj, ypj;
	int xmj, xpj, ymi, ypi;
	int xmk, xpk, ymh, yph;

	if ((rx < 0) || (ry < 0))
		return -1;

	if (rx == 0)
		return line(x, y - ry, x, y + ry, plot);
	if (ry == 0)
		return line(x - rx, y, x + rx, y, plot);

	oh = oi = oj = ok = 0xFFFF;

	if (rx > ry) {
		ix = 0;
		iy = rx * 64;

		do {
			h = (ix + 32) >> 6;
			i = (iy + 32) >> 6;
			j = (h * ry) / rx;
			k = (i * ry) / rx;

			if (((ok != k) && (oj != k)) || ((oj != j) && (ok != j)) || (k != j)) {
				xph = x + h;
				xmh = x - h;
				if (k > 0) {
					ypk = y + k;
					ymk = y - k;
					plot(xmh, ypk);
					plot(xph, ypk);
					plot(xmh, ymk);
					plot(xph, ymk);
					result += 4;
				} else {
					plot(xmh, y);
					plot(xph, y);
					result += 2;
				}
				ok = k;
				xpi = x + i;
				xmi = x - i;
				if (j > 0) {
					ypj = y + j;
					ymj = y - j;
					plot(xmi, ypj);
					plot(xpi, ypj);
					plot(xmi, ymj);
					plot(xpi, ymj);
					result += 4;
				} else {
					plot(xmi, y);
					plot(xpi, y);
					result += 2;
				}
				oj = j;
			}

			ix = ix + iy / rx;
			iy = iy - ix / rx;
		} while (i > h);
	} else {
		ix = 0;
		iy = ry * 64;

		do {
			h = (ix + 32) >> 6;
			i = (iy + 32) >> 6;
			j = (h * rx) / ry;
			k = (i * rx) / ry;

			if (((oi != i) && (oh != i)) || ((oh != h) && (oi != h) && (i != h))) {
				xmj = x - j;
				xpj = x + j;
				if (i > 0) {
					ypi = y + i;
					ymi = y - i;
					plot(xmj, ypi);
					plot(xpj, ypi);
					plot(xmj, ymi);
					plot(xpj, ymi);
					result += 4;
				} else {
					plot(xmj, y);
					plot(xpj, y);
					result += 2;
				}
				oi = i;
				xmk = x - k;
				xpk = x + k;
				if (h > 0) {
					yph = y + h;
					ymh = y - h;
					plot(xmk, yph);
					plot(xpk, yph);
					plot(xmk, ymh);
					plot(xpk, ymh);
					result += 4;
				} else {
					plot(xmk, y);
					plot(xpk, y);
					result += 2;
				}
				oh = h;
			}

			ix = ix + iy / ry;
			iy = iy - ix / ry;
		} while (i > h);
	}

	return result;
}

int ellipseFill(int x, int y, int rx, int ry, Plotter plot) {
	int result = 0;
	int ix, iy;
	int h, i, j, k;
	int oh, oi, oj, ok;
	int xmh, xph;
	int xmi, xpi;
	int xmj, xpj;
	int xmk, xpk;

	if ((rx < 0) || (ry < 0))
		return -1;

	if (rx == 0)
		return line(x, y - ry, x, y + ry, plot);
	if (ry == 0)
		return line(x - rx, y, x + rx, y, plot);

	oh = oi = oj = ok = 0xFFFF;

	if (rx > ry) {
		ix = 0;
		iy = rx * 64;

		do {
			h = (ix + 32) >> 6;
			i = (iy + 32) >> 6;
			j = (h * ry) / rx;
			k = (i * ry) / rx;

			if ((ok != k) && (oj != k)) {
				xph = x + h;
				xmh = x - h;
				if (k > 0) {
					result += line(xmh, y + k, xph, y + k, plot);
					result += line(xmh, y - k, xph, y - k, plot);
				} else {
					result += line(xmh, y, xph, y, plot);
				}
				ok = k;
			}
			if ((oj != j) && (ok != j) && (k != j)) {
				xmi = x - i;
				xpi = x + i;
				if (j > 0) {
					result += line(xmi, y + j, xpi, y + j, plot);
					result += line(xmi, y - j, xpi, y - j, plot);
				} else {
					result += line(xmi, y, xpi, y, plot);
				}
				oj = j;
			}

			ix = ix + iy / rx;
			iy = iy - ix / rx;
		} while (i > h);
	} else {
		ix = 0;
		iy = ry * 64;

		do {
			h = (ix + 32) >> 6;
			i = (iy + 32) >> 6;
			j = (h * rx) / ry;
			k = (i * rx) / ry;

			if ((oi != i) && (oh != i)) {
				xmj = x - j;
				xpj = x + j;
				if (i > 0) {
					result += line(xmj, y + i, xpj, y + i, plot);
					result += line(xmj, y - i, xpj, y - i, plot);
				} else {
					result += line(xmj, y, xpj, y, plot);
				}
				oi = i;
			}
			if ((oh != h) && (oi != h) && (i != h)) {
				xmk = x - k;
				xpk = x + k;
				if (h > 0) {
					result += line(xmk, y + h, xpk, y + h, plot);
					result += line(xmk, y - h, xpk, y - h, plot);
				} else {
					result += line(xmk, y, xpk, y, plot);
				}
				oh = h;
			}

			ix = ix + iy / ry;
			iy = iy - ix / ry;
		} while (i > h);
	}

	return result;
}

template<typename T> static int fill(
	int x, int y, int w, int h,
	int* rx0, int* ry0, int* rx1, int* ry1,
	const T &oldColor, const T &newColor,
	std::function<T(int, int)> get, std::function<void(int, int)> plot,
	std::function<bool(const T &, const T &)> hit
) {
	int result = 0;

	auto valid = [=] (int xx, int yy) -> bool {
		if (rx0 && xx < *rx0)
			return false;
		if (ry0 && yy < *ry0)
			return false;
		if (rx1 && xx > *rx1)
			return false;
		if (ry1 && yy > *ry1)
			return false;

		return xx >= 0 && xx < w && yy >= 0 && yy < h;
	};

	const T srcColor = oldColor;
	if (!valid(x, y) || srcColor == newColor)
		return result;

	std::queue<Math::Vec2i> analyzeQueue;
	analyzeQueue.push(Math::Vec2i(x, y));

	while (!analyzeQueue.empty()) {
		if (!hit(get(analyzeQueue.front().x, analyzeQueue.front().y), srcColor)) {
			analyzeQueue.pop();

			continue;
		}
		Math::Vec2i leftmostPt = analyzeQueue.front();
		leftmostPt.x -= 1;
		analyzeQueue.pop();
		Math::Vec2i rightmostPt = leftmostPt;
		rightmostPt.x += 2;
		while (valid(leftmostPt.x, leftmostPt.y) && hit(get(leftmostPt.x, leftmostPt.y), srcColor))
			--leftmostPt.x;

		while (valid(rightmostPt.x, rightmostPt.y) && hit(get(rightmostPt.x, rightmostPt.y), srcColor))
			++rightmostPt.x;

		bool checkAbove = true;
		bool checkBelow = true;
		Math::Vec2i pt = leftmostPt;
		++pt.x;
		for ( ; pt.x < rightmostPt.x; ++pt.x) {
			plot(pt.x, pt.y);
			++result;

			Math::Vec2i ptAbove = pt;
			--ptAbove.y;
			if (checkAbove) {
				if (valid(ptAbove.x, ptAbove.y) && hit(get(ptAbove.x, ptAbove.y), srcColor)) {
					analyzeQueue.push(ptAbove);
					checkAbove = false;
				}
			} else {
				checkAbove = (valid(ptAbove.x, ptAbove.y) && !hit(get(ptAbove.x, ptAbove.y), srcColor));
			}

			Math::Vec2i ptBelow = pt;
			++ptBelow.y;
			if (checkBelow) {
				if (valid(ptBelow.x, ptBelow.y) && hit(get(ptBelow.x, ptBelow.y), srcColor)) {
					analyzeQueue.push(ptBelow);
					checkBelow = false;
				}
			} else {
				checkBelow = (valid(ptBelow.x, ptBelow.y) && !hit(get(ptBelow.x, ptBelow.y), srcColor));
			}
		}
	}

	return result;
}

int fill(int x, int y, int w, int h, int* rx0, int* ry0, int* rx1, int* ry1, int oldColor, int newColor, Indexer get, Plotter plot) {
	return fill<int>(
		x, y, w, h,
		rx0, ry0, rx1, ry1,
		oldColor, newColor,
		get, plot,
		[] (const int &left, const int &right) -> bool {
			return left == right;
		}
	);
}

int fill(int x, int y, int w, int h, int* rx0, int* ry0, int* rx1, int* ry1, const Color &oldColor, const Color &newColor, Picker get, Plotter plot) {
	return fill<Color>(
		x, y, w, h,
		rx0, ry0, rx1, ry1,
		oldColor, newColor,
		get, plot,
		[] (const Color &left, const Color &right) -> bool {
			return left == right; // || (left.a == 0 && right.a == 0);
		}
	);
}

template<typename T> static int replace(
	int x, int y, int w, int h,
	int* rx0, int* ry0, int* rx1, int* ry1,
	const T &oldColor, const T &newColor,
	std::function<T(int, int)> get, std::function<void(int, int)> plot,
	std::function<bool(const T &, const T &)> hit
) {
	int result = 0;

	auto valid = [=] (int xx, int yy) -> bool {
		if (rx0 && xx < *rx0)
			return false;
		if (ry0 && yy < *ry0)
			return false;
		if (rx1 && xx > *rx1)
			return false;
		if (ry1 && yy > *ry1)
			return false;

		return xx >= 0 && xx < w && yy >= 0 && yy < h;
	};

	const T srcColor = oldColor;
	if (!valid(x, y) || srcColor == newColor)
		return result;

	for (int j = 0; j < h; ++j) {
		for (int i = 0; i < w; ++i) {
			if (!valid(i, j))
				continue;
			if (!hit(get(i, j), srcColor))
				continue;

			plot(i, j);
		}
	}

	return result;
}

int replace(int x, int y, int w, int h, int* rx0, int* ry0, int* rx1, int* ry1, int oldColor, int newColor, Indexer get, Plotter plot) {
	return replace<int>(
		x, y, w, h,
		rx0, ry0, rx1, ry1,
		oldColor, newColor,
		get, plot,
		[] (const int &left, const int &right) -> bool {
			return left == right;
		}
	);
}

int replace(int x, int y, int w, int h, int* rx0, int* ry0, int* rx1, int* ry1, const Color &oldColor, const Color &newColor, Picker get, Plotter plot) {
	return replace<Color>(
		x, y, w, h,
		rx0, ry0, rx1, ry1,
		oldColor, newColor,
		get, plot,
		[] (const Color &left, const Color &right) -> bool {
			return left == right || (left.a == 0 && right.a == 0);
		}
	);
}

}

/* ===========================================================================} */
