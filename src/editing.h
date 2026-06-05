/*
** Bitty
**
** An itty bitty 2D game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __EDITING_H__
#define __EDITING_H__

#include "bitty.h"
#include "bytes.h"
#include "color.h"
#include "plus.h"
#include "text.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef EDITING_ANONYMOUS
#	define EDITING_ANONYMOUS "anonymous"
#endif /* EDITING_ANONYMOUS */
#ifndef EDITING_NONAME
#	define EDITING_NONAME "noname"
#endif /* EDITING_NONAME */

#ifndef EDITING_ITEM_COUNT_PER_LINE
#	define EDITING_ITEM_COUNT_PER_LINE 5
#endif /* EDITING_ITEM_COUNT_PER_LINE */

/* ===========================================================================} */

/*
** {===========================================================================
** Forward declaration
*/

namespace Bitty {

class Renderer;
class Workspace;
class Project;
class Texture;
class Palette;
class Image;
class Sprite;
class Map;
class Sound;

}

/* ===========================================================================} */

/*
** {===========================================================================
** Editing
*/

namespace Bitty {

/**
 * @brief Editing and preview area, utilities.
 */
namespace Editing {

union Dots {
	Color* colored;
	int* indexed;

	Dots(Color* col);
	Dots(int* idx);
};

union Dot {
	typedef std::vector<Dot> Array;

	Color colored;
	int indexed;

	Dot();
	Dot(const Dot &other);

	Dot &operator = (const Dot &other);
};

struct Point {
	typedef std::set<Point> Set;

	Math::Vec2i position = Math::Vec2i(-1, -1);
	Dot dot;

	Point();
	Point(const Math::Vec2i &pos);
	Point(const Math::Vec2i &pos, const Color &col);
	Point(const Math::Vec2i &pos, int idx);

	bool operator < (const Point &other) const;
};

struct Painting : public NonCopyable {
private:
	bool _lastValue = false;
	bool _value = false;
	Math::Vec2f _lastPosition = Math::Vec2f(-1, -1);
	Math::Vec2f _position = Math::Vec2f(-1, -1);

public:
	Painting();

	operator bool (void) const;

	Painting &set(bool val);
	bool moved(void) const;
	bool down(void) const;
	bool up(void) const;

	void clear(void);
};

struct Brush {
	Math::Vec2i position = Math::Vec2i(-1, -1); // Position in pixels.
	Math::Vec2i size = Math::Vec2i(1, 1);       // Size in pixels.

	Brush();

	bool operator == (const Brush &other) const;
	bool operator != (const Brush &other) const;
	bool operator == (const Math::Recti &other) const;
	bool operator != (const Math::Recti &other) const;

	bool empty(void) const;

	Math::Vec2i unit(void) const;

	Math::Recti toRect(void) const;
	Brush &fromRect(const Math::Recti &other);
};

struct Frame {
	std::string animation;
	int index = -1;

	Frame();
	Frame(const std::string &anim, int idx);

	bool operator == (const Frame &other) const;
	bool operator != (const Frame &other) const;

	bool empty(void) const;
};

struct Tiler {
	Math::Vec2i position = Math::Vec2i(-1, -1); // Position in cels.
	Math::Vec2i size = Math::Vec2i(1, 1);       // Size in cels.

	Tiler();

	bool operator == (const Tiler &other) const;
	bool operator != (const Tiler &other) const;

	bool empty(void) const;
};

struct Tuner {
	enum Commands {
		NONE,
		PLAY,
		STOP
	};

	double position = 0;
	double length = 0;

	Tuner();

	bool operator == (const Tuner &other) const;
	bool operator != (const Tuner &other) const;

	bool empty(void) const;
};

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

typedef std::function<void(void)> SpriteAnimationAddingHandler;
typedef std::function<void(const char*)> SpriteAnimationRemovingHandler;
typedef std::function<void(const char*)> SpriteAnimationRenamingHandler;
typedef std::function<void(const Frame &)> SpriteFrameInsertingHandler;
typedef std::function<void(const Frame &)> SpriteFrameRemovingHandler;
typedef std::function<void(const Frame &)> SpriteFrameIntervalChangingHandler;

typedef std::function<int(const Math::Vec2i &)> MapCelGetter;

/**
 * @param[out] col
 * @param[in, out] cursor
 */
bool palette(
	Renderer* rnd,
	Workspace* ws,
	const Palette* ptr,
	float width = -1.0f,
	Color* col = nullptr,
	int* cursor = nullptr, bool showCursor = true,
	Texture* transparent = nullptr
);

/**
 * @param[in, out] cursor
 */
bool image(
	Renderer* rnd,
	Workspace* ws,
	const Image* ptr, Texture* tex,
	float width = -1.0f,
	Brush* cursor = nullptr, bool showCursor = true,
	const Brush* selection = nullptr,
	Texture* overlay = nullptr,
	const Math::Vec2i* gridSize = nullptr, bool showGrids = false,
	bool showTransparentBackbround = true,
	int mouseActionButton = 0
);

/**
 * @param[in, out] cursor
 * @param[in, out] cursorStamp
 */
bool image(
	Renderer* rnd,
	Workspace* ws,
	const Image* ptr, Texture* tex,
	float width = -1.0f,
	Brush* cursor = nullptr, bool showCursor = true, Brush &&cursorStamp = Brush(),
	const Brush* selection = nullptr,
	Texture* overlay = nullptr,
	const Math::Vec2i* gridSize = nullptr, bool showGrids = false,
	bool showTransparentBackbround = true,
	int mouseActionButton = 0
);

/**
 * @param[in, out] cursor
 */
bool sprite(
	Renderer* rnd,
	Workspace* ws,
	const Sprite* ptr,
	float width = -1.0f, float scale = 1.0f,
	Frame* cursor = nullptr, bool showCursor = true,
	SpriteAnimationAddingHandler addAnim = nullptr, SpriteAnimationRemovingHandler removeAnim = nullptr, SpriteAnimationRenamingHandler renameAnim = nullptr,
	SpriteFrameInsertingHandler insertFrame = nullptr, SpriteFrameRemovingHandler removeFrame = nullptr,
	SpriteFrameIntervalChangingHandler changeInterval = nullptr
);

/**
 * @param[in, out] cursor
 * @param[in, out] focus
 */
bool sprite(
	Renderer* rnd,
	Workspace* ws,
	const Sprite* ptr,
	float width = -1.0f, float scale = 1.0f,
	Frame* cursor = nullptr, bool showCursor = true,
	const Text::Array &anims = { },
	std::string* focus = nullptr,
	const Sprite* player = nullptr,
	SpriteAnimationAddingHandler addAnim = nullptr, SpriteAnimationRemovingHandler removeAnim = nullptr, SpriteAnimationRenamingHandler renameAnim = nullptr,
	SpriteFrameInsertingHandler insertFrame = nullptr, SpriteFrameRemovingHandler removeFrame = nullptr,
	SpriteFrameIntervalChangingHandler changeInterval = nullptr
);

/**
 * @param[in, out] cursor
 */
bool map(
	Renderer* rnd,
	Workspace* ws,
	const Map* ptr, const Math::Vec2i &tileSize,
	float width = -1.0f, 
	Tiler* cursor = nullptr, bool showCursor = true,
	const Tiler* selection = nullptr,
	const Math::Vec2i* showGrids = nullptr,
	bool showTransparentBackbround = true,
	MapCelGetter getCel = nullptr,
	int mouseActionButton = 0
);

/**
 * @param[in, out] cursor
 */
bool sound(
	Renderer* rnd,
	Workspace* ws,
	Sound* ptr,
	float width = -1.0f, float height = -1.0f,
	Tuner* cursor = nullptr, bool showCursor = true,
	Tuner::Commands* cmd = nullptr,
	const float* columns = nullptr, size_t columnCount = 0
);

namespace Tools {

enum Tools {
	MOVE,
	EYEDROPPER,
	PENCIL,
	PAINTBUCKET,
	LASSO,
	LINE,
	BOX,
	BOX_FILL,
	ELLIPSE,
	ELLIPSE_FILL,
	STAMP,

	COUNT
};

enum RotationsAndFlippings {
	ROTATE_CLOCKWISE,
	ROTATE_ANTICLOCKWISE,
	HALF_TURN,
	FLIP_VERTICALLY,
	FLIP_HORIZONTALLY,
	INVALID
};

typedef std::vector<const std::string*> TextPages;

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
 * @param[in, out] colors
 * @param[in, out] cursor
 */
bool colorable(
	Renderer* rnd,
	Workspace* ws,
	Color colors[EDITING_ITEM_COUNT_PER_LINE * 2],
	int* cursor /* nullable */,
	float width = -1.0f,
	bool allowShortcuts = true
);

/**
 * @param[in, out] cursor
 */
bool paintable(
	Renderer* rnd,
	Workspace* ws,
	Tools* cursor /* nullable */,
	float width = -1.0f,
	bool allowShortcuts = true,
	unsigned mask = 0xffffffff
);

/**
 * @param[in, out] cursor
 */
bool magnifiable(
	Renderer* rnd,
	Workspace* ws,
	int* cursor = nullptr,
	float width = -1.0f,
	bool allowShortcuts = true
);

/**
 * @param[in, out] val
 * @param[in, out] focused
 */
bool magnifiable(
	Renderer* rnd, Workspace* ws,
	int* val,
	int minVal, int maxVal,
	float width = -1.0f,
	bool allowShortcuts = true,
	bool* focused = nullptr
);

/**
 * @param[in, out] cursor
 */
bool weighable(
	Renderer* rnd,
	Workspace* ws,
	int* cursor /* nullable */,
	float width = -1.0f,
	bool allowShortcuts = true
);

/**
 * @param[in, out] cursor
 */
bool flippable(
	Renderer* rnd,
	Workspace* ws,
	RotationsAndFlippings* cursor /* nullable */,
	float width = -1.0f,
	bool allowShortcuts = true,
	unsigned mask = 0xffffffff
);

/**
 * @param[out] repeat
 * @param[in, out] focused
 */
bool repeatable(
	Renderer* rnd, Workspace* ws,
	Math::Recti* repeat /* nullable */,
	const Math::Recti &minVal, const Math::Recti &maxVal,
	float width = -1.0f,
	bool* focused = nullptr,
	bool disabled = false,
	const char* prompt = nullptr
);

/**
 * @param[in, out] cursor
 */
bool tickable(
	Renderer* rnd,
	Workspace* ws,
	double* cursor,
	float width = -1.0f
);

/**
 * @param[in, out] cursor
 * @param[in, out] initialized
 * @param[in, out] focused
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
 * @param[in, out] focused
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

/**
 * @param[in, out] cursor
 * @param[in, out] initialized
 * @param[in, out] focused
 * @param[in, out] what
 * @param[in, out] direction
 * @param[in, out] caseSensitive
 * @param[in, out] wholeWord
 * @param[in, out] globalSearch
 */
bool find(
	Renderer* rnd,
	Workspace* ws,
	Marker* cursor = nullptr,
	float width = -1.0f,
	bool* initialized = nullptr, bool* focused = nullptr,
	const TextPages* textPages = nullptr, std::string* what = nullptr,
	const Marker::Coordinates &max = Marker::Coordinates(),
	int* direction = nullptr,
	bool* caseSensitive = nullptr, bool* wholeWord = nullptr, bool* globalSearch = nullptr,
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

}

/* ===========================================================================} */

#endif /* __EDITING_H__ */
