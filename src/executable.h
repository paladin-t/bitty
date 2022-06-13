/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __EXECUTABLE_H__
#define __EXECUTABLE_H__

#include "bitty.h"
#include "mathematics.h"
#include "promise.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef EXECUTABLE_ANY_NAME
#	define EXECUTABLE_ANY_NAME "*"
#endif /* EXECUTABLE_ANY_NAME */

/* ===========================================================================} */

/*
** {===========================================================================
** Executable
*/

/**
 * @brief Executable interface.
 */
class Executable {
public:
	enum Languages {
		LUA = 1 << 0,
		NATIVE = 1 << 1
	};

	enum States {
		READY,
		RUNNING,
		PAUSED,
		HALTING
	};

	typedef std::function<bool(Variant*)> PromiseHandler;

	class Observer {
	public:
		/**
		 * @brief Clears output in the console window.
		 */
		virtual void clear(void) = 0;
		/**
		 * @brief Outputs a specific message to the console window.
		 */
		virtual bool print(const char* msg) = 0;
		/**
		 * @brief Outputs a specific warning to the console window.
		 */
		virtual bool warn(const char* msg) = 0;
		/**
		 * @brief Outputs a specific error to the console window.
		 */
		virtual bool error(const char* msg) = 0;
		/**
		 * @brief Gets whether there is pending promise.
		 */
		virtual bool promising(void) = 0;
		/**
		 * @brief Promises for custom handler.
		 */
		virtual void promise(Promise::Ptr &promise /* nullable */, PromiseHandler handler /* nullable */) = 0;
		/**
		 * @brief Promises for wait box.
		 */
		virtual void waitbox(Promise::Ptr &promise /* nullable */, const char* content /* nullable */) = 0;
		/**
		 * @brief Promises for message box.
		 */
		virtual void msgbox(Promise::Ptr &promise /* nullable */, const char* msg /* nullable */, const char* confirmTxt /* nullable */, const char* denyTxt /* nullable */, const char* cancelTxt /* nullable */) = 0;
		/**
		 * @brief Promises for input box.
		 */
		virtual void input(Promise::Ptr &promise /* nullable */, const char* prompt /* nullable */, const char* default_ /* nullable */) = 0;
		/**
		 * @brief Sets focus to a specific source file and line.
		 */
		virtual bool focus(const char* src, int ln) = 0;
		/**
		 * @brief Requires libraries.
		 */
		virtual void require(Executable* exec) = 0;
		/**
		 * @brief Stops execution.
		 */
		virtual void stop(void) = 0;
		/**
		 * @brief Gets the size of the application.
		 */
		virtual Math::Vec2i applicationSize(void) = 0;
		/**
		 * @brief Sets the size of the application.
		 */
		virtual bool resizeApplication(const Math::Vec2i &size) = 0;
		/**
		 * @brief Gets the size of the rendering canvas.
		 */
		virtual Math::Vec2i canvasSize(void) = 0;
		/**
		 * @brief Sets the size of the rendering canvas.
		 */
		virtual bool resizeCanvas(const Math::Vec2i &size) = 0;
		/**
		 * @brief Sets fullscreen effect.
		 */
		virtual void effect(const char* material) = 0;
	};

	typedef std::function<void(const char*, int)> BreakpointGetter;

	typedef std::function<bool(const char* &, const char* &, const Variant* &, bool &)> VariableGetter;
	typedef std::function<void(const char*, int, int, const char*, const char*, VariableGetter)> RecordGetter;

	typedef std::shared_ptr<void> Invokable;

public:
	virtual void* pointer(void) = 0;

	virtual bool open(
		Observer* observer, const class Project* project, const class Project* editing, class Primitives* primitives /* nullable */,
		unsigned fps, bool effectsEnabled
	) = 0;
	virtual bool close(void) = 0;

	virtual bool effectsEnabled(void) const = 0;

	virtual const class Project* project(void) const = 0;
	virtual const class Project* editing(void) const = 0;
	virtual class Primitives* primitives(void) const = 0;

	/**
	 * @brief Gets timeout option for a single invoking.
	 */
	virtual long long timeout(void) const = 0;
	/**
	 * @brief Sets timeout option for a single invoking.
	 *
	 * @param[in] val Timeout duration in nanoseconds. Positive value to enable timeout,
	 *   zero to disable once, negative to disable for the current and future invoking
	 *   until setting with a non-negative value again.
	 */
	virtual void timeout(long long val) = 0;
	virtual void activate(void) = 0;

	virtual Languages language(void) const = 0;

	virtual unsigned fps(void) const = 0;

	virtual void prepare(void) = 0;
	virtual void finish(void) = 0;

	virtual bool setup(void) = 0;
	virtual bool cycle(double delta) = 0;
	virtual bool focusLost(void) = 0;
	virtual bool focusGained(void) = 0;
	virtual bool renderTargetsReset(void) = 0;

	virtual bool update(double delta) = 0;

	virtual bool pending(void) const = 0;
	virtual void sync(double delta) = 0;

	virtual States current(void) const = 0;

	virtual bool exit(void) = 0;

	virtual bool run(void) = 0;
	virtual bool stop(void) = 0;

	virtual bool pause(void) = 0;
	virtual bool resume(void) = 0;

	virtual bool stepOver(void) = 0;
	virtual bool stepInto(void) = 0;
	virtual bool stepOut(void) = 0;

	virtual int getBreakpoints(const char* src /* nullable */, BreakpointGetter get /* nullable */) const = 0;
	virtual bool setBreakpoint(const char* src, int ln, bool brk) = 0;
	virtual int clearBreakpoints(const char* src /* nullable */) = 0;

	virtual int getRecords(RecordGetter get) const = 0;

	/**
	 * @param[out] type
	 * @param[out] var
	 */
	virtual bool getVariable(const char* name, const char* &type /* nullable */, Variant* &var /* nullable */) const = 0;
	virtual bool setVariable(const char* name, const Variant* var /* nullable */) const = 0;

	virtual bool debugRealNumberPrecisely(void) const = 0;
	virtual void debugRealNumberPrecisely(bool enabled) = 0;

	virtual Invokable getInvokable(const char* name) const = 0;
	virtual Variant invoke(Invokable func, int argc, const Variant* argv) = 0;
	Variant invoke(Invokable func) {
		return invoke(func, 0, (const Variant*)nullptr);
	}
	template<typename ...Args> Variant invoke(Invokable func, const Args &...args) {
		const size_t n = sizeof...(Args);
		const Variant argv[n] = { Variant(args)... };

		return invoke(func, (int)n, argv);
	}

	virtual void gc(void) = 0;
};

/* ===========================================================================} */

#endif /* __EXECUTABLE_H__ */
