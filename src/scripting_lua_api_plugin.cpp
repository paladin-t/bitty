/*
** Bitty
**
** An itty bitty 2D game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "bitty.h"
#include "bytes.h"
#include "filesystem.h"
#include "project.h"
#include "scripting_lua.h"
#include "scripting_lua_api_plugin.h"

/*
** {===========================================================================
** Utilities
*/

namespace Lua { // Library.

using namespace ::Bitty;

/**< Bytes. */

LUA_CHECK_OBJ(Bytes)
LUA_READ_OBJ(Bytes)
LUA_WRITE_OBJ(Bytes)
LUA_WRITE_OBJ_CONST(Bytes)

}

namespace Lua { // Application.

using namespace ::Bitty;

/**< Project. */

typedef std::shared_ptr<const Project> ProjectPtr;
LUA_CHECK_ALIAS(ProjectPtr, Project)
LUA_READ_ALIAS(ProjectPtr, Project)
LUA_WRITE_ALIAS(ProjectPtr, Project)
LUA_WRITE_ALIAS_CONST(ProjectPtr, Project)

}

using namespace ::Lua;

/* ===========================================================================} */

/*
** {===========================================================================
** Application
*/

namespace Bitty {

namespace Lua {

namespace Application {

/**< Plugin. */

static int Plugin_built(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	impl->observer()->built();

	return 0;
}

static void open_Plugin(lua_State* L) {
	req(
		L,
		array(
			luaL_Reg{
				"Plugin",
				[] (lua_State* L) -> int {
					lib(
						L,
						array(
							luaL_Reg{ "built", Plugin_built }, // Frame synchronized.
							luaL_Reg{ nullptr, nullptr }
						)
					);

					return 1;
				}
			},
			luaL_Reg{ nullptr, nullptr }
		)
	);
}

/**< Project. */

static int Project_write(lua_State* L) {
#if BITTY_TRIAL_ENABLED
	if (isPlugin(L)) {
		error(L, "The \"project:write(...)\" method is not available for trial.");

		return 0;
	}
#endif /* BITTY_TRIAL_ENABLED */

	// Get arguments.
	const int n = getTop(L);
	ProjectPtr* obj = nullptr;
	std::string name;
	Bytes::Ptr* bytes = nullptr;
	bool overwrite = true;
	if (n >= 4)
		read<>(L, obj, name, bytes, overwrite);
	else
		read<>(L, obj, name, bytes);

	// Prepare.
	if (!obj || !bytes)
		return write(L, false);

	const Project* project = obj->get();
	if (!project)
		return write(L, false);

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return write(L, false);

	if (prj->iterating()) {
		error(L, "Cannot write to project while iterating.");

		return write(L, false);
	}

	Asset* asset = prj->get(name.c_str());
	if (asset && !overwrite)
		return write(L, false);

	// Write.
	std::string ext;
	Path::split(name, nullptr, &ext, nullptr);
	const bool add = !asset;
	if (add) {
		asset = prj->factory().create(prj);
		const unsigned type = Asset::typeOf(ext, true);
		asset->link(type, bytes->get(), name.c_str(), nullptr);

		asset->dirty(true);

		prj->add(asset);
		prj->dirty(true);
	} else {
		Asset::States* states = asset->states();
		states->deactivate();
		states->deselect();

		asset->finish((Asset::Usages)(Asset::RUNNING | Asset::EDITING), false);
		asset->unload();
		prj->cleanup((Asset::Usages)(Asset::RUNNING | Asset::EDITING));

		const unsigned type = Asset::typeOf(ext, true);
		asset->link(type, bytes->get(), name.c_str(), nullptr);

		asset->dirty(true);

		prj->dirty(true);
	}

	// Process the asset.
	asset->prepare(Asset::EDITING, false);

	Asset::States* states = asset->states();
	states->activate(Asset::States::EDITABLE);
	states->focus();

	prj->bringToFront(asset);

	// Finish.
	return write(L, true);
}

static void open_Project(lua_State* L) {
	newMeta(L, "Project");
	{
		setTable(L, "write", Project_write);
	}
	pop(L);
}

/**< Categories. */

void plugin(class Executable* exec) {
	// Prepare.
	lua_State* L = (lua_State*)exec->pointer();

	// Plugin.
	open_Plugin(L);

	// Project.
	open_Project(L);
}

}

}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Editor
*/

namespace Bitty {

namespace Lua {

namespace Editor {

/**< Editor. */

static void open_Editor(lua_State* L) {
	def( // Undocumented.
		L, "Editor",
		LUA_LIB(
			array<luaL_Reg>()
		),
		array<luaL_Reg>(),
		array<luaL_Reg>(),
		nullptr, nullptr
	);
}

/**< Categories. */

void plugin(class Executable* exec) {
	// Prepare.
	lua_State* L = (lua_State*)exec->pointer();

	// Editor.
	open_Editor(L);
}

}

}

}

/* ===========================================================================} */
