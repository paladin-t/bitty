/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include "bitty.h"
#include "entry.h"
#include "executable.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef PLUGIN_BUILTIN_DIR
#	define PLUGIN_BUILTIN_DIR "../plugins/" /* Relative path. */
#endif /* PLUGIN_BUILTIN_DIR */
#ifndef PLUGIN_CUSTOM_DIR
#	define PLUGIN_CUSTOM_DIR "plugins/" /* Relative path. */
#endif /* PLUGIN_CUSTOM_DIR */

#ifndef PLUGIN_MENU_PROJECT_NAME
#	define PLUGIN_MENU_PROJECT_NAME "Project"
#endif /* PLUGIN_MENU_PROJECT_NAME */
#ifndef PLUGIN_MENU_PLUGIN_NAME
#	define PLUGIN_MENU_PLUGIN_NAME "Plugins"
#endif /* PLUGIN_MENU_PLUGIN_NAME */
#ifndef PLUGIN_MENU_HELP_NAME
#	define PLUGIN_MENU_HELP_NAME "Help"
#endif /* PLUGIN_MENU_HELP_NAME */

/* ===========================================================================} */

/*
** {===========================================================================
** Plugin
*/

/**
 * @brief Plugin.
 */
class Plugin : public NonCopyable {
public:
	typedef std::vector<Plugin*> Array;

	enum class Usages : unsigned {
		NONE = 0,
		MENU = 1 << 0,
		COMPILER = 1 << 1
	};

	enum class Functions : unsigned {
		NONE = (unsigned)Usages::NONE,
		SCHEMA = 1 << 2,
		MENU = (unsigned)Usages::MENU,
		COMPILER = (unsigned)Usages::COMPILER
	};

	struct Schema {
		std::string name;
		std::string extension;

		unsigned type(void) const;
	};

public:
	BITTY_PROPERTY(Entry, entry)
	BITTY_PROPERTY_READONLY(Usages, usage)
	BITTY_PROPERTY(unsigned, order)
	BITTY_PROPERTY(Schema, schema)

private:
	class Renderer* _renderer = nullptr; // Foreign.
	Executable::Observer* _observer = nullptr; // Foreign.
	const class Project* _editing = nullptr; // Foreign.
	std::string _path;

	class Project* _project = nullptr;
	Executable* _executable = nullptr;
	Executable::Invokable _schemaInvokable = nullptr;
	Executable::Invokable _menuInvokable = nullptr;
	Executable::Invokable _compilerInvokable = nullptr;
	double _ticks = 0.0;

public:
	Plugin(
		class Renderer* rnd,
		Executable::Observer* observer,
		const class Project* editing,
		const char* path
	);
	~Plugin();

	bool open(void);
	bool close(void);

	bool instant(void) const;

	bool is(Usages usage) const;

	Variant run(Functions function, const std::string &args);

	void update(double delta);

private:
	bool opened(void) const;
};

/* ===========================================================================} */

#endif /* __PLUGIN_H__ */
