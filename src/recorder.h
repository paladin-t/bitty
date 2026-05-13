/*
** Bitty
**
** An itty bitty 2D game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __RECORDER_H__
#define __RECORDER_H__

#include "bitty.h"
#include "image.h"
#if defined BITTY_CP_VC
#	pragma warning(push)
#	pragma warning(disable : 4100)
#endif /* BITTY_CP_VC */
#include "../lib/promise_cpp/promise.hpp"
#if defined BITTY_CP_VC
#	pragma warning(pop)
#endif /* BITTY_CP_VC */
#include <functional>

/*
** {===========================================================================
** Recorder
*/

namespace Bitty {

/**
 * @brief Recorder utilities.
 */
class Recorder {
public:
	typedef std::function<promise::Defer(void)> SaveHandler;

public:
	virtual ~Recorder();

	virtual bool recording(void) const = 0;

	virtual void startRecordingToImage(void) = 0;
	virtual Image::Ptr recordedImage(bool clear) = 0;

	virtual void start(int frameCount = -1) = 0;
	virtual void stop(void) = 0;

	virtual void update(class Window* wnd, class Renderer* rnd, class Texture* tex) = 0;

	static Recorder* create(SaveHandler save, unsigned fps);
	static void destroy(Recorder* ptr);
};

}

/* ===========================================================================} */

#endif /* __RECORDER_H__ */
