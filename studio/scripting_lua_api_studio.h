/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __SCRIPTING_LUA_API_STUDIO_H__
#define __SCRIPTING_LUA_API_STUDIO_H__

#include "../src/scripting_lua_api.h"

/*
** {===========================================================================
** Libraries
*/

namespace Lua {

namespace Libs {

void studio(class Executable* exec);

}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Engine
*/

namespace Lua {

namespace Engine {

void studio(class Executable* exec);

}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Application
*/

namespace Lua {

namespace Application {

void studio(class Executable* exec);

}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Studio
*/

namespace Lua {

namespace Studio {

void studio(class Executable* exec);

}

}

/* ===========================================================================} */

#endif /* __SCRIPTING_LUA_API_STUDIO_H__ */
