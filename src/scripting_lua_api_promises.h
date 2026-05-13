/*
** Bitty
**
** An itty bitty 2D game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __SCRIPTING_LUA_API_PROMISES_H__
#define __SCRIPTING_LUA_API_PROMISES_H__

#include "scripting_lua_api.h"

/*
** {===========================================================================
** Standard
*/

namespace Bitty {

namespace Lua {

namespace Standard {

void promise(class Executable* exec);

}

}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Libraries
*/

namespace Bitty {

namespace Lua {

namespace Libs {

void promise(class Executable* exec);

}

}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Application
*/

namespace Bitty {

namespace Lua {

namespace Application {

void promise(class Executable* exec);

}

}

}

/* ===========================================================================} */

#endif /* __SCRIPTING_LUA_API_PROMISES_H__ */
