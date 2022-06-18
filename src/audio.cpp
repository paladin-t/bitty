/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "audio.h"
#include "bytes.h"
#include "filesystem.h"
#include <SDL_mixer.h>

/*
** {===========================================================================
** Sound
*/

static uintptr_t _soundOccupation = 0;

class SoundImpl : public Sound {
private:
	mutable std::string _title;
	std::string _path;

	Bytes* _buffer = nullptr;
	Mix_Music* _music = nullptr;
	mutable bool _playing = false;
	FeedHandler _feeder = nullptr;
	StopHandler _stopper = nullptr;

public:
	SoundImpl() {
	}
	virtual ~SoundImpl() override {
		stop();

		clear();
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Object** ptr) const override { // Non-clonable.
		if (ptr)
			*ptr = nullptr;

		return false;
	}

	virtual const Byte* buffer(size_t* len) override {
		if (len)
			*len = 0;

		if (!_buffer)
			return nullptr;

		if (len)
			*len = _buffer->count();

		return _buffer->pointer();
	}

	virtual const char* path(size_t* len) const override {
		if (len)
			*len = _path.length();

		if (_path.empty())
			return nullptr;

		return _path.c_str();
	}
	virtual void path(const char* val, size_t len) override {
		if (len)
			_path.assign(val, len);
		else
			_path.assign(val);
	}

	virtual const char* title(void) const override {
		if (_title.empty()) {
#if !defined BITTY_OS_HTML
			if (_music) {
				const char* tmp = Mix_GetMusicTitle(_music);
				if (tmp)
					_title = tmp;
			}
#endif /* BITTY_OS_HTML */
			if (_title.empty()) {
				Path::split(_path, &_title, nullptr, nullptr);
			}
		}

		return _title.c_str();
	}
	virtual const char* artist(void) const override {
#if defined BITTY_OS_HTML
		return nullptr;
#else /* BITTY_OS_HTML */
		if (!_music)
			return nullptr;

		return Mix_GetMusicArtistTag(_music);
#endif /* BITTY_OS_HTML */
	}
	virtual const char* album(void) const override {
#if defined BITTY_OS_HTML
		return nullptr;
#else /* BITTY_OS_HTML */
		if (!_music)
			return nullptr;

		return Mix_GetMusicAlbumTag(_music);
#endif /* BITTY_OS_HTML */
	}
	virtual const char* copyright(void) const override {
#if defined BITTY_OS_HTML
		return nullptr;
#else /* BITTY_OS_HTML */
		if (!_music)
			return nullptr;

		return Mix_GetMusicCopyrightTag(_music);
#endif /* BITTY_OS_HTML */
	}

	virtual double length(void) const override {
#if defined BITTY_OS_HTML
		return 0;
#else /* BITTY_OS_HTML */
		if (!_music)
			return 0;

		return Mix_MusicDuration(_music);
#endif /* BITTY_OS_HTML */
	}

	virtual double position(void) const override {
#if defined BITTY_OS_HTML
		return 0;
#else /* BITTY_OS_HTML */
		if (!_music)
			return 0;

		return Mix_GetMusicPosition(_music);
#endif /* BITTY_OS_HTML */
	}
	virtual bool position(double pos) override {
		if (!_music)
			return false;

		return Mix_SetMusicPosition(pos) == 0;
	}

	virtual bool playing(void) const override {
		return _playing;
	}
	virtual bool play(bool loop, FeedHandler feeder, StopHandler stopper) override {
		assert(!_soundOccupation);

		_feeder = feeder;
		_stopper = stopper;

		_playing = true;

		Mix_PlayMusic(_music, loop ? -1 : 0);

		_soundOccupation = (uintptr_t)this;

		Mix_RegisterEffect(MIX_CHANNEL_POST, soundFed, soundDone, this);

		return true;
	}
	virtual bool paused(void) const override {
		if (_soundOccupation != (uintptr_t)this) {
			_playing = false;

			return false;
		}

		return !!Mix_PausedMusic();
	}
	virtual void pause(void) override {
		if (_soundOccupation != (uintptr_t)this) {
			_playing = false;

			return;
		}

		Mix_PauseMusic();
	}
	virtual void resume(void) override {
		if (_soundOccupation != (uintptr_t)this) {
			_playing = false;

			return;
		}

		Mix_ResumeMusic();
	}
	virtual void rewind(void) override {
		if (_soundOccupation != (uintptr_t)this) {
			_playing = false;

			return;
		}

		Mix_RewindMusic();
	}
	virtual bool stop(void) override {
		if (_soundOccupation != (uintptr_t)this) {
			_playing = false;

			_feeder = nullptr;
			_stopper = nullptr;

			return false;
		}

		if (_playing)
			_playing = false;

		Mix_SetMusicPosition(0);
		Mix_HaltMusic();

		_soundOccupation = 0;

		_feeder = nullptr;
		_stopper = nullptr;

		Mix_UnregisterEffect(MIX_CHANNEL_POST, soundFed);

		return true;
	}

	virtual void update(void) override {
		if (!_playing)
			return;
		_playing = !!Mix_PlayingMusic();
		if (_playing)
			return;

		if (_stopper)
			_stopper();

		Mix_SetMusicPosition(0);
		Mix_HaltMusic();

		_soundOccupation = 0;

		_feeder = nullptr;
		_stopper = nullptr;

		Mix_UnregisterEffect(MIX_CHANNEL_POST, soundFed);
	}

	virtual void clear(void) override {
		const uintptr_t inuse = _soundOccupation;
		if (inuse == (uintptr_t)this)
			_soundOccupation = 0;

		if (_playing)
			stop();
		if (_music) {
			Mix_FreeMusic(_music);
			_music = nullptr;
		}
		if (_buffer) {
			Bytes::destroy(_buffer);
			_buffer = nullptr;
		}

		_feeder = nullptr;
		_stopper = nullptr;

		if (inuse == (uintptr_t)this)
			Mix_UnregisterEffect(MIX_CHANNEL_POST, soundFed);
	}

	virtual bool toBytes(class Bytes* val) const override {
		val->clear();

		if (!_buffer)
			return false;

		_buffer->poke(0);
		_buffer->readBytes(val);

		return true;
	}
	virtual bool fromBytes(const Byte* val, size_t size) override {
		if (!val || size == 0)
			return false;

		if (_music) {
			Mix_FreeMusic(_music);
			_music = nullptr;
		}
		if (_buffer) {
			Bytes::destroy(_buffer);
			_buffer = nullptr;
		}

		_buffer = Bytes::create();
		_buffer->writeBytes(val, size);
		_buffer->poke(0);

		_title.clear();
		_path.clear();
		_music = Mix_LoadMUS_RW(SDL_RWFromConstMem(_buffer->pointer(), (int)_buffer->count()), SDL_TRUE);
		_playing = false;
		_feeder = nullptr;
		_stopper = nullptr;

		return true;
	}
	virtual bool fromBytes(const class Bytes* val) override {
		return fromBytes(val->pointer(), val->count());
	}

private:
	static void soundFed(int chan, void* stream, int len, void* udata) {
		(void)chan;

		assert(chan == MIX_CHANNEL_POST);

		SoundImpl* impl = (SoundImpl*)udata;
		FeedHandler feeder = impl->_feeder;

		if (!feeder)
			return;

		feeder(stream, len);
	}
	static void soundDone(int chan, void* /* stream */) {
		(void)chan;

		assert(chan == MIX_CHANNEL_POST);
	}
};

Sound* Sound::create(void) {
	SoundImpl* result = new SoundImpl();

	return result;
}

void Sound::destroy(Sound* ptr) {
	SoundImpl* impl = static_cast<SoundImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */

/*
** {===========================================================================
** Sfx
*/

struct SfxOccupation {
	uintptr_t _channels[AUDIO_SFX_CHANNEL_COUNT];

	SfxOccupation() {
		memset(_channels, 0, sizeof(uintptr_t) * AUDIO_SFX_CHANNEL_COUNT);
	}

	uintptr_t get(int channel) {
		if (channel < 0 || channel >= AUDIO_SFX_CHANNEL_COUNT)
			return 0;

		return _channels[channel];
	}
	void set(int channel, uintptr_t data) {
		if (channel < 0 || channel >= AUDIO_SFX_CHANNEL_COUNT)
			return;

		_channels[channel] = data;
	}

	static int findFreeChannel(void) {
		for (int i = 0; i < AUDIO_SFX_CHANNEL_COUNT; ++i) {
			if (!Mix_Playing(i))
				return i;
		}

		return -1;
	}
};

static SfxOccupation _sfxOccupation;

class SfxImpl : public Sfx {
private:
	Mix_Chunk* _chunk = nullptr;
	Bytes* _bytes = nullptr;

	mutable int _channel = -1;

public:
	SfxImpl() {
	}
	virtual ~SfxImpl() override {
		stop(nullptr);

		clear();
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Object** ptr) const override { // Non-clonable.
		if (ptr)
			*ptr = nullptr;

		return false;
	}

	virtual int play(bool loop, const int* fadeInMs, int channel) override {
		int ch = channel;
		if (ch < 0 || ch >= AUDIO_SFX_CHANNEL_COUNT)
			ch = SfxOccupation::findFreeChannel();
		if (ch < 0)
			return -1;

		if (fadeInMs)
			Mix_FadeInChannelTimed(ch, _chunk, loop ? -1 : 0, *fadeInMs, -1);
		else
			Mix_PlayChannelTimed(ch, _chunk, loop ? -1 : 0, -1);

		_channel = ch;

		_sfxOccupation.set(ch, (uintptr_t)this);

		return ch;
	}
	virtual bool paused(void) const override {
		if (_channel < 0 || _channel >= AUDIO_SFX_CHANNEL_COUNT)
			return false;

		const uintptr_t inuse = _sfxOccupation.get(_channel);
		if (inuse != (uintptr_t)this) {
			_channel = -1;

			return false;
		}

		return !!Mix_Paused(_channel);
	}
	virtual void pause(void) override {
		if (_channel < 0 || _channel >= AUDIO_SFX_CHANNEL_COUNT)
			return;

		const uintptr_t inuse = _sfxOccupation.get(_channel);
		if (inuse != (uintptr_t)this) {
			_channel = -1;

			return;
		}

		Mix_Pause(_channel);
	}
	virtual void resume(void) override {
		if (_channel < 0 || _channel >= AUDIO_SFX_CHANNEL_COUNT)
			return;

		const uintptr_t inuse = _sfxOccupation.get(_channel);
		if (inuse != (uintptr_t)this) {
			_channel = -1;

			return;
		}

		Mix_Resume(_channel);
	}
	virtual bool stop(const int* fadeOutMs) override {
		if (_channel == -1)
			return false;

		const uintptr_t inuse = _sfxOccupation.get(_channel);
		if (inuse != (uintptr_t)this) {
			_channel = -1;

			return false;
		}

		if (fadeOutMs)
			Mix_FadeOutChannel(_channel, *fadeOutMs);
		else
			Mix_HaltChannel(_channel);

		_sfxOccupation.set(_channel, 0);

		_channel = -1;

		return true;
	}

	virtual void clear(void) override {
		const uintptr_t inuse = _sfxOccupation.get(_channel);
		if (inuse == (uintptr_t)this) {
			_sfxOccupation.set(_channel, 0);

			_channel = -1;
		}

		if (_chunk) {
			Mix_FreeChunk(_chunk);
			_chunk = nullptr;
		}
		if (_bytes) {
			Bytes::destroy(_bytes);
			_bytes = nullptr;
		}
	}

	virtual bool toBytes(class Bytes* val) const override {
		val->clear();

		if (!_bytes)
			return false;

		_bytes->poke(0);
		_bytes->readBytes(val);

		return true;
	}
	virtual bool fromBytes(const Byte* val, size_t size, const AudioSpec &spec) override {
		if (!val)
			return false;

		if (_chunk) {
			Mix_FreeChunk(_chunk);
			_chunk = nullptr;
		}
		if (_bytes) {
			Bytes::destroy(_bytes);
			_bytes = nullptr;
		}

		SDL_AudioSpec srcSpec;
		memset(&srcSpec, 0, sizeof(SDL_AudioSpec));
		srcSpec.freq = spec.freq;
		srcSpec.format = (SDL_AudioFormat)spec.format;
		srcSpec.channels = spec.channels;
		srcSpec.silence = spec.silence;
		srcSpec.samples = spec.samples;
		srcSpec.padding = spec.padding;
		srcSpec.size = spec.size;
		srcSpec.callback = (SDL_AudioCallback)spec.callback;
		srcSpec.userdata = spec.userdata;
		SDL_AudioSpec dstSpec;
		SDL_AudioCVT cvt;
		memset(&dstSpec, 0, sizeof(SDL_AudioSpec));
		memset(&cvt, 0, sizeof(SDL_AudioCVT));
		dstSpec.freq = AUDIO_TARGET_SAMPLE_RATE;
		dstSpec.format = (SDL_AudioFormat)AUDIO_TARGET_FORMAT;
		dstSpec.channels = (Uint8)AUDIO_TARGET_CHANNEL_COUNT;
		SDL_BuildAudioCVT(
			&cvt,
			spec.format, spec.channels, spec.freq,
			dstSpec.format, dstSpec.channels, dstSpec.freq
		);
		cvt.len = (int)size;
		cvt.buf = (Uint8*)SDL_malloc(cvt.len * cvt.len_mult);
		memset(cvt.buf, 0, cvt.len * cvt.len_mult);
		if (cvt.needed) {
			SDL_memcpy(cvt.buf, val, size);
			SDL_ConvertAudio(&cvt);
		} else {
			SDL_memcpy(cvt.buf, val, size);
		}
		_bytes = Bytes::create();
		_bytes->writeBytes((Byte*)cvt.buf, (size_t)cvt.len_cvt);
		_bytes->poke(0);
		if (cvt.buf) {
			SDL_free(cvt.buf);
			cvt.buf = nullptr;
		}

		_chunk = Mix_QuickLoad_RAW((Uint8*)_bytes->pointer(), (Uint32)_bytes->count());

		return true;
	}
	virtual bool fromBytes(const class Bytes* val, const AudioSpec &spec) override {
		return fromBytes(val->pointer(), val->count(), spec);
	}
	virtual bool fromBytes(const Byte* val, size_t size) override {
		if (!val)
			return false;

		if (_chunk) {
			Mix_FreeChunk(_chunk);
			_chunk = nullptr;
		}
		if (_bytes) {
			Bytes::destroy(_bytes);
			_bytes = nullptr;
		}

		_bytes = Bytes::create();
		_bytes->writeBytes(val, size);
		_bytes->poke(0);

		_chunk = Mix_LoadWAV_RW(SDL_RWFromMem(_bytes->pointer(), (int)_bytes->count()), SDL_TRUE);

		return true;
	}
	virtual bool fromBytes(const class Bytes* val) override {
		return fromBytes(val->pointer(), val->count());
	}
};

Sfx* Sfx::create(void) {
	SfxImpl* result = new SfxImpl();

	return result;
}

void Sfx::destroy(Sfx* ptr) {
	SfxImpl* impl = static_cast<SfxImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */

/*
** {===========================================================================
** Music
*/

static uintptr_t _musicOccupation = 0;

class MusicImpl : public Music {
private:
	Mix_Music* _music = nullptr;
	Bytes* _bytes = nullptr;

	double _length = 0.0;
	mutable bool _playing = false;

public:
	MusicImpl() {
	}
	virtual ~MusicImpl() {
		stop(nullptr);

		clear();
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Object** ptr) const override { // Non-clonable.
		if (ptr)
			*ptr = nullptr;

		return false;
	}

	virtual double length(void) const override {
#if defined BITTY_OS_HTML
		return 0;
#else /* BITTY_OS_HTML */
		if (!_music)
			return 0;

		return _length;
#endif /* BITTY_OS_HTML */
	}

	virtual bool playing(void) const override {
		return _playing;
	}
	virtual bool play(bool loop, const int* fadeInMs, const double* pos) override {
		_playing = true;

		if (fadeInMs)
			Mix_FadeInMusic(_music, loop ? -1 : 0, *fadeInMs);
		else
			Mix_PlayMusic(_music, loop ? -1 : 0);
		if (pos)
			Mix_SetMusicPosition(*pos);

		_musicOccupation = (uintptr_t)this;

		return true;
	}
	virtual bool paused(void) const override {
		if (_musicOccupation != (uintptr_t)this) {
			_playing = false;

			return false;
		}

		return !!Mix_PausedMusic();
	}
	virtual void pause(void) override {
		if (_musicOccupation != (uintptr_t)this) {
			_playing = false;

			return;
		}

		Mix_PauseMusic();
	}
	virtual void resume(void) override {
		if (_musicOccupation != (uintptr_t)this) {
			_playing = false;

			return;
		}

		Mix_ResumeMusic();
	}
	virtual void rewind(void) override {
		if (_musicOccupation != (uintptr_t)this) {
			_playing = false;

			return;
		}

		Mix_RewindMusic();
	}
	virtual bool stop(const int* fadeOutMs) override {
		if (_musicOccupation != (uintptr_t)this) {
			_playing = false;

			return false;
		}

		if (_playing)
			_playing = false;

		if (fadeOutMs)
			Mix_FadeOutMusic(*fadeOutMs);
		else
			Mix_HaltMusic();

		_musicOccupation = 0;

		return true;
	}

	virtual void clear(void) override {
		if (_playing)
			stop(nullptr);
		_length = 0;
		if (_music) {
			Mix_FreeMusic(_music);
			_music = nullptr;
		}
		if (_bytes) {
			Bytes::destroy(_bytes);
			_bytes = nullptr;
		}

		if (_musicOccupation == (uintptr_t)this)
			_musicOccupation = 0;
	}

	virtual bool toBytes(class Bytes* val) const override {
		val->clear();

		if (!_bytes)
			return false;

		_bytes->poke(0);
		_bytes->readBytes(val);

		return true;
	}
	virtual bool fromBytes(const Byte* val, size_t size) override {
		if (!val)
			return false;

		if (_music) {
			Mix_FreeMusic(_music);
			_music = nullptr;
		}
		if (_bytes) {
			Bytes::destroy(_bytes);
			_bytes = nullptr;
		}

		_bytes = Bytes::create();
		_bytes->writeBytes(val, size);
		_bytes->poke(0);

		_music = Mix_LoadMUS_RW(SDL_RWFromMem(_bytes->pointer(), (int)_bytes->count()), SDL_TRUE);
#if defined BITTY_OS_HTML
		_length = 0.0;
#else /* BITTY_OS_HTML */
		_length = Mix_MusicDuration(_music);
#endif /* BITTY_OS_HTML */
		_playing = false;

		return true;
	}
	virtual bool fromBytes(const class Bytes* val) override {
		return fromBytes(val->pointer(), val->count());
	}
};

Music* Music::create(void) {
	MusicImpl* result = new MusicImpl();

	return result;
}

void Music::destroy(Music* ptr) {
	MusicImpl* impl = static_cast<MusicImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */

/*
** {===========================================================================
** Audio
*/

class AudioImpl : public Audio {
private:
	bool _opened = false;

public:
	AudioImpl() {
	}
	virtual ~AudioImpl() {
	}

	virtual bool open(void) override {
		if (_opened)
			return false;
		_opened = true;

		if (Mix_AllocateChannels(AUDIO_SFX_CHANNEL_COUNT) != AUDIO_SFX_CHANNEL_COUNT) {
			fprintf(stderr, "Unable to allocate mixing channels: %s\n", SDL_GetError());

			return false;
		}

		fprintf(stdout, "Audio opened.\n");

		return true;
	}
	virtual bool close(void) override {
		if (!_opened)
			return false;
		_opened = false;

		Mix_UnregisterAllEffects(MIX_CHANNEL_POST);

		fprintf(stdout, "Audio closed.\n");

		return true;
	}

	virtual SfxVolume sfxVolume(void) const override {
		SfxVolume result;
		for (int i = 0; i < AUDIO_SFX_CHANNEL_COUNT; ++i) {
			const int vol = Mix_Volume(i, -1);
			result[i] = Math::clamp(vol / (float)SDL_MIX_MAXVOLUME, 0.0f, 1.0f);
		}

		return result;
	}
	virtual void sfxVolume(const SfxVolume &vol) override {
		for (int i = 0; i < AUDIO_SFX_CHANNEL_COUNT; ++i) {
			if (vol[i] < 0)
				continue;

			const int vol_ = Math::clamp((int)(vol[i] * (float)SDL_MIX_MAXVOLUME), 0, SDL_MIX_MAXVOLUME);
			Mix_Volume(i, vol_);
		}
	}
	virtual void sfxVolume(float vol) override {
		if (vol < 0)
			return;

		const int vol_ = Math::clamp((int)(vol * (float)SDL_MIX_MAXVOLUME), 0, SDL_MIX_MAXVOLUME);
		Mix_Volume(-1, vol_);
	}

	virtual float musicVolume(void) const override {
		const int vol = Mix_VolumeMusic(-1);

		return Math::clamp(vol / (float)SDL_MIX_MAXVOLUME, 0.0f, 1.0f);
	}
	virtual void musicVolume(float vol) override {
		if (vol < 0)
			return;

		Mix_VolumeMusic(Math::clamp((int)(vol * (float)SDL_MIX_MAXVOLUME), 0, SDL_MIX_MAXVOLUME));
	}

	virtual int soundFonts(Text::Array &paths) const override {
		paths.clear();

		return Mix_EachSoundFont(
			[] (const char* path, void* userdata) -> int {
				Text::Array* coll = (Text::Array*)userdata;
				coll->push_back(path);

				return 1;
			},
			&paths
		);
	}
	virtual const char* soundFonts(void) const override {
		return Mix_GetSoundFonts();
	}
	virtual int soundFonts(const char* paths) override {
		return Mix_SetSoundFonts(paths);
	}

	virtual void update(double) override {
		if (_musicOccupation) {
			if (!Mix_PlayingMusic()) {
				MusicImpl* impl = (MusicImpl*)_musicOccupation;
				impl->stop(nullptr);
			}
		}
	}

	virtual void reset(void) override {
		Mix_Volume(-1, SDL_MIX_MAXVOLUME);
		Mix_VolumeMusic(SDL_MIX_MAXVOLUME);

		Mix_SetSoundFonts(nullptr);
	}
};

Audio* Audio::create(void) {
	AudioImpl* result = new AudioImpl();

	return result;
}

void Audio::destroy(Audio* ptr) {
	AudioImpl* impl = static_cast<AudioImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
