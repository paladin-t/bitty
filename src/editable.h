/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __EDITABLE_H__
#define __EDITABLE_H__

#include "bitty.h"
#include "dispatchable.h"

/*
** {===========================================================================
** Editable
*/

/**
 * @brief Editable interface.
 */
class Editable : public Dispatchable {
public:
	enum Messages : unsigned {
		SET_THEME_STYLE,
		SET_SHOW_SPACES,
		RECALCULATE,
		RESIZE,
		RESIZE_GRID,
		SELECT_ALL,
		INDENT,
		UNINDENT,
		FIND,
		FIND_NEXT,
		FIND_PREVIOUS,
		GOTO,
		GET_CURSOR,
		SET_CURSOR,
		GET_PROGRAM_POINTER,
		SET_PROGRAM_POINTER,
		GET_BREAKPOINT,
		SET_BREAKPOINT,
		GET_BREAKPOINTS,
		CLEAR_BREAKPOINTS,

		ON_TOGGLE_BREAKPOINT,

		MAX
	};

public:
	template<typename T> static bool is(const Editable* ptr) {
		return !!dynamic_cast<const T*>(ptr);
	}
	template<typename T> static const T* as(const Editable* ptr) {
		return dynamic_cast<const T*>(ptr);
	}
	template<typename T> static T* as(Editable* ptr) {
		return dynamic_cast<T*>(ptr);
	}

	virtual ~Editable();

	virtual void open(const class Project* project, const char* name, Object::Ptr obj, const char* ref /* nullable */) = 0;
	virtual void close(const class Project* project /* nullable */) = 0;

	virtual void flush(void) const = 0;

	virtual bool readonly(void) const = 0;
	virtual void readonly(bool ro) = 0;

	virtual bool hasUnsavedChanges(void) const = 0;
	virtual void markChangesSaved(const class Project* project) = 0;

	virtual void copy(void) = 0;
	virtual void cut(void) = 0;
	virtual bool pastable(void) const = 0;
	virtual void paste(void) = 0;
	virtual void del(void) = 0;
	virtual bool selectable(void) const = 0;

	virtual const char* redoable(void) const = 0;
	virtual const char* undoable(void) const = 0;

	virtual void redo(class Asset* asset) = 0;
	virtual void undo(class Asset* asset) = 0;

	virtual void update(
		class Window* wnd, class Renderer* rnd,
		class Workspace* ws, const class Project* project, class Executable* exec,
		const char* title,
		float x, float y, float width, float height,
		float scaleX, float scaleY,
		bool pending,
		double delta
	) = 0;

	virtual void played(class Renderer* rnd, const class Project* project) = 0;
	virtual void stopped(class Renderer* rnd, const class Project* project) = 0;

	/**
	 * @brief Callback when the editor window is resized.
	 */
	virtual void resized(class Renderer* rnd, const class Project* project) = 0;

	virtual void lostFocus(class Renderer* rnd, const class Project* project) = 0;
	virtual void gainFocus(class Renderer* rnd, const class Project* project) = 0;
};

/* ===========================================================================} */

#endif /* __EDITABLE_H__ */
