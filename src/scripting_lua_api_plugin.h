/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __SCRIPTING_LUA_API_PLUGIN_H__
#define __SCRIPTING_LUA_API_PLUGIN_H__

#include "scripting_lua_api.h"

/*
** {===========================================================================
** Application
*/

namespace Lua {

namespace Application {

void plugin(class Executable* exec);

}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Editor
*/

namespace Lua {

namespace Editor {

void plugin(class Executable* exec);

}

}

/* ===========================================================================} */

#endif /* __SCRIPTING_LUA_API_PLUGIN_H__ */
