/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2023 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "bytes.h"
#include "font.h"
#include "image.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "../lib/stb/stb_truetype.h"

/*
** {===========================================================================
** Font
*/

class FontImpl : public Font {
private:
	Bytes::Ptr _data = nullptr;
	int _permeation = 1;
	Bytes* _glyph = nullptr;

	int _imagePaletted = -1;
	int _imageWidth = -1;
	int _imageHeight = -1;
	int _imageCharacterWidth = -1;
	int _imageCharacterHeight = -1;

	stbtt_fontinfo _fontInfo;
	int _fontHeight = -1;
	float _fontScale = 1.0f;

public:
	FontImpl() {
	}
	virtual ~FontImpl() override {
		clear();

		if (_glyph) {
			Bytes::destroy(_glyph);
			_glyph = nullptr;
		}
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Object** ptr) const override { // Non-clonable.
		if (ptr)
			*ptr = nullptr;

		return false;
	}

	virtual void* pointer(void) override {
		if (!_data)
			return nullptr;

		return _data->pointer();
	}

	virtual bool measure(Codepoint cp, int* width, int* height) override {
		if (_imagePaletted >= 0 && _imageWidth > 0 && _imageHeight > 0 && _imageCharacterWidth > 0 && _imageCharacterHeight > 0)
			return renderWithImage(cp, nullptr, nullptr, width, height);
		else if (_fontHeight > 0)
			return renderWithFontInfo(cp, nullptr, nullptr, width, height);

		return false;
	}

	virtual bool render(
		Codepoint cp,
		class Bytes* out,
		const Color* color,
		int* width, int* height
	) override {
		if (_imagePaletted >= 0 && _imageWidth > 0 && _imageHeight > 0 && _imageCharacterWidth > 0 && _imageCharacterHeight > 0)
			return renderWithImage(cp, out, color, width, height);
		else if (_fontHeight > 0)
			return renderWithFontInfo(cp, out, color, width, height);

		return false;
	}

	virtual bool fromFont(const Font* font) override {
		clear();

		if (!font)
			return false;

		const FontImpl* impl = static_cast<const FontImpl*>(font);

		_data = impl->_data;
		_permeation = impl->_permeation;

		_imagePaletted = impl->_imagePaletted;
		_imageWidth = impl->_imageWidth;
		_imageHeight = impl->_imageHeight;
		_imageCharacterWidth = impl->_imageCharacterWidth;
		_imageCharacterHeight = impl->_imageCharacterHeight;

		memcpy(&_fontInfo, &impl->_fontInfo, sizeof(stbtt_fontinfo));
		_fontHeight = impl->_fontHeight;
		_fontScale = impl->_fontScale;
		if (_fontHeight > 0)
			return initializeWithFontInfo(_fontHeight);

		return false;
	}

	virtual bool fromImage(const class Image* src, int width, int height, int permeation) override {
		clear();

		if (!src || width <= 0 || height <= 0)
			return false;

		_data = Bytes::Ptr(Bytes::create());
		_permeation = permeation;
		const int paletted = src->paletted();
		if (paletted)
			_data->writeBytes(src->pixels(), src->width() * src->height() * paletted);
		else
			_data->writeBytes(src->pixels(), src->width() * src->height() * sizeof(Color));
		if (_glyph)
			_glyph->clear();

		_imagePaletted = paletted;
		_imageWidth = src->width();
		_imageHeight = src->height();
		_imageCharacterWidth = width;
		_imageCharacterHeight = height;

		return true;
	}

	virtual bool fromBytes(const Byte* data, size_t len, int size, int permeation) override {
		clear();

		if (!data || !len)
			return false;

		_data = Bytes::Ptr(Bytes::create());
		_data->writeBytes(data, len);
		_permeation = permeation;

		return initializeWithFontInfo(size);
	}

private:
	void clear(void) {
		_data = nullptr;
		_permeation = 1;

		_imagePaletted = -1;
		_imageWidth = -1;
		_imageHeight = -1;
		_imageCharacterWidth = -1;
		_imageCharacterHeight = -1;

		memset(&_fontInfo, 0, sizeof(stbtt_fontinfo));
		_fontHeight = -1;
		_fontScale = 1.0f;
	}

	bool renderWithImage(
		Codepoint cp,
		class Bytes* out /* nullable */,
		const Color* color,
		int* width /* nullable */, int* height /* nullable */
	) {
		if (!_data)
			return false;

		if (_imageCharacterWidth <= 0 || _imageCharacterHeight <= 0)
			return false;

		int width_ = width ? *width : -1;
		int height_ = height ? *height : -1;

		if (width_ <= 0)
			width_ = _imageCharacterWidth;
		if (height_ <= 0)
			height_ = _imageCharacterHeight;

		if (width)
			*width = width_;
		if (height)
			*height = height_;

		const int xCount = _imageWidth / _imageCharacterWidth;
		const int yCount = _imageHeight / _imageCharacterHeight;
		if (cp >= (Codepoint)(xCount * yCount))
			return false;
		if (xCount <= 0)
			return false;

		const std::div_t div = std::div((int)cp, xCount);
		const int xIndex = div.rem;
		const int yIndex = div.quot;

		if (out) {
			out->clear();
			out->resize(width_ * height_ * sizeof(Color));
			for (int j = 0; j < height_; ++j) {
				for (int i = 0; i < width_; ++i) {
					unsigned a = 0;
					const size_t index = (xIndex * _imageCharacterWidth + i) + (yIndex * _imageCharacterHeight + j) * _imageWidth;
					if (_imagePaletted > 0)
						a = _data->get(index);
					else
						a = _data->get(index * sizeof(Color) + 3);
					if (_permeation > 0)
						a = a >= (unsigned)_permeation ? 255 : 0;

					if (a == 0)
						continue;

					const int x = i;
					const int y = j;
					size_t pos = (x + y * width_) * sizeof(Color);
					if (pos + sizeof(Color) > out->count()) {
						assert(false && "Font position out of bounds.");

						continue;
					}
					Color col = *color;
					if (col.a == 255) {
						col.a = (Byte)a;
					} else {
						if (a < 255)
							col.a = (Byte)Math::clamp(col.a / 255.0f * a, 0.0f, 255.0f);
					}
					if (pos < out->count())
						out->set(pos++, col.r);
					if (pos < out->count())
						out->set(pos++, col.g);
					if (pos < out->count())
						out->set(pos++, col.b);
					if (pos < out->count())
						out->set(pos++, col.a);
				}
			}
		}

		return true;
	}

	bool initializeWithFontInfo(int size) {
		bool result = false;
		if (_data && !_data->empty() && (size > 0 && _fontHeight != size)) {
			_fontHeight = size;

			result = !!stbtt_InitFont(&_fontInfo, _data->pointer(), stbtt_GetFontOffsetForIndex(_data->pointer(), 0));
			_fontScale = stbtt_ScaleForPixelHeight(&_fontInfo, (float)_fontHeight);
		}

		return result;
	}
	bool renderWithFontInfo(
		Codepoint cp,
		class Bytes* out /* nullable */,
		const Color* color,
		int* width /* nullable */, int* height /* nullable */
	) {
		if (!_data)
			return false;

		auto measure = [] (const stbtt_fontinfo &font, Codepoint cp, float scale, int w) -> int {
			int advWidth = 0;
			int leftBearing = 0;
			stbtt_GetCodepointHMetrics(&font, cp, &advWidth, &leftBearing);

			return std::max(w, (int)(advWidth * scale));
		};

		if (!_glyph)
			_glyph = Bytes::create();

		int width_ = width ? *width : -1;
		int height_ = height ? *height : -1;

		int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
		stbtt_GetCodepointBitmapBox(&_fontInfo, cp, _fontScale, _fontScale, &x0, &y0, &x1, &y1);
		if (width_ <= 0)
			width_ = measure(_fontInfo, cp, _fontScale, width_);
		if (height_ <= 0)
			height_ = _fontHeight + y1;
		_glyph->resize(width_ * height_);
		stbtt_MakeCodepointBitmap(&_fontInfo, _glyph->pointer(), width_, height_, width_, _fontScale, _fontScale, cp);

		if (width)
			*width = width_;
		if (height)
			*height = height_;

		if (out) {
			out->clear();
			out->resize(width_ * height_ * sizeof(Color));
			for (int j = 0; j < height_; ++j) {
				for (int i = 0; i < width_; ++i) {
					unsigned a = _glyph->get(i + j * width_);
					if (_permeation > 0)
						a = a >= (unsigned)_permeation ? 255 : 0;

					if (a == 0)
						continue;

					const int x = i;
					const int y = j + height_ + y0 - y1;
					size_t pos = (x + y * width_) * sizeof(Color);
					if (pos + sizeof(Color) > out->count()) {
						assert(false && "Font position out of bounds.");

						continue;
					}
					Color col = *color;
					if (col.a == 255) {
						col.a = (Byte)a;
					} else {
						if (a < 255)
							col.a = (Byte)Math::clamp(col.a / 255.0f * a, 0.0f, 255.0f);
					}
					if (pos < out->count())
						out->set(pos++, col.r);
					if (pos < out->count())
						out->set(pos++, col.g);
					if (pos < out->count())
						out->set(pos++, col.b);
					if (pos < out->count())
						out->set(pos++, col.a);
				}
			}
		}

		_glyph->clear();

		return true;
	}
};

Font* Font::create() {
	FontImpl* result = new FontImpl();

	return result;
}

void Font::destroy(Font* ptr) {
	FontImpl* impl = static_cast<FontImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
