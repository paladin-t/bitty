/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "code.h"
#include "datetime.h"
#include "file_handle.h"
#include "hacks.h"
#include "platform.h"
#include "primitives.h"
#include "project.h"
#include "scripting_lua.h"
#include "scripting_lua_api.h"
#include "updatable.h"

/*
** {===========================================================================
** Macros and constants
*/

#if !BITTY_DEBUG_ENABLED
#	pragma message("Lua debug disabled.")
#endif /* BITTY_DEBUG_ENABLED */

#ifndef SCRIPTING_LUA_WAIT_DURATION
#	define SCRIPTING_LUA_WAIT_DURATION 16
#endif /* SCRIPTING_LUA_WAIT_DURATION */

#ifndef SCRIPTING_LUA_UNKNOWN_FRAME
#	define SCRIPTING_LUA_UNKNOWN_FRAME "=?"
#endif /* SCRIPTING_LUA_UNKNOWN_FRAME */
#ifndef SCRIPTING_LUA_C_UNKNOWN_FRAME
#	define SCRIPTING_LUA_C_UNKNOWN_FRAME "=[C]"
#endif /* SCRIPTING_LUA_C_UNKNOWN_FRAME */

#ifndef SCRIPTING_LUA_TEMPORARY_ID
#	define SCRIPTING_LUA_TEMPORARY_ID "(temporary)"
#endif /* SCRIPTING_LUA_TEMPORARY_ID */
#ifndef SCRIPTING_LUA_C_TEMPORARY_ID
#	define SCRIPTING_LUA_C_TEMPORARY_ID "(C temporary)"
#endif /* SCRIPTING_LUA_C_TEMPORARY_ID */

/* ===========================================================================} */

/*
** {===========================================================================
** Lua scripting
*/

ScriptingLua::ScriptingLua() {
	_fps = 0;

	_focusing = IDLE;
	_rendererResetting = false;
	_state = READY;

	_stepOver = 0;
	_stepInto = 0;
	_stepOut = 0;
}

ScriptingLua::~ScriptingLua() {
	close();
}

ScriptingLua* ScriptingLua::acquire(LockGuard<RecursiveMutex>::UniquePtr &guard) const {
	guard = LockGuard<RecursiveMutex>::UniquePtr(new LockGuard<RecursiveMutex>(_lock));

	return const_cast<ScriptingLua*>(this);
}

void* ScriptingLua::pointer(void) {
	return _L;
}

bool ScriptingLua::open(
	Observer* observer, const class Project* project, const class Project* editing, class Primitives* primitives,
	unsigned fps, bool effectsEnabled
) {
	if (!Scripting::open(observer, project, editing, primitives, fps, effectsEnabled))
		return false;

	_frameRate = fps;

	return true;
}

bool ScriptingLua::close(void) {
#if BITTY_MULTITHREAD_ENABLED
	if (_thread.joinable())
		_thread.join();
#endif /* BITTY_MULTITHREAD_ENABLED */

	finish();

	clearBreakpoints(nullptr);

	return Scripting::close();
}

long long ScriptingLua::timeout(void) const {
	return _timeout;
}

void ScriptingLua::timeout(long long val) {
	_timeout = Math::clamp(val, -1ll, std::numeric_limits<long long>::max());
}

void ScriptingLua::activate(void) {
	_activity = DateTime::ticks();
}

Executable::Languages ScriptingLua::language(void) const {
	return LUA;
}

unsigned ScriptingLua::fps(void) const {
	return _fps;
}

void ScriptingLua::prepare(void) {
	LockGuard<decltype(_lock)> guard(_lock);

	finish();

	if (_L)
		return;

	_L = Lua::create(
		[] (void* /* userdata */, void* ptr, size_t /* oldSize */, size_t newSize) -> void* {
			if (newSize == 0) {
				free(ptr);

				return nullptr;
			} else {
				return realloc(ptr, newSize);
			}
		},
		this
	);

	observer()->require(this);
	assert(Lua::getTop(_L) == 0 && "Polluted Lua stack.");

	Lua::setLoader(_L, require);

	hookNormal();
}

void ScriptingLua::finish(void) {
	do {
		LockGuard<decltype(_lock)> guard(_lock);

		if (_quit && _quit->valid()) {
			const int ret = Lua::invoke(
				_L,
				[] (lua_State* L, void* ud) -> void {
					ScriptingLua* impl = (ScriptingLua*)ud;

					check(L, Lua::call(L, *impl->_quit));
					assert(Lua::getTop(L) == 0 && "Polluted Lua stack.");
				},
				this
			);
			check(_L, ret);
		}
	} while (false);

	clearRecords();

	do {
		LockGuard<decltype(_lock)> guard(_lock);

		_updatables.clear();

		_code = LUA_OK;
		_delta = 0;
		_scope.clear();
		_activity = 0;

		_stepOver = 0;
		_stepInto = 0;
		_stepOut = 0;

		_focusing = IDLE;
		_rendererResetting = false;

		if (_update)
			_update = nullptr;
		if (_quit)
			_quit = nullptr;
		if (_focusLost)
			_focusLost = nullptr;
		if (_focusGained)
			_focusGained = nullptr;
		if (_rendererReset)
			_rendererReset = nullptr;

		_fps = 0;

		if (_timeout >= 0)
			_timeout = SCRIPTING_LUA_TIMEOUT_NANOSECONDS;

		_requirement.clear();
		_dependency.clear();

		if (_L) {
			Lua::destroy(_L);
			_L = nullptr;
		}
	} while (false);
}

bool ScriptingLua::setup(void) {
	std::string src, ent;

	do {
		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = _project->acquire(acquired);
		if (!prj)
			return false;

		Asset* main = prj->main();
		if (!main) {
			observer()->warn("Empty project.");

			return false;
		}

		main->prepare(Asset::RUNNING, true);
		Object::Ptr obj = main->object(Asset::RUNNING);
		if (!obj) {
			main->finish(Asset::RUNNING, true);

			observer()->warn("Cannot find main entry.");

			return false;
		}
		Code::Ptr code = Object::as<Code::Ptr>(obj);
		if (!code) {
			main->finish(Asset::RUNNING, true);

			observer()->warn("Invalid main entry.");

			return false;
		}

		size_t len = 0;
		const char* txt = code->text(&len);
		if (!txt || len == 0)
			return false;

		src.assign(txt, len);
		ent = prj->entry();

		len = 0;
		txt = nullptr;
		code = nullptr;
		obj = nullptr;
		main->finish(Asset::RUNNING, true);
		main = nullptr;
		prj = nullptr;
	} while (false);

	do {
		LockGuard<decltype(_lock)> guard(_lock);

		_activity = DateTime::ticks();

		std::string entry = ent;
		if (Text::endsWith(entry, "." BITTY_LUA_EXT, true))
			entry = entry.substr(0, entry.length() - strlen("." BITTY_LUA_EXT));
		_dependency.push_back(entry);

		entry = "=" + ent;
		if (check(_L, luaL_loadbuffer(_L, src.c_str(), src.size(), entry.c_str())) != LUA_OK) {
			_dependency.pop_back();
			assert(_dependency.empty());

			return false;
		}
		if (check(_L, lua_pcall(_L, 0, LUA_MULTRET, 0)) != LUA_OK) {
			_dependency.pop_back();
			assert(_dependency.empty());

			return false;
		}
		const int discarded = Lua::getTop(_L);
		if (discarded > 0) {
			Lua::pop(_L, discarded);

			const std::string msg = Text::cformat(
				discarded == 1 ?
					"Discarded %d unused return value." :
					"Discarded %d unused return values.",
				discarded
			);
			observer()->warn(msg.c_str());
		}

		_dependency.pop_back();
		assert(_dependency.empty());

		Lua::Function setup;
		Lua::getGlobal(_L, SCRIPTING_SETUP_FUNCTION_NAME);
		Lua::read(_L, setup);
		Lua::pop(_L);

		Lua::getGlobal(_L, SCRIPTING_UPDATE_FUNCTION_NAME);
		Lua::read(_L, _update);
		Lua::pop(_L);

		Lua::getGlobal(_L, SCRIPTING_QUIT_FUNCTION_NAME);
		Lua::read(_L, _quit);
		Lua::pop(_L);

		Lua::getGlobal(_L, SCRIPTING_FOCUS_LOST_FUNCTION_NAME);
		Lua::read(_L, _focusLost);
		Lua::pop(_L);

		Lua::getGlobal(_L, SCRIPTING_FOCUS_GAINED_FUNCTION_NAME);
		Lua::read(_L, _focusGained);
		Lua::pop(_L);

		Lua::getGlobal(_L, SCRIPTING_RENDERER_RESET_FUNCTION_NAME);
		Lua::read(_L, _rendererReset);
		Lua::pop(_L);

		assert(Lua::getTop(_L) == 0 && "Polluted Lua stack.");
		if (setup.valid()) {
			struct Context {
				ScriptingLua* impl = nullptr;
				Lua::Function func;
			};

			Context ctx;
			ctx.impl = this;
			ctx.func = setup;

			const int ret = Lua::invoke(
				_L,
				[] (lua_State* L, void* ud) -> void {
					Context* ctx = (Context*)ud;

					check(L, Lua::call(L, ctx->func));
					assert(Lua::getTop(L) == 0 && "Polluted Lua stack.");
				},
				&ctx
			);
			if (check(_L, ret) != LUA_OK)
				return false;
		}
	} while (false);

	return true;
}

bool ScriptingLua::cycle(double delta) {
	LockGuard<decltype(_lock)> guard(_lock);

	_delta = delta;
	_activity = DateTime::ticks();

	if (!_L)
		return false;

	if (!_update || !_update->valid())
		return false;

	if (_primitives)
		_primitives->newFrame();

	Lua::ProtectedFunction func = [] (lua_State* L, void* ud) -> void {
		ScriptingLua* impl = (ScriptingLua*)ud;

		impl->_code = check(L, Lua::call(L, *impl->_update, impl->_delta));
		assert(Lua::getTop(L) == 0 && "Polluted Lua stack.");
	};
#if BITTY_DEBUG_ENABLED
	int ret = LUA_OK;
	try {
		ret = Lua::invoke(_L, func, this);
	} catch (const std::bad_alloc &) {
		Lua::gc(_L);

		Lua::error(_L, "Memory overflow.");
	}
#else /* BITTY_DEBUG_ENABLED */
	const int ret = Lua::invoke(_L, func, this);
	if (_state == HALTING)
		return false;
#endif /* BITTY_DEBUG_ENABLED */
	if (check(_L, ret) != LUA_OK || _code != LUA_OK)
		return false;

	// For native developers who write gameplay code in mixed Lua/C++ or pure C++,
	// put your C++ entry here. You can also implement your own `Scripting` or
	// `Executable` instead of this `ScriptingLua`.
	// Eg.
#if false
	if (_primitives) {
		_primitives->text("Hello World!", 2, 2, nullptr, 1);
	}
#endif

	if (_primitives)
		_primitives->commit();

	sync(delta);

	return true;
}

bool ScriptingLua::focusLost(void) {
	_focusing = LOST;

	return true;
}

bool ScriptingLua::focusGained(void) {
	_focusing = GAINED;

	return true;
}

bool ScriptingLua::renderTargetsReset(void) {
	_rendererResetting = true;

	return true;
}

bool ScriptingLua::update(double delta) {
#if BITTY_MULTITHREAD_ENABLED
	(void)delta;
#else /* BITTY_MULTITHREAD_ENABLED */
	if (_state == RUNNING) {
		if (!cycle(delta)) {
			_state = READY;

			observer()->stop();
		}
	}
#endif /* BITTY_MULTITHREAD_ENABLED */

	return true;
}

bool ScriptingLua::pending(void) const {
	return !_updatables.empty();
}

void ScriptingLua::sync(double delta) {
	for (Updatable* up : _updatables)
		up->update(delta);

	do {
		switch (_focusing) {
		case IDLE: // Do nothing.
			break;
		case LOST:
			if (_focusLost && _focusLost->valid()) {
				const int ret = Lua::invoke(
					_L,
					[] (lua_State* L, void* ud) -> void {
						ScriptingLua* impl = (ScriptingLua*)ud;

						check(L, Lua::call(L, *impl->_focusLost));
						assert(Lua::getTop(L) == 0 && "Polluted Lua stack.");
					},
					this
				);
				check(_L, ret);
			}

			_focusing = IDLE;

			break;
		case GAINED:
			if (_focusGained && _focusGained->valid()) {
				const int ret = Lua::invoke(
					_L,
					[] (lua_State* L, void* ud) -> void {
						ScriptingLua* impl = (ScriptingLua*)ud;

						check(L, Lua::call(L, *impl->_focusGained));
						assert(Lua::getTop(L) == 0 && "Polluted Lua stack.");
					},
					this
				);
				check(_L, ret);
			}

			_focusing = IDLE;

			break;
		}
	} while (false);

	if (_rendererResetting) {
		if (_rendererReset && _rendererReset->valid()) {
			const int ret = Lua::invoke(
				_L,
				[] (lua_State* L, void* ud) -> void {
					ScriptingLua* impl = (ScriptingLua*)ud;

					check(L, Lua::call(L, *impl->_rendererReset));
					assert(Lua::getTop(L) == 0 && "Polluted Lua stack.");
				},
				this
			);
			check(_L, ret);
		}

		_rendererResetting = false;
	}
}

Executable::States ScriptingLua::current(void) const {
	return _state;
}

bool ScriptingLua::exit(void) {
	const States prev = _state;
	if (prev == RUNNING || prev == PAUSED) {
		_state = HALTING;

		return true;
	}

	return false;
}

bool ScriptingLua::run(void) {
	switch (_state) {
	case READY: // Do nothing.
		break;
	case RUNNING:
		return false;
	case PAUSED:
		_state = RUNNING;

		return true;
	case HALTING:
		return false;
	}

	if (!_project)
		return false;

	prepare();

#if BITTY_MULTITHREAD_ENABLED
	auto proc = [] (ScriptingLua* impl) -> void {
		Platform::threadName("LUA");

		DateTime::sleep(SCRIPTING_LUA_WAIT_DURATION);

		impl->_state = RUNNING;

		impl->hookNormal();

		if (impl->setup()) {
			constexpr const int STAT_INTERVAL = 3; // 3 seconds.
			long long stamp = DateTime::ticks();
			unsigned frames = 0;
			double ticks = 0;
			States last = impl->_state;
			while (true) {
				const long long begin = DateTime::ticks();
				const double delta = begin >= stamp ? DateTime::toSeconds(begin - stamp) : 0;
				stamp = begin;

				++frames;
				ticks += delta;
				if (ticks >= STAT_INTERVAL) {
					impl->_fps = (unsigned)(frames / ticks);
					frames = 0;
					ticks -= STAT_INTERVAL;
				}

				const States current = impl->_state;
				if (last != current) {
					if (current == PAUSED) {
						impl->_stepInto = 1;

						impl->hookBreak();
					} else if (last == PAUSED) {
						impl->hookNormal();

						impl->_activity = DateTime::ticks();
					}

					last = current;
				}
				if (current == PAUSED) {
					if (!(impl->_stepOver || impl->_stepInto || impl->_stepOut))
						continue;
				} else if (current == READY || current == HALTING) {
					break;
				}

				if (!impl->cycle(delta)) {
					if (impl->_state != HALTING)
						impl->_thread.detach();

					break;
				}

				const long long end = DateTime::ticks();
				const long long diff = end >= begin ? end - begin : 0;
				const double elapsed = DateTime::toSeconds(diff);
				const double expected = 1.0 / impl->_frameRate;
				const double rest = expected - elapsed;
				if (rest > 0)
					DateTime::sleep((int)(rest * 1000));
			}
		}

		impl->_state = HALTING;

		impl->finish();

		impl->_state = READY;

		impl->observer()->stop();

#if BITTY_THREADING_GUARD_ENABLED
		graphicsThreadingGuard.end();
#endif /* BITTY_THREADING_GUARD_ENABLED */
	};
	_thread = std::thread(proc, this);
#if BITTY_THREADING_GUARD_ENABLED
	graphicsThreadingGuard.begin(_thread);
#endif /* BITTY_THREADING_GUARD_ENABLED */
#else /* BITTY_MULTITHREAD_ENABLED */
	_state = RUNNING;

	if (!setup()) {
		_state = READY;

		observer()->stop();

		return false;
	}
#endif /* BITTY_MULTITHREAD_ENABLED */

	return true;
}

bool ScriptingLua::stop(void) {
#if BITTY_MULTITHREAD_ENABLED
	if (_state != RUNNING && _state != PAUSED) {
		if (_thread.joinable()) {
			_thread.join();

			return true;
		}

		return false;
	}

	_state = HALTING;

	_thread.join();

	return true;
#else /* BITTY_MULTITHREAD_ENABLED */
	if (_state != RUNNING && _state != PAUSED)
		return false;

	_state = HALTING;

	finish();

	_state = READY;

	observer()->stop();

	return true;
#endif /* BITTY_MULTITHREAD_ENABLED */
}

bool ScriptingLua::pause(void) {
	if (_state != RUNNING)
		return false;

	_state = PAUSED;

	return true;
}

bool ScriptingLua::resume(void) {
	if (_state != PAUSED)
		return false;

	_state = RUNNING;

	return true;
}

bool ScriptingLua::stepOver(void) {
	_stepOver = 1;

	return true;
}

bool ScriptingLua::stepInto(void) {
	_stepInto = 1;

	return true;
}

bool ScriptingLua::stepOut(void) {
	_stepOut = 1;

	return true;
}

int ScriptingLua::getBreakpoints(const char* src, BreakpointGetter get) const {
	LockGuard<decltype(_breakpoints.lock)> guard(_breakpoints.lock);

	if (!src) {
		if (get) {
			for (const Breakpoint &brk : _breakpoints)
				get(brk.source.c_str(), brk.line);
		}

		return (int)_breakpoints.count();
	}

	int result = 0;
	for (const Breakpoint &brk : _breakpoints) {
		if (brk.source == src) {
			if (get)
				get(brk.source.c_str(), brk.line);
			++result;
		} else if (result) {
			break;
		}
	}

	return result;
}

bool ScriptingLua::setBreakpoint(const char* src, int ln, bool brk) {
	LockGuard<decltype(_breakpoints.lock)> guard(_breakpoints.lock);

	const Breakpoint* exist = _breakpoints.find(src, ln);

	if (brk) {
		if (exist)
			return false;

		_breakpoints.add(Breakpoint(src, ln));
	} else {
		if (!exist)
			return false;

		_breakpoints.remove(_breakpoints.indexOf(exist));
	}

	return true;
}

int ScriptingLua::clearBreakpoints(const char* src) {
	LockGuard<decltype(_breakpoints.lock)> guard(_breakpoints.lock);

	int result = 0;
	if (src) {
		Breakpoints::Iterator it = _breakpoints.begin();
		while (it != _breakpoints.end()) {
			const Breakpoint &brk = *it;
			if (brk.source == src) {
				it = _breakpoints.erase(it);
				++result;
			} else {
				if (result)
					break;
				else
					++it;
			}
		}
	} else {
		result = (int)_breakpoints.count();
		_breakpoints.clear();
	}

	return result;
}

int ScriptingLua::getRecords(RecordGetter get) const {
	LockGuard<decltype(_records.lock)> guard(_records.lock);

	if (get) {
		for (const Record &record : _records) {
			Variable::List::Collection::const_iterator it = record.variables.begin();
			VariableGetter getVars = [&] (const char* &name, const char* &type, const Variant* &data, bool &isUpvalue) -> bool {
				if (it == record.variables.end())
					return false;

				const Variable &var = *it;
				name = var.name.c_str();
				type = var.type.c_str();
				data = &var.data;
				isUpvalue = var.isUpvalue;

				++it;

				return true;
			};

			get(record.source.c_str(), record.line, record.lineDefined, record.name.c_str(), record.what.c_str(), getVars);
		}
	}

	return (int)_records.count();
}

bool ScriptingLua::getVariable(const char* name_, const char* &type_, Variant* &var) const {
	if (!name_)
		return false;

	LockGuard<decltype(_lock)> guard(_lock);

	lua_Debug ar;
	int lv = 0;
	while (Lua::getStack(_L, lv++, &ar)) {
		Lua::getInfo(_L, "u", &ar);

		for (int i = 1; true; ++i) {
			const char* name = Lua::getLocal(_L, &ar, i);
			if (!name)
				break;

			if (strcmp(name_, name) == 0) {
				if (var)
					Lua::read(_L, var, Lua::Index(i));
				if (type_)
					type_ = Lua::typeNameOf(_L, i);

				Lua::pop(_L);

				return true;
			}

			Lua::pop(_L);
		}
	}

	return false;
}

bool ScriptingLua::setVariable(const char* name_, const Variant* var) const {
	if (!name_)
		return false;

	LockGuard<decltype(_lock)> guard(_lock);

	LockGuard<decltype(_records.lock)> guardRecords(_records.lock); // Forbid reading records.

	lua_Debug ar;
	int lv = 0;
	while (Lua::getStack(_L, lv++, &ar)) {
		Lua::getInfo(_L, "u", &ar);

		for (int i = 1; true; ++i) {
			const char* name = Lua::getLocal(_L, &ar, i);
			if (!name)
				break;

			if (strcmp(name_, name) == 0) {
				Lua::pop(_L);
				if (var)
					Lua::write(_L, var);
				else
					Lua::write(_L, nullptr);
				const char* modified = Lua::setLocal(_L, &ar, i);
				assert(strcmp(modified, name) == 0); (void)modified;

				Lua::pop(_L);

				return true;
			}

			Lua::pop(_L);
		}
	}

	if (var)
		Lua::write(_L, var);
	else
		Lua::write(_L, nullptr);
	Lua::setGlobal(_L, name_);

	return true;
}

bool ScriptingLua::debugRealNumberPrecisely(void) const {
	return _debugRealNumberPrecisely;
}

void ScriptingLua::debugRealNumberPrecisely(bool enabled) {
	_debugRealNumberPrecisely = enabled;
}

Executable::Invokable ScriptingLua::getInvokable(const char* name) const {
	LockGuard<decltype(_lock)> guard(_lock);

	if (!name || !*name)
		return nullptr;

	Invokable invokable(
		new Lua::Function(),
		[] (void* ptr) -> void {
			Lua::Function* func = (Lua::Function*)ptr;
			delete func;
		}
	);
	Lua::Function &func = *(Lua::Function*)invokable.get();
	Lua::getGlobal(_L, name);
	Lua::read(_L, func);
	Lua::pop(_L);
	if (!func.valid())
		return nullptr;

	return invokable;
}

Variant ScriptingLua::invoke(Invokable func, int argc, const Variant* argv) {
	LockGuard<decltype(_lock)> guard(_lock);

	Variant result(false);
	if (func) {
		struct Context {
			ScriptingLua* impl = nullptr;
			Variant* result = nullptr;
			Invokable func = nullptr;
			int argc = 0;
			const Variant* argv = nullptr;
		};

		Context ctx;
		ctx.impl = this;
		ctx.result = &result;
		ctx.func = func;
		ctx.argc = argc;
		ctx.argv = argv;

		const int ret = Lua::invoke(
			_L,
			[] (lua_State* L, void* ud) -> void {
				Context* ctx = (Context*)ud;

				check(
					L,
					Lua::call(
						ctx->result,
						L,
						*(Lua::Function*)ctx->func.get(),
						ctx->argc, ctx->argv
					)
				);
				assert(Lua::getTop(L) == 0 && "Polluted Lua stack.");
			},
			&ctx
		);
		if (check(_L, ret) != LUA_OK)
			return Variant(nullptr);
	}

	return result;
}

double ScriptingLua::delta(void) const {
	return _delta;
}

void ScriptingLua::gc(void) {
	Lua::gc(_L);
}

bool ScriptingLua::addUpdatable(class Updatable* ptr) {
	if (std::find(_updatables.begin(), _updatables.end(), ptr) != _updatables.end())
		return false;

	_updatables.push_back(ptr);

	return true;
}

bool ScriptingLua::removeUpdatable(class Updatable* ptr) {
	Updatables::iterator it = std::find(_updatables.begin(), _updatables.end(), ptr);
	if (it == _updatables.end())
		return false;

	_updatables.erase(it);

	return true;
}

int ScriptingLua::check(lua_State* L, int code) {
	ScriptingLua* impl = instanceOf(L);

	if (code == LUA_OK)
		return LUA_OK;

	std::string err;
	Lua::read(L, err, Lua::Index(-1));

	if (err.empty()) {
		impl->observer()->error("Unknown error.");
	} else {
		impl->observer()->error(err.c_str());

		Lua::pop(L, 1);
	}

	return code;
}

ScriptingLua* ScriptingLua::instanceOf(lua_State* L) {
	ScriptingLua* impl = (ScriptingLua*)Lua::userdata(L);

	return impl;
}

bool ScriptingLua::hasBreakpoint(const char* src, int ln) const {
	LockGuard<decltype(_breakpoints.lock)> guard(_breakpoints.lock);

	if (_breakpoints.empty())
		return false;

	if (src) {
		if (*src == '=') // Literal prefix.
			++src;
		else if (*src == '@') // File prefix.
			++src;
	}

	const Breakpoint* brk = _breakpoints.find(src, ln);

	return !!brk;
}

void ScriptingLua::hookNormal(void) {
#if BITTY_DEBUG_ENABLED
	Lua::setHook(_L, hookNormal, LUA_MASKLINE, 0);
#endif /* BITTY_DEBUG_ENABLED */
}

void ScriptingLua::hookBreak(void) {
#if BITTY_DEBUG_ENABLED
	Lua::setHook(_L, hookBreak, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE, 0);
#endif /* BITTY_DEBUG_ENABLED */
}

void ScriptingLua::hookWait(const char* srcHint, int lnHint) {
	fillRecords(srcHint, lnHint);

	focusRecord();

	while (true) {
		if (_stepInto)
			break;
		if (_stepOut)
			break;
		if (_stepOver)
			break;
		if (_state != PAUSED)
			break;

		DateTime::sleep(SCRIPTING_LUA_WAIT_DURATION);
	}

	if (_stepOver)
		fillScope(_scope); // Expect executing "next line" in the same scope.
	else if (_stepOut)
		fillScope(_scope, 1); // Expect returning back to the previous scope.
	else
		_scope.clear(); // Scope doesn't matter.

	clearRecords();
}

void ScriptingLua::fillRecords(const char* srcHint, int lnHint) {
	LockGuard<decltype(_records.lock)> guard(_records.lock);

	_records.clear();

	auto fillVar = [] (lua_State* L, Record &record, std::function<const char*(lua_State*, int)> get, bool isUpvalue) -> void {
		for (int i = 1; true; ++i) {
			const char* name = get(L, i);
			if (!name)
				break;

			if (strcmp(name, SCRIPTING_LUA_TEMPORARY_ID) == 0 || strcmp(name, SCRIPTING_LUA_C_TEMPORARY_ID) == 0) {
				Lua::pop(L);

				continue;
			}

			Variant var = nullptr;
			const int t = Lua::typeOf(L, -1);
			const char* y = Lua::typeNameOf(L, -1);
			Lua::TableOptions options;
			options.viewable = true;
			options.includeMetaTable = true;
			options.maxLevelCount = BITTY_DEBUG_TABLE_LEVEL_MAX_COUNT + 1;
			switch (t) {
			case LUA_TSTRING:
				Lua::read(L, &var, Lua::Index(-1), options);

				break;
			case LUA_TTABLE:
				// var = Variant(y); // Fill in its type name only.
				Lua::read(L, &var, Lua::Index(-1), options); // Retrieve its value.

				break;
			case LUA_TFUNCTION: {
					static constexpr const char* ctype = "function";
					var = (void*)ctype;
				}

				break;
			case LUA_TUSERDATA:
				Lua::read(L, &var, Lua::Index(-1), options);

				break;
			case LUA_TTHREAD: {
					static constexpr const char* ctype = "thread";
					var = (void*)ctype;
				}

				break;
			default:
				Lua::read(L, &var, Lua::Index(-1), options);
				if (var.type() == Variant::STRING) {
					std::string val = var.toString();
					val = Text::replace(val, "\r", "\\r");
					val = Text::replace(val, "\n", "\\n");
					val = Text::replace(val, "\t", "\\t");
					var = '"' + val + '"';
				}

				break;
			}
			record.variables.add(Variable(name, y, var, isUpvalue));

			Lua::pop(L);
		}
	};

	lua_Debug ar;
	int lv = 0;
	while (Lua::getStack(_L, lv++, &ar)) {
		Lua::getInfo(_L, "nSluf", &ar);

		const char* src = ar.source;
		if (strcmp(src, SCRIPTING_LUA_UNKNOWN_FRAME) == 0 || strcmp(src, SCRIPTING_LUA_C_UNKNOWN_FRAME) == 0)
			src = srcHint;
		if (*src == '=' || *src == '@')
			++src;
		int ln = ar.currentline;
		if (ln == -1)
			ln = lnHint;
		Record &record = _records.add(Record(src, ln, ar.linedefined, ar.name, ar.what));

		fillVar(
			_L, record,
			[&] (lua_State* L, int i) -> const char* {
				return Lua::getLocal(L, &ar, i);
			},
			false
		);

		fillVar(
			_L, record,
			[] (lua_State* L, int i) -> const char* {
				return Lua::getUpvalue(L, -1, i);
			},
			true
		);

		Lua::pop(_L);
	}
}

void ScriptingLua::clearRecords(void) {
	LockGuard<decltype(_records.lock)> guard(_records.lock);

	_records.clear();
}

void ScriptingLua::focusRecord(void) {
	LockGuard<decltype(_records.lock)> guard(_records.lock);

	Records::Iterator top = _records.begin();
	if (top != _records.end()) {
		const Record &r = *top;
		observer()->focus(r.source.c_str(), r.line);
	}
}

void ScriptingLua::fillScope(Scope &scope, int level) {
	scope.clear();

	lua_Debug ar;
	if (Lua::getStack(_L, level, &ar)) {
		Lua::getInfo(_L, "nSl", &ar);
		scope.fill(ar.source, ar.currentline, ar.linedefined, ar.name, ar.what);
	}
}

int ScriptingLua::require(lua_State* L) {
	const lua_CFunction loader = [] (lua_State* L) -> int {
		ScriptingLua* impl = instanceOf(L);

		int result = 0;

		const char* path = nullptr;
		Lua::check<>(L, path);
		if (!path)
			return result;

		std::string full = path;
		if (!Text::endsWith(full, "." BITTY_LUA_EXT, true))
			full += "." BITTY_LUA_EXT;

		Requirement::iterator required = impl->_requirement.find(path);
		if (required != impl->_requirement.end()) {
			if (impl->_dependency.empty()) {
				const std::string msg = Text::cformat("Ignored recursive requiring: \"%s\".", path);
				impl->observer()->warn(msg.c_str());
			} else {
				std::string stack;
				for (Dependency::reverse_iterator it = impl->_dependency.rbegin(); it != impl->_dependency.rend(); ++it) {
					stack += "\n  \"";
					stack += *it;
					stack += "\"";
				}
				stack += ".";
				const std::string msg = Text::cformat("Ignored recursive requiring: \"%s\" from%s", path, stack.c_str());
				impl->observer()->warn(msg.c_str());
			}

			return result;
		}

		do {
			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = impl->_project->acquire(acquired);
			if (!prj)
				break;

			Asset* asset = prj->get(full.c_str());
			if (!asset)
				break;

			asset->prepare(Asset::RUNNING, true);
			Object::Ptr obj = asset->object(Asset::RUNNING);
			if (!obj) {
				asset->finish(Asset::RUNNING, true);

				break;
			}
			Code::Ptr code = Object::as<Code::Ptr>(obj);
			if (!code) {
				asset->finish(Asset::RUNNING, true);

				break;
			}

			size_t len = 0;
			const char* txt = code->text(&len);
			if (!txt || len == 0) {
				asset->finish(Asset::RUNNING, true);

				break;
			}

			impl->_requirement.insert(path);
			impl->_dependency.push_back(path);

			check(L, luaL_loadbuffer(L, txt, len, full.c_str()));

			txt = nullptr;
			len = 0;
			code = nullptr;
			obj = nullptr;
			asset->finish(Asset::RUNNING, true);
			asset = nullptr;
			acquired.reset();
			prj = nullptr;

			check(L, lua_pcall(L, 0, LUA_MULTRET, 0));

			impl->_dependency.pop_back();

			result = 1;

			return result;
		} while (false);

		File* file = File::create();
		if (file->open(full.c_str(), Stream::READ)) {
			std::string code;
			file->readString(code);

			impl->_requirement.insert(path);
			impl->_dependency.push_back(path);

			check(L, luaL_loadbuffer(L, code.c_str(), code.length(), full.c_str()));

			check(L, lua_pcall(L, 0, LUA_MULTRET, 0));

			impl->_dependency.pop_back();

			result = 1;
		}
		File::destroy(file);

		if (!result) {
			const std::string msg = Text::cformat("Cannot require source code: \"%s\".", path);
			impl->observer()->error(msg.c_str());
		}

		return result;
	};

	return Lua::write(L, loader);
}

void ScriptingLua::hookNormal(lua_State* L, lua_Debug* ar) {
	ScriptingLua* impl = instanceOf(L);

	Lua::getInfo(L, "Sl", ar);
	if (impl->hasBreakpoint(ar->source, ar->currentline)) {
		if (impl->_state == RUNNING) {
			impl->_state = PAUSED;

			impl->hookWait(ar->source, ar->currentline);

			if (impl->_stepOver || impl->_stepInto || impl->_stepOut)
				impl->hookBreak();

			impl->_activity = DateTime::ticks();
		}
	}

	if (impl->_timeout > 0) {
		const long long now = DateTime::ticks();
		if (now > impl->_activity) {
			const long long diff = now - impl->_activity;
			if (diff >= impl->_timeout)
				Lua::error(L, "Invoking timeout.");
		}
	} else {
		if (impl->_state == HALTING) {
			Lua::setHook(L, nullptr, LUA_MASKLINE, 0); // Cancel the current normal hook.

			Lua::error(L, "User abort.");
		}
	}
}

void ScriptingLua::hookBreak(lua_State* L, lua_Debug* ar) {
	ScriptingLua* impl = instanceOf(L);

	Scope active;
	impl->fillScope(active);

	if (active.what == "C")
		return;

	Lua::getInfo(L, "Sl", ar);
	switch (ar->event) {
	case LUA_HOOKCALL: {
			if (impl->_stepOver) {
				if (impl->_scope == active) {
					impl->_stepOver = 0;
					impl->hookWait(ar->source, ar->currentline);
				}
			} else if (impl->_stepInto) {
				impl->_stepInto = 0;
				impl->hookWait(ar->source, ar->currentline);
			}
		}

		break;
	case LUA_HOOKRET: {
			if (impl->_stepOver) {
				if (impl->_scope == active) {
					impl->_stepOver = 0;
					impl->hookWait(ar->source, ar->currentline);

					impl->fillScope(impl->_scope, 1);
				}
			} else if (impl->_stepOut) {
				if (impl->_scope == active) {
					impl->_stepOut = 0;
					impl->hookWait(ar->source, ar->currentline);

					impl->fillScope(impl->_scope, 1);
				} else if (impl->_scope.empty()) {
					impl->_stepOut = 0;
					impl->_stepInto = 1;
				}
			}
		}

		break;
	case LUA_HOOKLINE: {
			if (impl->_stepOver) {
				if (impl->_scope.empty() || impl->_scope == active) {
					impl->_stepOver = 0;
					impl->hookWait(ar->source, ar->currentline);
				}
			} else if (impl->_stepInto) {
				impl->_stepInto = 0;
				impl->hookWait(ar->source, ar->currentline);
			}

			if (impl->hasBreakpoint(active.source.c_str(), active.line)) {
				if (impl->_stepOver) {
					if (impl->_scope == active)
						break;
					impl->_stepOver = 0;
				} else {
					impl->_stepOut = 0;
				}

				if (impl->_state == RUNNING)
					impl->_state = PAUSED;

				impl->hookWait(ar->source, ar->currentline);
			}
		}

		break;
	}

	if (impl->_state == HALTING) {
		Lua::setHook(L, nullptr, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE, 0); // Cancel the current break hook.

		Lua::error(L, "User abort.");
	}
}

/* ===========================================================================} */
