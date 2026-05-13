/*
** Bitty
**
** An itty bitty 2D game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "loader.h"
#include "plugin.h"
#include "project.h"
#include "scripting.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef PLUGIN_USAGE_FUNCTION
#	define PLUGIN_USAGE_FUNCTION "usage"
#endif /* PLUGIN_USAGE_FUNCTION */
#ifndef PLUGIN_MUTEX_FUNCTION
#	define PLUGIN_MUTEX_FUNCTION "mutex"
#endif /* PLUGIN_MUTEX_FUNCTION */
#ifndef PLUGIN_SCHEMA_INVOKABLE_NAME
#	define PLUGIN_SCHEMA_INVOKABLE_NAME "schema"
#endif /* PLUGIN_SCHEMA_INVOKABLE_NAME */
#ifndef PLUGIN_MENU_INVOKABLE_NAME
#	define PLUGIN_MENU_INVOKABLE_NAME "menu"
#endif /* PLUGIN_MENU_INVOKABLE_NAME */
#ifndef PLUGIN_COMPILER_INVOKABLE_NAME
#	define PLUGIN_COMPILER_INVOKABLE_NAME "compiler"
#endif /* PLUGIN_COMPILER_INVOKABLE_NAME */

#ifndef PLUGIN_USAGE_MENU
#	define PLUGIN_USAGE_MENU "menu"
#endif /* PLUGIN_USAGE_MENU */
#ifndef PLUGIN_USAGE_COMPILER
#	define PLUGIN_USAGE_COMPILER "compiler"
#endif /* PLUGIN_USAGE_COMPILER */

/* ===========================================================================} */

/*
** {===========================================================================
** Scripting
*/

namespace Bitty {

unsigned Plugin::Schema::type(void) const {
	const unsigned y = (unsigned)Math::hash(BITTY_MAKE_UINT32('P', 'L', 'G', 'N'), name);

	return y;
}

Plugin::Plugin(
	class Renderer* rnd,
	Executable::Observer* observer,
	const class Project* editing,
	const char* path
) : _renderer(rnd),
	_observer(observer),
	_editing(editing)
{
	_path = path;

	usage(Usages::NONE);

	order(0);
}

Plugin::~Plugin() {
	close();
}

bool Plugin::open(Errors* error) {
	close();

	LockGuard<RecursiveMutex>::UniquePtr acquired;

	Project* prj = _editing->acquire(acquired);
	if (!prj)
		return false;

	Loader* loader = nullptr;
	prj->loader()->clone(&loader);
	_project = new Project();
	_project->loader(loader);
	_project->factory(prj->factory());
	_project->open(_renderer);
	_project->load(_path.c_str());

	_executable = Scripting::create(Executable::LUA);
	_executable->open(_observer, _project, _editing, nullptr, BITTY_ACTIVE_FRAME_RATE, false);
	_executable->timeout(-1);
	_executable->prepare();
	_executable->setup();

	resolveUsage();

	entry(Entry(_project->title()));
	order(_project->order());
	_mutexInvokable = _executable->getInvokable(PLUGIN_MUTEX_FUNCTION);
	_schemaInvokable = _executable->getInvokable(PLUGIN_SCHEMA_INVOKABLE_NAME);
	_menuInvokable = _executable->getInvokable(PLUGIN_MENU_INVOKABLE_NAME);
	_compilerInvokable = _executable->getInvokable(PLUGIN_COMPILER_INVOKABLE_NAME);

	resolveMutexName();
	resolveSchema();

	if (!lockExclusivelyRunning(error))
		return false;

	return true;
}

bool Plugin::close(void) {
	unlockExclusivelyRunning();

	_mutexInvokable = nullptr;
	_schemaInvokable = nullptr;
	_menuInvokable = nullptr;
	_compilerInvokable = nullptr;

	if (_executable) {
		_executable->finish();
		_executable->close();

		Scripting::destroy(_executable);
		_executable = nullptr;
	}

	if (_project) {
		_project->unload();
		_project->close();
		delete _project->loader();
		_project->loader(nullptr);

		delete _project;
		_project = nullptr;
	}

	_ticks = 0.0;

	return true;
}

bool Plugin::instant(void) const {
	switch (usage()) {
	case Usages::NONE:
		return true;
	case Usages::MENU:
		return true;
	case Usages::COMPILER:
		return true;
	default:
		return true;
	}
}

bool Plugin::is(Usages usage_) const {
	return ((unsigned)usage() & (unsigned)usage_) != (unsigned)Usages::NONE;
}

Variant Plugin::run(Functions function, const std::string &args, Errors* error) {
	if (function == Functions::NONE)
		return nullptr;

	if (
		is(Usages::MENU) &&
		((unsigned)function & (unsigned)Functions::MENU) == (unsigned)Functions::NONE
	) {
		return nullptr;
	}
	if (
		is(Usages::COMPILER) && (
			((unsigned)function & (unsigned)Functions::SCHEMA) == (unsigned)Functions::NONE &&
			((unsigned)function & (unsigned)Functions::COMPILER) == (unsigned)Functions::NONE
		)
	) {
		return nullptr;
	}

	if (entry().empty())
		return nullptr;

	if (!opened()) {
		if (!open(error))
			return Variant(nullptr);
	} else {
		if (!lockExclusivelyRunning(error))
			return Variant(nullptr);
	}
	Executable::Invokable func = nullptr;
	switch (function) {
	case Functions::SCHEMA:
		func = _schemaInvokable;

		break;
	case Functions::MENU:
		func = _menuInvokable;

		break;
	case Functions::COMPILER:
		func = _compilerInvokable;

		break;
	default: // Do nothing.
		break;
	}
	Variant result = nullptr;
	if (args.empty())
		result = _executable->invoke(func);
	else
		result = _executable->invoke(func, args);
	func = nullptr;
	const bool pending = _executable->pending();
	if (!pending)
		close();

	return result;
}

bool Plugin::stop(void) {
	if (!_executable)
		return false;

	close();

	return true;
}

void Plugin::update(double delta) {
	if (!opened())
		return;

	_executable->sync(delta);

	_ticks += delta;
	if (_ticks >= 5) {
		_ticks -= 5;
		_executable->gc();
	}

	const bool pending = _executable->pending();
	if (!pending)
		close();
}

bool Plugin::opened(void) const {
	return _project && _executable;
}

bool Plugin::lockExclusivelyRunning(Errors* error) {
	if (error)
		*error = Errors::NONE;

	do {
		if (!is(Usages::MENU))
			break;

		if (mutex().empty())
			break;

		if (!_executionMutex.try_lock(mutex())) {
			if (error)
				*error = Errors::ALREADY_RUNNING;

			return false;
		}

		if (_exclusively)
			break;
		_exclusively = true;
	} while (false);

	return true;
}

void Plugin::unlockExclusivelyRunning(void) {
	do {
		if (!_exclusively)
			break;

		if (!is(Usages::MENU))
			break;

		if (mutex().empty())
			break;

		_executionMutex.unlock(mutex());

		_exclusively = false;
	} while (false);
}

void Plugin::resolveUsage(void) {
	if (usage() != Usages::NONE)
		return;

	Executable::Invokable func = _executable->getInvokable(PLUGIN_USAGE_FUNCTION);

	if (!func)
		return;

	const Variant usg = _executable->invoke(func);
	if (usg.type() != Variant::OBJECT)
		return;
	Object::Ptr obj = (Object::Ptr)usg;
	if (!obj)
		return;
	if (!Object::is<IList::Ptr>(obj))
		return;

	IList::Ptr lst = Object::as<IList::Ptr>(obj);
	for (int i = 0; i < lst->count(); ++i) {
		const Variant elem = lst->at(i);

		if (elem.type() != Variant::STRING) {
			const std::string str = elem.toString();
			fprintf(stderr, "Unknown usage option: %s.", str.c_str());

			continue;
		}

		const std::string item = (const std::string)elem;
		if (item == PLUGIN_USAGE_MENU)
			usage((Usages)((unsigned)usage() | (unsigned)Usages::MENU));
		else if (item == PLUGIN_USAGE_COMPILER)
			usage((Usages)((unsigned)usage() | (unsigned)Usages::COMPILER));
		else
			fprintf(stderr, "Unknown usage option: \"%s\".", item.c_str());
	}
}

void Plugin::resolveMutexName(void) {
	if (!is(Usages::MENU))
		return;

	Executable::Invokable func = _mutexInvokable;

	if (!func)
		return;

	const Variant mutexName_ = _executable->invoke(func);
	if (mutexName_.type() != Variant::STRING)
		return;
	const std::string mutexName = (std::string)mutexName_;

	mutex(mutexName);
}

void Plugin::resolveSchema(void) {
	if (!is(Usages::COMPILER))
		return;

	Executable::Invokable func = _schemaInvokable;

	if (!func)
		return;

	const Variant schema_ = _executable->invoke(func);
	if (schema_.type() != Variant::OBJECT)
		return;
	Object::Ptr obj = (Object::Ptr)schema_;
	if (!obj)
		return;
	if (!Object::is<IDictionary::Ptr>(obj))
		return;

	IDictionary::Ptr dict = Object::as<IDictionary::Ptr>(obj);
	if (dict->contains("name")) {
		const Variant elem = dict->get("name");
		do {
			if (elem.type() != Variant::STRING)
				break;

			const std::string str = elem.toString();
			schema().name = str;
		} while (false);
	}
	if (dict->contains("extension")) {
		const Variant elem = dict->get("extension");
		do {
			if (elem.type() != Variant::STRING)
				break;

			const std::string str = elem.toString();
			schema().extension = str;
		} while (false);
	}
}

}

/* ===========================================================================} */
