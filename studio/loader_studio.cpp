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
	// Resets this loader. Called when a project is being unloaded.
}

class Bytes* LoaderStudio::decode(const class Project* /* project */, const class Asset* /* asset */, class Bytes* buf) const {
	// Decodes a `Bytes` buffer after loading an asset. The return value
	// reuses the input object rather than creating new one.

	return buf;
}

class Bytes* LoaderStudio::encode(const class Project* /* project */, const class Asset* /* asset */, class Bytes* buf) const {
	// Encodes a `Bytes` buffer before saving an asset. The return value
	// reuses the input object rather than creating new one.

	return buf;
}

/* ===========================================================================} */
