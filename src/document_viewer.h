/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __DOCUMENT_VIEWER_H__
#define __DOCUMENT_VIEWER_H__

#include "bitty.h"
#include "bytes.h"
#include "json.h"
#include <map>

/*
** {===========================================================================
** Document viewer
*/

/**
 * @brief Document viewer object.
 */
class DocumentViewer : public virtual Object {
public:
	typedef std::shared_ptr<DocumentViewer> Ptr;
	typedef std::weak_ptr<DocumentViewer> WeakPtr;

	typedef std::map<std::string, Bytes::Ptr> FontData;
	typedef std::function<Bytes::Ptr(const std::string &)> FontResolver;

public:
	BITTY_CLASS_TYPE('D', 'O', 'C', 'V')

	virtual void open(const char* name) = 0;
	virtual void close(void) = 0;

	virtual void lock(void) = 0;
	virtual void unlock(void) = 0;
	virtual bool tryLock(void) = 0;

	virtual bool option(const std::string &key, const Variant &val) = 0;

	virtual bool useFont(const Json::Ptr &json, const FontData &fontData) = 0;

	virtual bool location(float &v) const = 0;
	virtual void locate(float v) = 0;

	virtual const char* text(size_t* len) const = 0;
	virtual void text(const char* txt, size_t len = 0) = 0;

	virtual void update(
		class Window* wnd, class Renderer* rnd,
		class Workspace* ws, const class Project* project, class Executable* exec,
		const char* title,
		float x, float y, float width, float height,
		int scale,
		bool pending,
		double delta
	) = 0;
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

	static DocumentViewer* create(void);
	static void destroy(DocumentViewer* ptr);
};

/* ===========================================================================} */

#endif /* __DOCUMENT_VIEWER_H__ */
