/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2023 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "bitty.h"
#include "bytes.h"
#include "datetime.h"
#include "filesystem.h"
#include "json.h"
#include "scripting_lua.h"
#include "scripting_lua_api_promises.h"
#include "web.h"
#if defined BITTY_CP_VC
#	pragma warning(push)
#	pragma warning(disable : 4800)
#	pragma warning(disable : 4819)
#endif /* BITTY_CP_VC */
#if defined BITTY_OS_WIN || defined BITTY_OS_MAC || defined BITTY_OS_LINUX
#	include "../lib/portable_file_dialogs/portable-file-dialogs.h"
#elif defined BITTY_OS_HTML
#	include "../lib/portable_file_dialogs_polyfill/portable-file-dialogs.h"
#else /* Platform macro. */
#	include "../lib/portable_file_dialogs_polyfill/portable-file-dialogs.h"
#endif /* Platform macro. */
#if defined BITTY_CP_VC
#	pragma warning(pop)
#endif /* BITTY_CP_VC */
#if defined BITTY_OS_HTML
#	include <emscripten.h>
#endif /* BITTY_OS_HTML */

/*
** {===========================================================================
** Utilities
*/

namespace Lua { // Standard.

/**< Promise. */

LUA_CHECK_OBJ(Promise)
LUA_READ_OBJ(Promise)
LUA_WRITE_OBJ(Promise)
LUA_WRITE_OBJ_CONST(Promise)

}

namespace Lua { // Library.

/**< Bytes. */

LUA_CHECK_OBJ(Bytes)
LUA_READ_OBJ(Bytes)
LUA_WRITE_OBJ(Bytes)
LUA_WRITE_OBJ_CONST(Bytes)

/**< JSON. */

LUA_CHECK_OBJ(Json)
LUA_READ_OBJ(Json)
LUA_WRITE_OBJ(Json)
LUA_WRITE_OBJ_CONST(Json)

}

#if defined BITTY_OS_HTML
EM_JS(
	const char*, scriptingLuaApiPromisesInput, (const char* pmt, const char* default_), {
		var ret = prompt(UTF8ToString(pmt), UTF8ToString(default_));
		if (ret == null) {
			ret = '(EMPTY)';
		}
		var lengthBytes = lengthBytesUTF8(ret) + 1;
		var stringOnWasmHeap = _malloc(lengthBytes);
		stringToUTF8(ret, stringOnWasmHeap, lengthBytes + 1);

		return stringOnWasmHeap;
	}
);
EM_JS(
	void, scriptingLuaApiPromisesFree, (void* ptr), {
		_free(ptr);
	}
);
#endif /* BITTY_OS_HTML */

/* ===========================================================================} */

/*
** {===========================================================================
** Standard
*/

namespace Lua {

namespace Standard {

/**< Promise. */

typedef std::pair<Function::Ptr, Ref::Ptr> PromisePair;

static int Promise_ctor(lua_State* L, Promise::Ptr &promise, bool write_) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	Promise::Ptr obj(
		Promise::create(),
		[impl] (Promise* promise) -> void {
			impl->removeUpdatable(promise);

			promise->clear();

			Promise::destroy(promise);
		}
	);
	if (!obj)
		return write(L, nullptr);

	impl->addUpdatable(obj.get());

	promise = obj;

	if (write_)
		return write(L, &obj);

	return 0;
}

static int Promise___gc(lua_State* L) {
	Promise::Ptr* obj = nullptr;
	check<>(L, obj);
	if (!obj)
		return 0;

	obj->~shared_ptr();

	return 0;
}

static int Promise_write(lua_State* L, const Variant &arg) {
	if (arg.type() != Variant::OBJECT)
		return write(L, &arg);

	Object::Ptr obj = (Object::Ptr)arg;
	if (!obj)
		return write(L, &arg);

	Bytes::Ptr bytes = nullptr;
	if (Object::is<Bytes::Ptr>(obj))
		bytes = Object::as<Bytes::Ptr>(obj);
	if (bytes)
		return write(L, &bytes);

	Json::Ptr json = nullptr;
	if (Object::is<Json::Ptr>(obj))
		json = Object::as<Json::Ptr>(obj);
	if (json)
		return write(L, &json);

	return write(L, &arg);
}

static int Promise_call(lua_State* L, Function::Ptr* ptr, const Variant &arg) {
	auto general = [] (lua_State* L, Function::Ptr* ptr, const Variant &arg) -> int {
		const Variant* argptr = &arg;

		return ScriptingLua::check(L, call(L, **ptr, argptr));
	};

	if (arg.type() != Variant::OBJECT)
		return general(L, ptr, arg);

	Object::Ptr obj = (Object::Ptr)arg;
	if (!obj)
		return general(L, ptr, arg);

	Bytes::Ptr bytes = nullptr;
	if (Object::is<Bytes::Ptr>(obj))
		bytes = Object::as<Bytes::Ptr>(obj);
	if (bytes)
		return ScriptingLua::check(L, call(L, **ptr, &bytes));

	Json::Ptr json = nullptr;
	if (Object::is<Json::Ptr>(obj))
		json = Object::as<Json::Ptr>(obj);
	if (json)
		return ScriptingLua::check(L, call(L, **ptr, &json));

	return general(L, ptr, arg);
}

static int Promise_thus(lua_State* L) {
	Promise::Ptr* obj = nullptr;
	Function::Ptr callback = nullptr;
	read<>(L, obj, callback);
	Ref::Ptr ref = nullptr;
	read<>(L, ref);

	if (obj && callback) {
		Promise::ThenHandler::Callback func = std::bind(
			[] (lua_State* L, Promise::ThenHandler* self, const Variant &arg) -> void {
				Function::Ptr* ptr = (Function::Ptr*)self->userdata().get();

				Promise_call(L, ptr, arg);

				self->clear();
			},
			L, std::placeholders::_1, std::placeholders::_2
		);
		Any ud(
			new PromisePair(callback, ref),
			[] (void* ptr) -> void {
				PromisePair* pair = (PromisePair*)ptr;
				pair->first = nullptr;
				pair->second = nullptr;
				delete pair;
			}
		);
		Promise::ThenHandler cb(func, ud);

		obj->get()->then(cb);
	}

	return write(L, Index(1));
}

static int Promise_catch(lua_State* L) {
	Promise::Ptr* obj = nullptr;
	Function::Ptr callback = nullptr;
	read<>(L, obj, callback);
	Ref::Ptr ref = nullptr;
	read<>(L, ref);

	if (obj && callback) {
		Promise::FailHandler::Callback func = std::bind(
			[] (lua_State* L, Promise::FailHandler* self, const Variant &arg) -> void {
				Function::Ptr* ptr = (Function::Ptr*)self->userdata().get();

				Promise_call(L, ptr, arg);

				self->clear();
			},
			L, std::placeholders::_1, std::placeholders::_2
		);
		Any ud(
			new PromisePair(callback, ref),
			[] (void* ptr) -> void {
				PromisePair* pair = (PromisePair*)ptr;
				pair->first = nullptr;
				pair->second = nullptr;
				delete pair;
			}
		);
		Promise::FailHandler cb(func, ud);

		obj->get()->fail(cb);
	}

	return write(L, Index(1));
}

static int Promise_finally(lua_State* L) {
	Promise::Ptr* obj = nullptr;
	Function::Ptr callback = nullptr;
	read<>(L, obj, callback);
	Ref::Ptr ref = nullptr;
	read<>(L, ref);

	if (obj && callback) {
		Promise::AlwaysHandler::Callback func = std::bind(
			[] (lua_State* L, Promise::AlwaysHandler* self) -> void {
				Function::Ptr* ptr = (Function::Ptr*)self->userdata().get();

				ScriptingLua::check(L, call(L, **ptr));

				self->clear();
			},
			L, std::placeholders::_1
		);
		Any ud(
			new PromisePair(callback, ref),
			[] (void* ptr) -> void {
				PromisePair* pair = (PromisePair*)ptr;
				pair->first = nullptr;
				pair->second = nullptr;
				delete pair;
			}
		);
		Promise::AlwaysHandler cb(func, ud);

		obj->get()->always(cb);
	}

	return write(L, Index(1));
}

static int Promise___index(lua_State* L) {
	Promise::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "state") == 0) {
		const Enum ret = (Enum)obj->get()->state();

		return write(L, ret);
	} else if (strcmp(field, "value") == 0) {
		const Variant ret = obj->get()->value();

		return Promise_write(L, ret);
	} else {
		return __index(L, field);
	}
}

static int Promise___newindex(lua_State* L) {
	Promise::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	return 0;
}

static void open_Promise(lua_State* L) {
	def(
		L, "Promise",
		LUA_LIB(
			array<luaL_Reg>()
		),
		array(
			luaL_Reg{ "__gc", Promise___gc },
			luaL_Reg{ "__tostring", __tostring<Promise::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "thus", Promise_thus },
			luaL_Reg{ "catch", Promise_catch },
			luaL_Reg{ "finally", Promise_finally },
			luaL_Reg{ nullptr, nullptr }
		),
		Promise___index, Promise___newindex
	);

	getGlobal(L, "Promise");
	setTable(
		L,
		"Pending", (Enum)Promise::PENDING,
		"Resolved", (Enum)Promise::RESOLVED,
		"Rejected", (Enum)Promise::REJECTED
	);
	pop(L);
}

/**< Standard. */

static int waitbox(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	std::string content;
	read<>(L, content);

	if (impl->observer()->promising()) {
		error(L, "Too many pending popups.");

		return 0;
	}

	Promise::Ptr promise = nullptr;
	const int result = Promise_ctor(L, promise, true);

	impl->observer()->waitbox(
		promise,
		content.c_str()
	);

	return result;
}

static int msgbox(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const bool plug = isPlugin(L);

	const int n = getTop(L);
	std::string msg;
	bool withConfirm = false, withDeny = false, withCancel = false;
	std::string confirmTxt, denyTxt, cancelTxt;
	if (plug) {
		if (n >= 4) {
			withConfirm = true;
			withDeny = true;
			withCancel = true;
			read<>(L, msg, confirmTxt, denyTxt, cancelTxt);
		} else if (n == 3) {
			withConfirm = true;
			withDeny = true;
			read<>(L, msg, confirmTxt, denyTxt);
		} else if (n == 2) {
			withConfirm = true;
			read<>(L, msg, confirmTxt);
		} else {
			read<>(L, msg);
		}
		if (withConfirm && confirmTxt.empty())
			confirmTxt = EXECUTABLE_ANY_NAME;
		if (withDeny && denyTxt.empty())
			denyTxt = EXECUTABLE_ANY_NAME;
		if (withCancel && cancelTxt.empty())
			cancelTxt = EXECUTABLE_ANY_NAME;
	} else {
		read<>(L, msg);
	}

	if (impl->observer()->promising()) {
		error(L, "Too many pending popups.");

		return 0;
	}

	Promise::Ptr promise = nullptr;
	const int result = Promise_ctor(L, promise, plug);

	impl->observer()->msgbox(
		promise,
		msg.c_str(),
		withConfirm ? confirmTxt.c_str() : nullptr,
		withDeny ? denyTxt.c_str() : nullptr,
		withCancel ? cancelTxt.c_str() : nullptr
	);

	return result;
}

#if defined BITTY_OS_HTML
static int input(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	std::string prompt;
	std::string default_;
	if (n >= 2)
		read<>(L, prompt, default_);
	else
		read<>(L, prompt);

	if (impl->observer()->promising()) {
		error(L, "Too many pending popups.");

		return 0;
	}

	const char* ret_ = scriptingLuaApiPromisesInput(prompt.c_str(), default_.empty() ? "" : default_.c_str());
	const std::string ret = ret_ ? ret_ : "";
	scriptingLuaApiPromisesFree((void*)ret_);

	return write(L, ret);
}
#else /* BITTY_OS_HTML */
static int input(lua_State* L) {
#if BITTY_MULTITHREAD_ENABLED
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const bool plug = isPlugin(L);

	const int n = getTop(L);
	std::string prompt;
	std::string default_;
	if (n >= 2)
		read<>(L, prompt, default_);
	else
		read<>(L, prompt);

	if (impl->observer()->promising()) {
		error(L, "Too many pending popups.");

		return 0;
	}

	Promise::Ptr promise = nullptr;
	int result = Promise_ctor(L, promise, plug);

	impl->observer()->input(promise, prompt.c_str(), default_.empty() ? nullptr : default_.c_str());

	Variant ret = nullptr;
	Promise::States state = Promise::PENDING;
	promise
		->then(
			Promise::ThenHandler(
				std::bind(
					[] (Variant* ret, Promise::States* state, Promise::ThenHandler* self, Variant arg) -> void {
						*ret = arg;
						*state = Promise::RESOLVED;

						self->clear();
					},
					&ret, &state, std::placeholders::_1, std::placeholders::_2
				)
			)
		)
		->fail(
			Promise::FailHandler(
				std::bind(
					[] (Variant* ret, Promise::States* state, Promise::FailHandler* self, const Variant &) -> void {
						*ret = nullptr;
						*state = Promise::REJECTED;

						self->clear();
					},
					&ret, &state, std::placeholders::_1, std::placeholders::_2
				)
			)
		);

	if (!plug) {
		while (state == Promise::PENDING) {
			constexpr const int STEP = 1;
			impl->sync(STEP / 1000.0);
			DateTime::sleep(STEP);
		}
		impl->activate();

		result = write(L, (const Variant*)&ret);
	}

	return result;
#else /* BITTY_MULTITHREAD_ENABLED */
	error(L, "The \"input(...)\" function is not available.");

	return 0;
#endif /* BITTY_MULTITHREAD_ENABLED */
}
#endif /* BITTY_OS_HTML */

static void open_Standard(lua_State* L) {
	reg(
		L,
		array(
			luaL_Reg{ "waitbox", waitbox }, // Undocumented. Asynchronized.
			luaL_Reg{ "msgbox", msgbox }, // Synchronized for main project, asynchronized for plugin.
			luaL_Reg{ "input", input }, // Synchronized for main project, asynchronized for plugin.
			luaL_Reg{ nullptr, nullptr }
		)
	);
}

/**< Categories. */

void promise(class Executable* exec) {
	// Prepare.
	lua_State* L = (lua_State*)exec->pointer();

	// Promise.
	open_Promise(L);

	// Standard.
	open_Standard(L);
}

}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Libraries
*/

namespace Lua {

namespace Libs {

/**< Platform. */

static int Platform_openFile_Promise(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	const char* title = "Open File";
	const char* filter_ = nullptr;
	Text::Array filter{ "All files (*.*)", "*" };
	std::string default_;
	bool multiselect = false;
	if (n >= 4)
		read<>(L, title, filter_, default_, multiselect);
	else if (n == 3)
		read<>(L, title, filter_, default_);
	else if (n == 2)
		read<>(L, title, filter_);
	else if (n == 1)
		read<>(L, title);

	if (filter_)
		filter = Text::split(filter_, ";");
	Path::diversify(default_);

#if BITTY_MULTITHREAD_ENABLED
	Promise::Ptr promise = nullptr;
	int result = Standard::Promise_ctor(L, promise, false);

	Executable::PromiseHandler handler = [title, default_, filter, multiselect] (Variant* ret) -> bool {
		if (ret)
			*ret = nullptr;

		pfd::opt options = pfd::opt::none;
		if (multiselect)
			options = options | pfd::opt::multiselect;
		pfd::open_file open(
			title,
			default_,
			filter,
			options
		);
		if (open.result().empty() || open.result().front().empty())
			return false;

		if (multiselect) {
			IList::Ptr ret_(List::create());
			for (std::string path : open.result()) {
				Path::uniform(path);
				ret_->add(path);
			}

			if (ret)
				*ret = Object::Ptr(ret_);
		} else {
			std::string ret_ = open.result().front();
			Path::uniform(ret_);

			if (ret)
				*ret = ret_;
		}

		return true;
	};
	impl->observer()->promise(promise, handler);

	Variant ret = nullptr;
	bool finished = false;
	promise
		->then(
			Promise::ThenHandler(
				std::bind(
					[] (Variant* ret, bool* finished, Promise::ThenHandler* self, Variant arg) -> void {
						*ret = arg;
						*finished = true;

						self->clear();
					},
					&ret, &finished, std::placeholders::_1, std::placeholders::_2
				)
			)
		)
		->fail(
			Promise::FailHandler(
				std::bind(
					[] (Variant* ret, bool* finished, Promise::FailHandler* self, const Variant &) -> void {
						*ret = nullptr;
						*finished = true;

						self->clear();
					},
					&ret, &finished, std::placeholders::_1, std::placeholders::_2
				)
			)
		);

	while (!finished) {
		constexpr const int STEP = 1;
		impl->sync(STEP / 1000.0);
		DateTime::sleep(STEP);
	}
	impl->activate();

	result = write(L, (const Variant*)&ret);

	return result;
#else /* BITTY_MULTITHREAD_ENABLED */
	(void)impl;

	pfd::opt options = pfd::opt::none;
	if (multiselect)
		options = options | pfd::opt::multiselect;
	pfd::open_file open(
		title,
		default_,
		filter,
		options
	);
	if (open.result().empty() || open.result().front().empty())
		return write(L, nullptr);

	if (multiselect) {
		Text::Array ret;
		for (std::string path : open.result()) {
			Path::uniform(path);
			ret.push_back(path);
		}

		return write(L, ret);
	} else {
		std::string ret = open.result().front();
		Path::uniform(ret);

		return write(L, ret);
	}
#endif /* BITTY_MULTITHREAD_ENABLED */
}

static int Platform_saveFile_Promise(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	const char* title = "Save File";
	const char* filter_ = nullptr;
	std::string default_;
	Text::Array filter{ "All files (*.*)", "*" };
	if (n >= 3)
		read<>(L, title, filter_, default_);
	else if (n == 2)
		read<>(L, title, filter_);
	else if (n == 1)
		read<>(L, title);

	if (filter_)
		filter = Text::split(filter_, ";");
	Path::diversify(default_);

#if BITTY_MULTITHREAD_ENABLED
	Promise::Ptr promise = nullptr;
	int result = Standard::Promise_ctor(L, promise, false);

	Executable::PromiseHandler handler = [title, default_, filter] (Variant* ret) -> bool {
		if (ret)
			*ret = nullptr;

		pfd::save_file save(
			title,
			default_,
			filter
		);
		if (save.result().empty())
			return false;

		std::string ret_ = save.result();
		Path::uniform(ret_);

		if (ret)
			*ret = ret_;

		return true;
	};
	impl->observer()->promise(promise, handler);

	Variant ret = nullptr;
	bool finished = false;
	promise
		->then(
			Promise::ThenHandler(
				std::bind(
					[] (Variant* ret, bool* finished, Promise::ThenHandler* self, Variant arg) -> void {
						*ret = arg;
						*finished = true;

						self->clear();
					},
					&ret, &finished, std::placeholders::_1, std::placeholders::_2
				)
			)
		)
		->fail(
			Promise::FailHandler(
				std::bind(
					[] (Variant* ret, bool* finished, Promise::FailHandler* self, const Variant &) -> void {
						*ret = nullptr;
						*finished = true;

						self->clear();
					},
					&ret, &finished, std::placeholders::_1, std::placeholders::_2
				)
			)
		);

	while (!finished) {
		constexpr const int STEP = 1;
		impl->sync(STEP / 1000.0);
		DateTime::sleep(STEP);
	}
	impl->activate();

	result = write(L, (const Variant*)&ret);

	return result;
#else /* BITTY_MULTITHREAD_ENABLED */
	(void)impl;

	pfd::save_file save(
		title,
		default_,
		filter
	);
	if (save.result().empty())
		return write(L, nullptr);

	std::string ret = save.result();
	Path::uniform(ret);

	return write(L, ret);
#endif /* BITTY_MULTITHREAD_ENABLED */
}

static int Platform_selectDirectory_Promise(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	const char* title = "Select Directory";
	std::string default_;
	if (n >= 2)
		read<>(L, title, default_);
	else if (n == 1)
		read<>(L, title);

	Path::diversify(default_);

#if BITTY_MULTITHREAD_ENABLED
	Promise::Ptr promise = nullptr;
	int result = Standard::Promise_ctor(L, promise, false);

	Executable::PromiseHandler handler = [title, default_] (Variant* ret) -> bool {
		if (ret)
			*ret = nullptr;

		pfd::select_folder open(
			title,
			default_
		);
		if (open.result().empty())
			return false;

		std::string ret_ = open.result();
		Path::uniform(ret_);

		if (ret)
			*ret = ret_;

		return true;
	};
	impl->observer()->promise(promise, handler);

	Variant ret = nullptr;
	bool finished = false;
	promise
		->then(
			Promise::ThenHandler(
				std::bind(
					[] (Variant* ret, bool* finished, Promise::ThenHandler* self, Variant arg) -> void {
						*ret = arg;
						*finished = true;

						self->clear();
					},
					&ret, &finished, std::placeholders::_1, std::placeholders::_2
				)
			)
		)
		->fail(
			Promise::FailHandler(
				std::bind(
					[] (Variant* ret, bool* finished, Promise::FailHandler* self, const Variant &) -> void {
						*ret = nullptr;
						*finished = true;

						self->clear();
					},
					&ret, &finished, std::placeholders::_1, std::placeholders::_2
				)
			)
		);

	while (!finished) {
		constexpr const int STEP = 1;
		impl->sync(STEP / 1000.0);
		DateTime::sleep(STEP);
	}
	impl->activate();

	result = write(L, (const Variant*)&ret);

	return result;
#else /* BITTY_MULTITHREAD_ENABLED */
	(void)impl;

	pfd::select_folder open(
		title,
		default_
	);
	if (open.result().empty())
		return write(L, nullptr);

	std::string ret = open.result();
	Path::uniform(ret);

	return write(L, ret);
#endif /* BITTY_MULTITHREAD_ENABLED */
}

static int Platform_openFile(lua_State* L) {
	const int n = getTop(L);
	const char* title = "Open File";
	const char* filter_ = nullptr;
	Text::Array filter{ "All files (*.*)", "*" };
	std::string default_;
	bool multiselect = false;
	if (n >= 4)
		read<>(L, title, filter_, default_, multiselect);
	else if (n == 3)
		read<>(L, title, filter_, default_);
	else if (n == 2)
		read<>(L, title, filter_);
	else if (n == 1)
		read<>(L, title);

	if (filter_)
		filter = Text::split(filter_, ";");
	Path::diversify(default_);

	pfd::opt options = pfd::opt::none;
	if (multiselect)
		options = options | pfd::opt::multiselect;
	pfd::open_file open(
		title,
		default_,
		filter,
		options
	);
	if (open.result().empty() || open.result().front().empty())
		return write(L, nullptr);

	if (multiselect) {
		Text::Array ret;
		for (std::string path : open.result()) {
			Path::uniform(path);
			ret.push_back(path);
		}

		return write(L, ret);
	} else {
		std::string ret = open.result().front();
		Path::uniform(ret);

		return write(L, ret);
	}
}

static int Platform_saveFile(lua_State* L) {
	const int n = getTop(L);
	const char* title = "Save File";
	const char* filter_ = nullptr;
	std::string default_;
	Text::Array filter{ "All files (*.*)", "*" };
	if (n >= 3)
		read<>(L, title, filter_, default_);
	else if (n == 2)
		read<>(L, title, filter_);
	else if (n == 1)
		read<>(L, title);

	if (filter_)
		filter = Text::split(filter_, ";");
	Path::diversify(default_);

	pfd::save_file save(
		title,
		default_,
		filter
	);
	if (save.result().empty())
		return write(L, nullptr);

	std::string ret = save.result();
	Path::uniform(ret);

	return write(L, ret);
}

static int Platform_selectDirectory(lua_State* L) {
	const int n = getTop(L);
	const char* title = "Select Directory";
	std::string default_;
	if (n >= 2)
		read<>(L, title, default_);
	else if (n == 1)
		read<>(L, title);

	Path::diversify(default_);

	pfd::select_folder open(
		title,
		default_
	);
	if (open.result().empty())
		return write(L, nullptr);

	std::string ret = open.result();
	Path::uniform(ret);

	return write(L, ret);
}

static int Platform_notify(lua_State* L) {
	const int n = getTop(L);
	const char* title = BITTY_NAME;
	std::string message;
	std::string icon = "info";
	if (n >= 3)
		read<>(L, title, message, icon);
	else if (n == 2)
		read<>(L, title, message);
	else if (n == 1)
		read<>(L, title);

	pfd::icon icon_ = pfd::icon::info;
	if (icon == "warning")
		icon_ = pfd::icon::warning;
	else if (icon == "error")
		icon_ = pfd::icon::error;
	else if (icon == "question")
		icon_ = pfd::icon::question;
	pfd::notify notify(title, message, icon_);

	return 0;
}

static void open_Platform(lua_State* L) {
	getGlobal(L, "Platform");
	if (isPlugin(L)) {
		setTable(
			L,
			"openFile", Platform_openFile, // Synchronized.
			"saveFile", Platform_saveFile, // Synchronized.
			"selectDirectory", Platform_selectDirectory, // Synchronized.
			"notify", Platform_notify // Undocumented. Synchronized.
		);
	} else {
		setTable(
			L,
			"openFile", Platform_openFile_Promise, // Synchronized.
			"saveFile", Platform_saveFile_Promise, // Synchronized.
			"selectDirectory", Platform_selectDirectory_Promise, // Synchronized.
			"notify", Platform_notify // Undocumented. Synchronized.
		);
	}
	pop(L);
}

/**< Web. */

#if BITTY_WEB_ENABLED

static int fetch(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	std::string url;
	read<>(L, url);
	Variant options = nullptr;
	if (n >= 2) {
		if (isUserdata(L, 2)) {
			Json::Ptr* json = nullptr;
			read<2>(L, json);

			if (json && *json)
				(*json)->toAny(options);
		} else if (isTable(L, 2)) {
			read<2>(L, &options);
		}
	}

	Fetch::Ptr web(
		Fetch::create(),
		[] (Fetch* web) -> void {
			Fetch::destroy(web);
		}
	);
	web->open();

	Promise::Ptr ret(
		Promise::create(),
		std::bind(
			[impl] (Promise* promise, Fetch::Ptr web) -> void {
				web->close();

				impl->removeUpdatable(web.get());
				impl->removeUpdatable(promise);

				promise->clear();

				Promise::destroy(promise);
			},
			std::placeholders::_1, web
		)
	);

	impl->addUpdatable(ret.get());
	impl->addUpdatable(web.get());

	web->callback(
		Fetch::RespondedHandler(
			Fetch::RespondedHandler::Callback(
				std::bind(
					[] (Promise::WeakPtr promise, Fetch* web, Fetch::RespondedHandler* self, const Byte* buf, size_t len) -> void {
						Variant rsp = nullptr;
						const Fetch::DataTypes type = web->dataType();
						switch (type) {
						case Fetch::BYTES: {
								std::string str;
								str.assign((const char*)buf, len);
								Bytes::Ptr bytes(Bytes::create());
								bytes->writeString(str);
								rsp = Variant(bytes);
							}

							break;
						case Fetch::JSON: {
								std::string str;
								str.assign((const char*)buf, len);
								Json::Ptr json(Json::create());
								if (json->fromString(str))
									rsp = Variant(json);
								else
									rsp = Variant(str);
							}

							break;
						case Fetch::STRING: // Fall through.
						default: {
								std::string str;
								str.assign((const char*)buf, len);
								rsp = Variant(str);
							}

							break;
						}
						if (!promise.expired()) {
							Promise::Ptr ptr = promise.lock();
							ptr->resolve(rsp);
						}

						self->clear();

						web->callback(Fetch::RespondedHandler());
						web->callback(Fetch::ErrorHandler());
					},
					Promise::WeakPtr(ret), web.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3
				)
			)
		)
	);
	web->callback(
		Fetch::ErrorHandler(
			Fetch::ErrorHandler::Callback(
				std::bind(
					[] (Promise::WeakPtr promise, Fetch* web, Fetch::ErrorHandler* self, const char* error) -> void {
						if (!promise.expired()) {
							Promise::Ptr ptr = promise.lock();
							ptr->reject(error);
						}

						self->clear();

						web->callback(Fetch::RespondedHandler());
						web->callback(Fetch::ErrorHandler());
					},
					Promise::WeakPtr(ret), web.get(), std::placeholders::_1, std::placeholders::_2
				)
			)
		)
	);
	web->url(url.c_str());
	web->options(options);
	web->perform();

	return write(L, &ret);
}

static void open_Web(lua_State* L) {
	reg(
		L,
		array(
			luaL_Reg{ "fetch", fetch }, // Asynchronized.
			luaL_Reg{ nullptr, nullptr }
		)
	);
}

#else /* BITTY_WEB_ENABLED */

static void open_Web(lua_State*) {
	// Do nothing.
}

#endif /* BITTY_WEB_ENABLED */

/**< Categories. */

void promise(class Executable* exec) {
	// Prepare.
	lua_State* L = (lua_State*)exec->pointer();

	// Platform.
	open_Platform(L);

	// Web.
	open_Web(L);
}

}

}

/* ===========================================================================} */
