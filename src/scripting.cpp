/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "scripting.h"
#include "scripting_lua.h"

/*
** {===========================================================================
** Scripting
*/

Scripting::Scripting() {
}

Scripting::~Scripting() {
}

unsigned Scripting::type(void) const {
	return TYPE();
}

bool Scripting::open(
	Observer* obsvr,
	const class Project* project,
	const class Project* editing,
	class Primitives* primitives,
	bool effectsEnabled
) {
	if (_opened)
		return false;
	_opened = true;

	observer(obsvr);

	_effectsEnabled = effectsEnabled;

	_project = project;
	_editing = editing;
	_primitives = primitives;

	std::string lang = "unknown";
	switch (language()) {
	case LUA:
		lang = "Lua";

		break;
	default:
		assert(false && "Unknown.");

		break;
	}
	fprintf(stdout, "Scripting opened: \"%s\".\n", lang.c_str());

	return true;
}

bool Scripting::close(void) {
	if (!_opened)
		return false;
	_opened = false;

	_project = nullptr;
	_editing = nullptr;
	_primitives = nullptr;

	observer(nullptr);

	std::string lang = "unknown";
	switch (language()) {
	case LUA:
		lang = "Lua";

		break;
	default:
		assert(false && "Unknown.");

		break;
	}
	fprintf(stdout, "Scripting closed: \"%s\".\n", lang.c_str());

	return true;
}

bool Scripting::effectsEnabled(void) const {
	return _effectsEnabled;
}

const class Project* Scripting::project(void) const {
	return _project;
}

const class Project* Scripting::editing(void) const {
	return _editing;
}

class Primitives* Scripting::primitives(void) const {
	return _primitives;
}

Scripting* Scripting::create(Languages language) {
	switch (language) {
	case LUA:
		return new ScriptingLua();
	default:
		assert(false && "Not implemented.");

		return nullptr;
	}
}

void Scripting::destroy(Executable* ptr) {
	switch (ptr->language()) {
	case LUA: {
			ScriptingLua* impl = static_cast<ScriptingLua*>(ptr);
			delete impl;
		}

		break;
	default:
		assert(false && "Not implemented.");

		break;
	}
}

/* ===========================================================================} */
