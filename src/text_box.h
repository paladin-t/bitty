/*
** Bitty
**
** An itty bitty 2D game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __TEXT_BOX_H__
#define __TEXT_BOX_H__

#include "bitty.h"
#include "bytes.h"
#include "editable.h"
#include "json.h"
#include <map>

/*
** {===========================================================================
** Text box
*/

/**
 * @brief Text box object.
 */
class TextBox : public Editable, public virtual Object {
public:
	typedef std::shared_ptr<TextBox> Ptr;
	typedef std::weak_ptr<TextBox> WeakPtr;

	typedef std::map<std::string, Bytes::Ptr> FontData;
	typedef std::function<Bytes::Ptr(const std::string &)> FontResolver;

public:
	BITTY_CLASS_TYPE('T', 'X', 'T', 'B')

	virtual void lock(void) = 0;
	virtual void unlock(void) = 0;
	virtual bool tryLock(void) = 0;

	virtual bool option(const std::string &key, const Variant &val) = 0;

	virtual bool useFont(const Json::Ptr &json, const FontData &fontData) = 0;

	virtual void paste(const char* txt) = 0;
	using Editable::paste;

	virtual bool focused(void) const = 0;
	virtual void focus(void) = 0;

	virtual bool location(int &ln, int &col) const = 0;
	virtual void locate(int ln, int col) = 0;
	virtual bool selection(int &ln0, int &col0, int &ln1, int &col1) const = 0;
	virtual void selectRange(int ln0, int col0, int ln1, int col1) = 0;
	virtual void selectAll(void) = 0;

	virtual int lineCount(void) const = 0;
	virtual std::string lineAt(int ln) const = 0;
	virtual int columnsAt(int ln) const = 0;

	virtual void indent(void) = 0;
	virtual void unindent(void) = 0;

	virtual void ensureCursorVisible(bool forceAbove, bool slowMode) = 0;

	virtual const char* text(size_t* len) const = 0;
	virtual void text(const char* txt, size_t len = 0) = 0;

	virtual void bake(
		class Window* wnd, class Renderer* rnd,
		class Workspace* ws,
		float x, float y, float width, float height
	) = 0;
	virtual void render(
		class Window* wnd, class Renderer* rnd,
		class Workspace* ws,
		float x, float y, float width, float height
	) = 0;

	virtual void translate(int &x0, int &y0, int &x1, int &y1, int camX, int camY) = 0;

	static bool parseFont(const Json::Ptr &json, FontData &fontData, FontResolver resolveFont);

	static TextBox* create(void);
	static void destroy(TextBox* ptr);
};

/* ===========================================================================} */

#endif /* __TEXT_BOX_H__ */
