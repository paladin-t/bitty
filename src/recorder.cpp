/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#define NOMINMAX
#include "bytes.h"
#include "encoding.h"
#include "file_handle.h"
#include "filesystem.h"
#include "image.h"
#include "platform.h"
#include "recorder.h"
#include "text.h"
#include "texture.h"
#include "../lib/jo_gif/jo_gif.h"
#include "../lib/lz4/lib/lz4.h"
#if defined BITTY_CP_VC
#	pragma warning(push)
#	pragma warning(disable : 4800)
#	pragma warning(disable : 4819)
#endif /* BITTY_CP_VC */
#if defined BITTY_OS_WIN || defined BITTY_OS_MAC || defined BITTY_OS_LINUX
#	include "../lib/portable_file_dialogs/portable-file-dialogs.h"
#elif defined BITTY_OS_HTML
#	include "../lib/portable_file_dialogs_polyfill/portable-file-dialogs.h"
#else /* Platform macro. */
#	include "../lib/portable_file_dialogs_polyfill/portable-file-dialogs.h"
#endif /* Platform macro. */
#if defined BITTY_CP_VC
#	pragma warning(pop)
#endif /* BITTY_CP_VC */
#include <SDL.h>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef RECORDER_SKIP_FRAME_COUNT
#	define RECORDER_SKIP_FRAME_COUNT 5
#endif /* RECORDER_SKIP_FRAME_COUNT */

#ifndef RECORDER_FOOTPRINT_LIMIT
#	define RECORDER_FOOTPRINT_LIMIT (1024 * 1024 * 512) // 512MB.
#endif /* RECORDER_FOOTPRINT_LIMIT */

/* ===========================================================================} */

/*
** {===========================================================================
** Recorder
*/

class RecorderImpl : public Recorder {
private:
	typedef std::list<Bytes::Ptr> Frames;

private:
	SaveHandler _save = nullptr;
	unsigned _fps = BITTY_ACTIVE_FRAME_RATE;
	unsigned _frameSkipping = 0;

	int _width = 0;
	int _height = 0;

	int _recording = 0;
	Frames _frames;

	Bytes::Ptr _cache = nullptr;
	int _footprint = 0;

public:
	RecorderImpl(SaveHandler save, unsigned fps) : _save(save), _fps(fps) {
	}
	virtual ~RecorderImpl() override {
		clear();
	}

	virtual bool recording(void) const override {
		return !!_recording;
	}

	virtual void start(int frameCount) override {
		_frameSkipping = RECORDER_SKIP_FRAME_COUNT;

		_recording = std::max(frameCount, 1);
		_footprint = 0;
	}
	virtual void stop(void) override {
		promise::Defer canSave = promise::newPromise([] (promise::Defer df) -> void { df.resolve(); });
		if (_save)
			canSave = _save();

		canSave
			.then(
				[&] (void) -> void {
					save();
				}
			)
			.always(
				[&] (void) -> void {
					clear();
				}
			);
	}

	virtual void update(class Window* /* wnd */, class Renderer* rnd, class Texture* tex) override {
		if (!_recording)
			return;

		if (_width == 0 && _height == 0) {
			_width = tex->width();
			_height = tex->height();
		}

		if (_width != tex->width() || _height != tex->height()) { // Size changed.
			stop();

			return;
		}

		if (++_frameSkipping == RECORDER_SKIP_FRAME_COUNT + 1) {
			_frameSkipping = 0;

			if (!_cache)
				_cache = Bytes::Ptr(Bytes::create());
			_cache->resize(_width * _height * sizeof(Color));
			tex->toBytes(rnd, _cache->pointer()); // Save the raw RGBA pixels.

			Bytes::Ptr compressed(Bytes::create());
			int n = LZ4_compressBound((int)_cache->count());
			compressed->resize((size_t)n);
			n = LZ4_compress_default( // Compress the raw RGBA pixels.
				(const char*)_cache->pointer(), (char*)compressed->pointer(),
				(int)_cache->count(), (int)compressed->count()
			);
			assert(n);
			if (n) {
				compressed->resize((size_t)n);

				_footprint += n;
			}
			_frames.push_back(compressed); // Add the compressed frame.
			_cache->clear();

			if (_recording > 0 && --_recording == 0) // Frame limit reached.
				stop();
			else if (_footprint >= RECORDER_FOOTPRINT_LIMIT) // Footprint limit reached.
				stop();
		}
	}

private:
	void save(void) {
		// Prepare.
		if (_frames.empty())
			return;

		const bool single = _frames.size() == 1;

		fprintf(stdout, "Recorded %d frames in %d bytes.\n", (int)_frames.size(), _footprint);

		// Ask for a saving path.
		pfd::save_file save(
			BITTY_NAME,
			"",
			single ? std::vector<std::string>{ "PNG files (*.png)", "*.png" } : std::vector<std::string>{ "GIF files (*.gif)", "*.gif" }
		);
		std::string path = save.result();
		Path::uniform(path);
		if (path.empty())
			return;
		if (!Text::endsWith(path, single ? ".png" : ".gif", true))
			path += single ? ".png" : ".gif";

		// Save.
		Image::Ptr img(Image::create(nullptr));
		if (single) {
			// Get the only frame.
			Bytes::Ptr compressed = _frames.front();

			toImage(compressed, img); // Decompress and load the frame to an image object.

			img->toBytes(_cache.get(), "png"); // Save the image object to cache as PNG.

			// Save to PNG file.
			File::Ptr file(File::create());
			if (file->open(path.c_str(), Stream::WRITE)) {
				file->writeBytes(_cache.get()); // Save the png cache to file.
				file->close();
			}

			// Clear the cache.
			_cache->clear();
		} else {
			// Get the initial.
			const Color RECORDER_DEFAULT_COLORS_ARRAY[] = PALETTE_DEFAULT_COLORS;
			constexpr const int COLOR_COUNT = std::min((int)BITTY_COUNTOF(RECORDER_DEFAULT_COLORS_ARRAY), 255);
			std::vector<Color> colors;
			for (int i = 0; i < 16; ++i)
				colors.push_back(RECORDER_DEFAULT_COLORS_ARRAY[i]);

			// Fill colors with some frames.
			struct Less {
				bool operator () (const Color &left, const Color &right) const {
					return left.toRGBA() < right.toRGBA();
				}
			};
			std::set<Color, Less> filled;
			auto fillColor = [&] (Image::Ptr &img, const Bytes::Ptr &frame) -> void {
				if ((int)colors.size() >= COLOR_COUNT)
					return;

				toImage(frame, img);

				for (int j = 0; j < img->height(); ++j) {
					for (int i = 0; i < img->width(); ++i) {
						Color col;
						img->get(i, j, col);
						auto it = filled.find(col);
						if (it == filled.end()) {
							filled.insert(col);
							colors.push_back(col);
							if ((int)colors.size() >= COLOR_COUNT)
								break;
						}
					}
					if ((int)colors.size() >= COLOR_COUNT)
						break;

					Platform::idle();
				}
			};

			if (_frames.size() >= 1) {
				Bytes::Ptr frame = _frames.front();
				fillColor(img, frame);
			}
			if (_frames.size() >= 2) {
				Bytes::Ptr frame = _frames.back();
				fillColor(img, frame);
			}
			if (_frames.size() >= 3) {
				Frames::iterator it = _frames.begin();
				std::advance(it, _frames.size() / 2);
				Bytes::Ptr frame = *it;
				fillColor(img, frame);
			}

			while ((int)colors.size() < COLOR_COUNT)
				colors.push_back(Color(0x80, 0x80, 0x80));

			// Create a GIF object.
			std::string osstr = Unicode::toOs(path);
			jo_gif_t gif = jo_gif_start( // Start a GIF encoding.
				osstr.c_str(),
				(short)_width, (short)_height,
				0, COLOR_COUNT
			);

			// Fill palette.
			for (int i = 0; i < COLOR_COUNT; ++i) {
				const Color col = colors[i];
				gif.palette[i * 3 + 0] = col.r;
				gif.palette[i * 3 + 1] = col.g;
				gif.palette[i * 3 + 2] = col.b;
			}

			// Stream to GIF.
			const short INTERVAL = std::max((short)((1.0f / _fps) * 100 * (RECORDER_SKIP_FRAME_COUNT + 1)), (short)1);
			for (Bytes::Ptr compressed : _frames) {
				toImage(compressed, img); // Decompress and load a frame to an image object.

				jo_gif_frame(&gif, img->pixels(), INTERVAL, false, true); // Add a GIF frame.

				Platform::idle();
			}

			// Finish streaming to GIF.
			jo_gif_end(&gif); // End the GIF encoding.
		}

		// Finish.
		FileInfo::Ptr fileInfo = FileInfo::make(path.c_str());
		if (fileInfo->exists()) {
			const std::string osstr = Unicode::toOs(fileInfo->parentPath());
			Platform::browse(osstr.c_str());
		}
	}

	void clear(void) {
		_frameSkipping = 0;

		_width = 0;
		_height = 0;

		_recording = 0;
		_frames.clear();

		_cache = nullptr;
		_footprint = 0;
	}

	void toImage(Bytes::Ptr compressed, Image::Ptr img) {
		constexpr const Byte IMAGE_COLORED_HEADER_BYTES[] = IMAGE_COLORED_HEADER;
		const size_t headerSize = BITTY_COUNTOF(IMAGE_COLORED_HEADER_BYTES) +
			sizeof(int) + sizeof(int) +
			sizeof(int);
		_cache->resize(headerSize + _width * _height * sizeof(Color));
		for (int i = 0; i < BITTY_COUNTOF(IMAGE_COLORED_HEADER_BYTES); ++i)
			_cache->writeByte(IMAGE_COLORED_HEADER_BYTES[i]);
		_cache->writeInt32(_width);
		_cache->writeInt32(_height);
		_cache->writeInt32(0);

		const int n = LZ4_decompress_safe(
			(const char*)compressed->pointer(), (char*)_cache->pointer() + headerSize,
			(int)compressed->count(), (int)_cache->count() - headerSize
		);
		(void)n;
		assert(n && n == (int)(_cache->count() - headerSize)); (void)n;

		img->fromBytes(_cache.get());
		_cache->clear();
	}
};

Recorder::~Recorder() {
}

Recorder* Recorder::create(SaveHandler save, unsigned fps) {
	RecorderImpl* result = new RecorderImpl(save, fps);

	return result;
}

void Recorder::destroy(Recorder* ptr) {
	RecorderImpl* impl = static_cast<RecorderImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
