/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __HACKS_H__
#define __HACKS_H__

#include "bitty.h"
#if BITTY_MULTITHREAD_ENABLED && defined BITTY_DEBUG
#	include <thread>
#endif /* BITTY_MULTITHREAD_ENABLED && BITTY_DEBUG */

/*
** {===========================================================================
** Macros and constants
*/

#ifndef BITTY_THREADING_GUARD_ENABLED
#	if BITTY_MULTITHREAD_ENABLED && defined BITTY_DEBUG
#		define BITTY_THREADING_GUARD_ENABLED 1
#	else /* BITTY_MULTITHREAD_ENABLED && BITTY_DEBUG */
#		define BITTY_THREADING_GUARD_ENABLED 0
#	endif /* BITTY_MULTITHREAD_ENABLED && BITTY_DEBUG */
#endif /* BITTY_THREADING_GUARD_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Forward declaration
*/

class Renderer;
struct SDL_Surface;
struct SDL_Texture;

/* ===========================================================================} */

/*
** {===========================================================================
** ImGuiSDL
*/

namespace ImGuiSDL {

/**
 * @brief Texture structure alias of the `Texture` struct in `ImGuiSDL`.
 */
struct Texture final {
	SDL_Surface* surface = nullptr;
	SDL_Texture* source = nullptr;

	Texture(Renderer* rnd, unsigned char* pixels, int width, int height);
	~Texture();
};

}

/* ===========================================================================} */

/*
** {===========================================================================
** Threading guard
*/

#if BITTY_THREADING_GUARD_ENABLED
class ThreadingGuard {
private:
	std::thread::id _executableThreadId;

public:
	ThreadingGuard();
	~ThreadingGuard();

	void begin(const std::thread &thread);
	void end(void);

	void validate(void);
};

extern ThreadingGuard graphicsThreadingGuard;
#else /* BITTY_THREADING_GUARD_ENABLED */
class ThreadingGuard {
public:
	ThreadingGuard();
	~ThreadingGuard();

	void validate(void);
};

extern ThreadingGuard graphicsThreadingGuard;
#endif /* BITTY_THREADING_GUARD_ENABLED */

/* ===========================================================================} */

#endif /* __HACKS_H__ */
