/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __LOADER_STUDIO_H__
#define __LOADER_STUDIO_H__

#include "../src/loader.h"
#include "../src/text.h"

/*
** {===========================================================================
** Studio loader
*/

class LoaderStudio : public Loader {
public:
	LoaderStudio();
	virtual ~LoaderStudio() override;

	virtual bool clone(Loader** ptr) const override;

	virtual void reset(void) override;

	virtual class Bytes* decode(const class Project* project, const class Asset* asset, class Bytes* buf) const override;
	virtual class Bytes* encode(const class Project* project, const class Asset* asset, class Bytes* buf) const override;
};

/* ===========================================================================} */

#endif /* __LOADER_STUDIO_H__ */
