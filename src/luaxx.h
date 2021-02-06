/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __LUAXX_H__
#define __LUAXX_H__

#include "../lib/lua/src/lua.hpp"
#include <array>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>

/*
** {===========================================================================
** C++ wrapper around Lua
**
** @note The principle of this wrapper is to be as thin as possible. Since the
**   original Lua is implemented in C, this wraps the C API into a C++(11)
**   flavor.
*/

namespace Lua {

typedef unsigned Enum;

class Index {
private:
	int _index = 0;

public:
	Index();
	explicit Index(int idx);

	Index &operator = (int idx) = delete;

	Index &operator ++ (void);
	Index operator ++ (int _);
	Index &operator += (int val);
	Index operator + (int val) const;

	Index &operator -- (void);
	Index operator -- (int _);
	Index &operator -= (int val);
	Index operator - (int val) const;

	operator int (void) const;
};

class Placeholder {
	friend void check(lua_State*, Placeholder &, Index);
	friend void read(lua_State*, Placeholder &, Index);

private:
	int _index = 0;

public:
	Placeholder();

	operator int (void) const;
};

class Ref {
public:
	typedef std::shared_ptr<Ref> Ptr;

	friend void check(lua_State*, Ref &, Index);
	friend void check(lua_State*, Ptr &, Index);
	friend void read(lua_State*, Ref &, Index);
	friend void read(lua_State*, Ptr &, Index);

private:
	lua_State* _L = nullptr;

	int _handle = LUA_NOREF;

public:
	Ref();
	Ref(const Ref &other);
	~Ref();

	Ref &operator = (const Ref &other);

	operator int (void) const;

	bool valid(void) const;

private:
	Ref(lua_State* L);
};

class Function {
public:
	typedef std::shared_ptr<Function> Ptr;

	friend void check(lua_State*, Function &, Index);
	friend void check(lua_State*, Ptr &, Index);
	friend void read(lua_State*, Function &, Index);
	friend void read(lua_State*, Ptr &, Index);

private:
	lua_State* _L = nullptr;

	int _handle = LUA_NOREF;

public:
	Function();
	Function(const Function &other);
	~Function();

	Function &operator = (const Function &other);

	operator int (void) const;

	bool valid(void) const;

private:
	Function(lua_State* L);
};

typedef void (* ProtectedFunction)(lua_State*, void*);

lua_State* create(lua_Alloc f, void* ud /* nullable */);
void destroy(lua_State* L);

void* userdata(lua_State* L);

int ref(lua_State* L);
void unref(lua_State* L, int ref);
int refed(lua_State* L, int ref);

int absIndex(lua_State* L, int idx);
int getTop(lua_State* L);
void setTop(lua_State* L, int idx);
void pop(lua_State* L, int count = 1);
void push(lua_State* L, int idx);
int get(lua_State* L, int idx);
int get(lua_State* L, int idx, int n);
void set(lua_State* L, int idx);
void set(lua_State* L, int idx, int n);
void rotate(lua_State* L, int idx, int n);
void copy(lua_State* L, int fromidx, int toidx);
int checkStack(lua_State* L, int n);
void xmove(lua_State* from, lua_State* to, int n);
lua_Unsigned len(lua_State* L, int idx);
int next(lua_State* L, int idx);

int getMetaOf(lua_State* L, int idx = 1);
int setMetaOf(lua_State* L, const char* meta);
int newMeta(lua_State* L, const char* name);

int newTable(lua_State* L);
int newTable(lua_State* L, int capacity);

int typeOf(lua_State* L, int idx);
const char* typeNameOf(lua_State* L, int idx, bool detail = true);

const char* toString(lua_State* L, int idx, size_t* len);

bool isNil(lua_State* L, int idx = 1);
bool isBoolean(lua_State* L, int idx = 1);
bool isInteger(lua_State* L, int idx = 1);
bool isNumber(lua_State* L, int idx = 1);
bool isString(lua_State* L, int idx = 1);
bool isTable(lua_State* L, int idx = 1);
bool isArray(lua_State* L, int idx = 1);
bool isUserdata(lua_State* L, int idx = 1);
bool isCFunction(lua_State* L, int idx = 1);
bool isFunction(lua_State* L, int idx = 1);

void check(lua_State* L, Placeholder &ret, Index idx = Index(1));
void check(lua_State* L, Ref &ret, Index idx = Index(1));
void check(lua_State* L, Ref::Ptr &ret, Index idx = Index(1));
void check(lua_State* L, bool &ret, Index idx = Index(1));
void check(lua_State* L, signed char &ret, Index idx = Index(1));
void check(lua_State* L, unsigned char &ret, Index idx = Index(1));
void check(lua_State* L, short &ret, Index idx = Index(1));
void check(lua_State* L, unsigned short &ret, Index idx = Index(1));
void check(lua_State* L, int &ret, Index idx = Index(1));
void check(lua_State* L, unsigned int &ret, Index idx = Index(1));
void check(lua_State* L, long &ret, Index idx = Index(1));
void check(lua_State* L, unsigned long &ret, Index idx = Index(1));
void check(lua_State* L, long long &ret, Index idx = Index(1));
void check(lua_State* L, unsigned long long &ret, Index idx = Index(1));
void check(lua_State* L, float &ret, Index idx = Index(1));
void check(lua_State* L, double &ret, Index idx = Index(1));
void check(lua_State* L, const char* &ret, Index idx = Index(1));
void check(lua_State* L, std::string &ret, Index idx = Index(1));
void check(lua_State* L, lua_CFunction &ret, Index idx = Index(1));
void check(lua_State* L, Function &ret, Index idx = Index(1));
void check(lua_State* L, Function::Ptr &ret, Index idx = Index(1));
void check(lua_State* L, void* &ret, Index idx = Index(1), const char* type = nullptr);
template<typename Class> void check(lua_State* L, Class* &ret, Index idx, const char* type /* nullable */) {
	ret = nullptr;

	void* ptr = nullptr;
	check(L, ptr, idx, type);
	if (ptr)
		ret = (Class*)ptr;
}
template<int Idx = 1, typename Ret> void check(lua_State* L, Ret &ret) {
	check(L, ret, Index(Idx));
}
template<int Idx = 1, typename Car, typename ...Cdr> void check(lua_State* L, Car &car, Cdr &...cdr) {
	check<Idx>(L, car);
	check<Idx + 1>(L, cdr...);
}
template<template<typename T, typename A = std::allocator<T> > class Coll, typename Val> void check(lua_State* L, Coll<Val, std::allocator<Val> > &ret, Index idx = Index(1)) {
	ret.clear();

	const size_t size = len(L, idx);
	for (size_t i = 1; i <= size; ++i) { // 1-based.
		Val val;
		check(L, val);
		ret.push_back(val);
	}
}

void read(lua_State* L, Placeholder &ret, Index idx = Index(1));
void read(lua_State* L, Ref &ret, Index idx = Index(1));
void read(lua_State* L, Ref::Ptr &ret, Index idx = Index(1));
void read(lua_State* L, bool &ret, Index idx = Index(1));
void read(lua_State* L, signed char &ret, Index idx = Index(1));
void read(lua_State* L, unsigned char &ret, Index idx = Index(1));
void read(lua_State* L, short &ret, Index idx = Index(1));
void read(lua_State* L, unsigned short &ret, Index idx = Index(1));
void read(lua_State* L, int &ret, Index idx = Index(1));
void read(lua_State* L, unsigned int &ret, Index idx = Index(1));
void read(lua_State* L, long &ret, Index idx = Index(1));
void read(lua_State* L, unsigned long &ret, Index idx = Index(1));
void read(lua_State* L, long long &ret, Index idx = Index(1));
void read(lua_State* L, unsigned long long &ret, Index idx = Index(1));
void read(lua_State* L, float &ret, Index idx = Index(1));
void read(lua_State* L, double &ret, Index idx = Index(1));
void read(lua_State* L, const char* &ret, Index idx = Index(1));
void read(lua_State* L, std::string &ret, Index idx = Index(1));
void read(lua_State* L, lua_CFunction &ret, Index idx = Index(1));
void read(lua_State* L, Function &ret, Index idx = Index(1));
void read(lua_State* L, Function::Ptr &ret, Index idx = Index(1));
void read(lua_State* L, void* &ret, Index idx = Index(1), const char* type = nullptr);
template<typename Class> void read(lua_State* L, Class* &ret, Index idx, const char* type /* nullable */) {
	ret = nullptr;

	void* ptr = nullptr;
	read(L, ptr, idx, type);
	if (ptr)
		ret = (Class*)ptr;
}
template<int Idx = 1, typename Ret> void read(lua_State* L, Ret &ret) {
	read(L, ret, Index(Idx));
}
template<int Idx = 1, typename Car, typename ...Cdr> void read(lua_State* L, Car &car, Cdr &...cdr) {
	read<Idx>(L, car);
	read<Idx + 1>(L, cdr...);
}
template<template<typename T, typename A = std::allocator<T> > class Coll, typename Val> int read(lua_State* L, Coll<Val, std::allocator<Val> > &ret, Index idx = Index(1)) {
	ret.clear();

	if (!isArray(L, idx))
		return 0;

	const lua_Unsigned size = len(L, idx);
	for (int i = 1; i <= (int)size; ++i) { // 1-based.
		lua_pushinteger(L, i);
		get(L, -2, i);

		Val val;
		read(L, val, Index(-1));
		ret.push_back(val);

		pop(L, 2);
	}

	return 1;
}

int write(lua_State* L, const Index &val);
int write(lua_State* L, const Ref &val);
int write(lua_State* L, std::nullptr_t);
int write(lua_State* L, bool val);
int write(lua_State* L, signed char val);
int write(lua_State* L, unsigned char val);
int write(lua_State* L, short val);
int write(lua_State* L, unsigned short val);
int write(lua_State* L, int val);
int write(lua_State* L, unsigned int val);
int write(lua_State* L, long val);
int write(lua_State* L, unsigned long val);
int write(lua_State* L, long long val);
int write(lua_State* L, unsigned long long val);
int write(lua_State* L, float val);
int write(lua_State* L, double val);
int write(lua_State* L, const char* val);
int write(lua_State* L, const std::string &val);
int write(lua_State* L, std::string &val);
int write(lua_State* L, lua_CFunction val);
int write(lua_State* L, const Function &val);
int write(lua_State* L, Function &val);
int write(lua_State* L, void* &val, size_t size);
template<typename Class> int write(lua_State* L, const Class* val);
template<typename Class> int write(lua_State* L, Class* val);
template<typename Val> int write(lua_State* L, const Val &val);
template<typename Val> int write(lua_State* L, Val &val);
template<template<typename T, typename A = std::allocator<T> > class Coll, typename Val> int write(lua_State* L, const Coll<Val, std::allocator<Val> > &val) {
	newTable(L, (int)val.size());
	const int tbl = getTop(L);
	int index = 1; // 1-based.
	for (auto it = val.begin(); it != val.end(); ++it) {
		write(L, *it);
		set(L, tbl, index);
		++index;
	}

	return 1;
}
template<template<typename T, typename A = std::allocator<T> > class Coll, typename Val> int write(lua_State* L, Coll<Val, std::allocator<Val> > &val) {
	newTable(L, (int)val.size());
	const int tbl = getTop(L);
	int index = 1; // 1-based.
	for (auto it = val.begin(); it != val.end(); ++it) {
		write(L, *it);
		set(L, tbl, index);
		++index;
	}

	return 1;
}
template<typename Car, typename ...Cdr> int write(lua_State* L, const Car &car, const Cdr &...cdr) {
	int result = write(L, car);
	result += write(L, cdr...);

	return result;
}
template<typename Class> int writeClass(lua_State* L, const Class* val, const char* name) {
	void* ud = nullptr;
	if (!write(L, ud, sizeof(Class)))
		return 0;

	Class* ptr = (Class*)ud;
	ptr = new (ptr) Class();
	*ptr = *val;

	setMetaOf(L, name);

	return 1;
}
template<typename Class> int writeClass(lua_State* L, Class* val, const char* name) {
	void* ud = nullptr;
	if (!write(L, ud, sizeof(Class)))
		return 0;

	Class* ptr = (Class*)ud;
	ptr = new (ptr) Class();
	*ptr = *val;

	setMetaOf(L, name);

	return 1;
}
template<typename Val> int writeValue(lua_State* L, const Val &val) {
	return write(L, val);
}
template<typename Val> int writeValue(lua_State* L, Val &val) {
	return write(L, val);
}

int function(lua_State* L, const char* func);
int function(lua_State* L, const Function &func);
int invoke(lua_State* L, int argc, int retc);
int invoke(lua_State* L, ProtectedFunction func, void* ud);
int end(lua_State* L);
int call(lua_State* L, const char* func);
int call(lua_State* L, const Function &func);
template<typename Func, typename ...Args> int call(lua_State* L, const Func &func, const Args &...args) {
	function(L, func);
	write(L, args...);
	const int result = invoke(L, sizeof...(Args), 0);
	if (result == LUA_OK || result == LUA_YIELD)
		end(L);

	return result;
}
template<typename Func, typename Ret> int call(Ret &ret, lua_State* L, const Func &func) {
	function(L, func);
	const int result = invoke(L, 0, 1);
	if (result == LUA_OK || result == LUA_YIELD) {
		check(L, ret, Index(-1));
		end(L);
	}

	return result;
}
template<typename Func, typename Ret, typename ...Args> int call(Ret &ret, lua_State* L, const Func &func, const Args &...args) {
	function(L, func);
	write(L, args...);
	const int result = invoke(L, sizeof...(Args), 1);
	if (result == LUA_OK || result == LUA_YIELD) {
		check(L, ret, Index(-1));
		end(L);
	}

	return result;
}
template<typename Func, typename ...Rets> int call(std::tuple<Rets...> &ret, lua_State* L, const Func &func) {
	function(L, func);
	const int result = invoke(L, 0, sizeof...(Rets));
	if (result == LUA_OK || result == LUA_YIELD) {
		check<-(int)sizeof...(Rets)>(L, std::get<Rets>(ret)...);
		end(L);
	}

	return result;
}
template<typename Func, typename ...Rets, typename ...Args> int call(std::tuple<Rets...> &ret, lua_State* L, const Func &func, const Args &...args) {
	function(L, func);
	write(L, args...);
	const int result = invoke(L, sizeof...(Args), sizeof...(Rets));
	if (result == LUA_OK || result == LUA_YIELD) {
		check<-(int)sizeof...(Rets)>(L, std::get<Rets>(ret)...);
		end(L);
	}

	return result;
}

int setMeta(lua_State* L, const luaL_Reg* meta, const luaL_Reg* methods, lua_CFunction index /* nullable */, lua_CFunction newindex /* nullable */);
template<typename ...Fields> int setMeta(lua_State* L, const luaL_Reg* meta, const luaL_Reg* methods, lua_CFunction index /* nullable */, lua_CFunction newindex /* nullable */, const Fields &...fields) {
	setTable(L, fields...);

	return setMeta(L, meta, methods, index, newindex);
}
int readMeta(lua_State* L, int idx, const char* field);

int readTable(lua_State* L, int idx);
int readTable(lua_State* L, int idx, const char* field);
int readTable(lua_State* L, int idx, lua_Integer n);
void writeTable(lua_State* L, int idx);
void writeTable(lua_State* L, int idx, const char* field);
void writeTable(lua_State* L, int idx, lua_Integer n);
template<typename Key> int getTable(lua_State* L, const Key &key) { // Before: ...table (top); after: ...value, table (top)
	write(L, key);
	const int result = get(L, -2);
	rotate(L, -2, 1);

	return !!result ? 1 : 0;
}
template<typename Key, typename Ret> int getTable(lua_State* L, const Key &key, Ret &ret) { // Before: ...table (top); after: ...table (top)
	write(L, key);
	const int result = get(L, -2);
	read(L, ret, Index(-1));

	pop(L);

	return !!result ? 1 : 0;
}
template<typename Key, typename Ret, typename ...Rest> int getTable(lua_State* L, const Key &key, Ret &ret, const char* next, Rest &...rest) { // Before: ...table (top); after: ...table (top)
	int result = getTable(L, key, ret);
	result += getTable(L, next, rest...);

	return result;
}
template<typename Key> int setTable(lua_State* L, const Key &key) { // Before: ...table, value (top); after: ...table (top)
	const int n = getTop(L);
	write(L, key);
	push(L, n);
	set(L, n - 1);

	pop(L);

	return 1;
}
template<typename Key, typename Val> int setTable(lua_State* L, const Key &key, const Val &val) { // Before: ...table (top); after: ...table (top)
	write(L, key, val);
	set(L, -3);

	return 1;
}
template<typename Key, typename Val, typename ...Rest> int setTable(lua_State* L, const Key &key, const Val &val, const Rest &...rest) { // Before: ...table (top); after: ...table (top)
	int result = setTable(L, key, val);
	result += setTable(L, rest...);

	return result;
}

void setFunctions(lua_State* L, const luaL_Reg* l, bool nup);

int getGlobal(lua_State* L, const char* name);
void setGlobal(lua_State* L, const char* name);

const char* getLocal(lua_State* L, const lua_Debug* ar, int n);
const char* setLocal(lua_State* L, const lua_Debug* ar, int n);

const char* getUpvalue(lua_State* L, int funcindex, int n);
const char* setUpvalue(lua_State* L, int funcindex, int n);
void* upvalueId(lua_State* L, int fidx, int n);
void upvalueJoin(lua_State* L, int fidx0, int n0, int fidx1, int n1);

template<typename ...Val> constexpr std::array<typename std::decay<typename std::common_type<Val...>::type>::type, 0> array(void) {
	return std::array<typename std::decay<typename std::common_type<Val...>::type>::type, 0>{ };
}
template<typename ...Val> constexpr std::array<typename std::decay<typename std::common_type<Val...>::type>::type, sizeof...(Val)> array(Val &&...vals) {
	return std::array<typename std::decay<typename std::common_type<Val...>::type>::type, sizeof...(Val)>{ std::forward<Val>(vals)... };
}

void reg(lua_State* L, const char* name, lua_CFunction function);
void reg(lua_State* L, const luaL_Reg* functions);
template<size_t Size> void reg(lua_State* L, const std::array<luaL_Reg, Size> &functions) {
	reg(L, functions.empty() ? nullptr : functions.data());
}

void req(lua_State* L, const char* name, lua_CFunction module);
void req(lua_State* L, const luaL_Reg* modules);
template<size_t Size> void req(lua_State* L, const std::array<luaL_Reg, Size> &modules) {
	req(L, modules.empty() ? nullptr : modules.data());
}

bool def(lua_State* L, const char* name, lua_CFunction ctor /* nullable */, const luaL_Reg* meta, const luaL_Reg* methods, lua_CFunction index /* nullable */, lua_CFunction newindex /* nullable */);
template<typename ...Fields> bool def(lua_State* L, const char* name, lua_CFunction ctor /* nullable */, const luaL_Reg* meta, const luaL_Reg* methods, lua_CFunction index /* nullable */, lua_CFunction newindex /* nullable */, const Fields &...fields) {
	bool result = false;

	if (ctor)
		req(L, array(luaL_Reg{ name, ctor }, luaL_Reg{ nullptr, nullptr }));

	if (meta || methods || index || newindex) {
		if (newMeta(L, name)) {
			result = true;
			setMeta(L, meta, methods, index, newindex, fields...);
			pop(L);
		}
		pop(L);
	}

	return result;
}
template<size_t MetaSize, size_t MethodSize> bool def(lua_State* L, const char* name, lua_CFunction ctor /* nullable */, const std::array<luaL_Reg, MetaSize> &meta, const std::array<luaL_Reg, MethodSize> &methods, lua_CFunction index /* nullable */, lua_CFunction newindex /* nullable */) {
	return def(L, name, ctor, meta.empty() ? nullptr : meta.data(), methods.empty() ? nullptr : methods.data(), index, newindex);
}
template<size_t MetaSize, size_t MethodSize, typename ...Fields> bool def(lua_State* L, const char* name, lua_CFunction ctor /* nullable */, const std::array<luaL_Reg, MetaSize> &meta, const std::array<luaL_Reg, MethodSize> &methods, lua_CFunction index /* nullable */, lua_CFunction newindex /* nullable */, const Fields &...fields) {
	return def(L, name, ctor, meta.empty() ? nullptr : meta.data(), methods.empty() ? nullptr : methods.data(), index, newindex, fields...);
}

void lib(lua_State* L, const luaL_Reg* functions, int size);
template<size_t Size> void lib(lua_State* L, const std::array<luaL_Reg, Size> &functions) {
	lib(L, functions.empty() ? nullptr : functions.data(), Size);
}

template<typename Class> int __gc(lua_State* L) {
	Class* ptr = nullptr;
	check(L, ptr);
	if (!ptr)
		return 0;

	ptr->~Class();

	return 0;
}
template<typename Class> int __tostring(lua_State* L) {
	auto toHex = [] (uintptr_t val, bool toupper) -> std::string {
		std::stringstream stream;
		if (toupper)
			stream.setf(std::ios::uppercase);
		stream <<
			std::setfill('0') <<
			std::setw(sizeof(decltype(val)) * 2) <<
			std::hex <<
			val;

		return stream.str();
	};

	Class* ptr = nullptr;
	check(L, ptr);
	if (!ptr) {
		write(L, "unknown");

		return 1;
	}

	std::string str;
	if (getMetaOf(L)) {
		getTable(L, "__name", str);
		pop(L);
	}

	str += "@0x";
	str += toHex((uintptr_t)ptr, false);

	return write(L, str);
}
int __index(lua_State* L, const char* field);
bool __newindex(lua_State* L, const char* field, int valIdx);

int getStack(lua_State* L, int level, lua_Debug* ar);
int getInfo(lua_State* L, const char* what, lua_Debug* ar);
void setHook(lua_State* L, lua_Hook func, int mask, int count);
lua_Hook getHook(lua_State* L);
int getHookMask(lua_State* L);
int getHookCount(lua_State* L);

int gc(lua_State* L);
int gc(lua_State* L, int opt);
int gc(lua_State* L, int opt, int arg0);
int gc(lua_State* L, int opt, int arg0, int arg1);
int gc(lua_State* L, int opt, int arg0, int arg1, int arg2);

int error(lua_State* L, const char* msg);

void setLoader(lua_State* L, lua_CFunction loader);

}

/* ===========================================================================} */

#endif /* __LUAXX_H__ */
