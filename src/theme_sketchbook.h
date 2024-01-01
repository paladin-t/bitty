/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2024 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __THEME_SKETCHBOOK_H__
#define __THEME_SKETCHBOOK_H__

#include "theme.h"

/*
** {===========================================================================
** Sketchbook theme
**
** @note Specialized theme.
*/

/**
 * @brief Specialized theme.
 */
class ThemeSketchbook : public Theme {
public:
	ThemeSketchbook();
	virtual ~ThemeSketchbook() override;

	virtual Styles styleIndex(void) const override;
	virtual void styleIndex(Styles idx) override;

	virtual bool open(class Renderer* rnd) override;
	virtual bool close(class Renderer* rnd) override;

	virtual bool load(class Renderer* rnd) override;
	virtual bool save(void) const override;
};

/* ===========================================================================} */

#endif /* __THEME_SKETCHBOOK_H__ */
