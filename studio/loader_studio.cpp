/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "loader_studio.h"

/*
** {===========================================================================
** Studio loader
*/

LoaderStudio::LoaderStudio() {
}

LoaderStudio::~LoaderStudio() {
}

bool LoaderStudio::clone(Loader** ptr) const {
	if (!ptr)
		return false;

	*ptr = new LoaderStudio();

	return true;
}

void LoaderStudio::reset(void) {
	// Do nothing.
}

class Bytes* LoaderStudio::decode(const class Project*, const class Asset*, class Bytes* buf) const {
	return buf;
}

class Bytes* LoaderStudio::encode(const class Project*, const class Asset*, class Bytes* buf) const {
	return buf;
}

/* ===========================================================================} */
