/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "luaxx.h"
extern "C" {
#	include "../lib/lua/src/ldo.h"
}
#include <assert.h>

/*
** {===========================================================================
** C++ wrapper around Lua
*/

namespace Lua {

Index::Index() {
}

Index::Index(int idx) : _index(idx) {
}

Index &Index::operator ++ (void) {
	++_index;

	return *this;
}

Index Index::operator ++ (int _) {
	(void)_;

	return Index(_index++);
}

Index &Index::operator += (int val) {
	_index += val;

	return *this;
}

Index Index::operator + (int val) const {
	return Index(_index + val);
}

Index &Index::operator -- (void) {
	--_index;

	return *this;
}

Index Index::operator -- (int _) {
	(void)_;

	return Index(_index--);
}

Index &Index::operator -= (int val) {
	_index -= val;

	return *this;
}

Index Index::operator - (int val) const {
	return Index(_index - val);
}

Index::operator int (void) const {
	return _index;
}

Placeholder::Placeholder() {
}

Placeholder::operator int (void) const {
	return _index;
}

Ref::Ref() {
}

Ref::Ref(const Ref &other) {
	_handle = other._handle;
}

Ref::~Ref() {
	if (_L) {
		luaL_unref(_L, LUA_REGISTRYINDEX, _handle);

		_handle = 0;
		_L = nullptr;
	}
}

Ref &Ref::operator = (const Ref &other) {
	_handle = other._handle;

	return *this;
}

Ref::operator int (void) const {
	return _handle;
}

bool Ref::valid(void) const {
	return _handle != LUA_NOREF;
}

Ref::Ref(lua_State* L) {
	_handle = luaL_ref(L, LUA_REGISTRYINDEX);
}

Function::Function() {
}

Function::Function(const Function &other) {
	_handle = other._handle;
}

Function::~Function() {
	if (_L) {
		luaL_unref(_L, LUA_REGISTRYINDEX, _handle);

		_handle = 0;
		_L = nullptr;
	}
}

Function &Function::operator = (const Function &other) {
	_handle = other._handle;

	return *this;
}

Function::operator int (void) const {
	return _handle;
}

bool Function::valid(void) const {
	return _handle != LUA_NOREF;
}

Function::Function(lua_State* L) {
	_handle = luaL_ref(L, LUA_REGISTRYINDEX);
}

lua_State* create(lua_Alloc f, void* ud) {
	lua_State* L = lua_newstate(f, ud);
	lua_gc(L, LUA_GCGEN, 0, 0);

	return L;
}

void destroy(lua_State* L) {
	lua_close(L);
}

void* userdata(lua_State* L) {
	return G(L)->ud;
}

int ref(lua_State* L) {
	return luaL_ref(L, LUA_REGISTRYINDEX);
}

void unref(lua_State* L, int ref) {
	luaL_unref(L, LUA_REGISTRYINDEX, ref);
}

int refed(lua_State* L, int ref) {
	return lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
}

int absIndex(lua_State* L, int idx) {
	return lua_absindex(L, idx);
}

int getTop(lua_State* L) {
	return lua_gettop(L);
}

void setTop(lua_State* L, int idx) {
	lua_settop(L, idx);
}

void pop(lua_State* L, int count) {
	lua_pop(L, count);
}

void push(lua_State* L, int idx) {
	lua_pushvalue(L, idx);
}

int get(lua_State* L, int idx) {
	return lua_rawget(L, idx);
}

int get(lua_State* L, int idx, int n) {
	return lua_rawgeti(L, idx, n);
}

void set(lua_State* L, int idx) {
	lua_rawset(L, idx);
}

void set(lua_State* L, int idx, int n) {
	lua_rawseti(L, idx, n);
}

void rotate(lua_State* L, int idx, int n) {
	lua_rotate(L, idx, n);
}

void copy(lua_State* L, int fromidx, int toidx) {
	lua_copy(L, fromidx, toidx);
}

int checkStack(lua_State* L, int n) {
	return lua_checkstack(L, n);
}

void xmove(lua_State* from, lua_State* to, int n) {
	lua_xmove(from, to, n);
}

lua_Unsigned len(lua_State* L, int idx) {
	return lua_rawlen(L, idx);
}

int next(lua_State* L, int idx) {
	return lua_next(L, idx);
}

int getMetaOf(lua_State* L, int idx) {
	return lua_getmetatable(L, idx);
}

int setMetaOf(lua_State* L, const char* meta) {
	luaL_getmetatable(L, meta);

	return lua_setmetatable(L, -2);
}

int newMeta(lua_State* L, const char* name) {
	return luaL_newmetatable(L, name);
}

int newTable(lua_State* L) {
	lua_newtable(L);

	return 1;
}

int newTable(lua_State* L, int capacity) {
	lua_createtable(L, capacity, 0);

	return 1;
}

int typeOf(lua_State* L, int idx) {
	return lua_type(L, idx);
}

const char* typeNameOf(lua_State* L, int idx, bool detail) {
	const int y = lua_type(L, idx);
	if (y == LUA_TUSERDATA && detail) {
		const char* str = nullptr;
		if (getMetaOf(L, idx)) {
			write(L, "__name");
			get(L, -2);
			read(L, str, Index(-1));
			pop(L, 2);
		}

		if (str)
			return str;
	}

	return lua_typename(L, y);
}

const char* toString(lua_State* L, int idx, size_t* len) {
	return luaL_tolstring(L, idx, len);
}

bool isNil(lua_State* L, int idx) {
	return !!lua_isnil(L, idx);
}

bool isNoneOrNil(lua_State* L, int idx) {
	return !!lua_isnoneornil(L, idx);
}

bool isThread(lua_State* L, int idx) {
	return !!lua_isthread(L, idx);
}

bool isBoolean(lua_State* L, int idx) {
	return !!lua_isboolean(L, idx);
}

bool isInteger(lua_State* L, int idx) {
	return !!lua_isinteger(L, idx);
}

bool isNumber(lua_State* L, int idx) {
	return !!lua_isnumber(L, idx);
}

bool isString(lua_State* L, int idx) {
	return !!lua_isstring(L, idx);
}

bool isTable(lua_State* L, int idx) {
	return !!lua_istable(L, idx);
}

bool isArray(lua_State* L, int idx) {
	if (typeOf(L, idx) != LUA_TTABLE)
		return false;

	int last = 0;
	write(L, nullptr);
	if (idx < 0)
		--idx;
	while (next(L, idx)) {
		if (isNumber(L, -2)) {
			int current = -1;
			read(L, current, Index(-2));
			if (current == last + 1) { // 1-based.
				last = current;
			} else {
				pop(L, 2);

				return false;
			}
		} else {
			pop(L, 2);

			return false;
		}

		pop(L);
	}

	return true;
};

bool isUserdata(lua_State* L, int idx) {
	return !!lua_isuserdata(L, idx);
}

bool isCFunction(lua_State* L, int idx) {
	return !!lua_iscfunction(L, idx);
}

bool isFunction(lua_State* L, int idx) {
	return !!lua_isfunction(L, idx);
}

bool isLightuserdata(lua_State* L, int idx) {
	return !!lua_islightuserdata(L, idx);
}

void optional(lua_State* L, signed char &ret, Index idx, signed char d) {
	ret = (signed char)luaL_optinteger(L, idx, d);
}

void optional(lua_State* L, unsigned char &ret, Index idx, unsigned char d) {
	ret = (unsigned char)luaL_optinteger(L, idx, d);
}

void optional(lua_State* L, short &ret, Index idx, short d) {
	ret = (short)luaL_optinteger(L, idx, d);
}

void optional(lua_State* L, unsigned short &ret, Index idx, unsigned short d) {
	ret = (unsigned short)luaL_optinteger(L, idx, d);
}

void optional(lua_State* L, int &ret, Index idx, int d) {
	ret = (int)luaL_optinteger(L, idx, d);
}

void optional(lua_State* L, unsigned int &ret, Index idx, unsigned int d) {
	ret = (unsigned int)luaL_optinteger(L, idx, d);
}

void optional(lua_State* L, long &ret, Index idx, long d) {
	ret = (long)luaL_optinteger(L, idx, d);
}

void optional(lua_State* L, unsigned long &ret, Index idx, unsigned long d) {
	ret = (unsigned long)luaL_optinteger(L, idx, d);
}

void optional(lua_State* L, long long &ret, Index idx, long long d) {
	ret = (long long)luaL_optinteger(L, idx, d);
}

void optional(lua_State* L, unsigned long long &ret, Index idx, unsigned long long d) {
	ret = (unsigned long long)luaL_optinteger(L, idx, d);
}

void optional(lua_State* L, float &ret, Index idx, float d) {
	ret = (float)luaL_optnumber(L, idx, d);
}

void optional(lua_State* L, double &ret, Index idx, double d) {
	ret = (double)luaL_optnumber(L, idx, d);
}

void optional(lua_State* L, const char* &ret, Index idx, const char* d) {
	ret = luaL_optstring(L, idx, d);
}

void optional(lua_State* L, std::string &ret, Index idx, const std::string &d) {
	ret = luaL_optstring(L, idx, d.c_str());
}

void check(lua_State* L, Placeholder &ret, Index idx) {
	luaL_checkany(L, idx);

	ret._index = idx;
}

void check(lua_State* L, Ref &ret, Index idx) {
	lua_pushvalue(L, idx);
	ret = Ref(L);
	lua_pop(L, 1);

	ret._L = L;
}

void check(lua_State* L, Ref::Ptr &ret, Index idx) {
	lua_pushvalue(L, idx);
	ret = Ref::Ptr(new Ref(L));
	lua_pop(L, 1);

	ret->_L = L;
}

void check(lua_State* L, bool &ret, Index idx) {
	if (lua_isboolean(L, idx))
		ret = !!lua_toboolean(L, idx);
	else
		luaL_error(L, "Boolean expected.");
}

void check(lua_State* L, signed char &ret, Index idx) {
	ret = (signed char)luaL_checkinteger(L, idx);
}

void check(lua_State* L, unsigned char &ret, Index idx) {
	ret = (unsigned char)luaL_checkinteger(L, idx);
}

void check(lua_State* L, short &ret, Index idx) {
	ret = (short)luaL_checkinteger(L, idx);
}

void check(lua_State* L, unsigned short &ret, Index idx) {
	ret = (unsigned short)luaL_checkinteger(L, idx);
}

void check(lua_State* L, int &ret, Index idx) {
	ret = (int)luaL_checkinteger(L, idx);
}

void check(lua_State* L, unsigned int &ret, Index idx) {
	ret = (unsigned int)luaL_checkinteger(L, idx);
}

void check(lua_State* L, long &ret, Index idx) {
	ret = (long)luaL_checkinteger(L, idx);
}

void check(lua_State* L, unsigned long &ret, Index idx) {
	ret = (unsigned long)luaL_checkinteger(L, idx);
}

void check(lua_State* L, long long &ret, Index idx) {
	ret = (long long)luaL_checkinteger(L, idx);
}

void check(lua_State* L, unsigned long long &ret, Index idx) {
	ret = (unsigned long long)luaL_checkinteger(L, idx);
}

void check(lua_State* L, float &ret, Index idx) {
	ret = (float)luaL_checknumber(L, idx);
}

void check(lua_State* L, double &ret, Index idx) {
	ret = (double)luaL_checknumber(L, idx);
}

void check(lua_State* L, const char* &ret, Index idx) {
	ret = nullptr;

	ret = luaL_checkstring(L, idx);
}

void check(lua_State* L, std::string &ret, Index idx) {
	ret.clear();

	const char* str = luaL_checkstring(L, idx);
	if (str)
		ret = str;
}

void check(lua_State* L, lua_CFunction &ret, Index idx) {
	ret = nullptr;

	if (lua_iscfunction(L, idx)) {
		ret = lua_tocfunction(L, idx);
	} else {
		luaL_error(L, "CFunction expected.");
	}
}

void check(lua_State* L, Function &ret, Index idx) {
	if (lua_isfunction(L, idx)) {
		lua_pushvalue(L, idx);
		ret = Function(L);
		lua_pop(L, 1);

		ret._L = L;
	} else {
		luaL_error(L, "Function expected.");
	}
}

void check(lua_State* L, Function::Ptr &ret, Index idx) {
	ret = nullptr;

	if (lua_isfunction(L, idx)) {
		lua_pushvalue(L, idx);
		ret = Function::Ptr(new Function(L));
		lua_pop(L, 1);

		ret->_L = L;
	} else {
		luaL_error(L, "Function expected.");
	}
}

void check(lua_State* L, void* &ret, Index idx, const char* type) {
	ret = nullptr;

	if (type) {
		ret = luaL_checkudata(L, idx, type);
	} else {
		if (lua_isuserdata(L, idx))
			ret = lua_touserdata(L, idx);
		else
			luaL_error(L, "Userdata expected.");
	}
}

void check(lua_State* L, LightUserdata &ret, Index idx) {
	ret.data = nullptr;

	if (lua_islightuserdata(L, idx))
		ret.data = lua_touserdata(L, idx);
	else
		luaL_error(L, "LightUserdata expected.");
}

void read(lua_State* L, Placeholder &ret, Index idx) {
	(void)L;

	ret._index = idx;
}

void read(lua_State* L, Ref &ret, Index idx) {
	lua_pushvalue(L, idx);
	ret = Ref(L);

	ret._L = L;
}

void read(lua_State* L, Ref::Ptr &ret, Index idx) {
	ret = nullptr;

	lua_pushvalue(L, idx);
	ret = Ref::Ptr(new Ref(L));

	ret->_L = L;
}

void read(lua_State* L, bool &ret, Index idx) {
	ret = !!lua_toboolean(L, idx);
}

template<typename Val> static void read_(lua_State* L, Val &ret, Index idx) {
	int isnum = 0;
	ret = (Val)lua_tointegerx(L, idx, &isnum);
	if (!isnum)
		ret = (Val)lua_tonumberx(L, idx, &isnum);
}

void read(lua_State* L, signed char &ret, Index idx) {
	read_(L, ret, idx);
}

void read(lua_State* L, unsigned char &ret, Index idx) {
	read_(L, ret, idx);
}

void read(lua_State* L, short &ret, Index idx) {
	read_(L, ret, idx);
}

void read(lua_State* L, unsigned short &ret, Index idx) {
	read_(L, ret, idx);
}

void read(lua_State* L, int &ret, Index idx) {
	read_(L, ret, idx);
}

void read(lua_State* L, unsigned int &ret, Index idx) {
	read_(L, ret, idx);
}

void read(lua_State* L, long &ret, Index idx) {
	read_(L, ret, idx);
}

void read(lua_State* L, unsigned long &ret, Index idx) {
	read_(L, ret, idx);
}

void read(lua_State* L, long long &ret, Index idx) {
	read_(L, ret, idx);
}

void read(lua_State* L, unsigned long long &ret, Index idx) {
	read_(L, ret, idx);
}

void read(lua_State* L, float &ret, Index idx) {
	ret = (float)lua_tonumber(L, idx);
}

void read(lua_State* L, double &ret, Index idx) {
	ret = (double)lua_tonumber(L, idx);
}

void read(lua_State* L, const char* &ret, Index idx) {
	ret = nullptr;

	ret = lua_tostring(L, idx);
}

void read(lua_State* L, std::string &ret, Index idx) {
	ret.clear();

	const char* str = lua_tostring(L, idx);
	if (str)
		ret = str;
}

void read(lua_State* L, lua_CFunction &ret, Index idx) {
	ret = lua_tocfunction(L, idx);
}

void read(lua_State* L, Function &ret, Index idx) {
	if (lua_isfunction(L, idx)) {
		lua_pushvalue(L, idx);
		ret = Function(L);

		ret._L = L;
	}
}

void read(lua_State* L, Function::Ptr &ret, Index idx) {
	ret = nullptr;

	if (lua_isfunction(L, idx)) {
		lua_pushvalue(L, idx);
		ret = Function::Ptr(new Function(L));

		ret->_L = L;
	}
}

void read(lua_State* L, void* &ret, Index idx, const char* type) {
	if (type)
		ret = luaL_testudata(L, idx, type);
	else
		ret = lua_touserdata(L, idx);
}

void read(lua_State* L, LightUserdata &ret, Index idx) {
	ret.data = lua_touserdata(L, idx);
}

void read(lua_State* L, lua_State* &ret, Index idx) {
	ret = lua_tothread(L, idx);
}

int write(lua_State* L, const Index &val) {
	lua_pushvalue(L, val);

	return 1;
}

int write(lua_State* L, const Ref &val) {
	lua_rawgeti(L, LUA_REGISTRYINDEX, val);

	return 1;
}

int write(lua_State* L, std::nullptr_t) {
	lua_pushnil(L);

	return 1;
}

int write(lua_State* L, bool val) {
	lua_pushboolean(L, val ? 1 : 0);

	return 1;
}

int write(lua_State* L, signed char val) {
	lua_pushinteger(L, val);

	return 1;
}

int write(lua_State* L, unsigned char val) {
	lua_pushinteger(L, val);

	return 1;
}

int write(lua_State* L, short val) {
	lua_pushinteger(L, val);

	return 1;
}

int write(lua_State* L, unsigned short val) {
	lua_pushinteger(L, val);

	return 1;
}

int write(lua_State* L, int val) {
	lua_pushinteger(L, val);

	return 1;
}

int write(lua_State* L, unsigned int val) {
	lua_pushinteger(L, val);

	return 1;
}

int write(lua_State* L, long val) {
	lua_pushinteger(L, val);

	return 1;
}

int write(lua_State* L, unsigned long val) {
	lua_pushinteger(L, val);

	return 1;
}

int write(lua_State* L, long long val) {
	lua_pushinteger(L, val);

	return 1;
}

int write(lua_State* L, unsigned long long val) {
	lua_pushinteger(L, val);

	return 1;
}

int write(lua_State* L, float val) {
	lua_pushnumber(L, val);

	return 1;
}

int write(lua_State* L, double val) {
	lua_pushnumber(L, val);

	return 1;
}

int write(lua_State* L, const char* val) {
	lua_pushstring(L, val);

	return 1;
}

int write(lua_State* L, const std::string &val) {
	lua_pushstring(L, val.c_str());

	return 1;
}

int write(lua_State* L, std::string &val) {
	lua_pushstring(L, val.c_str());

	return 1;
}

int write(lua_State* L, lua_CFunction val) {
	lua_pushcfunction(L, val);

	return 1;
}

int write(lua_State* L, const Function &val) {
	lua_rawgeti(L, LUA_REGISTRYINDEX, val);

	return 1;
}

int write(lua_State* L, Function &val) {
	lua_rawgeti(L, LUA_REGISTRYINDEX, val);

	return 1;
}

int write(lua_State* L, void* &val, size_t size) {
	val = lua_newuserdata(L, size);

	return 1;
}

int write(lua_State* L, const LightUserdata &val) {
	lua_pushlightuserdata(L, val.data);

	return 1;
}

int function(lua_State* L, const char* func) {
	if (!func)
		return 0;

	return lua_getglobal(L, func);
}

int function(lua_State* L, const Function &func) {
	return lua_rawgeti(L, LUA_REGISTRYINDEX, func);
}

int invoke(lua_State* L, int argc, int retc) {
	return lua_pcall(L, argc, retc, 0);
}

int invoke(lua_State* L, ProtectedFunction func, void* ud) {
	return luaD_rawrunprotected(L, func, ud);
}

int end(lua_State* L) {
	const int n = lua_gettop(L);
	lua_pop(L, n);

	return n;
}

int call(lua_State* L, const char* func) {
	function(L, func);
	const int result = invoke(L, 0, 0);
	if (result == LUA_OK)
		end(L);

	return result;
}

int call(lua_State* L, const Function &func) {
	function(L, func);
	const int result = invoke(L, 0, 0);
	if (result == LUA_OK)
		end(L);

	return result;
}

int setMeta(lua_State* L, const luaL_Reg* meta, const luaL_Reg* methods, lua_CFunction index, lua_CFunction newindex) {
	newTable(L);
	if (methods)
		setFunctions(L, methods, 0);

	push(L, -2);
	if (meta)
		setFunctions(L, meta, 0);

	write(L, "__index");
	push(L, -3);
	set(L, -3);

	write(L, "__metatable");
	push(L, -3);
	set(L, -3);

	if (index) {
		write(L, "__index");
		write(L, index);
		set(L, -3);
	}

	if (newindex) {
		write(L, "__newindex");
		write(L, newindex);
		set(L, -3);
	}

	pop(L);

	return 1;
}

int readMeta(lua_State* L, int idx, const char* field) {
	return luaL_getmetafield(L, idx, field);
}

int readTable(lua_State* L, int idx) {
	return lua_gettable(L, idx);
}

int readTable(lua_State* L, int idx, const char* field) {
	return lua_getfield(L, idx, field);
}

int readTable(lua_State* L, int idx, lua_Integer n) {
	return lua_geti(L, idx, n);
}

void writeTable(lua_State* L, int idx) {
	lua_settable(L, idx);
}

void writeTable(lua_State* L, int idx, const char* field) {
	lua_setfield(L, idx, field);
}

void writeTable(lua_State* L, int idx, lua_Integer n) {
	lua_seti(L, idx, n);
}

void setFunctions(lua_State* L, const luaL_Reg* l, bool nup) {
	luaL_setfuncs(L, l, nup ? 1 : 0);
}

int getGlobal(lua_State* L, const char* name) {
	return lua_getglobal(L, name);
}

void setGlobal(lua_State* L, const char* name) {
	lua_setglobal(L, name);
}

const char* getLocal(lua_State* L, const lua_Debug* ar, int n) {
	return lua_getlocal(L, ar, n);
}

const char* setLocal(lua_State* L, const lua_Debug* ar, int n) {
	return lua_setlocal(L, ar, n);
}

const char* getUpvalue(lua_State* L, int funcindex, int n) {
	return lua_getupvalue(L, funcindex, n);
}

const char* setUpvalue(lua_State* L, int funcindex, int n) {
	return lua_setupvalue(L, funcindex, n);
}

void* upvalueId(lua_State* L, int fidx, int n) {
	return lua_upvalueid(L, fidx, n);
}

void upvalueJoin(lua_State* L, int fidx0, int n0, int fidx1, int n1) {
	lua_upvaluejoin(L, fidx0, n0, fidx1, n1);
}

void reg(lua_State* L, const char* name, lua_CFunction function) {
	lua_register(L, name, function);
}

void reg(lua_State* L, const luaL_Reg* functions) {
	for (const luaL_Reg* lib = functions; lib->func; ++lib)
		reg(L, lib->name, lib->func);
}

void req(lua_State* L, const char* name, lua_CFunction module) {
	luaL_requiref(L, name, module, 1);
	lua_pop(L, 1);
}

void req(lua_State* L, const luaL_Reg* modules) {
	for (const luaL_Reg* lib = modules; lib->func; ++lib)
		req(L, lib->name, lib->func);
}

bool def(lua_State* L, const char* name, lua_CFunction ctor, const luaL_Reg* meta, const luaL_Reg* methods, lua_CFunction index, lua_CFunction newindex) {
	assert(name);

	bool result = false;

	if (ctor)
		req(L, array(luaL_Reg{ name, ctor }, luaL_Reg{ nullptr, nullptr }));

	if (meta || methods || index || newindex) {
		if (newMeta(L, name)) {
			result = true;
			setMeta(L, meta, methods, index, newindex);
			pop(L);
		}
		pop(L);
	}

	return result;
}

void lib(lua_State* L, const luaL_Reg* functions, int size) {
	lua_createtable(L, 0, size > 0 ? size - 1 : 0);
	if (functions)
		luaL_setfuncs(L, functions, 0);
}

int __index(lua_State* L, const char* field) {
	if (getMetaOf(L)) {
		int result = getTable(L, field);

		pop(L);

		if (!result) {
			pop(L);

			if (readMeta(L, 1, "__metatable")) {
				result = getTable(L, field);

				pop(L);
			}
		}

		return result;
	}

	return 0;
}

bool __newindex(lua_State* L, const char* field, int valIdx) {
	if (getMetaOf(L)) {
		if (readMeta(L, 1, "__metatable")) {
			push(L, valIdx);
			setTable(L, field);

			pop(L, 2);

			return true;
		}

		pop(L);
	}

	return false;
}

int getStack(lua_State* L, int level, lua_Debug* ar) {
	return lua_getstack(L, level, ar);
}

int getInfo(lua_State* L, const char* what, lua_Debug* ar) {
	return lua_getinfo(L, what, ar);
}

void setHook(lua_State* L, lua_Hook func, int mask, int count) {
	lua_sethook(L, func, mask, count);
}

lua_Hook getHook(lua_State* L) {
	return lua_gethook(L);
}

int getHookMask(lua_State* L) {
	return lua_gethookmask(L);
}

int getHookCount(lua_State* L) {
	return lua_gethookcount(L);
}

void traceback(lua_State* L, lua_State* L1, const char* msg, int level) {
	luaL_traceback(L, L1, msg, level);
}

int gc(lua_State* L) {
	return lua_gc(L, LUA_GCCOLLECT);
}

int gc(lua_State* L, int opt) {
	return lua_gc(L, opt);
}

int gc(lua_State* L, int opt, int arg0) {
	return lua_gc(L, opt, arg0);
}

int gc(lua_State* L, int opt, int arg0, int arg1) {
	return lua_gc(L, opt, arg0, arg1);
}

int gc(lua_State* L, int opt, int arg0, int arg1, int arg2) {
	return lua_gc(L, opt, arg0, arg1, arg2);
}

int error(lua_State* L, const char* msg) {
	return luaL_error(L, msg);
}

void setLoader(lua_State* L, lua_CFunction loader) {
	lua_getglobal(L, LUA_LOADLIBNAME);
	if (lua_istable(L, -1)) {
		lua_getfield(L, -1, "searchers");
		if (lua_istable(L, -1)) {
			lua_pushcfunction(L, loader);
			lua_rawseti(L, -2, 2);

			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);
}

}

/* ===========================================================================} */
