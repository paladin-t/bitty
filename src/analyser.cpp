/*
** Bitty
**
** An itty bitty 2D game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "analyser.h"
#include "../lib/kissfft/kiss_fft.h"
#include <SDL_mixer.h>

/*
** {===========================================================================
** Utilities
*/

typedef std::function<Complex(void* &, int, int)> Reader;

template<typename T> Complex analyserReadStream(void* &stream, int len, int channels, T max) {
	const int step = len / sizeof(T) / ANALYSER_COLUMN_COUNT;
	T* ptr = (T*)stream;
	const T r = *ptr;
	const T i = channels == 2 ? *(ptr + 1) : r;
	stream = ptr + step;

	if (std::is_unsigned<T>()) {
		return Complex(
			(float)r / max * 2.0f - 1.0f,
			(float)i / max * 2.0f - 1.0f
		);
	} else {
		return Complex(
			(float)r / max,
			(float)i / max
		);
	}
}

static Reader analyserReader(const Bitty::Analyser::Spec &spec) {
	const int bitSize = SDL_AUDIO_BITSIZE(spec.format);
	const bool isFloat = !!SDL_AUDIO_ISFLOAT(spec.format);
	const bool isInt = !!SDL_AUDIO_ISINT(spec.format);
	const bool isUnsigned = !!SDL_AUDIO_ISUNSIGNED(spec.format);
	if (isFloat && bitSize == sizeof(Bitty::Single) * 8) {
		return std::bind(
			analyserReadStream<Bitty::Single>,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
			1.0f
		);
	} else if (isInt && bitSize == sizeof(Bitty::UInt8) * 8) {
		if (isUnsigned) {
			return std::bind(
				analyserReadStream<Bitty::UInt8>,
				std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
				std::numeric_limits<Bitty::UInt8>::max()
			);
		} else {
			return std::bind(
				analyserReadStream<Bitty::Int8>,
				std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
				std::numeric_limits<Bitty::Int8>::max()
			);
		}
	} else if (isInt && bitSize == sizeof(Bitty::UInt16) * 8) {
		if (isUnsigned) {
			return std::bind(
				analyserReadStream<Bitty::UInt16>,
				std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
				std::numeric_limits<Bitty::UInt16>::max()
			);
		} else {
			return std::bind(
				analyserReadStream<Bitty::Int16>,
				std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
				std::numeric_limits<Bitty::Int16>::max()
			);
		}
	} else if (isInt && bitSize == sizeof(Bitty::UInt32) * 8) {
		if (isUnsigned) {
			return std::bind(
				analyserReadStream<Bitty::UInt32>,
				std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
				std::numeric_limits<Bitty::UInt32>::max()
			);
		} else {
			return std::bind(
				analyserReadStream<Bitty::Int32>,
				std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
				std::numeric_limits<Bitty::Int32>::max()
			);
		}
	} else {
		fprintf(stderr, "Unsupported audio spec: %dHz, by %d formatted, with %d channels.\n", spec.frequency, spec.format, spec.channels);

		return std::bind(
			analyserReadStream<Bitty::UInt8>,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
			std::numeric_limits<Bitty::UInt8>::max()
		);
	}
}

/* ===========================================================================} */

/*
** {===========================================================================
** Analyser
*/

namespace Bitty {

Analyser::Spec Analyser::deviceSpec(void) {
	Spec result;
	Mix_QuerySpec(&result.frequency, &result.format, &result.channels);

	return result;
}

void Analyser::analyse(const Spec &spec, void* stream, int len, Complex* complexes, int size) {
	kiss_fft_cfg fft = kiss_fft_alloc(size, 0, 0, 0);
	kiss_fft_cpx* in = new kiss_fft_cpx[size];
	kiss_fft_cpx* out = new kiss_fft_cpx[size];

	void* ptr = stream;
	for (int i = 0; i < size; ++i) {
		const Complex r = analyserReader(spec)(ptr, len, spec.channels);
		in[i] = kiss_fft_cpx{ r.real(), r.imag() };
	}
	kiss_fft(fft, in, out);

	for (int i = 0; i < size; ++i)
		complexes[i] = Complex(out[i].r + out[i].i);
	kiss_fft_free(fft);

	delete [] in;
	delete [] out;
}

void Analyser::analyse(const Spec &spec, void* stream, int len, Complex complexes[ANALYSER_COLUMN_COUNT]) {
	kiss_fft_cfg fft = kiss_fft_alloc(ANALYSER_COLUMN_COUNT, 0, 0, 0);
	kiss_fft_cpx in[ANALYSER_COLUMN_COUNT];
	kiss_fft_cpx out[ANALYSER_COLUMN_COUNT];

	void* ptr = stream;
	for (int i = 0; i < ANALYSER_COLUMN_COUNT; ++i) {
		const Complex r = analyserReader(spec)(ptr, len, spec.channels);
		in[i] = kiss_fft_cpx{ r.real(), r.imag() };
	}
	kiss_fft(fft, in, out);

	for (int i = 0; i < ANALYSER_COLUMN_COUNT; ++i)
		complexes[i] = Complex(out[i].r + out[i].i);
	kiss_fft_free(fft);
}

void Analyser::analyse(const Spec &spec, void* stream, int len, float* columns, int size) {
	kiss_fft_cfg fft = kiss_fft_alloc(size, 0, 0, 0);
	kiss_fft_cpx* in = new kiss_fft_cpx[size];
	kiss_fft_cpx* out = new kiss_fft_cpx[size];

	void* ptr = stream;
	for (int i = 0; i < size; ++i) {
		const Complex r = analyserReader(spec)(ptr, len, spec.channels);
		in[i] = kiss_fft_cpx{ r.real(), r.imag() };
	}
	kiss_fft(fft, in, out);

	for (int i = 0; i < size; ++i)
		columns[i] = Math::clamp((out[i].r + out[i].i) * 0.5f, -0.9f, 0.9f);
	kiss_fft_free(fft);

	delete [] in;
	delete [] out;
}

void Analyser::analyse(const Spec &spec, void* stream, int len, float columns[ANALYSER_COLUMN_COUNT]) {
	kiss_fft_cfg fft = kiss_fft_alloc(ANALYSER_COLUMN_COUNT, 0, 0, 0);
	kiss_fft_cpx in[ANALYSER_COLUMN_COUNT];
	kiss_fft_cpx out[ANALYSER_COLUMN_COUNT];

	void* ptr = stream;
	for (int i = 0; i < ANALYSER_COLUMN_COUNT; ++i) {
		const Complex r = analyserReader(spec)(ptr, len, spec.channels);
		in[i] = kiss_fft_cpx{ r.real(), r.imag() };
	}
	kiss_fft(fft, in, out);

	for (int i = 0; i < ANALYSER_COLUMN_COUNT; ++i)
		columns[i] = Math::clamp((out[i].r + out[i].i) * 0.5f, -0.9f, 0.9f);
	kiss_fft_free(fft);
}

}

/* ===========================================================================} */
