/*
** Bitty
**
** An itty bitty 2D game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
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

namespace Bitty {

namespace Lua {

namespace Libs {

void studio(class Executable* exec);

}

}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Engine
*/

namespace Bitty {

namespace Lua {

namespace Engine {

void studio(class Executable* exec);

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

void studio(class Executable* exec);

}

}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Studio
*/

namespace Bitty {

namespace Lua {

namespace Studio {

void studio(class Executable* exec);

}

}

}

/* ===========================================================================} */

#endif /* __SCRIPTING_LUA_API_STUDIO_H__ */
