/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "renderer.h"
#include "texture.h"
#include "window.h"
#include <SDL.h>

/*
** {===========================================================================
** Renderer
*/

class RendererImpl : public Renderer {
private:
	SDL_Renderer* _renderer = nullptr;
	Texture* _target = nullptr;
	int _scale = 1;
	SDL_BlendMode _blend = SDL_BLENDMODE_NONE;

public:
	RendererImpl() {
	}
	virtual ~RendererImpl() {
	}

	virtual const void* pointer(void) const override {
		return _renderer;
	}
	virtual void* pointer(void) override {
		return _renderer;
	}

	virtual bool open(class Window* wnd) override {
		if (_renderer)
			return false;

		_renderer = SDL_CreateRenderer(
			(SDL_Window*)wnd->pointer(),
			-1,
			SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE
		);

		SDL_RendererInfo info;
		SDL_GetRendererInfo(_renderer, &info);

		fprintf(stdout, "Renderer opened [%s], max texture size %dx%d.\n", info.name, info.max_texture_width, info.max_texture_height);

		return true;
	}
	virtual bool close(void) override {
		if (!_renderer)
			return false;

		SDL_DestroyRenderer(_renderer);
		_renderer = nullptr;

		fprintf(stdout, "Renderer closed.\n");

		return true;
	}

	virtual const char* driver(void) const override {
		static const char* drv = nullptr; // Shared.
		if (!drv) {
			SDL_RendererInfo info;
			SDL_GetRendererInfo(_renderer, &info);
			drv = info.name;
		}

		return drv;
	}

	virtual bool renderTargetSupported(void) const override {
		return !!SDL_RenderTargetSupported(_renderer);
	}

	virtual int maxTextureWidth(void) const override {
		SDL_RendererInfo info;
		SDL_GetRendererInfo(_renderer, &info);

		return info.max_texture_width;
	}
	virtual int maxTextureHeight(void) const override {
		SDL_RendererInfo info;
		SDL_GetRendererInfo(_renderer, &info);

		return info.max_texture_height;
	}

	virtual int width(void) const override {
		int width = 0, height = 0;
		SDL_GetRendererOutputSize(_renderer, &width, &height);
		if (_scale != 1)
			width /= _scale;

		return width;
	}
	virtual int height(void) const override {
		int width = 0, height = 0;
		SDL_GetRendererOutputSize(_renderer, &width, &height);
		if (_scale != 1)
			height /= _scale;

		return height;
	}

	virtual int scale(void) const override {
		return _scale;
	}
	virtual void scale(int val) override {
		val = std::max(val, 1);
		if (_scale == val)
			return;

		_scale = val;

		SDL_RenderSetScale(_renderer, (float)_scale, (float)_scale);
	}

	virtual class Texture* target(void) override {
		if (!pointer())
			return nullptr;

		return _target;
	}
	virtual void target(class Texture* tex) override {
		if (!pointer())
			return;

		_target = tex;
		if (_target)
			SDL_SetRenderTarget(_renderer, (SDL_Texture*)_target->pointer(this));
		else
			SDL_SetRenderTarget(_renderer, nullptr);
	}

	virtual unsigned blend(void) const override {
		return _blend;
	}
	virtual void blend(unsigned mode) override {
		_blend = (SDL_BlendMode)mode;
		SDL_SetRenderDrawBlendMode(_renderer, _blend);
	}

	virtual void clip(int x, int y, int width, int height) override {
		if (!pointer())
			return;

		const SDL_Rect rect{ x, y, width, height };
		SDL_RenderSetClipRect(_renderer, &rect);
	}
	virtual void clip(void) override {
		if (!pointer())
			return;

		SDL_RenderSetClipRect(_renderer, nullptr);
	}

	virtual void clear(const Color* col) override {
		if (col)
			SDL_SetRenderDrawColor(_renderer, col->r, col->g, col->b, col->a);
		else
			SDL_SetRenderDrawColor(_renderer, 0x2e, 0x32, 0x38, 0xff);
		SDL_RenderClear(_renderer);
	}

	virtual void render(class Texture* tex, const Math::Recti* srcRect, const Math::Recti* dstRect, const double* rotAngle, const Math::Vec2f* rotCenter, bool hFlip, bool vFlip) override {
		// Prepare.
		if (!tex || !tex->pointer(this))
			return;

		SDL_Texture* texture = (SDL_Texture*)tex->pointer(this);

		SDL_Rect src{ 0, 0, tex->width(), tex->height() };
		SDL_Rect dst{ 0, 0, width(), height() };
		SDL_Point ctr;
		if (srcRect)
			src = { srcRect->xMin(), srcRect->yMin(), srcRect->width(), srcRect->height() };
		if (dstRect)
			dst = { dstRect->xMin(), dstRect->yMin(), dstRect->width(), dstRect->height() };
		if (rotCenter) {
			if (dstRect)
				ctr = { (int)(rotCenter->x * dst.w), (int)(rotCenter->y * dst.h) };
			else if (srcRect)
				ctr = { (int)(rotCenter->x * src.w), (int)(rotCenter->y * src.h) };
			else
				ctr = { (int)(rotCenter->x * width()), (int)(rotCenter->y * height()) };
		}
		SDL_RendererFlip flip = SDL_FLIP_NONE;
		if (hFlip)
			flip = (SDL_RendererFlip)(flip | SDL_FLIP_HORIZONTAL);
		if (vFlip)
			flip = (SDL_RendererFlip)(flip | SDL_FLIP_VERTICAL);

		// Copy.
		if (rotAngle || flip != SDL_FLIP_NONE) {
			SDL_RenderCopyEx(
				_renderer, texture,
				srcRect ? &src : nullptr, dstRect ? &dst : nullptr,
				rotAngle ? *rotAngle : 0.0, rotCenter ? &ctr : nullptr,
				flip
			);
		} else {
			SDL_RenderCopy(
				_renderer, texture,
				srcRect ? &src : nullptr, dstRect ? &dst : nullptr
			);
		}
	}

	virtual void present(void) override {
		SDL_RenderPresent(_renderer);
	}
};

Renderer* Renderer::create(void) {
	RendererImpl* result = new RendererImpl();

	return result;
}

void Renderer::destroy(Renderer* ptr) {
	RendererImpl* impl = static_cast<RendererImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */