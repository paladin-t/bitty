/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2024 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __SCRIPTING_LUA_DBG_H__
#define __SCRIPTING_LUA_DBG_H__

#include "object.h"
#include "plus.h"
#include <vector>

/*
** {===========================================================================
** Lua scripting debug
*/

struct Breakpoint {
	std::string source;
	int line = -1; // 1-based.

	Breakpoint();
	Breakpoint(const char* src, int ln);

	bool operator < (const Breakpoint &other) const;
};

struct Breakpoints {
	typedef std::vector<Breakpoint> Collection;

	typedef std::vector<Breakpoint>::iterator Iterator;
	typedef std::vector<Breakpoint>::const_iterator ConstIterator;

	Collection collection;
	mutable Mutex lock;

	size_t count(void) const;
	Breakpoint &add(const Breakpoint &brk);
	bool remove(size_t index);
	int indexOf(const Breakpoint* brk) const;
	const Breakpoint* find(const char* src, int ln) const;
	bool empty(void) const;
	void clear(void);

	Iterator begin(void);
	Iterator end(void);
	ConstIterator begin(void) const;
	ConstIterator end(void) const;
	Iterator erase(ConstIterator it);
};

struct Variable {
	struct List {
		typedef std::list<Variable> Collection;

		typedef std::list<Variable>::iterator Iterator;
		typedef std::list<Variable>::const_iterator ConstIterator;

		Collection collection;

		Variable &add(const Variable &var);

		Iterator begin(void);
		Iterator end(void);
		ConstIterator begin(void) const;
		ConstIterator end(void) const;
		Iterator erase(ConstIterator it);
	};

	std::string name;
	std::string type;
	Variant data = nullptr;
	bool isUpvalue = false;

	Variable();
	Variable(const char* n, const char* y, const Variant &d, bool up);
};

struct Record {
	std::string source;
	int line = -1;        // 1-based.
	int lineDefined = -1; // 1-based.
	std::string name;
	std::string what;
	Variable::List variables;

	Record();
	Record(const char* src, int ln, int lnDef, const char* n, const char* w);
};

struct Records {
	typedef std::vector<Record> Collection;

	typedef std::vector<Record>::iterator Iterator;
	typedef std::vector<Record>::const_iterator ConstIterator;

	Collection collection;
	mutable Mutex lock;

	size_t count(void) const;
	Record &add(const Record &rec);
	void clear(void);

	Iterator begin(void);
	Iterator end(void);
	ConstIterator begin(void) const;
	ConstIterator end(void) const;
	Iterator erase(ConstIterator it);
};

struct Scope {
	std::string source;
	int line = -1;        // 1-based. This field doesn't participate in the equality operator.
	int lineDefined = -1; // 1-based.
	std::string name;
	std::string what;

	Scope();

	void fill(const char* src, int ln, int lnDef, const char* n, const char* w);
	bool empty(void) const;
	void clear(void);

	bool operator == (const Scope &other) const;
	bool operator != (const Scope &other) const;
};

/* ===========================================================================} */

#endif /* __SCRIPTING_LUA_DBG_H__ */
