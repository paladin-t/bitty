/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __SCRIPTING_LUA_H__
#define __SCRIPTING_LUA_H__

#include "luaxx.h"
#include "scripting.h"
#include "scripting_lua_dbg.h"
#include <set>
#if BITTY_MULTITHREAD_ENABLED
#	include <thread>
#endif /* BITTY_MULTITHREAD_ENABLED */

/*
** {===========================================================================
** Macros and constants
*/

#ifndef SCRIPTING_LUA_TIMEOUT_NANOSECONDS
#	define SCRIPTING_LUA_TIMEOUT_NANOSECONDS 10000000000ll /* 10 seconds. */
#endif /* SCRIPTING_LUA_TIMEOUT_NANOSECONDS */

/* ===========================================================================} */

/*
** {===========================================================================
** Lua scripting
*/

class ScriptingLua : public Scripting {
protected:
	typedef std::set<std::string> Requirement;
	typedef std::list<std::string> Dependency;

	enum FocusStates {
		IDLE,
		LOST,
		GAINED
	};

	typedef std::list<class Updatable*> Updatables;

protected:
	lua_State* _L = nullptr;

	Requirement _requirement;                               // By the Lua thread.
	Dependency _dependency;                                 // By the Lua thread.

	bool _debugRealNumberPrecisely = false;                 // By the Lua thread.
	long long _timeout = SCRIPTING_LUA_TIMEOUT_NANOSECONDS; // By the Lua thread.
	unsigned _frameRate = BITTY_ACTIVE_FRAME_RATE;          // By the Lua thread.

	Atomic<unsigned> _fps;                                  // By the Lua, graphics threads.

	Lua::Function::Ptr _update = nullptr;                   // By the Lua thread.
	Lua::Function::Ptr _quit = nullptr;                     // By the Lua thread.
	Lua::Function::Ptr _focusLost = nullptr;                // By the Lua thread.
	Lua::Function::Ptr _focusGained = nullptr;              // By the Lua thread.
	Lua::Function::Ptr _rendererReset = nullptr;            // By the Lua thread.

	Atomic<FocusStates> _focusing;                          // By the Lua, graphics threads.
	Atomic<bool> _rendererResetting;                        // By the Lua, graphics threads.
	Atomic<States> _state;                                  // By the Lua, graphics threads.

	Atomic<int> _stepOver;                                  // By the Lua, graphics threads.
	Atomic<int> _stepInto;                                  // By the Lua, graphics threads.
	Atomic<int> _stepOut;                                   // By the Lua, graphics threads.

	Breakpoints _breakpoints;                               // By the Lua, graphics threads.
	Records _records;                                       // By the Lua, graphics threads.

	int _code = 0;                                          // By the Lua thread.
	double _delta = 0.0;                                    // By the Lua thread.
	Scope _scope;                                           // By the Lua thread.
	long long _activity = 0;                                // By the Lua thread.

	Updatables _updatables;                                 // By the Lua thread.

#if BITTY_MULTITHREAD_ENABLED
	std::thread _thread;
#endif /* BITTY_MULTITHREAD_ENABLED */

	mutable RecursiveMutex _lock;

public:
	ScriptingLua();
	virtual ~ScriptingLua() override;

	ScriptingLua* acquire(LockGuard<RecursiveMutex>::UniquePtr &guard) const;

	/**
	 * @return `lua_State*`.
	 */
	virtual void* pointer(void) override;

	virtual bool open(
		Observer* obsvr,
		const class Project* project,
		const class Project* editing,
		class Primitives* primitives,
		bool effectsEnabled
	) override;
	virtual bool close(void) override;

	virtual long long timeout(void) const override;
	virtual void timeout(long long val) override;
	virtual void activate(void) override;

	virtual Languages language(void) const override;

	virtual unsigned fps(void) const override;

	virtual void prepare(void) override;
	virtual void finish(void) override;

	virtual bool setup(void) override;
	virtual bool cycle(double delta) override;
	virtual bool focusLost(void) override;
	virtual bool focusGained(void) override;
	virtual bool renderTargetsReset(void) override;

	virtual bool update(double delta) override;

	virtual bool pending(void) const override;
	virtual void sync(double delta) override;

	virtual States current(void) const override;

	virtual bool exit(void) override;

	virtual bool run(void) override;
	virtual bool stop(void) override;

	virtual bool pause(void) override;
	virtual bool resume(void) override;

	virtual bool stepOver(void) override;
	virtual bool stepInto(void) override;
	virtual bool stepOut(void) override;

	virtual int getBreakpoints(const char* src, BreakpointGetter access) const override;
	virtual bool setBreakpoint(const char* src, int ln, bool brk) override;
	virtual int clearBreakpoints(const char* src) override;

	virtual int getRecords(RecordGetter access) const override;

	virtual bool getVariable(const char* name, const char* &type, Variant* &var) const override;
	virtual bool setVariable(const char* name, const Variant* var) const override;

	virtual bool debugRealNumberPrecisely(void) const override;
	virtual void debugRealNumberPrecisely(bool enabled) override;

	virtual Invokable getInvokable(const char* name) const override;
	virtual Variant invoke(Invokable func, int argc, const Variant* argv) override;

	double delta(void) const;

	virtual void gc(void) override;

	bool addUpdatable(class Updatable* ptr);
	bool removeUpdatable(class Updatable* ptr);

	static int check(lua_State* L, int code);

	static ScriptingLua* instanceOf(lua_State* L);

protected:
	bool hasBreakpoint(const char* src, int ln) const;

	void hookNormal(void);
	void hookBreak(void);
	void hookWait(const char* srcHint, int lnHint);

	void fillRecords(const char* srcHint, int lnHint);
	void clearRecords(void);
	void focusRecord(void);

	void fillScope(Scope &scope, int level = 0);

	static int require(lua_State* L);

	static void hookNormal(lua_State* L, lua_Debug* ar);
	static void hookBreak(lua_State* L, lua_Debug* ar);
};

/* ===========================================================================} */

#endif /* __SCRIPTING_LUA_H__ */
