/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __EDITOR_CODE_H__
#define __EDITOR_CODE_H__

#include "editable.h"

/*
** {===========================================================================
** Macros and constants
*/

static constexpr const char* const EDITOR_CODE_KEYWORDS[] = {
	"warn",
	"waitbox",
	"msgbox",
	"input",
	"exit",
	"fetch"
};
static constexpr const char* const EDITOR_CODE_MODULES[] = {
	// Library.
	"Noiser", "Pathfinder", "Random", "Raycaster", "Walker", // Algorithms.
	"Archive",
	"Bytes",
	"Color",
	"DateTime",
	"Base64", "Lz4", // Encoding.
	"File",
	"Path", "FileInfo", "DirectoryInfo",
	"Image",
	"Json",
	"Vec2", "Vec3", "Vec4", "Rect", "Recti", "Rot",
	"Math",
	"Network",
	"Platform",
	"Promise",
	"Stream",
	"Web",
	// Engine.
	"Resources",
	"Asset",
	"Palette", "Texture", "Sprite", "Map",
	"Sfx", "Music",
	"Font",
	"Physics",
	// Application.
	"Application",
	"Canvas",
	"Project",
	"Debug",
	// Editor.
	"Editor"
};
static constexpr const char* const EDITOR_CODE_PRIMITIVES[] = {
	"cls",
	"blend",
	"camera", "clip",
	"color",
	"plot", "line",
	"circ", "ellipse", "pie", "rect",
	"font", "measure", "text",
	"tri",
	"tex", "spr", "map",
	"pget", "pset",
	"mget", "mset",
	"volume", "play", "stop",
	"btn", "btnp", "rumble",
	"key", "keyp",
	"mouse",
	"sync"
};

/* ===========================================================================} */

/*
** {===========================================================================
** Code editor
*/

class EditorCode : public Editable, public virtual Object {
private:
	static int refCount;

public:
	BITTY_CLASS_TYPE('C', 'O', 'D', 'E')

	virtual void addKeyword(const char* str) = 0;
	virtual void addIdentifier(const char* str) = 0;
	virtual void addPreprocessor(const char* str) = 0;

	/**
	 * @param[out] len
	 */
	virtual const char* text(size_t* len) const = 0;
	virtual void text(const char* txt, size_t len = 0) = 0;

	static EditorCode* create(void);
	static void destroy(EditorCode* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_CODE_H__ */
