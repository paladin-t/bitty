/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __THEME_STUDIO_H__
#define __THEME_STUDIO_H__

#include "../src/theme.h"

/*
** {===========================================================================
** Studio theme
**
** @note Specialized theme for Pro version.
*/

class ThemeStudio : public Theme {
public:
	BITTY_PROPERTY(Style, styleDark)
	BITTY_PROPERTY(Style, styleClassic)
	BITTY_PROPERTY(Style, styleLight)
	BITTY_FIELD(Styles, styleType)

	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Theme)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Style)

public:
	ThemeStudio();
	virtual ~ThemeStudio() override;

	virtual Styles styleIndex(void) const override;
	virtual void styleIndex(Styles idx) override;

	virtual bool open(class Renderer* rnd) override;
	virtual bool close(class Renderer* rnd) override;

	virtual bool load(class Renderer* rnd) override;
	virtual bool save(void) const override;

protected:
	virtual void setColor(const std::string &key, ImGuiCol idx, const ImColor &col) override;
	virtual void setColor(const std::string &key, const std::string &idx, const ImColor &col) override;
};

/* ===========================================================================} */

#endif /* __THEME_STUDIO_H__ */
