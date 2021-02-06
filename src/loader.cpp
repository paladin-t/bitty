/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "loader.h"

/*
** {===========================================================================
** Loader
*/

Loader::Loader() {
}

Loader::~Loader() {
}

bool Loader::clone(Loader** ptr) const {
	if (!ptr)
		return false;

	*ptr = new Loader();

	return true;
}

class Bytes* Loader::decode(const class Project*, const class Asset*, class Bytes* buf) const {
	return buf;
}

class Bytes* Loader::encode(const class Project*, const class Asset*, class Bytes* buf) const {
	return buf;
}

/* ===========================================================================} */
