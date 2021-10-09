/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "bytes.h"
#include "image.h"
#include "plus.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../lib/stb/stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "../lib/stb/stb_image_resize.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../lib/stb/stb_image_write.h"
#include <SDL.h>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef IMAGE_LOCK_SURFACE
#	define IMAGE_LOCK_SURFACE(SUR) \
	ProcedureGuard<bool> __LOCK##__LINE__( \
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
#endif /* IMAGE_LOCK_SURFACE */

static const Byte IMAGE_PALETTED_HEADER_BYTES[] = IMAGE_PALETTED_HEADER;
static const Byte IMAGE_COLORED_HEADER_BYTES[] = IMAGE_COLORED_HEADER;

/* ===========================================================================} */

/*
** {===========================================================================
** Image
*/

class ImageImpl : public Image {
private:
	bool _blank = true;
	Palette::Ptr _palette = nullptr;
	int _palettedBits = 0;
	Byte* _pixels = nullptr;
	int _width = 0;
	int _height = 0;
	int _channels = 0;

	SDL_Surface* _surface = nullptr;

	int _quantizationRedWeight = 1;
	int _quantizationGreenWeight = 1;
	int _quantizationBlueWeight = 1;
	int _quantizationAlphaWeight = 4;

public:
	ImageImpl(Palette::Ptr palette) : _palette(palette) {
	}
	virtual ~ImageImpl() override {
		clear();
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Image** ptr) const override {
		if (!ptr)
			return false;

		ImageImpl* result = static_cast<ImageImpl*>(Image::create(_palette));

		result->_blank = _blank;
		result->_palettedBits = _palettedBits;
		result->_pixels = (Byte*)malloc(_width * _height * _channels);
		memcpy(result->_pixels, _pixels, _width * _height * _channels);
		result->_width = _width;
		result->_height = _height;
		result->_channels = _channels;

		result->_quantizationRedWeight = _quantizationRedWeight;
		result->_quantizationGreenWeight = _quantizationGreenWeight;
		result->_quantizationBlueWeight = _quantizationBlueWeight;
		result->_quantizationAlphaWeight = _quantizationAlphaWeight;

		*ptr = result;

		return true;
	}
	virtual bool clone(Object** ptr) const override {
		Image* obj = nullptr;
		if (!clone(&obj))
			return false;

		*ptr = obj;

		return true;
	}

	virtual void* pointer(void) override {
		return surface();
	}
	virtual void pointer(std::nullptr_t) override {
		surface(nullptr);
	}

	virtual bool blank(void) const override {
		return _blank;
	}

	virtual const Palette::Ptr palette(void) const override {
		return _palette;
	}
	virtual void palette(Palette::Ptr val) override {
		_palette = val;
	}
	virtual int paletted(void) const override {
		return _palettedBits;
	}

	virtual const Byte* pixels(void) const override {
		return _pixels;
	}
	virtual Byte* pixels(void) override {
		return _pixels;
	}

	virtual int width(void) const override {
		return _width;
	}
	virtual int height(void) const override {
		return _height;
	}

	virtual int channels(void) const override {
		return _channels;
	}

	virtual bool resize(int width, int height, bool stretch) override {
		if (width <= 0 || height <= 0)
			return false;

		if (width > BITTY_TEXTURE_SAFE_MAX_WIDTH || height > BITTY_TEXTURE_SAFE_MAX_HEIGHT)
			return false;

		if (_palettedBits) {
			if (_pixels && stretch) {
				return false;
			} else if (_pixels && !stretch) {
				const bool blank = _blank;
				Byte* tmp = (Byte*)malloc(width * height * sizeof(Byte));
				memset(tmp, 0, width * height * sizeof(Byte));
				for (int j = 0; j < height; ++j) {
					for (int i = 0; i < width; ++i) {
						int index = 0;
						if (get(i, j, index)) {
							Byte* unit = &tmp[(i + j * width) * _channels];
							*unit = (Byte)index;
						}
					}
				}
				clear();
				_blank = blank;
				_pixels = tmp;
				_width = width;
				_height = height;
				_channels = 1;
			} else {
				Byte* tmp = (Byte*)malloc(width * height * sizeof(Byte));
				memset(tmp, 0, width * height * sizeof(Byte));
				_pixels = tmp;
				_width = width;
				_height = height;
				_channels = 1;
			}
		} else {
			if (_pixels && stretch) {
				const bool blank = _blank;
				Byte* tmp = (Byte*)malloc(width * height * sizeof(Color));
				stbir_resize_uint8(_pixels, _width, _height, 0, tmp, width, height, 0, _channels);
				clear();
				_blank = blank;
				_pixels = tmp;
				_width = width;
				_height = height;
				_channels = 4;
			} else if (_pixels && !stretch) {
				const bool blank = _blank;
				Byte* tmp = (Byte*)malloc(width * height * sizeof(Color));
				memset(tmp, 0, width * height * sizeof(Color));
				for (int j = 0; j < height; ++j) {
					for (int i = 0; i < width; ++i) {
						Color col(0, 0, 0, 0);
						if (get(i, j, col)) {
							Byte* unit = &tmp[(i + j * width) * _channels];
							memcpy(unit, &col, sizeof(Color));
						}
					}
				}
				clear();
				_blank = blank;
				_pixels = tmp;
				_width = width;
				_height = height;
				_channels = 4;
			} else {
				Byte* tmp = (Byte*)malloc(width * height * sizeof(Color));
				memset(tmp, 0, width * height * sizeof(Color));
				_pixels = tmp;
				_width = width;
				_height = height;
				_channels = 4;
			}
		}

		surface(nullptr);

		return true;
	}

	virtual bool get(int x, int y, Color &col) const override {
		if (_palettedBits) {
			int idx = 0;
			if (!get(x, y, idx))
				return false;

			if (!_palette)
				return false;

			if (!_palette->get(idx, col))
				return false;

			return true;
		}

		if (x < 0 || x >= _width || y < 0 || y >= _height)
			return false;

		Byte* unit = &_pixels[(x + y * _width) * _channels];
		memcpy(&col, unit, sizeof(Color));

		return true;
	}
	virtual bool set(int x, int y, const Color &col) override {
		if (_palettedBits)
			return false;

		if (x < 0 || x >= _width || y < 0 || y >= _height)
			return false;

		Byte* unit = &_pixels[(x + y * _width) * _channels];
		memcpy(unit, &col, sizeof(Color));

		if (_surface) {
			IMAGE_LOCK_SURFACE(_surface)
			Color* pixels = (Color*)_surface->pixels;
			pixels[x + y * _width] = col;
		}

		_blank = false;

		return true;
	}
	virtual bool get(int x, int y, int &index) const override {
		if (!_palettedBits)
			return false;

		if (x < 0 || x >= _width || y < 0 || y >= _height)
			return false;

		Byte* unit = &_pixels[(x + y * _width) * _channels];
		index = *unit;

		return true;
	}
	virtual bool set(int x, int y, int index) override {
		if (!_palettedBits)
			return false;

		if (x < 0 || x >= _width || y < 0 || y >= _height)
			return false;

		if (index < 0 || index >= std::pow(2, _palettedBits))
			return false;

		Byte* unit = &_pixels[(x + y * _width) * _channels];
		*unit = (Byte)index;

		if (_surface) {
			IMAGE_LOCK_SURFACE(_surface)
			Byte* pixels = (Byte*)_surface->pixels;
			pixels[x + y * _width] = (Byte)index;
		}

		_blank = false;

		return true;
	}

	virtual void weight(int r, int g, int b, int a) override {
		_quantizationRedWeight = r;
		_quantizationGreenWeight = g;
		_quantizationBlueWeight = b;
		_quantizationAlphaWeight = a;
	}
	virtual bool quantize(const Color* colors, int colorCount, bool p2p) override {
		if (p2p)
			return quantizeNearest(colors, colorCount);

		return quantizeLinear(colors, colorCount);
	}

	virtual bool blit(Image* dst, int x, int y, int w, int h, int sx, int sy) const override {
		if (!dst)
			return false;

		if (dst == this)
			return false;

		auto plot = [] (const Image* src, Image* dst, int sx, int sy, int dx, int dy, bool paletted) -> void {
			if (paletted) {
				int idx = 0;
				if (src->get(sx, sy, idx))
					dst->set(dx, dy, idx);
			} else {
				Color col;
				if (src->get(sx, sy, col))
					dst->set(dx, dy, col);
			}
		};
		if (w == 0)
			w = dst->width();
		if (h == 0)
			h = dst->height();
		for (int y_ = 0; y_ < h; ++y_) {
			const int sy_ = sy + y_;
			const int dy_ = y + y_;
			for (int x_ = 0; x_ < w; ++x_) {
				const int sx_ = sx + x_;
				const int dx_ = x + x_;
				plot(this, dst, sx_, sy_, dx_, dy_, !!_palettedBits);
			}
		}

		return true;
	}

	virtual bool fromBlank(int width, int height, int paletted) override {
		clear();

		if (width <= 0 || height <= 0)
			return false;

		if (width > BITTY_TEXTURE_SAFE_MAX_WIDTH || height > BITTY_TEXTURE_SAFE_MAX_HEIGHT)
			return false;

		_palettedBits = paletted ? IMAGE_PALETTE_BITS : 0;
		_width = width;
		_height = height;
		if (_palettedBits) {
			_channels = 1;
			_pixels = (Byte*)malloc(_width * _height * sizeof(Byte));
			memset(_pixels, 0, _width * _height * sizeof(Byte));
		} else {
			_palette = nullptr;
			_channels = 4;
			_pixels = (Byte*)malloc(_width * _height * sizeof(Color));
			memset(_pixels, 0, _width * _height * sizeof(Color));
		}

		_blank = true;

		return true;
	}

	virtual bool fromImage(const Image* src) override {
		if (!src)
			return false;

		if (src == this)
			return false;

		if (!fromBlank(src->width(), src->height(), src->paletted()))
			return false;

		auto plot = [] (const Image* src, Image* dst, int x, int y, bool paletted) -> void {
			if (paletted) {
				int idx = 0;
				if (src->get(x, y, idx))
					dst->set(x, y, idx);
			} else {
				Color col;
				if (src->get(x, y, col))
					dst->set(x, y, col);
			}
		};
		for (int y = 0; y < _height && y < src->height(); ++y) {
			for (int x = 0; x < _width && x < src->width(); ++x)
				plot(src, this, x, y, !!_palettedBits);
		}

		_blank = src->blank();

		return true;
	}

	virtual bool toBytes(class Bytes* val, const char* type) const override {
		val->clear();

		if (!_pixels)
			return false;

		if (_palettedBits) {
			const size_t headerSize = BITTY_COUNTOF(IMAGE_PALETTED_HEADER_BYTES) +
				sizeof(int) + sizeof(int) +
				sizeof(int) +
				_width * _height;
			val->resize(headerSize);
			Byte* ptr = val->pointer();
			memcpy(ptr, IMAGE_PALETTED_HEADER_BYTES, BITTY_COUNTOF(IMAGE_PALETTED_HEADER_BYTES));
			ptr += BITTY_COUNTOF(IMAGE_PALETTED_HEADER_BYTES);
			memcpy(ptr, &_width, sizeof(int));
			ptr += sizeof(int);
			memcpy(ptr, &_height, sizeof(int));
			ptr += sizeof(int);
			memcpy(ptr, &_palettedBits, sizeof(int));
			ptr += sizeof(int);
			memcpy(ptr, _pixels, _width * _height);
			val->poke(val->count());

			return true;
		} else {
			stbi_write_func* toStream = [] (void* context, void* data, int len) -> void {
				Bytes* bytes = (Bytes*)context;
				if (len == 1) {
					bytes->writeByte(*(Byte*)data);
				} else {
					const size_t count = bytes->count();
					bytes->resize(count + len);
					Byte* ptr = bytes->pointer() + count;
					if (len > 0)
						memcpy(ptr, data, len);
					bytes->poke(count + len);
				}
			};

			if (!strcmp(type, "png")) {
				return !!stbi_write_png_to_func(toStream, val, _width, _height, 4, _pixels, 0);
			} else if (!strcmp(type, "jpg")) {
				return !!stbi_write_jpg_to_func(toStream, val, _width, _height, 4, _pixels, 100);
			} else if (!strcmp(type, "bmp")) {
				return !!stbi_write_bmp_to_func(toStream, val, _width, _height, 4, _pixels);
			} else if (!strcmp(type, "tga")) {
				return !!stbi_write_tga_to_func(toStream, val, _width, _height, 4, _pixels);
			} else {
				const size_t headerSize = BITTY_COUNTOF(IMAGE_COLORED_HEADER_BYTES) +
					sizeof(int) + sizeof(int) +
					sizeof(int);
				val->resize(headerSize);
				Byte* ptr = val->pointer();
				memcpy(ptr, IMAGE_COLORED_HEADER_BYTES, BITTY_COUNTOF(IMAGE_COLORED_HEADER_BYTES));
				ptr += BITTY_COUNTOF(IMAGE_COLORED_HEADER_BYTES);
				memcpy(ptr, &_width, sizeof(int));
				ptr += sizeof(int);
				memcpy(ptr, &_height, sizeof(int));
				ptr += sizeof(int);
				memcpy(ptr, &_palettedBits, sizeof(int));
				ptr += sizeof(int);
				memcpy(ptr, _pixels, _width * _height);
				for (int j = 0; j < _height; ++j) {
					for (int i = 0; i < _width; ++i) {
						Color col;
						get(i, j, col);
						val->writeUInt32(col.toRGBA());
					}
				}

				return true;
			}
		}
	}
	virtual bool fromBytes(const Byte* val, size_t size) override {
		clear();

		if (!val)
			return false;

		if (size > BITTY_COUNTOF(IMAGE_PALETTED_HEADER_BYTES) && memcmp(val, IMAGE_PALETTED_HEADER_BYTES, BITTY_COUNTOF(IMAGE_PALETTED_HEADER_BYTES)) == 0) {
			val += BITTY_COUNTOF(IMAGE_PALETTED_HEADER_BYTES);
			const int* iptr = (int*)val;
			const int width = *iptr++;
			const int height = *iptr++;
			const int bitCount = *iptr++;

			if (width > BITTY_TEXTURE_SAFE_MAX_WIDTH || height > BITTY_TEXTURE_SAFE_MAX_HEIGHT)
				return false;

			_pixels = (Byte*)malloc(width * height);
			memcpy(_pixels, iptr, width * height);
			_width = width;
			_height = height;
			_palettedBits = bitCount; assert(_palettedBits == 0 || _palettedBits == IMAGE_PALETTE_BITS);
		}
		if (_pixels) {
			_channels = 1;

			_blank = false;

			return !!_pixels;
		}

		if (size > BITTY_COUNTOF(IMAGE_COLORED_HEADER_BYTES) && memcmp(val, IMAGE_COLORED_HEADER_BYTES, BITTY_COUNTOF(IMAGE_COLORED_HEADER_BYTES)) == 0) {
			val += BITTY_COUNTOF(IMAGE_COLORED_HEADER_BYTES);
			const int* iptr = (int*)val;
			const int width = *iptr++;
			const int height = *iptr++;
			const int bitCount = *iptr++;

			if (width > BITTY_TEXTURE_SAFE_MAX_WIDTH || height > BITTY_TEXTURE_SAFE_MAX_HEIGHT)
				return false;

			_pixels = (Byte*)malloc(width * height * 4);
			memcpy(_pixels, iptr, width * height * 4);
			_width = width;
			_height = height;
			_palettedBits = bitCount; assert(_palettedBits == 0 || _palettedBits == IMAGE_PALETTE_BITS);
		}
		if (_pixels) {
			_channels = 4;

			_blank = false;

			return !!_pixels;
		}

		_pixels = stbi_load_from_memory(val, (int)size, &_width, &_height, &_channels, 4);
		_channels = 4;

		_blank = false;

		return !!_pixels;
	}
	virtual bool fromBytes(const class Bytes* val) override {
		return fromBytes(val->pointer(), val->count());
	}

	virtual bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const override {
		val.SetObject();

		rapidjson::Value jstrwidth, jstrheight;
		jstrwidth.SetString("width", doc.GetAllocator());
		jstrheight.SetString("height", doc.GetAllocator());
		rapidjson::Value jvalwidth, jvalheight;
		jvalwidth.SetInt(_width);
		jvalheight.SetInt(_height);
		val.AddMember(jstrwidth, jvalwidth, doc.GetAllocator());
		val.AddMember(jstrheight, jvalheight, doc.GetAllocator());

		rapidjson::Value jstrdetpth;
		jstrdetpth.SetString("depth", doc.GetAllocator());
		rapidjson::Value jvaldepth;
		jvaldepth.SetInt(_palettedBits);
		val.AddMember(jstrdetpth, jvaldepth, doc.GetAllocator());

		rapidjson::Value jstrdata;
		jstrdata.SetString("data", doc.GetAllocator());
		rapidjson::Value jvaldata;
		jvaldata.SetArray();
		for (int j = 0; j < _height; ++j) {
			for (int i = 0; i < _width; ++i) {
				if (_palettedBits) {
					int idx = 0;
					get(i, j, idx);

					jvaldata.PushBack(idx, doc.GetAllocator());
				} else {
					Color col;
					get(i, j, col);

					jvaldata.PushBack(col.toRGBA(), doc.GetAllocator());
				}
			}
		}
		val.AddMember(jstrdata, jvaldata, doc.GetAllocator());

		return true;
	}
	virtual bool toJson(rapidjson::Document &val) const override {
		return toJson(val, val);
	}
	virtual bool fromJson(const rapidjson::Value &val) override {
		clear();

		if (!val.IsObject())
			return false;

		rapidjson::Value::ConstMemberIterator jw = val.FindMember("width");
		rapidjson::Value::ConstMemberIterator jh = val.FindMember("height");
		if (jw == val.MemberEnd() || jh == val.MemberEnd())
			return false;
		if (!jw->value.IsInt() || !jh->value.IsInt())
			return false;
		const int width = jw->value.GetInt();
		const int height = jh->value.GetInt();

		int depth = 0;
		rapidjson::Value::ConstMemberIterator jd = val.FindMember("depth");
		if (jd != val.MemberEnd() && jd->value.IsInt())
			depth = jd->value.GetInt();

		if (!fromBlank(width, height, depth))
			return false;

		rapidjson::Value::ConstMemberIterator jdata = val.FindMember("data");
		if (jdata != val.MemberEnd() && jdata->value.IsArray()) {
			rapidjson::Value::ConstArray data = jdata->value.GetArray();
			int idx = 0;
			for (int j = 0; j < height; ++j) {
				for (int i = 0; i < width; ++i) {
					idx = i + j * width;
					if (idx >= (int)data.Size())
						return false;

					if (!data[idx].IsUint())
						return false;

					if (_palettedBits) {
						set(i, j, (int)data[idx].GetUint());
					} else {
						Color col;
						col.fromRGBA(data[idx].GetUint());
						set(i, j, col);
					}
				}
			}

			_blank = false;
		} else {
			for (int j = 0; j < height; ++j) {
				for (int i = 0; i < width; ++i) {
					if (_palettedBits) {
						set(i, j, 0);
					} else {
						const Color col(0, 0, 0, 0);
						set(i, j, col);
					}
				}
			}

			_blank = true;
		}

		return true;
	}
	virtual bool fromJson(const rapidjson::Document &val) override {
		const rapidjson::Value &jval = val;

		return fromJson(jval);
	}

private:
	SDL_Surface* surface(void) {
		if (_surface)
			return _surface;

		if (_channels == 1) {
			_surface = SDL_CreateRGBSurfaceFrom(
				_pixels,
				_width, _height,
				_palettedBits, _width,
				0, 0, 0, 0
			);

			SDL_Palette* palette = nullptr;
			if (_palette)
				palette = (SDL_Palette*)_palette->pointer();
			if (palette)
				SDL_SetSurfacePalette(_surface, palette);
		} else if (_channels == 4) {
			_surface = SDL_CreateRGBSurfaceFrom(
				_pixels,
				_width, _height,
				32, _width * 4,
				0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
			);
		}

		return _surface;
	}
	void surface(std::nullptr_t) {
		if (_surface) {
			SDL_FreeSurface(_surface);
			_surface = nullptr;
		}
	}

	void clear(void) {
		surface(nullptr);

		_blank = true;
		_palettedBits = 0;
		if (_pixels) {
			free(_pixels);
			_pixels = nullptr;
		}
		_width = 0;
		_height = 0;
		_channels = 0;

		_quantizationRedWeight = 1;
		_quantizationGreenWeight = 1;
		_quantizationBlueWeight = 1;
		_quantizationAlphaWeight = 4;
	}

	bool quantizeNearest(const Color* colors, int colorCount) {
		if (_palettedBits)
			return true;

		const int size = _width * _height;
		const Color* const palette = colors;
		Byte* palettedPixels = (Byte*)malloc(size * sizeof(Byte));

		for (int k = 0; k < size; ++k) {
			const Color c = ((Color*)_pixels)[k];
			int bestd = std::numeric_limits<int>::max(), best = -1;
			for (int i = 0; i < colorCount; ++i) {
				const int red = (int)palette[i].r - (int)c.r;
				const int green = (int)palette[i].g - (int)c.g;
				const int blue = (int)palette[i].b - (int)c.b;
				const int alpha = (int)palette[i].a - (int)c.a;
				int d =
					blue * blue * _quantizationBlueWeight +
					green * green * _quantizationGreenWeight +
					red * red * _quantizationRedWeight;
				d += alpha * alpha * _quantizationAlphaWeight; // Alpha is usually more weighted.
				if (d < bestd) {
					bestd = d;
					best = i;
				}
			}
			if (best == -1)
				best = 0;
			palettedPixels[k] = (Byte)best;
		}

		free(_pixels);
		_pixels = palettedPixels;

		_palettedBits = IMAGE_PALETTE_BITS;
		_channels = 1;

		surface(nullptr);

		return true;
	}
	bool quantizeLinear(const Color* colors, int colorCount) {
		if (_palettedBits)
			return true;

		const int size = _width * _height;
		const Byte* const palette = (Byte*)colors;
		Byte* palettedPixels = (Byte*)malloc(size * sizeof(Byte));
		constexpr const int BPP = (sizeof(Color) / sizeof(Byte));

		Byte* ditheredPixels = new Byte[size * 4];
		if (_channels == 4) {
			memcpy(ditheredPixels, _pixels, size * 4);
		} else {
			for (int i = 0; i < size; ++i) {
				ditheredPixels[i * 4] = _pixels[i * 3];
				ditheredPixels[i * 4 + 1] = _pixels[i * 3 + 1];
				ditheredPixels[i * 4 + 2] = _pixels[i * 3 + 2];
				ditheredPixels[i * 4 + 3] = 255;
			}
		}
		for (int k = 0; k < size * 4; k += 4) {
			const int rgb[4] = { ditheredPixels[k + 0], ditheredPixels[k + 1], ditheredPixels[k + 2], ditheredPixels[k + 3] };
			int bestd = std::numeric_limits<int>::max(), best = -1;
			for (int i = 0; i < colorCount; ++i) {
				const int blue = palette[i * BPP + 0] - rgb[0];
				const int green = palette[i * BPP + 1] - rgb[1];
				const int red = palette[i * BPP + 2] - rgb[2];
				const int alpha = palette[i * BPP + 3] - rgb[3];
				int d =
					blue * blue * _quantizationBlueWeight +
					green * green * _quantizationGreenWeight +
					red * red * _quantizationRedWeight;
				d += alpha * alpha * _quantizationAlphaWeight; // Alpha is usually more weighted.
				if (d < bestd) {
					bestd = d;
					best = i;
				}
			}
			if (best == -1)
				best = 0;
			palettedPixels[k / 4] = (Byte)best;
			int diff[4] = {
				ditheredPixels[k + 0] - palette[palettedPixels[k / 4] * BPP + 0],
				ditheredPixels[k + 1] - palette[palettedPixels[k / 4] * BPP + 1],
				ditheredPixels[k + 2] - palette[palettedPixels[k / 4] * BPP + 2],
				ditheredPixels[k + 3] - palette[palettedPixels[k / 4] * BPP + 3]
			};
			if (k + 4 < size * 4) {
				ditheredPixels[k + 4 + 0] = (Byte)Math::clamp(ditheredPixels[k + 4 + 0] + (diff[0] * 7 / 16), 0, 255);
				ditheredPixels[k + 4 + 1] = (Byte)Math::clamp(ditheredPixels[k + 4 + 1] + (diff[1] * 7 / 16), 0, 255);
				ditheredPixels[k + 4 + 2] = (Byte)Math::clamp(ditheredPixels[k + 4 + 2] + (diff[2] * 7 / 16), 0, 255);
				ditheredPixels[k + 4 + 3] = (Byte)Math::clamp(ditheredPixels[k + 4 + 3] + (diff[3] * 7 / 16), 0, 255);
			}
			if (k + _width * 4 + 4 < size * 4) {
				for (int i = 0; i < 3; ++i) {
					ditheredPixels[k + _width * 4 - 4 + i] = (Byte)Math::clamp(ditheredPixels[k + _width * 4 - 4 + i] + (diff[i] * 3 / 16), 0, 255);
					ditheredPixels[k + _width * 4 + i] = (Byte)Math::clamp(ditheredPixels[k + _width * 4 + i] + (diff[i] * 5 / 16), 0, 255);
					ditheredPixels[k + _width * 4 + 4 + i] = (Byte)Math::clamp(ditheredPixels[k + _width * 4 + 4 + i] + (diff[i] * 1 / 16), 0, 255);
				}
			}
		}
		delete [] ditheredPixels;

		free(_pixels);
		_pixels = palettedPixels;

		_palettedBits = IMAGE_PALETTE_BITS;
		_channels = 1;

		surface(nullptr);

		return true;
	}
};

Image* Image::create(Palette::Ptr palette) {
	ImageImpl* result = new ImageImpl(palette);

	return result;
}

void Image::destroy(Image* ptr) {
	ImageImpl* impl = static_cast<ImageImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
