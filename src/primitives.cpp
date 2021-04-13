/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "datetime.h"
#include "encoding.h"
#include "primitives.h"
#include "project.h"
#include "renderer.h"
#include "resource/inline_resource.h"
#include "../lib/sdl_gfx/SDL2_gfxPrimitives.h"

/*
** {===========================================================================
** Macros and constants
*/

#if !BITTY_MULTITHREAD_ENABLED
#	pragma message("Multithread disabled.")
#endif /* BITTY_MULTITHREAD_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Primitive command
*/

struct Cmd {
public:
	enum Types {
		NONE,
		TARGET,
		CLS,
		BLEND,
		PLOT,
		LINE,
		CIRC,
		ELLIPSE,
		RECT,
		TRI,
		FONT,
		TEXT,
		TEX,
		SPR,
		MAP,
		PGET,
		PSET,
		MGET,
		MSET,
		VOLUME,
		PLAY_SFX,
		PLAY_MUSIC,
		STOP_SFX,
		STOP_MUSIC,
		RUMBLE,
		CURSOR,
		FUNCTION
	};

	typedef void (* Dtor)(Cmd*);

public:
	Types type = NONE;
	Dtor dtor = [] (Cmd*) -> void { /* Do nothing. */ };
};

class CmdClippable {
private:
	bool _clipping = false;
	int _clippingX = 0, _clippingY = 0, _clippingW = 0, _clippingH = 0;

public:
	~CmdClippable() {
		_clipping = false;
		_clippingX = 0;
		_clippingY = 0;
		_clippingW = 0;
		_clippingH = 0;
	}

	void clip(int x, int y, int w, int h) {
		_clipping = true;

		_clippingX = x;
		_clippingY = y;
		_clippingW = w;
		_clippingH = h;
	}

protected:
	bool clip(int* x, int* y, int* w, int* h) const {
		if (!_clipping)
			return false;

		if (x)
			*x = _clippingX;
		if (y)
			*y = _clippingY;
		if (w)
			*w = _clippingW;
		if (h)
			*h = _clippingH;

		return true;
	}
	void clip(Renderer* rnd, bool c) const {
		if (_clipping) {
			if (c)
				rnd->clip(_clippingX, _clippingY, _clippingW, _clippingH);
			else
				rnd->clip();
		}
	}
};

class CmdColored {
private:
	bool _colorChanged = false;
	bool _alphaChanged = false;
	Color _color;

public:
	~CmdColored() {
		_colorChanged = false;
		_alphaChanged = false;
		_color = Color(255, 255, 255, 255);
	}

	void colored(const Color &col) {
		_colorChanged = col.r != 255 || col.g != 255 || col.b != 255;
		_alphaChanged = col.a != 255;

		_color = col;
	}

protected:
	bool colored(Color* col, bool* colorChanged, bool* alphaChanged) const {
		if (!_colorChanged && !_alphaChanged)
			return false;

		if (col)
			*col = _color;
		if (colorChanged)
			*colorChanged = _colorChanged;
		if (alphaChanged)
			*alphaChanged = _alphaChanged;

		return _colorChanged || _alphaChanged;
	}
};

class CmdTarget : public Cmd {
private:
	Resources::Texture::Ptr _texture = nullptr;

public:
	CmdTarget() {
		type = TARGET;
		dtor = [] (Cmd* cmd) -> void {
			CmdTarget* self = reinterpret_cast<CmdTarget*>(cmd);
			self->~CmdTarget();
		};
	}
	CmdTarget(Resources::Texture::Ptr tex) {
		type = TARGET;
		dtor = [] (Cmd* cmd) -> void {
			CmdTarget* self = reinterpret_cast<CmdTarget*>(cmd);
			self->~CmdTarget();
		};

		_texture = tex;
	}

	void run(Primitives* primitives, Renderer* rnd, const Project* project, Resources* res) {
		if (_texture) {
			Texture::Ptr ptr = res->load(project, *_texture);

			if (ptr)
				rnd->target(ptr.get());
			else
				rnd->target(primitives->canvas().get());
		} else {
			rnd->target(primitives->canvas().get());
		}
	}
};

class CmdCls : public Cmd {
private:
	Color _color;

public:
	CmdCls() {
		type = CLS;
		dtor = [] (Cmd* cmd) -> void {
			CmdCls* self = reinterpret_cast<CmdCls*>(cmd);
			(void)self;
		};
	}
	CmdCls(const Color &col) {
		type = CLS;
		dtor = [] (Cmd* cmd) -> void {
			CmdCls* self = reinterpret_cast<CmdCls*>(cmd);
			(void)self;
		};

		_color = col;
	}

	void run(Renderer* rnd) {
		rnd->clear(&_color);
	}
};

class CmdBlend : public Cmd {
private:
	Resources::Texture::Ptr _texture = nullptr;
	SDL_BlendMode _mode = SDL_BLENDMODE_NONE;

public:
	CmdBlend() {
		type = BLEND;
		dtor = [] (Cmd* cmd) -> void {
			CmdBlend* self = reinterpret_cast<CmdBlend*>(cmd);
			self->~CmdBlend();
		};
	}
	CmdBlend(SDL_BlendMode mode) {
		type = BLEND;
		dtor = [] (Cmd* cmd) -> void {
			CmdBlend* self = reinterpret_cast<CmdBlend*>(cmd);
			self->~CmdBlend();
		};

		_mode = mode;
	}
	CmdBlend(Resources::Texture::Ptr tex, SDL_BlendMode mode) {
		type = BLEND;
		dtor = [] (Cmd* cmd) -> void {
			CmdBlend* self = reinterpret_cast<CmdBlend*>(cmd);
			self->~CmdBlend();
		};

		_texture = tex;
		_mode = mode;
	}

	void run(Renderer* rnd, const Project* project, Resources* res) {
		if (_texture) {
			Texture::Ptr ptr = res->load(project, *_texture);
			if (!ptr)
				return;

			ptr->blend((Texture::BlendModes)_mode);
		} else {
			rnd->blend(_mode);
		}
	}
};

class CmdPlot : public Cmd, public CmdClippable {
private:
	int _x = 0, _y = 0;
	Color _color;

public:
	CmdPlot() {
		type = PLOT;
		dtor = [] (Cmd* cmd) -> void {
			CmdPlot* self = reinterpret_cast<CmdPlot*>(cmd);
			(void)self;
		};
	}
	CmdPlot(int x, int y, const Color &col) {
		type = PLOT;
		dtor = [] (Cmd* cmd) -> void {
			CmdPlot* self = reinterpret_cast<CmdPlot*>(cmd);
			(void)self;
		};

		_x = x;
		_y = y;
		_color = col;
	}

	void run(Renderer* rnd) {
		clip(rnd, true);

		SDL_Renderer* renderer = (SDL_Renderer*)rnd->pointer();

		const Uint32 c = _color.toRGBA();
		pixelColor(renderer, (Sint16)_x, (Sint16)_y, c);

		clip(rnd, false);
	}
};

class CmdLine : public Cmd, public CmdClippable {
private:
	int _x0 = 0, _y0 = 0, _x1 = 0, _y1 = 0;
	Color _color;

public:
	CmdLine() {
		type = LINE;
		dtor = [] (Cmd* cmd) -> void {
			CmdLine* self = reinterpret_cast<CmdLine*>(cmd);
			(void)self;
		};
	}
	CmdLine(int x0, int y0, int x1, int y1, const Color &col) {
		type = LINE;
		dtor = [] (Cmd* cmd) -> void {
			CmdLine* self = reinterpret_cast<CmdLine*>(cmd);
			(void)self;
		};

		_x0 = x0;
		_y0 = y0;
		_x1 = x1;
		_y1 = y1;
		_color = col;
	}

	void run(Renderer* rnd) {
		clip(rnd, true);

		SDL_Renderer* renderer = (SDL_Renderer*)rnd->pointer();

		const Uint32 c = _color.toRGBA();
		lineColor(renderer, (Sint16)_x0, (Sint16)_y0, (Sint16)_x1, (Sint16)_y1, c);

		clip(rnd, false);
	}
};

class CmdCirc : public Cmd, public CmdClippable {
private:
	int _x = 0, _y = 0, _r = 0;
	bool _fill = false;
	Color _color;

public:
	CmdCirc() {
		type = CIRC;
		dtor = [] (Cmd* cmd) -> void {
			CmdCirc* self = reinterpret_cast<CmdCirc*>(cmd);
			(void)self;
		};
	}
	CmdCirc(int x, int y, int r, bool fill, const Color &col) {
		type = CIRC;
		dtor = [] (Cmd* cmd) -> void {
			CmdCirc* self = reinterpret_cast<CmdCirc*>(cmd);
			(void)self;
		};

		_x = x;
		_y = y;
		_r = r;
		_fill = fill;
		_color = col;
	}

	void run(Renderer* rnd) {
		clip(rnd, true);

		SDL_Renderer* renderer = (SDL_Renderer*)rnd->pointer();

		const Uint32 c = _color.toRGBA();
		if (_fill)
			filledCircleColor(renderer, (Sint16)_x, (Sint16)_y, (Sint16)_r, c);
		else
			circleColor(renderer, (Sint16)_x, (Sint16)_y, (Sint16)_r, c);

		clip(rnd, false);
	}
};

class CmdEllipse : public Cmd, public CmdClippable {
private:
	int _x = 0, _y = 0, _rx = 0, _ry = 0;
	bool _fill = false;
	Color _color;

public:
	CmdEllipse() {
		type = ELLIPSE;
		dtor = [] (Cmd* cmd) -> void {
			CmdEllipse* self = reinterpret_cast<CmdEllipse*>(cmd);
			(void)self;
		};
	}
	CmdEllipse(int x, int y, int rx, int ry, bool fill, const Color &col) {
		type = ELLIPSE;
		dtor = [] (Cmd* cmd) -> void {
			CmdEllipse* self = reinterpret_cast<CmdEllipse*>(cmd);
			(void)self;
		};

		_x = x;
		_y = y;
		_rx = rx;
		_ry = ry;
		_fill = fill;
		_color = col;
	}

	void run(Renderer* rnd) {
		clip(rnd, true);

		SDL_Renderer* renderer = (SDL_Renderer*)rnd->pointer();

		const Uint32 c = _color.toRGBA();
		if (_fill)
			filledEllipseColor(renderer, (Sint16)_x, (Sint16)_y, (Sint16)_rx, (Sint16)_ry, c);
		else
			ellipseColor(renderer, (Sint16)_x, (Sint16)_y, (Sint16)_rx, (Sint16)_ry, c);

		clip(rnd, false);
	}
};

class CmdRect : public Cmd, public CmdClippable {
private:
	int _x0 = 0, _y0 = 0, _x1 = 0, _y1 = 0;
	bool _fill = false;
	Color _color;
	int _rad = -1;

public:
	CmdRect() {
		type = RECT;
		dtor = [] (Cmd* cmd) -> void {
			CmdRect* self = reinterpret_cast<CmdRect*>(cmd);
			(void)self;
		};
	}
	CmdRect(int x0, int y0, int x1, int y1, bool fill, const Color &col, const int* rad) {
		type = RECT;
		dtor = [] (Cmd* cmd) -> void {
			CmdRect* self = reinterpret_cast<CmdRect*>(cmd);
			(void)self;
		};

		_x0 = x0;
		_y0 = y0;
		_x1 = x1;
		_y1 = y1;
		_fill = fill;
		_color = col;
		_rad = rad ? *rad : -1;
	}

	void run(Renderer* rnd) {
		clip(rnd, true);

		SDL_Renderer* renderer = (SDL_Renderer*)rnd->pointer();

		const Uint32 c = _color.toRGBA();
		if (_rad > 0) {
			if (_fill)
				roundedBoxColor(renderer, (Sint16)_x0, (Sint16)_y0, (Sint16)_x1, (Sint16)_y1, (Sint16)_rad, c);
			else
				roundedRectangleColor(renderer, (Sint16)_x0, (Sint16)_y0, (Sint16)_x1, (Sint16)_y1, (Sint16)_rad, c);
		} else {
			if (_fill)
				boxColor(renderer, (Sint16)_x0, (Sint16)_y0, (Sint16)_x1, (Sint16)_y1, c);
			else
				rectangleColor(renderer, (Sint16)_x0, (Sint16)_y0, (Sint16)_x1, (Sint16)_y1, c);
		}

		clip(rnd, false);
	}
};

class CmdTri : public Cmd, public CmdClippable {
private:
	Math::Vec3f _p0, _p1, _p2;
	Resources::Texture::Ptr _texture = nullptr;
	Math::Vec2f _uv0, _uv1, _uv2;
	bool _bothSides = false;
	bool _depth = true;
	bool _fill = false;
	Color _color;

public:
	CmdTri() {
		type = TRI;
		dtor = [] (Cmd* cmd) -> void {
			CmdTri* self = reinterpret_cast<CmdTri*>(cmd);
			self->~CmdTri();
		};
	}
	CmdTri(const Math::Vec2f &p0, const Math::Vec2f &p1, const Math::Vec2f &p2, bool fill, const Color &col) {
		type = TRI;
		dtor = [] (Cmd* cmd) -> void {
			CmdTri* self = reinterpret_cast<CmdTri*>(cmd);
			self->~CmdTri();
		};

		_p0 = Math::Vec3f(p0.x, p0.y, 0);
		_p1 = Math::Vec3f(p1.x, p1.y, 0);
		_p2 = Math::Vec3f(p2.x, p2.y, 0);
		_fill = fill;
		_color = col;
	}
	CmdTri(const Math::Vec2f &p0, const Math::Vec2f &p1, const Math::Vec2f &p2, Resources::Texture::Ptr tex, const Math::Vec2f &uv0, const Math::Vec2f &uv1, const Math::Vec2f &uv2) {
		type = TRI;
		dtor = [] (Cmd* cmd) -> void {
			CmdTri* self = reinterpret_cast<CmdTri*>(cmd);
			self->~CmdTri();
		};

		_p0 = Math::Vec3f(p0.x, p0.y, 0);
		_p1 = Math::Vec3f(p1.x, p1.y, 0);
		_p2 = Math::Vec3f(p2.x, p2.y, 0);
		_texture = tex;
		_uv0 = uv0;
		_uv1 = uv1;
		_uv2 = uv2;
	}

	void run(Renderer* rnd, const Project* project, Resources* res) {
		clip(rnd, true);

		bool drew = false;
		do {
			if (!res)
				break;

			if (!_texture)
				break;

			Texture::Ptr ptr = res->load(project, *_texture);
			if (!ptr)
				break;

			Image::Ptr src = _texture->source.lock();
			if (!src)
				break;

			drew = true;

			const int side = Math::sign((_p1.x - _p0.x) * (_p2.y - _p0.y) - (_p1.y - _p0.y) * (_p2.x - _p0.x));
			if (side == 0)
				break;

			const bool swapped = side == 1 && _bothSides;
			if (side == -1 || swapped) {
				Math::Vec4f points[3] = {
					Math::Vec4f(_p0.x, _p0.y, _uv0.x * ptr->width(), _uv0.y * ptr->height()),
					Math::Vec4f(_p1.x, _p1.y, _uv1.x * ptr->width(), _uv1.y * ptr->height()),
					Math::Vec4f(_p2.x, _p2.y, _uv2.x * ptr->width(), _uv2.y * ptr->height())
				};
				Real depth[3] = {
					_p0.z, _p1.z, _p2.z
				};
				Real* pdepth = _depth ? depth : nullptr;
				int x, y, w, h;
				Math::Vec4f clipping;
				Math::Vec4f* clipptr = nullptr;
				if (clip(&x, &y, &w, &h))
					clipptr = &clipping;
				clipping = Math::Vec4f(x, y, w, h);
				if (swapped) {
					std::swap(points[1], points[2]);
					if (pdepth)
						std::swap(pdepth[1], pdepth[2]);
				}

				renderTriangle(rnd, src, points, pdepth, clipptr);
			}
		} while (false);

		if (!drew) {
			SDL_Renderer* renderer = (SDL_Renderer*)rnd->pointer();

			const Uint32 c = _color.toRGBA();
			if (_fill)
				filledTrigonColor(renderer, (Sint16)_p0.x, (Sint16)_p0.y, (Sint16)_p1.x, (Sint16)_p1.y, (Sint16)_p2.x, (Sint16)_p2.y, c);
			else
				trigonColor(renderer, (Sint16)_p0.x, (Sint16)_p0.y, (Sint16)_p1.x, (Sint16)_p1.y, (Sint16)_p2.x, (Sint16)_p2.y, c);
		}

		clip(rnd, false);
	}

private:
	static Real getDet(const Math::Vec4f points[3]) {
		return (points[1].y - points[2].y) * (points[0].x - points[2].x) + (points[2].x - points[1].x) * (points[0].y - points[2].y);
	}
	static void getBoundingBox(const Math::Vec4f points[3], int* sx, int* sy, int* ex, int* ey) {
		*sx = (int)std::min(points[0].x, std::min(points[1].x, points[2].x));
		*sy = (int)std::min(points[0].y, std::min(points[1].y, points[2].y));

		*ex = (int)std::max(points[0].x, std::max(points[1].x, points[2].x));
		*ey = (int)std::max(points[0].y, std::max(points[1].y, points[2].y));
	}
	static void getBarycentric(const Math::Vec4f points[3], int x, int y, Real det, Real &alpha, Real &beta, Real &gamma) {
		alpha = ((points[1].y - points[2].y) * (x - points[2].x) + (points[2].x - points[1].x) * (y - points[2].y)) / det;
		beta = ((points[2].y - points[0].y) * (x - points[2].x) + (points[0].x - points[2].x) * (y - points[2].y)) / det;
		gamma = (1.0f - alpha - beta);
	}
	static void getCartesian(const Math::Vec4f points[3], Real l0, Real l1, Real l2, Real &u, Real &v) {
		u = l0 * points[0].z + l1 * points[1].z + l2 * points[2].z;
		v = l0 * points[0].w + l1 * points[1].w + l2 * points[2].w;
	}
	static void sampleTexture(Image::Ptr tex, Real u, Real v, Uint8 &r, Uint8 &g, Uint8 &b, Uint8 &a) {
		Color col;
		tex->get((int)std::round(u), (int)std::round(v), col);
		r = col.r;
		g = col.g;
		b = col.b;
		a = col.a;
	}

	static void plotTriangle(SDL_Renderer* rnd, Image::Ptr tex, const Math::Vec4f points[3], const Real* /* pdepth */, Real l0, Real l1, int x, int y) {
		Real l2 = (1.0f - l0 - l1);
		Real u, v;
		getCartesian(points, l0, l1, l2, u, v);

		Uint8 r, g, b, a;
		sampleTexture(tex, u, v, r, g, b, a);
		SDL_SetRenderDrawColor(rnd, r, g, b, a);
		SDL_RenderDrawPoint(rnd, x, y);
	}

	static void renderTriangle(Renderer* rnd, Image::Ptr tex, const Math::Vec4f points[3], const Real* pdepth, const Math::Vec4f* clipping) {
		SDL_Renderer* renderer = (SDL_Renderer*)rnd->pointer();

		const Real det = getDet(points);
		int sx, sy, ex, ey;
		getBoundingBox(points, &sx, &sy, &ex, &ey);

		for (int y = sy; y <= ey; ++y) {
			if (y < 0 || y >= rnd->height())
				continue;

			if (clipping) {
				if (y < (int)std::floor(clipping->y) || y > (int)std::ceil(clipping->y + clipping->w))
					continue;
			}

			for (int x = sx; x <= ex; ++x) {
				if (x < 0 || x >= rnd->width())
					continue;

				if (clipping) {
					if (x < (int)std::floor(clipping->x) || x > (int)std::ceil(clipping->x + clipping->z))
						continue;
				}

				Real alpha, beta, gamma;
				getBarycentric(points, x, y, det, alpha, beta, gamma);

				if (alpha >= 0.0f && beta >= 0.0f && gamma >= -1e-6f) { // Inside the triangle.
					plotTriangle(renderer, tex, points, pdepth, alpha, beta, x, y);
				}
			}
		}
	}
};

class CmdFont : public Cmd {
private:
	Font::Ptr _font = nullptr;

public:
	CmdFont() {
		type = FONT;
		dtor = [] (Cmd* cmd) -> void {
			CmdFont* self = reinterpret_cast<CmdFont*>(cmd);
			self->~CmdFont();
		};
	}
	CmdFont(Font::Ptr font) {
		type = FONT;
		dtor = [] (Cmd* cmd) -> void {
			CmdFont* self = reinterpret_cast<CmdFont*>(cmd);
			self->~CmdFont();
		};

		_font = font;
	}

	void run(Renderer*, Resources* res) {
		if (_font)
			res->font(_font.get());
		else
			res->font(nullptr);
	}
};

class CmdText : public Cmd, public CmdClippable, public CmdColored {
private:
	std::wstring _text;
	int _x = 0, _y = 0;
	int _margin = 0;

public:
	CmdText() {
		type = TEXT;
		dtor = [] (Cmd* cmd) -> void {
			CmdText* self = reinterpret_cast<CmdText*>(cmd);
			self->~CmdText();
		};
	}
	CmdText(const char* text, int x, int y, int margin) {
		type = TEXT;
		dtor = [] (Cmd* cmd) -> void {
			CmdText* self = reinterpret_cast<CmdText*>(cmd);
			self->~CmdText();
		};

		_text = Unicode::toWide(text);
		_x = x;
		_y = y;
		_margin = margin;
	}

	void run(Renderer* rnd, Resources* res) {
		clip(rnd, true);

		const wchar_t* text = _text.c_str();
		int x = _x, y = _y;
		while (*text) {
			if (!res)
				break;

			const Resources::Id cp = *text++;

			const Color WHITE(255, 255, 255, 255);
			int width = -1, height = -1;
			Resources::Glyph glyph(cp, &WHITE);
			Texture::Ptr ptr = res->load(rnd, glyph, &width, &height);
			if (!ptr)
				continue;

			const Math::Recti dstRect(
				x, y,
				x + ptr->width() - 1, y + ptr->height() - 1
			);

			Color col;
			bool colorChanged = false, alphaChanged = false;
			colored(&col, &colorChanged, &alphaChanged);

			rnd->render(
				ptr.get(),
				nullptr, &dstRect,
				nullptr, nullptr,
				false, false,
				&col, colorChanged, alphaChanged
			);
			x += dstRect.width();
			if (*text)
				x += _margin;
		}

		clip(rnd, false);
	}
};

class CmdTex : public Cmd, public CmdClippable, public CmdColored {
private:
	Resources::Texture::Ptr _texture = nullptr;
	int _x = 0, _y = 0, _width = 0, _height = 0;
	int _sx = 0, _sy = 0, _swidth = 0, _sheight = 0;
	double _rotAngle = 0.0;
	Math::Vec2f _rotCenter;
	bool _rotated = false;
	bool _hFlip = false, _vFlip = false;

public:
	CmdTex() {
		type = TEX;
		dtor = [] (Cmd* cmd) -> void {
			CmdTex* self = reinterpret_cast<CmdTex*>(cmd);
			self->~CmdTex();
		};
	}
	CmdTex(Resources::Texture::Ptr tex, int x, int y, int width, int height, int sx, int sy, int swidth, int sheight, const double* rotAngle, const Math::Vec2f* rotCenter, bool hFlip, bool vFlip) {
		type = TEX;
		dtor = [] (Cmd* cmd) -> void {
			CmdTex* self = reinterpret_cast<CmdTex*>(cmd);
			self->~CmdTex();
		};

		_texture = tex;
		_x = x;
		_y = y;
		_width = width;
		_height = height;
		_sx = sx;
		_sy = sy;
		_swidth = swidth;
		_sheight = sheight;
		if (rotAngle) {
			_rotAngle = *rotAngle;
			_rotCenter = rotCenter ? *rotCenter : Math::Vec2f(0.5f, 0.5f);
			_rotated = true;
		}
		_hFlip = hFlip;
		_vFlip = vFlip;
	}

	void run(Primitives* primitives, Renderer* rnd, const Project* project, Resources* res) {
		clip(rnd, true);

		do {
			if (!res)
				break;

			Texture::Ptr ptr = nullptr;
			if (_texture)
				ptr = res->load(project, *_texture);
			else
				ptr = primitives->canvas();
			if (!ptr)
				break;

			if (_width <= 0)
				_width = ptr->width();
			if (_height <= 0)
				_height = ptr->height();
			if (_swidth <= 0)
				_swidth = ptr->width();
			if (_sheight <= 0)
				_sheight = ptr->height();
			const Math::Recti dstRect = Math::Recti::byXYWH(_x, _y, _width, _height);
			const Math::Recti srcRect = Math::Recti::byXYWH(_sx, _sy, _swidth, _sheight);

			Color col;
			bool colorChanged = false, alphaChanged = false;
			colored(&col, &colorChanged, &alphaChanged);

			rnd->render(
				ptr.get(),
				&srcRect, &dstRect,
				_rotated ? &_rotAngle : nullptr, &_rotCenter,
				_hFlip, _vFlip,
				&col, colorChanged, alphaChanged
			);
		} while (false);

		clip(rnd, false);
	}
};

class CmdSpr : public Cmd, public CmdClippable, public CmdColored {
private:
	Resources::Sprite::Ptr _sprite = nullptr;
	int _x = 0, _y = 0, _width = -1, _height = -1;
	double _rotAngle = 0.0;
	Math::Vec2f _rotCenter;
	bool _rotated = false;
	double _delta = 0.0;

public:
	CmdSpr() {
		type = SPR;
		dtor = [] (Cmd* cmd) -> void {
			CmdSpr* self = reinterpret_cast<CmdSpr*>(cmd);
			self->~CmdSpr();
		};
	}
	CmdSpr(Resources::Sprite::Ptr spr, int x, int y, int width, int height, const double* rotAngle, const Math::Vec2f* rotCenter, double delta) {
		type = SPR;
		dtor = [] (Cmd* cmd) -> void {
			CmdSpr* self = reinterpret_cast<CmdSpr*>(cmd);
			self->~CmdSpr();
		};

		_sprite = spr;
		_x = x;
		_y = y;
		_width = width;
		_height = height;
		if (rotAngle) {
			_rotAngle = *rotAngle;
			_rotCenter = rotCenter ? *rotCenter : Math::Vec2f(0.5f, 0.5f);
			_rotated = true;
		}
		_delta = delta;
	}

	void run(Renderer* rnd, const Project* project, Resources* res, const double* delta) {
		clip(rnd, true);

		do {
			if (!res)
				break;

			Sprite::Ptr ptr = res->load(project, *_sprite);
			if (!ptr)
				break;

			LockGuard<RecursiveMutex> guard(_sprite->lock);

			ptr->update(delta ? *delta : _delta);

			if (_width <= 0)
				_width = ptr->width();
			if (_height <= 0)
				_height = ptr->height();

			Color col;
			bool colorChanged = false, alphaChanged = false;
			colored(&col, &colorChanged, &alphaChanged);

			ptr->render(
				rnd,
				_x, _y, _width, _height,
				_rotated ? &_rotAngle : nullptr, &_rotCenter,
				&col, colorChanged, alphaChanged
			);
		} while (false);

		clip(rnd, false);
	}
};

class CmdMap : public Cmd, public CmdClippable, public CmdColored {
private:
	Resources::Map::Ptr _map = nullptr;
	int _x = 0, _y = 0;
	double _delta = 0.0;

public:
	CmdMap() {
		type = MAP;
		dtor = [] (Cmd* cmd) -> void {
			CmdMap* self = reinterpret_cast<CmdMap*>(cmd);
			self->~CmdMap();
		};
	}
	CmdMap(Resources::Map::Ptr map, int x, int y, double delta) {
		type = MAP;
		dtor = [] (Cmd* cmd) -> void {
			CmdMap* self = reinterpret_cast<CmdMap*>(cmd);
			self->~CmdMap();
		};

		_map = map;
		_x = x;
		_y = y;
		_delta = delta;
	}

	void run(Renderer* rnd, const Project* project, Resources* res, const double* delta) {
		clip(rnd, true);

		do {
			if (!res)
				break;

			Map::Ptr ptr = res->load(project, *_map);
			if (!ptr)
				break;

			ptr->update(delta ? *delta : _delta);

			Color col;
			bool colorChanged = false, alphaChanged = false;
			colored(&col, &colorChanged, &alphaChanged);

			ptr->render(
				rnd,
				_x, _y,
				&col, colorChanged, alphaChanged
			);
		} while (false);

		clip(rnd, false);
	}
};

class CmdPGet : public Cmd {
private:
	Resources::Palette::Ptr _palette = nullptr;
	int _index = -1;

public:
	CmdPGet() {
		type = PGET;
		dtor = [] (Cmd* cmd) -> void {
			CmdPGet* self = reinterpret_cast<CmdPGet*>(cmd);
			self->~CmdPGet();
		};
	}
	CmdPGet(Resources::Palette::Ptr plt, int idx) {
		type = PGET;
		dtor = [] (Cmd* cmd) -> void {
			CmdPGet* self = reinterpret_cast<CmdPGet*>(cmd);
			self->~CmdPGet();
		};

		_palette = plt;
		_index = idx;
	}

	void wait(Color &col) {
		col = Color();

		if (!_palette)
			return;

		if (_palette->shadow)
			_palette->shadow->get(_index, col);

		const Palette::Ptr &plt = _palette->pointer;
		if (!plt)
			return;

		LockGuard<Mutex> guard(_palette->lock);

		plt->get(_index, col);
	}
};

class CmdPSet : public Cmd {
private:
	Resources::Palette::Ptr _palette = nullptr;
	int _index = -1;
	Color _color;

public:
	CmdPSet() {
		type = PSET;
		dtor = [] (Cmd* cmd) -> void {
			CmdPSet* self = reinterpret_cast<CmdPSet*>(cmd);
			self->~CmdPSet();
		};
	}
	CmdPSet(Resources::Palette::Ptr plt, int idx, const Color &col) {
		type = PSET;
		dtor = [] (Cmd* cmd) -> void {
			CmdPSet* self = reinterpret_cast<CmdPSet*>(cmd);
			self->~CmdPSet();
		};

		_palette = plt;
		_index = idx;
		_color = col;
	}

	void wait(void) {
		if (!_palette)
			return;

		Palette::Ptr shadow = _palette->shadow;
		if (!shadow) {
			const Palette::Ptr &plt = _palette->pointer;
			Palette* ptr = nullptr;

			LockGuard<Mutex> guard(_palette->lock);

			if (plt && plt->clone(&ptr, false) && ptr)
				shadow = _palette->shadow = Palette::Ptr(ptr);
		}

		if (shadow)
			shadow->set(_index, &_color);
	}

	void run(void) {
		if (!_palette)
			return;

		Palette::Ptr plt = _palette->pointer;
		if (!plt)
			return;

		LockGuard<Mutex> guard(_palette->lock);

		plt->set(_index, &_color);
		plt->validate();
	}
};

class CmdMGet : public Cmd {
private:
	Resources::Map::Ptr _map = nullptr;
	int _x = -1;
	int _y = -1;

public:
	CmdMGet() {
		type = MGET;
		dtor = [] (Cmd* cmd) -> void {
			CmdMGet* self = reinterpret_cast<CmdMGet*>(cmd);
			self->~CmdMGet();
		};
	}
	CmdMGet(Resources::Map::Ptr map, int x, int y) {
		type = MGET;
		dtor = [] (Cmd* cmd) -> void {
			CmdMGet* self = reinterpret_cast<CmdMGet*>(cmd);
			self->~CmdMGet();
		};

		_map = map;
		_x = x;
		_y = y;
	}

	void wait(int &cel) {
		cel = Map::INVALID();

		if (!_map)
			return;

		if (_map->shadow) {
			cel = _map->shadow->get(_x, _y);

			return;
		}

		const Map::Ptr &map = _map->pointer;
		if (!map)
			return;

		LockGuard<Mutex> guard(_map->lock);

		cel = map->get(_x, _y);
	}
};

class CmdMSet : public Cmd {
private:
	Resources::Map::Ptr _map = nullptr;
	int _x = -1;
	int _y = -1;
	int _cel = Map::INVALID();

public:
	CmdMSet() {
		type = MSET;
		dtor = [] (Cmd* cmd) -> void {
			CmdMSet* self = reinterpret_cast<CmdMSet*>(cmd);
			self->~CmdMSet();
		};
	}
	CmdMSet(Resources::Map::Ptr map, int x, int y, int cel) {
		type = MSET;
		dtor = [] (Cmd* cmd) -> void {
			CmdMSet* self = reinterpret_cast<CmdMSet*>(cmd);
			self->~CmdMSet();
		};

		_map = map;
		_x = x;
		_y = y;
		_cel = cel;
	}

	void wait(void) {
		if (!_map)
			return;

		Map::Ptr shadow = _map->shadow;
		if (!shadow) {
			const Map::Ptr &map = _map->pointer;
			Map* ptr = nullptr;

			LockGuard<Mutex> guard(_map->lock);

			if (map && map->clone(&ptr, false) && ptr)
				shadow = _map->shadow = Map::Ptr(ptr);
		}

		if (shadow)
			shadow->set(_x, _y, _cel, false);
	}

	void run(void) {
		if (!_map)
			return;

		Map::Ptr map = _map->pointer;
		if (!map)
			return;

		LockGuard<Mutex> guard(_map->lock);

		map->set(_x, _y, _cel, false);
	}
};

class CmdVolume : public Cmd {
private:
	int _sfxVolumeCount = 1;
	union {
		Audio::SfxVolume _sfxVolumes;
		float _sfxVolume = 1;
	};
	float _musicVolume = 1;

public:
	CmdVolume() {
		type = VOLUME;
		dtor = [] (Cmd* cmd) -> void {
			CmdVolume* self = reinterpret_cast<CmdVolume*>(cmd);
			(void)self;
		};
	}
	CmdVolume(float sfxVol, float musicVol) {
		type = VOLUME;
		dtor = [] (Cmd* cmd) -> void {
			CmdVolume* self = reinterpret_cast<CmdVolume*>(cmd);
			(void)self;
		};

		_sfxVolume = sfxVol;
		_musicVolume = musicVol;
	}
	CmdVolume(const Audio::SfxVolume &sfxVol, float musicVol) {
		type = VOLUME;
		dtor = [] (Cmd* cmd) -> void {
			CmdVolume* self = reinterpret_cast<CmdVolume*>(cmd);
			(void)self;
		};

		_sfxVolumeCount = (int)sfxVol.size();
		_sfxVolumes = sfxVol;
		_musicVolume = musicVol;
	}

	void run(Audio* audio) {
		if (audio) {
			if (_sfxVolumeCount == 1)
				audio->sfxVolume(_sfxVolume);
			else
				audio->sfxVolume(_sfxVolumes);
			audio->musicVolume(_musicVolume);
		}
	}
};

class CmdPlaySfx : public Cmd {
private:
	Resources::Sfx::Ptr _sfx = nullptr;
	bool _loop = false;
	int _fadeInMs = -1;
	int _channel = -1;

public:
	CmdPlaySfx() {
		type = PLAY_SFX;
		dtor = [] (Cmd* cmd) -> void {
			CmdPlaySfx* self = reinterpret_cast<CmdPlaySfx*>(cmd);
			self->~CmdPlaySfx();
		};
	}
	CmdPlaySfx(Resources::Sfx::Ptr sfx, bool loop, const int* fadeInMs, int channel) {
		type = PLAY_SFX;
		dtor = [] (Cmd* cmd) -> void {
			CmdPlaySfx* self = reinterpret_cast<CmdPlaySfx*>(cmd);
			self->~CmdPlaySfx();
		};

		_sfx = sfx;
		_loop = loop;
		_fadeInMs = fadeInMs ? *fadeInMs : -1;
		_channel = channel;
	}

	void run(const Project* project, Resources* res) {
		if (!res)
			return;

		Sfx::Ptr ptr = res->load(project, *_sfx);
		if (!ptr)
			return;

		ptr->play(_loop, _fadeInMs <= 0 ? nullptr : &_fadeInMs, _channel);
	}
};

class CmdPlayMusic : public Cmd {
private:
	Resources::Music::Ptr _music = nullptr;
	bool _loop = false;
	int _fadeInMs = -1;

public:
	CmdPlayMusic() {
		type = PLAY_MUSIC;
		dtor = [] (Cmd* cmd) -> void {
			CmdPlayMusic* self = reinterpret_cast<CmdPlayMusic*>(cmd);
			self->~CmdPlayMusic();
		};
	}
	CmdPlayMusic(Resources::Music::Ptr mus, bool loop, const int* fadeInMs) {
		type = PLAY_MUSIC;
		dtor = [] (Cmd* cmd) -> void {
			CmdPlayMusic* self = reinterpret_cast<CmdPlayMusic*>(cmd);
			self->~CmdPlayMusic();
		};

		_music = mus;
		_loop = loop;
		_fadeInMs = fadeInMs ? *fadeInMs : -1;
	}

	void run(const Project* project, Resources* res) {
		if (!res)
			return;

		Music::Ptr ptr = res->load(project, *_music);
		if (!ptr)
			return;

		ptr->play(_loop, _fadeInMs <= 0 ? nullptr : &_fadeInMs);
	}
};

class CmdStopSfx : public Cmd {
private:
	Resources::Sfx::Ptr _sfx = nullptr;
	int _fadeOutMs = -1;

public:
	CmdStopSfx() {
		type = STOP_SFX;
		dtor = [] (Cmd* cmd) -> void {
			CmdStopSfx* self = reinterpret_cast<CmdStopSfx*>(cmd);
			self->~CmdStopSfx();
		};
	}
	CmdStopSfx(Resources::Sfx::Ptr sfx, const int* fadeOutMs) {
		type = STOP_SFX;
		dtor = [] (Cmd* cmd) -> void {
			CmdStopSfx* self = reinterpret_cast<CmdStopSfx*>(cmd);
			self->~CmdStopSfx();
		};

		_sfx = sfx;
		_fadeOutMs = fadeOutMs ? *fadeOutMs : -1;
	}

	void run(const Project* project, Resources* res) {
		if (!res)
			return;

		Sfx::Ptr ptr = res->load(project, *_sfx);
		if (!ptr)
			return;

		ptr->stop(_fadeOutMs <= 0 ? nullptr : &_fadeOutMs);
	}
};

class CmdStopMusic : public Cmd {
private:
	Resources::Music::Ptr _music = nullptr;
	int _fadeOutMs = -1;

public:
	CmdStopMusic() {
		type = STOP_MUSIC;
		dtor = [] (Cmd* cmd) -> void {
			CmdStopMusic* self = reinterpret_cast<CmdStopMusic*>(cmd);
			self->~CmdStopMusic();
		};
	}
	CmdStopMusic(Resources::Music::Ptr mus, const int* fadeOutMs) {
		type = STOP_MUSIC;
		dtor = [] (Cmd* cmd) -> void {
			CmdStopMusic* self = reinterpret_cast<CmdStopMusic*>(cmd);
			self->~CmdStopMusic();
		};

		_music = mus;
		_fadeOutMs = fadeOutMs ? *fadeOutMs : -1;
	}

	void run(const Project* project, Resources* res) {
		if (!res)
			return;

		Music::Ptr ptr = res->load(project, *_music);
		if (!ptr)
			return;

		ptr->stop(_fadeOutMs <= 0 ? nullptr : &_fadeOutMs);
	}
};

class CmdRumble : public Cmd {
private:
	int _index = 0;
	int _lowHz = 100, _hiHz = 100;
	int _ms = -1;

public:
	CmdRumble() {
		type = RUMBLE;
		dtor = [] (Cmd* cmd) -> void {
			CmdRumble* self = reinterpret_cast<CmdRumble*>(cmd);
			(void)self;
		};
	}
	CmdRumble(int idx, int lowHz, int hiHz, int ms) {
		type = RUMBLE;
		dtor = [] (Cmd* cmd) -> void {
			CmdRumble* self = reinterpret_cast<CmdRumble*>(cmd);
			(void)self;
		};

		_index = idx;
		_lowHz = lowHz;
		_hiHz = hiHz;
		_ms = ms;
	}

	void run(Primitives* primitives) {
		Input* input = primitives->input();
		if (_index >= 0)
			input->rumbleGamepad(_index, _lowHz, _hiHz, _ms);
		else
			input->rumbleController(-_index - 1, _lowHz, _hiHz, _ms); // -1-based to 0-based.
	}
};

class CmdCursor : public Cmd {
private:
	Image::Ptr _image = nullptr;
	float _x = 0;
	float _y = 0;

public:
	CmdCursor() {
		type = CURSOR;
		dtor = [] (Cmd* cmd) -> void {
			CmdCursor* self = reinterpret_cast<CmdCursor*>(cmd);
			self->~CmdCursor();
		};
	}
	CmdCursor(Image::Ptr img, float x, float y) {
		type = CURSOR;
		dtor = [] (Cmd* cmd) -> void {
			CmdCursor* self = reinterpret_cast<CmdCursor*>(cmd);
			self->~CmdCursor();
		};

		_image = img;
		_x = x;
		_y = y;
	}

	void run(Primitives* primitives) {
		primitives->indicator(_image, _x, _y);
	}
};

class CmdFunction : public Cmd {
private:
	Primitives::Function _function = nullptr;
	Variant _arg = nullptr;

public:
	CmdFunction() {
		type = FUNCTION;
		dtor = [] (Cmd* cmd) -> void {
			CmdFunction* self = reinterpret_cast<CmdFunction*>(cmd);
			self->~CmdFunction();
		};
	}
	CmdFunction(Primitives::Function func, const Variant &arg) {
		type = FUNCTION;
		dtor = [] (Cmd* cmd) -> void {
			CmdFunction* self = reinterpret_cast<CmdFunction*>(cmd);
			self->~CmdFunction();
		};

		_function = func;
		_arg = arg;
	}

	void run(Primitives*) {
		_function(_arg);
	}
};

union CmdVariant {
public:
	Cmd cmd;
	CmdTarget target;
	CmdCls cls;
	CmdBlend blend;
	CmdPlot plot;
	CmdLine line;
	CmdCirc circ;
	CmdEllipse ellipse;
	CmdRect rect;
	CmdTri tri;
	CmdFont font;
	CmdText text;
	CmdTex tex;
	CmdSpr spr;
	CmdMap map;
	CmdPGet pget;
	CmdPSet pset;
	CmdMGet mget;
	CmdMSet mset;
	CmdVolume volume;
	CmdPlaySfx playSfx;
	CmdPlayMusic playMusic;
	CmdStopSfx stopSfx;
	CmdStopMusic stopMusic;
	CmdRumble rumble;
	CmdCursor cursor;
	CmdFunction function;

public:
	CmdVariant() {
		memset(this, 0, sizeof(CmdVariant));
		new (&cmd) Cmd();
	}
	CmdVariant(const CmdVariant &other) {
		memset(this, 0, sizeof(CmdVariant));
		new (&cmd) Cmd();

		switch (other.cmd.type) {
		case Cmd::TARGET:
			new (&target) CmdTarget();
			target = other.target;

			break;
		case Cmd::CLS:
			new (&cls) CmdCls();
			cls = other.cls;

			break;
		case Cmd::BLEND:
			new (&blend) CmdBlend();
			blend = other.blend;

			break;
		case Cmd::PLOT:
			new (&plot) CmdPlot();
			plot = other.plot;

			break;
		case Cmd::LINE:
			new (&line) CmdLine();
			line = other.line;

			break;
		case Cmd::CIRC:
			new (&circ) CmdCirc();
			circ = other.circ;

			break;
		case Cmd::ELLIPSE:
			new (&ellipse) CmdEllipse();
			ellipse = other.ellipse;

			break;
		case Cmd::RECT:
			new (&rect) CmdRect();
			rect = other.rect;

			break;
		case Cmd::TRI:
			new (&tri) CmdTri();
			tri = other.tri;

			break;
		case Cmd::FONT:
			new (&font) CmdFont();
			font = other.font;

			break;
		case Cmd::TEXT:
			new (&text) CmdText();
			text = other.text;

			break;
		case Cmd::TEX:
			new (&tex) CmdTex();
			tex = other.tex;

			break;
		case Cmd::SPR:
			new (&spr) CmdSpr();
			spr = other.spr;

			break;
		case Cmd::MAP:
			new (&map) CmdMap();
			map = other.map;

			break;
		case Cmd::PGET:
			new (&pget) CmdPGet();
			pget = other.pget;

			break;
		case Cmd::PSET:
			new (&pset) CmdPSet();
			pset = other.pset;

			break;
		case Cmd::MGET:
			new (&mget) CmdMGet();
			mget = other.mget;

			break;
		case Cmd::MSET:
			new (&mset) CmdMSet();
			mset = other.mset;

			break;
		case Cmd::VOLUME:
			new (&volume) CmdVolume();
			volume = other.volume;

			break;
		case Cmd::PLAY_SFX:
			new (&playSfx) CmdPlaySfx();
			playSfx = other.playSfx;

			break;
		case Cmd::PLAY_MUSIC:
			new (&playMusic) CmdPlayMusic();
			playMusic = other.playMusic;

			break;
		case Cmd::STOP_SFX:
			new (&stopSfx) CmdStopSfx();
			stopSfx = other.stopSfx;

			break;
		case Cmd::STOP_MUSIC:
			new (&stopMusic) CmdStopMusic();
			stopMusic = other.stopMusic;

			break;
		case Cmd::RUMBLE:
			new (&rumble) CmdRumble();
			rumble = other.rumble;

			break;
		case Cmd::CURSOR:
			new (&cursor) CmdCursor();
			cursor = other.cursor;

			break;
		case Cmd::FUNCTION:
			new (&function) CmdFunction();
			function = other.function;

			break;
		default:
			assert(false && "Not implemented.");

			break;
		}
	}
	~CmdVariant() {
		cmd.dtor(&cmd);

		memset(this, 0, sizeof(CmdVariant));
		new (&cmd) Cmd();
	}

	CmdVariant &operator = (const CmdVariant &other) {
		cmd.dtor(&cmd);

		memset(this, 0, sizeof(CmdVariant));
		new (&cmd) Cmd();

		switch (other.cmd.type) {
		case Cmd::TARGET:
			new (&target) CmdTarget();
			target = other.target;

			break;
		case Cmd::CLS:
			new (&cls) CmdCls();
			cls = other.cls;

			break;
		case Cmd::BLEND:
			new (&blend) CmdBlend();
			blend = other.blend;

			break;
		case Cmd::PLOT:
			new (&plot) CmdPlot();
			plot = other.plot;

			break;
		case Cmd::LINE:
			new (&line) CmdLine();
			line = other.line;

			break;
		case Cmd::CIRC:
			new (&circ) CmdCirc();
			circ = other.circ;

			break;
		case Cmd::ELLIPSE:
			new (&ellipse) CmdEllipse();
			ellipse = other.ellipse;

			break;
		case Cmd::RECT:
			new (&rect) CmdRect();
			rect = other.rect;

			break;
		case Cmd::TRI:
			new (&tri) CmdTri();
			tri = other.tri;

			break;
		case Cmd::FONT:
			new (&font) CmdFont();
			font = other.font;

			break;
		case Cmd::TEXT:
			new (&text) CmdText();
			text = other.text;

			break;
		case Cmd::TEX:
			new (&tex) CmdTex();
			tex = other.tex;

			break;
		case Cmd::SPR:
			new (&spr) CmdSpr();
			spr = other.spr;

			break;
		case Cmd::MAP:
			new (&map) CmdMap();
			map = other.map;

			break;
		case Cmd::PGET:
			new (&pget) CmdPGet();
			pget = other.pget;

			break;
		case Cmd::PSET:
			new (&pset) CmdPSet();
			pset = other.pset;

			break;
		case Cmd::MGET:
			new (&mget) CmdMGet();
			mget = other.mget;

			break;
		case Cmd::MSET:
			new (&mset) CmdMSet();
			mset = other.mset;

			break;
		case Cmd::VOLUME:
			new (&volume) CmdVolume();
			volume = other.volume;

			break;
		case Cmd::PLAY_SFX:
			new (&playSfx) CmdPlaySfx();
			playSfx = other.playSfx;

			break;
		case Cmd::PLAY_MUSIC:
			new (&playMusic) CmdPlayMusic();
			playMusic = other.playMusic;

			break;
		case Cmd::STOP_SFX:
			new (&stopSfx) CmdStopSfx();
			stopSfx = other.stopSfx;

			break;
		case Cmd::STOP_MUSIC:
			new (&stopMusic) CmdStopMusic();
			stopMusic = other.stopMusic;

			break;
		case Cmd::RUMBLE:
			new (&rumble) CmdRumble();
			rumble = other.rumble;

			break;
		case Cmd::CURSOR:
			new (&cursor) CmdCursor();
			cursor = other.cursor;

			break;
		case Cmd::FUNCTION:
			new (&function) CmdFunction();
			function = other.function;

			break;
		default:
			assert(false && "Not implemented.");

			break;
		}

		return *this;
	}

	void run(Primitives* primitives, Renderer* rnd, const Project* project, Resources* res, Audio* audio, const double* delta) {
		switch (cmd.type) {
		case Cmd::TARGET:
			target.run(primitives, rnd, project, res);

			break;
		case Cmd::CLS:
			cls.run(rnd);

			break;
		case Cmd::BLEND:
			blend.run(rnd, project, res);

			break;
		case Cmd::PLOT:
			plot.run(rnd);

			break;
		case Cmd::LINE:
			line.run(rnd);

			break;
		case Cmd::CIRC:
			circ.run(rnd);

			break;
		case Cmd::ELLIPSE:
			ellipse.run(rnd);

			break;
		case Cmd::RECT:
			rect.run(rnd);

			break;
		case Cmd::TRI:
			tri.run(rnd, project, res);

			break;
		case Cmd::FONT:
			font.run(rnd, res);

			break;
		case Cmd::TEXT:
			text.run(rnd, res);

			break;
		case Cmd::TEX:
			tex.run(primitives, rnd, project, res);

			break;
		case Cmd::SPR:
			spr.run(rnd, project, res, delta);

			break;
		case Cmd::MAP:
			map.run(rnd, project, res, delta);

			break;
		case Cmd::PGET:
			assert(false && "Impossible.");

			break;
		case Cmd::PSET:
			pset.run();

			break;
		case Cmd::MGET:
			assert(false && "Impossible.");

			break;
		case Cmd::MSET:
			mset.run();

			break;
		case Cmd::VOLUME:
			volume.run(audio);

			break;
		case Cmd::PLAY_SFX:
			playSfx.run(project, res);

			break;
		case Cmd::PLAY_MUSIC:
			playMusic.run(project, res);

			break;
		case Cmd::STOP_SFX:
			stopSfx.run(project, res);

			break;
		case Cmd::STOP_MUSIC:
			stopMusic.run(project, res);

			break;
		case Cmd::RUMBLE:
			rumble.run(primitives);

			break;
		case Cmd::CURSOR:
			cursor.run(primitives);

			break;
		case Cmd::FUNCTION:
			function.run(primitives);

			break;
		default:
			assert(false && "Not implemented.");

			break;
		}
	}
};

/* ===========================================================================} */

/*
** {===========================================================================
** Primitive command queue
*/

/**
 * @brief Queue for primitive commands.
 */
class CmdQueue {
private:
	typedef std::vector<CmdVariant> Cmds;

private:
	Cmds _cmds;

public:
	/**
	 * @brief Runs through all commands in the queue.
	 */
	void run(Primitives* primitives, Renderer* rnd, const Project* project, Resources* res, Audio* audio, const double* delta) {
		for (CmdVariant &var : _cmds)
			var.run(primitives, rnd, project, res, audio, delta);
	}

	/**
	 * @brief Takes all commands from another queue into this queue.
	 */
	void take(CmdQueue &other) {
		std::copy(std::begin(other._cmds), std::end(other._cmds), std::back_inserter(_cmds));

		other.clear(false);
	}
	/**
	 * @brief Adds a command into queue.
	 */
	void add(const CmdVariant &cmd) {
		_cmds.push_back(cmd);
	}
	/**
	 * @brief Gets command count.
	 */
	size_t size(void) const {
		return _cmds.size();
	}

	/**
	 * @brief Clears all commands.
	 */
	void clear(bool shrink) {
		_cmds.clear();

		if (shrink)
			_cmds.shrink_to_fit();
	}
};

/**
 * @brief Producer-consumer queue for primitive commands.
 */
class CmdBuffer {
private:
	CmdQueue _consuming;
	CmdQueue _producing;
	CmdQueue _discarded;

	bool _blocking = false;
	bool _syncing = false;
	bool _forbidden = false;

	Mutex _lock;

public:
	/**
	 * @brief Consumes a queue of commands.
	 */
	void pop(CmdQueue &q) {
		LockGuard<decltype(_lock)> guard(_lock);

		q = _consuming;
		_discarded.clear(false);

		_syncing = false;
	}

	/**
	 * @brief Produces a command into queue.
	 */
	void add(const CmdVariant &cmd) {
		_producing.add(cmd);
	}
	/**
	 * @brief Produces a command into queue.
	 */
	void add(const CmdVariant &cmd, bool block) {
		_producing.add(cmd);

		LockGuard<decltype(_lock)> guard(_lock);

		_blocking |= block;
	}
	/**
	 * @brief Commits the producing queue to consuming, and gets ready for future producing.
	 */
	int commit(void) {
		_lock.lock();

		const bool blocking = _blocking;
		_blocking = false;
		if (blocking) {
			_lock.unlock();

			return sync();
		}

		std::swap(_consuming, _producing);
		_discarded.take(_producing);

		const int result = (int)_consuming.size();

		_lock.unlock();

		return result;
	}
	/**
	 * @brief Synchronizes the producing queue to consuming, and waits until it's consumed.
	 */
	int sync(void) {
		int result = 0;
		do {
			LockGuard<decltype(_lock)> guard(_lock);

			if (_forbidden)
				return result;

			std::swap(_consuming, _producing);
			_discarded.take(_producing);

			result = (int)_consuming.size();

			_syncing = true;
		} while (false);

		while (true) {
			if (_lock.tryLock()) {
				const bool syncing = _syncing;
				_lock.unlock();
				if (!syncing)
					break;
			}

			constexpr const int STEP = 1;
			DateTime::sleep(STEP);
		}

		return result;
	}

	/**
	 * @brief Forbids command synchronizing.
	 */
	void forbid(void) {
		LockGuard<decltype(_lock)> guard(_lock);

		_consuming.clear(true);
		_discarded.clear(true);

		_blocking = false;
		_syncing = false;
		_forbidden = true;
	}
	/**
	 * @brief Clears all queues.
	 */
	void reset(void) {
		LockGuard<decltype(_lock)> guard(_lock);

		_consuming.clear(true);
		_producing.clear(true);
		_discarded.clear(true);

		_blocking = false;
		_syncing = false;
		_forbidden = false;
	}
};

/* ===========================================================================} */

/*
** {===========================================================================
** Primitives
*/

class PrimitivesImpl : public Primitives {
private:
	bool _opened = false;

	Window* _window = nullptr; // Foreign.
	Renderer* _renderer = nullptr; // Foreign.
	const Project* _project = nullptr; // Foreign.
	Resources* _resources = nullptr; // Foreign.
	class Effects* _effects = nullptr; // Foreign.
	Audio* _audio = nullptr;
	Input* _input = nullptr;

	Texture::Ptr _canvas = nullptr; // By the graphics thread.
	Resources::Texture::Ptr _canvasTarget = nullptr;
	unsigned _canvasBlend = SDL_BLENDMODE_BLEND; // By the graphics thread.
	Math::Vec2i _canvasCull = Math::Vec2i(BITTY_CANVAS_DEFAULT_WIDTH, BITTY_CANVAS_DEFAULT_HEIGHT);
	Math::Vec2i _canvasSize = Math::Vec2i(BITTY_CANVAS_DEFAULT_WIDTH, BITTY_CANVAS_DEFAULT_HEIGHT); // By the graphics thread.
	Mutex _canvasSizeLock;

	Image::Ptr _indicatorImage = nullptr;
	float _indicatorX = 0;
	float _indicatorY = 0;
	SDL_Cursor* _indicatorCursor = nullptr;

	Color _clsColor = Color(30, 30, 30, 255);
	bool _autoCls = true;

	unsigned _blend = SDL_BLENDMODE_BLEND;
	bool _blendChanged = false;

	Math::Vec2i _camera;
	bool _cameraChanged = false;

	Math::Recti _clip;
	bool _clipChanged = false;

	Color _color;

	mutable Font::Ptr _measurer = nullptr;

#if BITTY_MULTITHREAD_ENABLED
	mutable CmdBuffer _buffer;
#else /* BITTY_MULTITHREAD_ENABLED */
	mutable int _commited = 0;
#endif /* BITTY_MULTITHREAD_ENABLED */
	mutable unsigned _commands = 0;

	Resources::List<Resources::Asset::Ptr> _loads;
	Resources::List<Resources::Asset::Ptr> _unloads;

	Resources::List<Object::Ptr> _disposing;
	bool _collect = false;

public:
	PrimitivesImpl(bool withAudio) {
		if (withAudio)
			_audio = Audio::create();

		_input = Input::create();
	}
	virtual ~PrimitivesImpl() {
		Input::destroy(_input);
		_input = nullptr;

		if (_audio) {
			Audio::destroy(_audio);
			_audio = nullptr;
		}
	}

	virtual bool open(class Window* wnd, class Renderer* rnd, const class Project* project, Resources* res, class Effects* effects) override {
		if (_opened)
			return false;
		_opened = true;

		_window = wnd;
		_renderer = rnd;
		_project = project;
		_resources = res;
		_effects = effects;
		if (_audio)
			_audio->open();
		_input->open();

		fprintf(stdout, "Primitives opened.\n");

		return true;
	}
	virtual bool close(void) override {
		if (!_opened)
			return false;
		_opened = false;

		_input->close();
		if (_audio)
			_audio->close();
		_effects = nullptr;
		_resources = nullptr;
		_project = nullptr;
		_renderer = nullptr;
		_window = nullptr;

		fprintf(stdout, "Primitives closed.\n");

		return true;
	}

	virtual class Window* window(void) override {
		return _window;
	}

	virtual class Effects* effects(void) override {
		return _effects;
	}

	virtual Input* input(void) override {
		return _input;
	}

	virtual unsigned commands(void) const override {
		return _commands;
	}

	virtual Resources::Texture::Ptr target(void) const override {
		return _canvasTarget;
	}
	virtual void target(Resources::Texture::Ptr tex) override {
		_canvasTarget = tex;

		CmdVariant var;
		new (&var.target) CmdTarget(tex);

		commit(var, nullptr, true);
	}

	virtual bool autoCls(void) const override {
		return _autoCls;
	}
	virtual void autoCls(bool cls) override {
		_autoCls = cls;
	}

	virtual Color cls(const Color* col) override {
		const Color oldCol = _clsColor;
		if (col)
			_clsColor = *col;

		CmdVariant var;
		new (&var.cls) CmdCls(_clsColor);

		if (_autoCls)
			commit(var, nullptr);
		else
			commit(var, nullptr, true);

		return oldCol;
	}
	virtual void blend(Resources::Texture::Ptr tex, unsigned mode) override {
		CmdVariant var;
		new (&var.blend) CmdBlend(tex, (SDL_BlendMode)mode);

		commit(var, nullptr, true);
	}
	virtual void blend(unsigned mode) override {
		_blend = mode;
		_blendChanged = mode != SDL_BLENDMODE_BLEND;

		CmdVariant var;
		new (&var.blend) CmdBlend((SDL_BlendMode)mode);

		commit(var, nullptr, true);
	}
	virtual void blend(void) override {
		_blend = SDL_BLENDMODE_BLEND;
		_blendChanged = false;

		CmdVariant var;
		new (&var.blend) CmdBlend();

		commit(var, nullptr, true);
	}
	virtual bool camera(int* x, int* y) const override {
		if (x)
			*x = _camera.x;
		if (y)
			*y = _camera.y;

		return _cameraChanged;
	}
	virtual void camera(int x, int y) override {
		_camera = Math::Vec2i(x, y);
		_cameraChanged = _camera.x != 0 || _camera.y != 0;
	}
	virtual void camera(void) override {
		_camera = Math::Vec2i();
		_cameraChanged = false;
	}
	virtual bool clip(int* x, int* y, int* width, int* height) const override {
		if (x)
			*x = _clip.xMin();
		if (y)
			*y = _clip.yMin();
		if (width)
			*width = _clip.width();
		if (height)
			*height = _clip.height();

		return _clipChanged;
	}
	virtual void clip(int x, int y, int width, int height) override {
		translated(x, y);

		_clip = Math::Recti::byXYWH(x, y, width, height);
		_clipChanged = true;
	}
	virtual void clip(void) override {
		_clip = Math::Recti();
		_clipChanged = false;
	}
	virtual Color color(const Color* col) override {
		const Color oldCol = _color;
		if (col)
			_color = *col;
		else
			_color = Color();

		return oldCol;
	}
	virtual Color color(void) override {
		const Color oldCol = _color;
		_color = Color();

		return oldCol;
	}
	virtual void plot(int x, int y, const Color* col) const override {
		translated(x, y);

		if (culled(Math::Vec2i(x, y)))
			return;

		CmdVariant var;
		new (&var.plot) CmdPlot(x, y, col ? *col : _color);
		int clpX = 0, clpY = 0, clpW = 0, clpH = 0;
		if (clipped(clpX, clpY, clpW, clpH))
			var.plot.clip(clpX, clpY, clpW, clpH);

		commit(var, nullptr);
	}
	virtual void line(int x0, int y0, int x1, int y1, const Color* col) const override {
		translated(x0, y0);
		translated(x1, y1);

		const Math::Recti aabb(x0, y0, x1, y1);
		if (culled(aabb))
			return;

		CmdVariant var;
		new (&var.line) CmdLine(x0, y0, x1, y1, col ? *col : _color);
		int clpX = 0, clpY = 0, clpW = 0, clpH = 0;
		if (clipped(clpX, clpY, clpW, clpH))
			var.line.clip(clpX, clpY, clpW, clpH);

		commit(var, nullptr);
	}
	virtual void circ(int x, int y, int r, bool fill, const Color* col) const override {
		translated(x, y);

		const Math::Recti aabb(x - r, y - r, x + r, y + r);
		if (culled(aabb))
			return;

		CmdVariant var;
		new (&var.circ) CmdCirc(x, y, r, fill, col ? *col : _color);
		int clpX = 0, clpY = 0, clpW = 0, clpH = 0;
		if (clipped(clpX, clpY, clpW, clpH))
			var.circ.clip(clpX, clpY, clpW, clpH);

		commit(var, nullptr);
	}
	virtual void ellipse(int x, int y, int rx, int ry, bool fill, const Color* col) const override {
		translated(x, y);

		const Math::Recti aabb(x - rx, y - ry, x + rx, y + ry);
		if (culled(aabb))
			return;

		CmdVariant var;
		new (&var.ellipse) CmdEllipse(x, y, rx, ry, fill, col ? *col : _color);
		int clpX = 0, clpY = 0, clpW = 0, clpH = 0;
		if (clipped(clpX, clpY, clpW, clpH))
			var.ellipse.clip(clpX, clpY, clpW, clpH);

		commit(var, nullptr);
	}
	virtual void rect(int x0, int y0, int x1, int y1, bool fill, const Color* col, const int* rad) const override {
		translated(x0, y0);
		translated(x1, y1);

		const Math::Recti aabb(x0, y0, x1, y1);
		if (culled(aabb))
			return;

		CmdVariant var;
		new (&var.rect) CmdRect(x0, y0, x1, y1, fill, col ? *col : _color, rad);
		int clpX = 0, clpY = 0, clpW = 0, clpH = 0;
		if (clipped(clpX, clpY, clpW, clpH))
			var.rect.clip(clpX, clpY, clpW, clpH);

		commit(var, nullptr);
	}
	virtual void tri(const Math::Vec2f &p0, const Math::Vec2f &p1, const Math::Vec2f &p2, bool fill, const Color* col) const override {
		Math::Vec2f p0_ = p0, p1_ = p1, p2_ = p2;
		translated(p0_.x, p0_.y);
		translated(p1_.x, p1_.y);
		translated(p2_.x, p2_.y);

		Math::Recti aabb((Int)std::round(p0_.x), (Int)std::round(p0_.y), (Int)std::round(p1_.x), (Int)std::round(p1_.y));
		aabb = aabb + Math::Vec2i((Int)std::round(p2_.x), (Int)std::round(p2_.y));
		if (culled(aabb))
			return;

		CmdVariant var;
		new (&var.tri) CmdTri(p0_, p1_, p2_, fill, col ? *col : _color);
		int clpX = 0, clpY = 0, clpW = 0, clpH = 0;
		if (clipped(clpX, clpY, clpW, clpH))
			var.tri.clip(clpX, clpY, clpW, clpH);

		commit(var, nullptr);
	}
	virtual void tri(const Math::Vec2f &p0, const Math::Vec2f &p1, const Math::Vec2f &p2, Resources::Texture::Ptr tex, const Math::Vec2f &uv0, const Math::Vec2f &uv1, const Math::Vec2f &uv2) const override {
		if (!tex)
			return;

		Math::Vec2f p0_ = p0, p1_ = p1, p2_ = p2;
		translated(p0_.x, p0_.y);
		translated(p1_.x, p1_.y);
		translated(p2_.x, p2_.y);

		Math::Recti aabb((Int)std::round(p0_.x), (Int)std::round(p0_.y), (Int)std::round(p1_.x), (Int)std::round(p1_.y));
		aabb = aabb + Math::Vec2i((Int)std::round(p2_.x), (Int)std::round(p2_.y));
		if (culled(aabb))
			return;

		CmdVariant var;
		new (&var.tri) CmdTri(p0_, p1_, p2_, tex, uv0, uv1, uv2);
		int clpX = 0, clpY = 0, clpW = 0, clpH = 0;
		if (clipped(clpX, clpY, clpW, clpH))
			var.tri.clip(clpX, clpY, clpW, clpH);

		commit(var, nullptr);
	}
	virtual void font(Font::Ptr font) override {
		CmdVariant var;
		new (&var.font) CmdFont(font);

		commit(var, nullptr, true);
	}
	virtual void font(void) override {
		CmdVariant var;
		new (&var.font) CmdFont();

		commit(var, nullptr, true);
	}
	virtual Math::Vec2f measure(const char* text, Font::Ptr font, int margin) const override {
		Math::Vec2f result;

		if (!font) {
			if (!_measurer) {
				_measurer = Font::Ptr(Font::create());
				_measurer->fromBytes(RES_FONT_PROGGY_CLEAN, BITTY_COUNTOF(RES_FONT_PROGGY_CLEAN), RESOURCES_FONT_DEFAULT_SIZE, 0);
			}
			font = _measurer;
		}

		if (!font)
			return result;

		std::wstring wstr = Unicode::toWide(text);
		const wchar_t* wtext = wstr.c_str();
		int x = 0;
		while (*wtext) {
			const Resources::Id cp = *wtext++;

			int width = -1, height = -1;
			if (!font->measure(cp, &width, &height))
				continue;

			x += width;
			if (*wtext)
				x += margin;

			result.x += width;
			if (*wtext)
				result.x += margin;
			if (height > result.y)
				result.y = height;
		}

		return result;
	}
	virtual void text(const char* text, int x, int y, const Color* col, int margin) const override {
		if (!text)
			return;

		translated(x, y);

		CmdVariant var;
		new (&var.text) CmdText(text, x, y, margin);
		int clpX = 0, clpY = 0, clpW = 0, clpH = 0;
		if (clipped(clpX, clpY, clpW, clpH))
			var.text.clip(clpX, clpY, clpW, clpH);
		var.text.colored(col ? *col : _color);

		commit(var, nullptr);
	}
	virtual void tex(Resources::Texture::Ptr tex, int x, int y, int width, int height, int sx, int sy, int swidth, int sheight, const double* rotAngle, const Math::Vec2f* rotCenter, bool hFlip, bool vFlip, const Color* col) const override {
		translated(x, y);

		const Math::Recti aabb(x, y, x + width, y + height);
		if ((!rotAngle || *rotAngle == 0) && width && height && culled(aabb))
			return;

		CmdVariant var;
		new (&var.tex) CmdTex(tex, x, y, width, height, sx, sy, swidth, sheight, rotAngle, rotCenter, hFlip, vFlip);
		int clpX = 0, clpY = 0, clpW = 0, clpH = 0;
		if (clipped(clpX, clpY, clpW, clpH))
			var.tex.clip(clpX, clpY, clpW, clpH);
		if (col)
			var.tex.colored(*col);

		commit(var, nullptr);
	}
	virtual void spr(Resources::Sprite::Ptr spr, int x, int y, int width, int height, const double* rotAngle, const Math::Vec2f* rotCenter, double delta, const Color* col) const override {
		if (!spr)
			return;

		translated(x, y);

		CmdVariant var;
		new (&var.spr) CmdSpr(spr, x, y, width, height, rotAngle, rotCenter, delta);
		int clpX = 0, clpY = 0, clpW = 0, clpH = 0;
		if (clipped(clpX, clpY, clpW, clpH))
			var.spr.clip(clpX, clpY, clpW, clpH);
		if (col)
			var.spr.colored(*col);

		commit(var, nullptr);
	}
	virtual void map(Resources::Map::Ptr map, int x, int y, double delta, const Color* col) const override {
		if (!map)
			return;

		translated(x, y);

		CmdVariant var;
		new (&var.map) CmdMap(map, x, y, delta);
		int clpX = 0, clpY = 0, clpW = 0, clpH = 0;
		if (clipped(clpX, clpY, clpW, clpH))
			var.map.clip(clpX, clpY, clpW, clpH);
		if (col)
			var.map.colored(*col);

		commit(var, nullptr);
	}
	virtual void pget(Resources::Palette::Ptr plt, int idx, Color &col) const override {
		CmdVariant var;
		new (&var.pget) CmdPGet(plt, idx);

		var.pget.wait(col);
	}
	virtual void pset(Resources::Palette::Ptr plt, int idx, const Color &col) override {
		CmdVariant var;
		new (&var.pset) CmdPSet(plt, idx, col);

		var.pset.wait();

		commit(var, nullptr, true);
	}
	virtual void mget(Resources::Map::Ptr map, int x, int y, int &cel) const override {
		CmdVariant var;
		new (&var.mget) CmdMGet(map, x, y);

		var.mget.wait(cel);
	}
	virtual void mset(Resources::Map::Ptr map, int x, int y, int cel) override {
		CmdVariant var;
		new (&var.mset) CmdMSet(map, x, y, cel);

		var.mset.wait();

		commit(var, nullptr, true);
	}

	virtual void volume(const Audio::SfxVolume &sfxVol, float musicVol) const override {
		Audio::SfxVolume sfxVol_ = sfxVol;
		for (int i = 0; i < AUDIO_SFX_CHANNEL_COUNT; ++i)
			sfxVol_[i] = std::min(sfxVol_[i], 1.0f);
		musicVol = std::min(musicVol, 1.0f);

		CmdVariant var;
		new (&var.volume) CmdVolume(sfxVol, musicVol);

		commit(var, nullptr, true);
	}
	virtual void volume(float sfxVol, float musicVol) const override {
		sfxVol = std::min(sfxVol, 1.0f);
		musicVol = std::min(musicVol, 1.0f);

		CmdVariant var;
		new (&var.volume) CmdVolume(sfxVol, musicVol);

		commit(var, nullptr, true);
	}
	virtual void play(Resources::Sfx::Ptr sfx, bool loop, const int* fadeInMs, int channel) const override {
		if (!sfx)
			return;

		CmdVariant var;
		new (&var.playSfx) CmdPlaySfx(sfx, loop, fadeInMs, channel);

		commit(var, nullptr, true);
	}
	virtual void play(Resources::Music::Ptr mus, bool loop, const int* fadeInMs) const override {
		if (!mus)
			return;

		CmdVariant var;
		new (&var.playMusic) CmdPlayMusic(mus, loop, fadeInMs);

		commit(var, nullptr, true);
	}
	virtual void stop(Resources::Sfx::Ptr sfx, const int* fadeOutMs) const override {
		if (!sfx)
			return;

		CmdVariant var;
		new (&var.stopSfx) CmdStopSfx(sfx, fadeOutMs);

		commit(var, nullptr, true);
	}
	virtual void stop(Resources::Music::Ptr mus, const int* fadeOutMs) const override {
		if (!mus)
			return;

		CmdVariant var;
		new (&var.stopMusic) CmdStopMusic(mus, fadeOutMs);

		commit(var, nullptr, true);
	}

	virtual int btn(int btn, int idx) const override {
		if (idx >= 0)
			return _input->buttonDown(btn, idx);

		return _input->controllerDown(btn, -idx - 1); // -1-based to 0-based.
	}
	virtual int btnp(int btn, int idx) const override {
		if (idx >= 0)
			return _input->buttonUp(btn, idx);

		return _input->controllerUp(btn, -idx - 1); // -1-based to 0-based.
	}
	virtual void rumble(int idx, int lowHz, int hiHz, unsigned ms) const override {
		CmdVariant var;
		new (&var.rumble) CmdRumble(idx, lowHz, hiHz, ms);

		commit(var, nullptr, true);
	}
	virtual bool key(int key) const override {
		return _input->keyDown(key);
	}
	virtual bool keyp(int key) const override {
		return _input->keyUp(key);
	}
	virtual bool mouse(int btn, int* x, int* y, bool* b0, bool* b1, bool* b2, int* wheelX, int* wheelY) const override {
		return _input->mouse(btn, x, y, b0, b1, b2, wheelX, wheelY);
	}
	virtual void cursor(Image::Ptr img, float x, float y) const override {
		CmdVariant var;
		new (&var.cursor) CmdCursor(img, x, y);

		commit(var, nullptr, true);
	}
	virtual void function(Function func, const Variant &arg, bool block) const override {
		if (!func)
			return;

		CmdVariant var;
		new (&var.function) CmdFunction(func, arg);

		if (block)
			commit(var, nullptr, block);
		else
			commit(var, nullptr);
	}

	virtual int newFrame(void) override {
		int result = 0;

#if !BITTY_MULTITHREAD_ENABLED
		saveStates();
#endif /* BITTY_MULTITHREAD_ENABLED */

		if (_canvasTarget) {
			CmdVariant var;
			new (&var.target) CmdTarget(_canvasTarget);

			commit(var, nullptr, true);

			++result;
		}

		if (_autoCls) {
			CmdVariant var;
			new (&var.cls) CmdCls(_clsColor);

			commit(var, nullptr);

			++result;
		}

		if (_blendChanged) {
			CmdVariant var;
			new (&var.blend) CmdBlend((SDL_BlendMode)_blend);

			commit(var, nullptr, true);

			++result;
		}

		return result;
	}
	virtual int commit(void) override {
		do {
			LockGuard<decltype(_canvasSizeLock)> guardCanvasSize(_canvasSizeLock);

			_canvasCull = _canvasSize;
		} while (false);

		_input->sync();

#if BITTY_MULTITHREAD_ENABLED
		return _buffer.commit();
#else /* BITTY_MULTITHREAD_ENABLED */
		const int result = _commited;
		_commited = 0;
		_commands = 0;

		restoreStates();

		return result;
#endif /* BITTY_MULTITHREAD_ENABLED */
	}
	virtual int sync(void) override {
#if BITTY_MULTITHREAD_ENABLED
		return _buffer.sync();
#else /* BITTY_MULTITHREAD_ENABLED */
		const int result = _commited;

		return result;
#endif /* BITTY_MULTITHREAD_ENABLED */
	}

	virtual bool load(const Resources::Asset::Ptr &res) override {
		// Prepare.
		if (!res)
			return false;

		// Discard if already pending for unloading.
		do {
			LockGuard<decltype(_unloads.lock)> guardLoads(_unloads.lock);

			Resources::List<Resources::Asset::Ptr>::Iterator it = std::find_if(
				_unloads.begin(), _unloads.end(),
				[&] (const Resources::Asset::Ptr &ptr) -> bool {
					return ptr == res;
				}
			);
			if (it == _unloads.end())
				break;

			_unloads.remove(it); // Cancel loading.
		} while (false);

		// Schedule for loading.
		LockGuard<decltype(_loads.lock)> guardLoads(_loads.lock);

		Resources::List<Resources::Asset::Ptr>::Iterator it = std::find_if(
			_loads.begin(), _loads.end(),
			[&] (const Resources::Asset::Ptr &ptr) -> bool {
				return ptr == res;
			}
		);
		if (it != _loads.end())
			return false;

		_loads.add(res);

		// Finish.
#if !BITTY_MULTITHREAD_ENABLED
		processResourceLoadingAndUnloading(); // Process instantly for single thread build.
#endif /* BITTY_MULTITHREAD_ENABLED */

		return true;
	}
	virtual bool unload(const Resources::Asset::Ptr &res) override {
		// Prepare.
		if (!res)
			return false;

		// Discard if already pending for loading.
		do {
			LockGuard<decltype(_loads.lock)> guardLoads(_loads.lock);

			Resources::List<Resources::Asset::Ptr>::Iterator it = std::find_if(
				_loads.begin(), _loads.end(),
				[&] (const Resources::Asset::Ptr &ptr) -> bool {
					return ptr == res;
				}
			);
			if (it == _loads.end())
				break;

			_loads.remove(it); // Cancel loading.
		} while (false);

		// Schedule for unloading.
		LockGuard<decltype(_unloads.lock)> guardUnloads(_unloads.lock);

		Resources::List<Resources::Asset::Ptr>::Iterator it = std::find_if(
			_unloads.begin(), _unloads.end(),
			[&] (const Resources::Asset::Ptr &ptr) -> bool {
				return ptr == res;
			}
		);
		if (it != _unloads.end())
			return false;

		_unloads.add(res);

		// Finish.
#if !BITTY_MULTITHREAD_ENABLED
		processResourceLoadingAndUnloading(); // Process instantly for single thread build.
#endif /* BITTY_MULTITHREAD_ENABLED */

		return true;
	}

	virtual bool dispose(const Object::Ptr &obj) override {
		// Prepare.
		if (!obj)
			return false;

		// Schedule for disposing.
		LockGuard<decltype(_disposing.lock)> guardDisposing(_disposing.lock);

		Resources::List<Object::Ptr>::Iterator it = std::find_if(
			_disposing.begin(), _disposing.end(),
			[&] (const Object::Ptr &ptr) -> bool {
				return ptr == obj;
			}
		);
		if (it != _disposing.end())
			return false;

		_disposing.add(obj);

		// Finish.
		return true;
	}
	virtual void collect(void) override {
		// Schedule for collecting.
		LockGuard<decltype(_unloads.lock)> guardUnloads(_unloads.lock);

		_collect = true;
	}

	virtual Texture::Ptr canvas(void) const override {
		return _canvas;
	}
	virtual void canvas(Texture::Ptr tex) override {
		_canvas = tex;
	}

	virtual Image::Ptr indicator(float* x, float* y) const override {
		if (x)
			*x = _indicatorX;
		if (y)
			*y = _indicatorY;

		return _indicatorImage;
	}
	virtual void indicator(Image::Ptr img, float x, float y) override {
		_indicatorImage = nullptr;
		_indicatorX = 0;
		_indicatorY = 0;
		if (_indicatorCursor) {
			SDL_FreeCursor(_indicatorCursor);
			_indicatorCursor = nullptr;
		}

		if (img) {
			_indicatorImage = img;
			_indicatorX = x;
			_indicatorY = y;

			SDL_Surface* sur = (SDL_Surface*)_indicatorImage->pointer();
			if (sur) {
				const int x_ = Math::clamp((int)(_indicatorImage->width() * x), 0, _indicatorImage->width() - 1);
				const int y_ = Math::clamp((int)(_indicatorImage->height() * y), 0, _indicatorImage->height() - 1);
				_indicatorCursor = SDL_CreateColorCursor(sur, x_, y_);
			}
		}
	}

	virtual void forbid(void) override {
#if BITTY_MULTITHREAD_ENABLED
		_buffer.forbid();

		processResourceLoadingAndUnloading();

		processResourceDisposingAndCollecting();
#else /* BITTY_MULTITHREAD_ENABLED */
		processResourceDisposingAndCollecting();
#endif /* BITTY_MULTITHREAD_ENABLED */
	}
	virtual void reset(void) override {
		_canvas = nullptr;
		_canvasTarget = nullptr;

		_indicatorImage = nullptr;
		_indicatorX = 0;
		_indicatorY = 0;
		if (_indicatorCursor) {
			SDL_FreeCursor(_indicatorCursor);
			_indicatorCursor = nullptr;
		}

		_clsColor = Color(30, 30, 30, 255);
		_autoCls = true;

		_blend = SDL_BLENDMODE_BLEND;
		_blendChanged = false;
		CmdBlend blend;
		blend.run(_renderer, _project, _resources);

		_camera = Math::Vec2i();
		_cameraChanged = false;

		_clip = Math::Recti();
		_clipChanged = false;

		_color = Color();

		_measurer = nullptr;

#if BITTY_MULTITHREAD_ENABLED
		_buffer.reset();
#else /* BITTY_MULTITHREAD_ENABLED */
		_commited = 0;
#endif /* BITTY_MULTITHREAD_ENABLED */
		_commands = 0;

		int loadingCount = 0;
		int unloadingCount = 0;
		clearResourceLoadingAndUnloading(loadingCount, unloadingCount);

		int disposingCount = 0;
		clearResourceDisposingAndCollecting(disposingCount);

		fprintf(
			stdout,
			"Primitives reset:\n"
			"  abandoned %d loading, %d unloading,\n"
			"  abandoned %d disposing.\n",
			loadingCount,
			unloadingCount,
			disposingCount
		);

		_resources->reset();
		if (_audio)
			_audio->reset();
		_input->reset();
	}

	virtual bool update(const Math::Rectf* clientArea, const Math::Vec2i* canvasSize, int scale, double delta, bool hovering, bool* indicated) override {
		do {
			LockGuard<decltype(_canvasSizeLock)> guard(_canvasSizeLock);

			_canvasSize = *canvasSize;
		} while (false);

		if (indicated)
			*indicated = !!_indicatorCursor;
		if (hovering && _indicatorCursor)
			SDL_SetCursor(_indicatorCursor);

#if BITTY_MULTITHREAD_ENABLED
		saveStates();

		processResourceLoadingAndUnloading();

		processResourceDisposingAndCollecting();

		_input->update(_window, _renderer, clientArea, canvasSize, scale);

		CmdQueue q;
		_buffer.pop(q);
		_commands = (unsigned)q.size();
		q.run(this, _renderer, _project, _resources, _audio, &delta);

		restoreStates();
#else /* BITTY_MULTITHREAD_ENABLED */
		(void)delta;

		processResourceDisposingAndCollecting();

		_input->update(_window, _renderer, clientArea, canvasSize, scale);
#endif /* BITTY_MULTITHREAD_ENABLED */

		return true;
	}

private:
	template<typename T> bool translated(T &x, T &y) const {
		if (!_cameraChanged)
			return false;

		x -= _camera.x;
		y -= _camera.y;

		return true;
	}
	template<typename T> bool culled(const T &other) const {
		if (!_canvas)
			return false;

		return !Math::intersects(
			Math::Recti::byXYWH(0, 0, _canvasCull.x, _canvasCull.y),
			other
		);
	}
	template<typename T> bool clipped(T &x, T &y, T &w, T &h) const {
		if (!_clipChanged)
			return false;

		x = (T)_clip.xMin();
		y = (T)_clip.yMin();
		w = (T)_clip.width();
		h = (T)_clip.height();

		return true;
	}

	void saveStates(void) {
		// Save blend mode.
		_canvasBlend = _renderer->blend();

		_renderer->blend(SDL_BLENDMODE_BLEND);
	}
	void restoreStates(void) {
		// Restore render target.
		if (_canvas && _canvasTarget)
			_renderer->target(_canvas.get());

		// Restore blend mode.
		_renderer->blend(_canvasBlend);
	}

	void commit(CmdVariant &var, const double* delta) const {
#if BITTY_MULTITHREAD_ENABLED
		(void)delta;

		_buffer.add(var);
#else /* BITTY_MULTITHREAD_ENABLED */
		var.run(const_cast<PrimitivesImpl*>(this), _renderer, _project, _resources, _audio, delta);
		++_commited;
		++_commands;
#endif /* BITTY_MULTITHREAD_ENABLED */
	}
	void commit(CmdVariant &var, const double* delta, bool block) const {
#if BITTY_MULTITHREAD_ENABLED
		(void)delta;

		_buffer.add(var, block);
#else /* BITTY_MULTITHREAD_ENABLED */
		(void)block;

		var.run(const_cast<PrimitivesImpl*>(this), _renderer, _project, _resources, _audio, delta);
		++_commited;
		++_commands;
#endif /* BITTY_MULTITHREAD_ENABLED */
	}

	void processResourceLoadingAndUnloading(void) {
		// Process loading.
		do {
			// Load pending requests.
			LockGuard<decltype(_loads.lock)> guardLoads(_loads.lock);

			if (_loads.empty())
				break;

			for (Resources::Asset::Ptr ptr : _loads) {
				if (ptr)
					_resources->load(_project, *ptr);
			}
			_loads.clear();
		} while (false);

		// Process unloading.
		do {
			// Unload pending requests.
			LockGuard<decltype(_unloads.lock)> guardUnloads(_unloads.lock);

			if (_unloads.empty())
				break;

			for (Resources::Asset::Ptr ptr : _unloads) {
				if (ptr) {
					_resources->unload(*ptr);
				} else {
					_resources->cleanup();

					break;
				}
			}
			_unloads.clear();

			// Clean up project.
			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = _project->acquire(acquired);
			if (!prj)
				break;

			prj->cleanup(Asset::RUNNING);
		} while (false);
	}
	void clearResourceLoadingAndUnloading(int &loadingCount, int &unloadingCount) {
		// Clear loading.
		do {
			LockGuard<decltype(_loads.lock)> guardLoads(_loads.lock);

			loadingCount = _loads.count();
			_loads.clear();
		} while (false);

		// Clear unloading.
		do {
			LockGuard<decltype(_unloads.lock)> guardUnloads(_unloads.lock);

			unloadingCount = _unloads.count();
			_unloads.clear();
		} while (false);
	}

	void processResourceDisposingAndCollecting(void) {
		// Process disposing.
		do {
			LockGuard<decltype(_disposing.lock)> guardUnloads(_disposing.lock);

			_disposing.clear();
		} while (false);

		// Process collecting.
		do {
			LockGuard<decltype(_unloads.lock)> guardUnloads(_unloads.lock);

			if (_collect) {
				_resources->collect();

				_collect = false;
			}
		} while (false);
	}
	void clearResourceDisposingAndCollecting(int &disposingCount) {
		// Clear disposing.
		do {
			LockGuard<decltype(_disposing.lock)> guardUnloads(_disposing.lock);

			disposingCount = _disposing.count();
			_disposing.clear();
		} while (false);

		// Clear collecting.
		do {
			LockGuard<decltype(_unloads.lock)> guardUnloads(_unloads.lock);

			_collect = false;
		} while (false);
	}
};

Primitives* Primitives::create(bool withAudio) {
	PrimitivesImpl* result = new PrimitivesImpl(withAudio);

	return result;
}

void Primitives::destroy(Primitives* ptr) {
	PrimitivesImpl* impl = static_cast<PrimitivesImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
