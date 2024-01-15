/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2024 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __EDITING_H__
#define __EDITING_H__

#include "bitty.h"
#include "bytes.h"

/*
** {===========================================================================
** Forward declaration
*/

class Renderer;
class Workspace;
class Project;

/* ===========================================================================} */

/*
** {===========================================================================
** Editing
*/

/**
 * @brief Editing utilities.
 */
namespace Editing {

struct Shortcut {
	int key = 0;
	bool ctrl = false;
	bool shift = false;
	bool alt = false;
	bool numLock = false;
	bool capsLock = false;
	bool super = false;

	Shortcut(
		int key,
		bool ctrl = false, bool shift = false, bool alt = false,
		bool numLock = false, bool capsLock = false,
		bool super = false
	);

	bool pressed(bool repeat = true) const;
	bool released(void) const;
};

namespace Tools {

struct Marker {
	struct Coordinates {
		int index = 0;
		int line = -1;
		int column = -1;

		Coordinates();
		Coordinates(int ln, int col);
		Coordinates(int idx, int ln, int col);

		bool operator == (const Coordinates &other) const;
		bool operator < (const Coordinates &other) const;
		bool operator > (const Coordinates &other) const;

		int compare(const Coordinates &other) const;

		bool empty(void) const;
		void clear(void);
	};

	Coordinates begin;
	Coordinates end;

	Marker();
	Marker(const Coordinates &begin, const Coordinates &end);

	const Coordinates &min(void) const;
	const Coordinates &max(void) const;

	bool empty(void) const;
	void clear(void);
};

typedef std::function<std::string(const Marker::Coordinates &, Marker &)> TextWordGetter;

/**
 * @param[in, out] cursor
 * @param[in, out] initialized
 */
bool jump(
	Renderer* rnd,
	Workspace* ws,
	int* cursor = nullptr,
	float width = -1.0f,
	bool* initialized = nullptr, bool* focused = nullptr,
	int min = -1, int max = -1
);

/**
 * @param[in, out] cursor
 * @param[in, out] initialized
 * @param[in, out] what
 * @param[in, out] direction
 * @param[in, out] caseSensitive
 * @param[in, out] wholeWord
 */
bool find(
	Renderer* rnd,
	Workspace* ws,
	Marker* cursor = nullptr,
	float width = -1.0f,
	bool* initialized = nullptr, bool* focused = nullptr,
	const char* text = nullptr, std::string* what = nullptr,
	const Marker::Coordinates &max = Marker::Coordinates(),
	int* direction = nullptr,
	bool* caseSensitive = nullptr, bool* wholeWord = nullptr,
	bool visible = true,
	TextWordGetter getWord = nullptr
);

}

namespace Data {

struct Checkpoint {
	Bytes::Ptr bytes = nullptr;
	size_t originalSize = 0;
	bool compressed = false;

	Checkpoint();

	bool empty(void) const;
	void fill(void);
	void clear(void);
};

/**
 * @param[out] bytes
 */
bool toCheckpoint(const Project* project, const char* name, Checkpoint &checkpoint);
bool fromCheckpoint(const Project* project, const char* name, Checkpoint &checkpoint);

}

}

/* ===========================================================================} */

#endif /* __EDITING_H__ */
