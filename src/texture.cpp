/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "hacks.h"
#include "image.h"
#include "renderer.h"
#include "texture.h"
#include <SDL.h>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef TEXTURE_LOCK_SURFACE
#	define TEXTURE_LOCK_SURFACE(SUR) \
	ProcedureGuard<bool> BITTY_UNIQUE_NAME(__LOCK__)( \
		std::bind( \
			[&] (SDL_Surface* surface) -> bool* { \
				bool* result = (bool*)(uintptr_t)(SDL_MUSTLOCK(SUR) ? 0x1 : 0x0); \
				if (result) \
					SDL_LockSurface(surface); \
				return result; \
			}, \
			(SUR) \
		), \
		std::bind( \
			[&] (SDL_Surface* surface, bool* locked) -> void { \
				if (locked) \
					SDL_UnlockSurface(surface); \
			}, \
			(SUR), std::placeholders::_1 \
		) \
	);
#endif /* TEXTURE_LOCK_SURFACE */

#if SDL_VERSION_ATLEAST(2, 0, 12)
static_assert((unsigned)Texture::NEAREST == (unsigned)SDL_ScaleModeNearest, "Value does not match.");
static_assert((unsigned)Texture::LINEAR == (unsigned)SDL_ScaleModeLinear, "Value does not match.");
static_assert((unsigned)Texture::ANISOTROPIC == (unsigned)SDL_ScaleModeBest, "Value does not match.");
#endif /* SDL_VERSION_ATLEAST(2, 0, 12) */

static_assert((unsigned)Texture::STATIC == (unsigned)SDL_TEXTUREACCESS_STATIC, "Value does not match.");
static_assert((unsigned)Texture::STREAMING == (unsigned)SDL_TEXTUREACCESS_STREAMING, "Value does not match.");
static_assert((unsigned)Texture::TARGET == (unsigned)SDL_TEXTUREACCESS_TARGET, "Value does not match.");

static_assert((unsigned)Texture::NONE == (unsigned)SDL_BLENDMODE_NONE, "Value does not match.");
static_assert((unsigned)Texture::BLEND == (unsigned)SDL_BLENDMODE_BLEND, "Value does not match.");
static_assert((unsigned)Texture::ADD == (unsigned)SDL_BLENDMODE_ADD, "Value does not match.");
static_assert((unsigned)Texture::MOD == (unsigned)SDL_BLENDMODE_MOD, "Value does not match.");
#if SDL_VERSION_ATLEAST(2, 0, 12)
static_assert((unsigned)Texture::MUL == (unsigned)SDL_BLENDMODE_MUL, "Value does not match.");
#endif /* SDL_VERSION_ATLEAST(2, 0, 12) */
static_assert((unsigned)Texture::INVALID == (unsigned)SDL_BLENDMODE_INVALID, "Value does not match.");

/* ===========================================================================} */

/*
** {===========================================================================
** Texture
*/

class TextureImpl : public Texture {
private:
	Usages _usage = STATIC;
	int _width = 0;
	int _height = 0;
	int _paletted = 0;

	SDL_Texture* _texture = nullptr;

	SDL_Surface* _palettedSurface = nullptr; // Cached surface for paletted texture, will re-generate the texture when the palette version has been changed.
	Uint32 _palettedVersion = 0;

public:
	TextureImpl() {
		graphicsThreadingGuard.validate();
	}
	virtual ~TextureImpl() override {
		graphicsThreadingGuard.validate();

		clear();
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual void* pointer(class Renderer* rnd) override {
		return texture(rnd);
	}

	virtual Usages usage(void) const override {
		return _usage;
	}

	virtual ScaleModes scale(void) const override {
#if SDL_VERSION_ATLEAST(2, 0, 12)
		if (!_texture)
			return NEAREST;

		SDL_ScaleMode scale = SDL_ScaleModeNearest;
		if (SDL_GetTextureScaleMode(_texture, &scale))
			return NEAREST;

		return (ScaleModes)scale;
#else /* SDL_VERSION_ATLEAST(2, 0, 12) */
		return NEAREST;
#endif /* SDL_VERSION_ATLEAST(2, 0, 12) */
	}
	virtual void scale(ScaleModes scale) override {
#if SDL_VERSION_ATLEAST(2, 0, 12)
		if (!_texture)
			return;

		SDL_SetTextureScaleMode(_texture, (SDL_ScaleMode)scale);
#else /* SDL_VERSION_ATLEAST(2, 0, 12) */
		(void)scale;
#endif /* SDL_VERSION_ATLEAST(2, 0, 12) */
	}

	virtual BlendModes blend(void) const override {
		if (!_texture)
			return INVALID;

		SDL_BlendMode blend = SDL_BLENDMODE_INVALID;
		if (SDL_GetTextureBlendMode(_texture, &blend))
			return INVALID;

		return (BlendModes)blend;
	}
	virtual void blend(BlendModes blend) override {
		if (!_texture)
			return;

		SDL_SetTextureBlendMode(_texture, (SDL_BlendMode)blend);
	}

	virtual int paletted(void) const override {
		return _paletted;
	}

	virtual int width(void) const override {
		return _width;
	}
	virtual int height(void) const override {
		return _height;
	}

	virtual bool resize(class Renderer* rnd, int expWidth, int expHeight) override {
		// Prepare.
		if (!rnd)
			return false;
		if (!_texture)
			return false;

		if (rnd->maxTextureWidth() > 0 && rnd->maxTextureHeight() > 0) {
			if (expWidth > rnd->maxTextureWidth() || expHeight > rnd->maxTextureHeight())
				return false;
		}

		// Save properties.
#if SDL_VERSION_ATLEAST(2, 0, 12)
		SDL_ScaleMode scale = SDL_ScaleModeNearest;
		SDL_GetTextureScaleMode(_texture, &scale);
#endif /* SDL_VERSION_ATLEAST(2, 0, 12) */

		SDL_BlendMode blend = SDL_BLENDMODE_INVALID;
		SDL_GetTextureBlendMode(_texture, &blend);

		// Resize.
		SDL_Renderer* renderer = (SDL_Renderer*)rnd->pointer();

		const int bytes = _paletted ? 1 : 4;
		const Uint32 format = _paletted ? SDL_PIXELFORMAT_INDEX8 : SDL_PIXELFORMAT_ABGR8888;
		switch (_usage) {
		case STREAMING: {
				// Read the old pixels.
				void* raw = nullptr;
				int pitch = 0;
				if (SDL_LockTexture(_texture, nullptr, &raw, &pitch))
					return false;

				Color* pixels = new Color[_width * _height];
				memcpy(pixels, raw, _width * _height * sizeof(Color));
				SDL_UnlockTexture(_texture);
				SDL_DestroyTexture(_texture);

				// Create a new texture.
				_texture = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_STREAMING, expWidth, expHeight);
				assert(_texture);

				// Fill with the old pixels.
				const SDL_Rect rect{ 0, 0, _width, _height };
				SDL_UpdateTexture(_texture, &rect, pixels, _width * bytes);
				if (_palettedSurface) {
					TEXTURE_LOCK_SURFACE(_palettedSurface)
					Byte* buf = (Byte*)_palettedSurface->pixels;
					memcpy(buf, pixels, _width * _height * bytes);
				}
				delete [] pixels;
			}

			break;
		case TARGET: {
				// Read the old pixels.
				Uint32 fmt = 0;
				int access = 0;
				int oldw = 0;
				int oldh = 0;
				SDL_QueryTexture(_texture, &fmt, &access, &oldw, &oldh);
				if (expWidth <= 0)
					expWidth = oldw;
				if (expHeight <= 0)
					expHeight = oldh;

				const int size = std::max(expWidth * expHeight, oldw * oldh);
				Color* pixels = new Color[size];
				memset(pixels, 0, size * sizeof(Color));
				const SDL_Rect rect = { 0, 0, oldw, oldh };
				{
					BITTY_RENDER_TARGET(rnd, this)
#if SDL_VERSION_ATLEAST(2, 0, 12)
					BITTY_RENDER_SCALE(rnd, 1)
#endif /* SDL_VERSION_ATLEAST(2, 0, 12) */
					SDL_RenderReadPixels(renderer, &rect, 0, pixels, oldw * bytes);
				}
				SDL_DestroyTexture(_texture);

				// Create a new texture.
				_texture = SDL_CreateTexture(renderer, fmt, SDL_TEXTUREACCESS_TARGET, expWidth, expHeight);
				assert(_texture);

				// Fill with the old pixels.
				SDL_Texture* tmptex = SDL_CreateTexture(renderer, fmt, SDL_TEXTUREACCESS_STATIC, _width, _height);
				SDL_UpdateTexture(tmptex, nullptr, pixels, _width * bytes);
				{
					BITTY_RENDER_TARGET(rnd, this)
#if SDL_VERSION_ATLEAST(2, 0, 12)
					BITTY_RENDER_SCALE(rnd, 1)
#endif /* SDL_VERSION_ATLEAST(2, 0, 12) */
					SDL_RenderCopy(renderer, tmptex, nullptr, nullptr);
				}
				SDL_DestroyTexture(tmptex);
				if (_palettedSurface) {
					TEXTURE_LOCK_SURFACE(_palettedSurface)
					Byte* buf = (Byte*)_palettedSurface->pixels;
					memcpy(buf, pixels, _width * _height * bytes);
				}
				delete [] pixels;
			}

			break;
		default:
			return false;
		}

		// Restore properties.
		if (_texture) {
#if SDL_VERSION_ATLEAST(2, 0, 12)
			SDL_SetTextureScaleMode(_texture, scale);
#endif /* SDL_VERSION_ATLEAST(2, 0, 12) */
			SDL_SetTextureBlendMode(_texture, blend);
		}

		// Finish.
		_width = expWidth;
		_height = expHeight;

		return true;
	}

	virtual bool set(int x, int y, const Color &col) override {
		if (!_texture)
			return false;

		if (_usage != STREAMING)
			return false;

		if (_paletted)
			return false;

		if (x < 0 || x >= _width)
			return false;
		if (y < 0 || y >= _height)
			return false;

		const SDL_Rect rect{ x, y, 1, 1 };
		void* raw = nullptr;
		int pitch = 0;
		if (SDL_LockTexture(_texture, &rect, &raw, &pitch))
			return false;

		Color* pixels = (Color*)raw;
		*pixels = col;
		SDL_UnlockTexture(_texture);

		if (_palettedSurface) {
			TEXTURE_LOCK_SURFACE(_palettedSurface)
			Color* buf = (Color*)_palettedSurface->pixels;
			buf[x + y * _width] = col;
		}

		return true;
	}
	virtual bool set(int x, int y, int index) override {
		if (!_texture)
			return false;

		if (_usage != STREAMING)
			return false;

		if (!_paletted)
			return false;

		if (x < 0 || x >= _width)
			return false;
		if (y < 0 || y >= _height)
			return false;

		const SDL_Rect rect{ x, y, 1, 1 };
		void* raw = nullptr;
		int pitch = 0;
		if (SDL_LockTexture(_texture, &rect, &raw, &pitch))
			return false;

		Byte* pixels = (Byte*)raw;
		*pixels = (Byte)index;
		SDL_UnlockTexture(_texture);

		if (_palettedSurface) {
			TEXTURE_LOCK_SURFACE(_palettedSurface)
			Byte* buf = (Byte*)_palettedSurface->pixels;
			buf[x + y * _width] = (Byte)index;
		}

		return true;
	}

	virtual bool fromImage(class Renderer* rnd, Usages usg, class Image* img, ScaleModes scaleMode) override {
		// Prepare.
		if (_texture)
			clear();

		if (!rnd || !rnd->pointer())
			return false;

		if (!img || !img->pointer())
			return false;

		if (rnd->maxTextureWidth() > 0 && rnd->maxTextureHeight() > 0) {
			if (img->width() > rnd->maxTextureWidth() || img->height() > rnd->maxTextureHeight())
				return false;
		}

		// Create.
		if (usg != STATIC) {
			if (fromBytes(rnd, usg, img->pixels(), img->width(), img->height(), img->paletted(), scaleMode))
				return true;
		}

		SDL_Renderer* renderer = (SDL_Renderer*)rnd->pointer();
		SDL_Surface* surface = (SDL_Surface*)img->pointer();
		SDL_Texture* tex = nullptr;
		if (img->paletted()) {
			_palettedSurface = SDL_DuplicateSurface(surface);

			do {
				TEXTURE_LOCK_SURFACE(surface)
				SDL_PixelFormat* fmt = surface->format;
				if (!fmt)
					break;
				SDL_Palette* plt = fmt->palette;
				if (!plt)
					break;

				if (!_palettedSurface) {
					_palettedSurface = SDL_CreateRGBSurfaceFrom(
						surface->pixels,
						surface->w, surface->h,
						fmt->BitsPerPixel, surface->w,
						0, 0, 0, 0
					);
				}
				if (_palettedSurface) {
					SDL_SetSurfacePalette(_palettedSurface, plt);

					_palettedVersion = plt->version;
				}
			} while (false);

			if (_palettedSurface) {
				SDL_Surface* tmp = SDL_CreateRGBSurface(
					0,
					_palettedSurface->w, _palettedSurface->h,
					32,
					0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
				);
				SDL_BlitSurface(_palettedSurface, nullptr, tmp, nullptr);
				tex = SDL_CreateTextureFromSurface(renderer, tmp);
				SDL_FreeSurface(tmp);
			}
		}
		if (!tex)
			tex = SDL_CreateTextureFromSurface(renderer, surface);

#if SDL_VERSION_ATLEAST(2, 0, 12)
		SDL_SetTextureScaleMode(tex, (SDL_ScaleMode)scaleMode);
#endif /* SDL_VERSION_ATLEAST(2, 0, 12) */
		Uint32 format = 0;
		int access = 0;
		SDL_QueryTexture(tex, &format, &access, nullptr, nullptr);
		_texture = tex;
		if (!tex)
			return false;

		// Finish.
		_usage = (Usages)access;
		_width = img->width();
		_height = img->height();
		_paletted = img->paletted();

		return true;
	}

	virtual int toBytes(class Renderer* rnd, Byte* pixels) override {
		int result = _width * _height * sizeof(Color);

		if (!rnd)
			return result;
		if (!_texture)
			return result;

		if (!pixels)
			return result;

		SDL_Renderer* renderer = (SDL_Renderer*)rnd->pointer();

		const int bytes = _paletted ? 1 : 4;
		switch (_usage) {
		case STREAMING: {
				void* raw = nullptr;
				if (SDL_LockTexture(_texture, nullptr, &raw, nullptr))
					return 0;

				memcpy(pixels, raw, _width * _height * sizeof(Color));
				SDL_UnlockTexture(_texture);
			}

			break;
		case TARGET: {
				int width = 0;
				int height = 0;
				SDL_QueryTexture(_texture, nullptr, nullptr, &width, &height);

				memset(pixels, 0, _width * _height * sizeof(Color));
				const SDL_Rect rect = { 0, 0, width, height };
				BITTY_RENDER_TARGET(rnd, this)
#if SDL_VERSION_ATLEAST(2, 0, 12)
				BITTY_RENDER_SCALE(rnd, 1)
#endif /* SDL_VERSION_ATLEAST(2, 0, 12) */
				const Uint32 fmt = SDL_PIXELFORMAT_ABGR8888;
				SDL_RenderReadPixels(renderer, &rect, fmt, pixels, width * bytes);
			}

			break;
		default:
			return 0;
		}

		return result;
	}
	virtual bool fromBytes(class Renderer* rnd, Usages usg, const Byte* pixels, int expWidth, int expHeight, int paletted, ScaleModes scaleMode) override {
		// Prepare.
		if (_texture)
			clear();

		if (!rnd || !rnd->pointer() || expWidth <= 0 || expHeight <= 0)
			return false;

		if (rnd->maxTextureWidth() > 0 && rnd->maxTextureHeight() > 0) {
			if (expWidth > rnd->maxTextureWidth() || expHeight > rnd->maxTextureHeight())
				return false;
		}

		// Create.
		SDL_Renderer* renderer = (SDL_Renderer*)rnd->pointer();

		const int bytes = paletted ? 1 : 4;
		const Uint32 format = paletted ? SDL_PIXELFORMAT_INDEX8 : SDL_PIXELFORMAT_ABGR8888;
		int access = SDL_TEXTUREACCESS_STATIC;
		switch (usg) {
		case STATIC:
			access = SDL_TEXTUREACCESS_STATIC;

			break;
		case STREAMING:
			access = SDL_TEXTUREACCESS_STREAMING;

			break;
		case TARGET:
			access = SDL_TEXTUREACCESS_TARGET;

			break;
		default:
			return false;
		}
		SDL_Texture* tex = SDL_CreateTexture(renderer, format, access, expWidth, expHeight);
#if SDL_VERSION_ATLEAST(2, 0, 12)
		SDL_SetTextureScaleMode(tex, (SDL_ScaleMode)scaleMode);
#endif /* SDL_VERSION_ATLEAST(2, 0, 12) */
		_texture = tex;
		if (!tex)
			return false;

		// Fill.
		if (pixels) {
			switch (usg) {
			case STATIC:
				SDL_UpdateTexture(tex, nullptr, pixels, expWidth * bytes);

				break;
			case STREAMING: {
					void* raw = nullptr;
					int pitch = 0;
					if (SDL_LockTexture(_texture, nullptr, &raw, &pitch))
						break;

					memcpy(raw, pixels, expHeight * pitch);
					SDL_UnlockTexture(_texture);
				}

				break;
			case TARGET: {
					SDL_Texture* tmptex = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_STATIC, expWidth, expHeight);
					SDL_UpdateTexture(tmptex, nullptr, pixels, expWidth * bytes);
					SDL_Texture* prev = SDL_GetRenderTarget(renderer);
					SDL_SetRenderTarget(renderer, tex);
					SDL_RenderCopy(renderer, tmptex, nullptr, nullptr);
					SDL_SetRenderTarget(renderer, prev);
					SDL_DestroyTexture(tmptex);
				}

				break;
			}
		}

		// Finish.
		_usage = usg;
		_width = expWidth;
		_height = expHeight;
		_paletted = paletted;

		return true;
	}

private:
	SDL_Texture* texture(Renderer* rnd) {
		validate(rnd);

		return _texture;
	}
	void texture(std::nullptr_t) {
		if (_texture) {
			SDL_DestroyTexture(_texture);
			_texture = nullptr;
		}
	}

	bool clear(void) {
		_palettedVersion = 0;
		if (_palettedSurface) {
			SDL_FreeSurface(_palettedSurface);
			_palettedSurface = nullptr;
		}
		texture(nullptr);

		_usage = STATIC;
		_width = 0;
		_height = 0;
		_paletted = 0;

		return true;
	}

	void validate(Renderer* rnd) {
		if (!_texture)
			return;
		if (!_palettedSurface)
			return;

		Uint32 ver = 0;
		do {
			TEXTURE_LOCK_SURFACE(_palettedSurface)
			SDL_PixelFormat* fmt = _palettedSurface->format;
			if (!fmt)
				return;
			SDL_Palette* plt = fmt->palette;
			if (!plt)
				return;
			if (plt->version == _palettedVersion)
				return;

			ver = plt->version;
		} while (false);

#if SDL_VERSION_ATLEAST(2, 0, 12)
		SDL_ScaleMode scale = SDL_ScaleModeNearest;
		SDL_GetTextureScaleMode(_texture, &scale);
#endif /* SDL_VERSION_ATLEAST(2, 0, 12) */

		SDL_BlendMode blend = SDL_BLENDMODE_INVALID;
		SDL_GetTextureBlendMode(_texture, &blend);

		SDL_DestroyTexture(_texture);
		_texture = nullptr;

		SDL_Renderer* renderer = (SDL_Renderer*)rnd->pointer();
		SDL_Texture* tex = nullptr;
		if (_palettedSurface) {
			SDL_Surface* tmp = SDL_CreateRGBSurface(
				0,
				_palettedSurface->w, _palettedSurface->h,
				32,
				0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
			);
			SDL_BlitSurface(_palettedSurface, nullptr, tmp, nullptr);
			tex = SDL_CreateTextureFromSurface(renderer, tmp);
			SDL_FreeSurface(tmp);
		}
#if SDL_VERSION_ATLEAST(2, 0, 12)
		SDL_SetTextureScaleMode(tex, scale);
#endif /* SDL_VERSION_ATLEAST(2, 0, 12) */
		SDL_SetTextureBlendMode(tex, blend);
		Uint32 format = 0;
		int access = 0;
		SDL_QueryTexture(tex, &format, &access, nullptr, nullptr);
		_texture = tex;
		if (!tex)
			return;

		_usage = (Usages)access;

		_palettedVersion = ver;
	}
};

Texture* Texture::create(void) {
	TextureImpl* result = new TextureImpl();

	return result;
}

void Texture::destroy(Texture* ptr) {
	TextureImpl* impl = static_cast<TextureImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
