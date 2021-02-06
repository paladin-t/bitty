/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "hacks.h"
#include "renderer.h"
#include <SDL.h>

/*
** {===========================================================================
** ImGuiSDL
*/

namespace ImGuiSDL {
	
Texture::Texture(Renderer* rnd, unsigned char* pixels, int width, int height) {
	SDL_Renderer* renderer = (SDL_Renderer*)rnd->pointer();

	surface = SDL_CreateRGBSurfaceFrom(
		pixels,
		width, height,
		32, width * 4,
		0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
	);

	source = SDL_CreateTextureFromSurface(renderer, surface);
}

Texture::~Texture() {
	SDL_FreeSurface(surface);
	surface = nullptr;

	SDL_DestroyTexture(source);
	source = nullptr;
}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Threading guard
*/

#if BITTY_THREADING_GUARD_ENABLED
ThreadingGuard::ThreadingGuard() {
}

ThreadingGuard::~ThreadingGuard() {
}

void ThreadingGuard::begin(const std::thread &thread) {
	_executableThreadId = thread.get_id();
}

void ThreadingGuard::end(void) {
	_executableThreadId = std::thread::id();
}

void ThreadingGuard::validate(void) {
	const std::thread::id current = std::this_thread::get_id();
	assert(_executableThreadId != current && "Cannot access from this thread.");
}

ThreadingGuard graphicsThreadingGuard;
#else /* BITTY_THREADING_GUARD_ENABLED */
ThreadingGuard::ThreadingGuard() {
}

ThreadingGuard::~ThreadingGuard() {
}

void ThreadingGuard::validate(void) {
	// Do nothing.
}

ThreadingGuard graphicsThreadingGuard;
#endif /* BITTY_THREADING_GUARD_ENABLED */

/* ===========================================================================} */
