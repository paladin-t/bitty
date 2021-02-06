/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "scripting_lua_dbg.h"
#include <algorithm>
#include <cstddef>

/*
** {===========================================================================
** Utilities
*/

static int scriptingLuaDbgCompare(const Breakpoint &left, const Breakpoint &right) {
	if (left.source < right.source)
		return -1;
	else if (left.source > right.source)
		return 1;

	if (left.line < right.line)
		return -1;
	else if (left.line > right.line)
		return 1;

	return 0;
}

static int scriptingLuaDbgCompare(const void* lptr, const void* rptr) {
	return scriptingLuaDbgCompare(*(const Breakpoint*)lptr, *(const Breakpoint*)rptr);
}

/* ===========================================================================} */

/*
** {===========================================================================
** Lua scripting debug
*/

Breakpoint::Breakpoint() {
}

Breakpoint::Breakpoint(const char* src, int ln) : line(ln) {
	if (src)
		source = src;
}

bool Breakpoint::operator < (const Breakpoint &other) const {
	return scriptingLuaDbgCompare(*this, other) < 0;
}

size_t Breakpoints::count(void) const {
	return collection.size();
}

Breakpoint &Breakpoints::add(const Breakpoint &brk) {
	return *collection.insert(std::upper_bound(collection.begin(), collection.end(), brk), brk);
}

bool Breakpoints::remove(size_t index) {
	if (index >= collection.size())
		return false;

	collection.erase(collection.begin() + index);

	return true;
}

int Breakpoints::indexOf(const Breakpoint* brk) const {
	ptrdiff_t result = brk - &collection.front();
	if (result < 0 || result >= (ptrdiff_t)collection.size())
		return -1;

	return (int)result;
}

const Breakpoint* Breakpoints::find(const char* src, int ln) const {
	if (!src || ln < 0)
		return nullptr;

	if (collection.empty())
		return nullptr;

	const Breakpoint key(src, ln);
	void* ptr = std::bsearch(&key, &collection.front(), collection.size(), sizeof(Breakpoint), scriptingLuaDbgCompare);
	if (!ptr)
		return nullptr;

	return (const Breakpoint*)ptr;
}

bool Breakpoints::empty(void) const {
	return collection.empty();
}

void Breakpoints::clear(void) {
	collection.clear();
}

Breakpoints::Iterator Breakpoints::begin(void) {
	return collection.begin();
}

Breakpoints::Iterator Breakpoints::end(void) {
	return collection.end();
}

Breakpoints::ConstIterator Breakpoints::begin(void) const {
	return collection.begin();
}

Breakpoints::ConstIterator Breakpoints::end(void) const {
	return collection.end();
}

Breakpoints::Iterator Breakpoints::erase(ConstIterator it) {
	return collection.erase(it);
}

Variable &Variable::List::add(const Variable &var) {
	collection.push_back(var);

	return collection.back();
}

Variable::List::Iterator Variable::List::begin(void) {
	return collection.begin();
}

Variable::List::Iterator Variable::List::end(void) {
	return collection.end();
}

Variable::List::ConstIterator Variable::List::begin(void) const {
	return collection.begin();
}

Variable::List::ConstIterator Variable::List::end(void) const {
	return collection.end();
}

Variable::List::Iterator Variable::List::erase(ConstIterator it) {
	return collection.erase(it);
}

Variable::Variable() {
}

Variable::Variable(const char* n, const char* y, const Variant &d, bool up) : data(d), isUpvalue(up) {
	if (n)
		name = n;
	if (y)
		type = y;
}

Record::Record() {
}

Record::Record(const char* src, int ln, int lnDef, const char* n, const char* w) : line(ln), lineDefined(lnDef) {
	if (src)
		source = src;
	if (n)
		name = n;
	if (w)
		what = w;
}

size_t Records::count(void) const {
	return collection.size();
}

Record &Records::add(const Record &rec) {
	collection.push_back(rec);

	return collection.back();
}

void Records::clear(void) {
	collection.clear();
}

Records::Iterator Records::begin(void) {
	return collection.begin();
}

Records::Iterator Records::end(void) {
	return collection.end();
}

Records::ConstIterator Records::begin(void) const {
	return collection.begin();
}

Records::ConstIterator Records::end(void) const {
	return collection.end();
}

Records::Iterator Records::erase(ConstIterator it) {
	return collection.erase(it);
}

Scope::Scope() {
}

void Scope::fill(const char* src, int ln, int lnDef, const char* n, const char* w) {
	if (src)
		source = src;
	else
		source.clear();

	line = ln;

	lineDefined = lnDef;

	if (n)
		name = n;
	else
		name.clear();

	if (w)
		what = w;
	else
		what.clear();
}

bool Scope::empty(void) const {
	return source.empty() && line == -1 && lineDefined == -1 && name.empty() && what.empty();
}

void Scope::clear(void) {
	source.clear();
	line = -1;
	lineDefined = -1;
	name.clear();
	what.clear();
}

bool Scope::operator == (const Scope &other) const {
	return source == other.source &&
		lineDefined == other.lineDefined &&
		name == other.name &&
		what == other.what;
}

bool Scope::operator != (const Scope &other) const {
	return source != other.source ||
		lineDefined != other.lineDefined ||
		name != other.name ||
		what != other.what;
}

/* ===========================================================================} */
