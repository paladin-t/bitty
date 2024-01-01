/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2024 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "theme_sketchbook.h"

/*
** {===========================================================================
** Sketchbook theme
*/

ThemeSketchbook::ThemeSketchbook() {
}

ThemeSketchbook::~ThemeSketchbook() {
}

Theme::Styles ThemeSketchbook::styleIndex(void) const {
	return DARK;
}

void ThemeSketchbook::styleIndex(Styles) {
	style(&styleDefault());

	memcpy(ImGui::GetStyle().Colors, style()->builtin, sizeof(ImGui::GetStyle().Colors));
}

bool ThemeSketchbook::open(class Renderer* rnd) {
	if (!Theme::open(rnd))
		return false;

	styleIndex(DARK);

	fprintf(stdout, "Theme opened.\n");

	return true;
}

bool ThemeSketchbook::close(class Renderer* rnd) {
	if (!Theme::close(rnd))
		return false;

	fprintf(stdout, "Theme closed.\n");

	return true;
}

bool ThemeSketchbook::load(class Renderer* rnd) {
	bool result = true;

	if (!Theme::load(rnd))
		result = false;

	return result;
}

bool ThemeSketchbook::save(void) const {
	bool result = true;

	if (!Theme::save())
		result = false;

	return result;
}

/* ===========================================================================} */
