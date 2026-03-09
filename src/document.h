/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __DOCUMENT_H__
#define __DOCUMENT_H__

#include "bitty.h"
#include <functional>
#include <string>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef DOCUMENT_MARKDOWN_DIR
#	define DOCUMENT_MARKDOWN_DIR "../docs/" /* Relative path. */
#endif /* DOCUMENT_MARKDOWN_DIR */
#ifndef DOCUMENT_MARKDOWN_EXT
#	define DOCUMENT_MARKDOWN_EXT "md"
#endif /* DOCUMENT_MARKDOWN_EXT */

/* ===========================================================================} */

/*
** {===========================================================================
** Document
*/

/**
 * @brief Document viewer.
 */
class Document {
public:
	typedef std::function<bool(const char*, std::string &, std::string &)> ContentResolver;
	typedef std::function<bool(const char*, class Image* &)> ImageResolver;

public:
	virtual ~Document();

	virtual const std::string &directory(void) const = 0;
	virtual void directory(const std::string &dir) = 0;

	virtual ContentResolver contentResolver(void) const = 0;
	virtual void contentResolver(ContentResolver resolve) = 0;

	virtual ImageResolver imageResolver(void) const = 0;
	virtual void imageResolver(ImageResolver resolve) = 0;

	virtual bool useThemedSpanCode(void) const = 0;
	virtual void useThemedSpanCode(bool val) = 0;

	virtual void font(struct ImFont* regular, struct ImFont* bold) = 0;

	virtual const std::string &title(void) = 0;

	virtual const std::string &content(void) const = 0;
	virtual void content(const std::string &val) = 0;

	virtual bool withTableOfContent(void) const = 0;
	virtual void withTableOfContent(bool val) = 0;

	virtual const char* shown(void) = 0;
	virtual void show(const char* doc) = 0;
	virtual void hide(void) = 0;

	virtual void bringToFront(void) = 0;

	virtual void update(class Window* wnd, class Renderer* rnd, const class Theme* theme, bool windowed) = 0;

	static Document* create(void);
	static void destroy(Document* ptr);
};

/* ===========================================================================} */

#endif /* __DOCUMENT_H__ */
