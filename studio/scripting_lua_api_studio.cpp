/*
** Bitty
**
** An itty bitty 2D game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "../src/bitty.h"
#include "../src/bytes.h"
#include "../src/scripting_lua.h"
#include "scripting_lua_api_studio.h"

/*
** {===========================================================================
** Utilities
*/

namespace Lua { // Library.

/**< Bytes. */

LUA_CHECK_OBJ(Bytes)
LUA_READ_OBJ(Bytes)
LUA_WRITE_OBJ(Bytes)
LUA_WRITE_OBJ_CONST(Bytes)

}

/* ===========================================================================} */

/*
** {===========================================================================
** Libraries
*/

namespace Lua {

namespace Libs {

/**< Categories. */

void studio(class Executable* exec) {
	// Prepare.
	lua_State* L = (lua_State*)exec->pointer();

	(void)L;
}

}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Engine
*/

namespace Lua {

namespace Engine {

/**< Categories. */

void studio(class Executable* exec) {
	// Prepare.
	lua_State* L = (lua_State*)exec->pointer();

	(void)L;
}

}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Application
*/

namespace Lua {

namespace Application {

/**< Categories. */

void studio(class Executable* exec) {
	// Prepare.
	lua_State* L = (lua_State*)exec->pointer();

	(void)L;
}

}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Studio
*/

namespace Lua {

namespace Studio {

/**< Categories. */

void studio(class Executable* exec) {
	// Prepare.
	lua_State* L = (lua_State*)exec->pointer();

	(void)L;
}

}

}

/* ===========================================================================} */
