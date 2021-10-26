/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __SCRIPTING_LUA_API_H__
#define __SCRIPTING_LUA_API_H__

#include "luaxx.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef LUA_CHECK_ALIAS
#	define LUA_CHECK_ALIAS(Y, A) static inline void check(lua_State* L, Y* &ret, Index idx = Index(1), const char* type = #A) { check<>(L, ret, idx, type); }
#endif /* LUA_CHECK_ALIAS */
#ifndef LUA_READ_ALIAS
#	define LUA_READ_ALIAS(Y, A) static inline void read(lua_State* L, Y* &ret, Index idx = Index(1), const char* type = #A) { read<>(L, ret, idx, type); }
#endif /* LUA_READ_ALIAS */
#ifndef LUA_WRITE_ALIAS
#	define LUA_WRITE_ALIAS(Y, A) template<> inline int write(lua_State* L, Y* val) { return writeClass(L, val, #A); }
#endif /* LUA_WRITE_ALIAS */
#ifndef LUA_WRITE_ALIAS_CONST
#	define LUA_WRITE_ALIAS_CONST(Y, A) template<> inline int write(lua_State* L, const Y* val) { return writeClass(L, val, #A); }
#endif /* LUA_WRITE_ALIAS_CONST */

#ifndef LUA_CHECK_CAST
#	define LUA_CHECK_CAST(DST, SRC, CAST) static inline void check(lua_State* L, DST &ret, Index idx = Index(1)) { SRC* src = nullptr; check(L, src, idx); if (src) ret = CAST(*src); }
#endif /* LUA_CHECK_CAST */
#ifndef LUA_READ_CAST
#	define LUA_READ_CAST(DST, SRC, CAST) static inline void read(lua_State* L, DST &ret, Index idx = Index(1)) { SRC* src = nullptr; read(L, src, idx); if (src) ret = CAST(*src); }
#endif /* LUA_READ_CAST */
#ifndef LUA_WRITE_CAST
#	define LUA_WRITE_CAST(DST, SRC, CAST) template<> inline int write(lua_State* L, SRC &val) { DST dst = CAST(val); return write<>(L, &dst); }
#endif /* LUA_WRITE_CAST */
#ifndef LUA_WRITE_CAST_CONST
#	define LUA_WRITE_CAST_CONST(DST, SRC, CAST) template<> inline int write(lua_State* L, const SRC &val) { DST dst = CAST(val); return write<>(L, &dst); }
#endif /* LUA_WRITE_CAST_CONST */

#ifndef LUA_CHECK
#	define LUA_CHECK(Y) LUA_CHECK_ALIAS(Y, Y)
#endif /* LUA_CHECK */
#ifndef LUA_READ
#	define LUA_READ(Y) LUA_READ_ALIAS(Y, Y)
#endif /* LUA_READ */
#ifndef LUA_WRITE
#	define LUA_WRITE(Y) LUA_WRITE_ALIAS(Y, Y)
#endif /* LUA_WRITE */
#ifndef LUA_WRITE_CONST
#	define LUA_WRITE_CONST(Y) LUA_WRITE_ALIAS_CONST(Y, Y)
#endif /* LUA_WRITE_CONST */

#ifndef LUA_CHECK_OBJ
#	define LUA_CHECK_OBJ(Y) LUA_CHECK_ALIAS(Y::Ptr, Y)
#endif /* LUA_CHECK_OBJ */
#ifndef LUA_READ_OBJ
#	define LUA_READ_OBJ(Y) LUA_READ_ALIAS(Y::Ptr, Y)
#endif /* LUA_READ_OBJ */
#ifndef LUA_WRITE_OBJ
#	define LUA_WRITE_OBJ(Y) LUA_WRITE_ALIAS(Y::Ptr, Y)
#endif /* LUA_WRITE_OBJ */
#ifndef LUA_WRITE_OBJ_CONST
#	define LUA_WRITE_OBJ_CONST(Y) LUA_WRITE_ALIAS_CONST(Y::Ptr, Y)
#endif /* LUA_WRITE_OBJ_CONST */

#ifndef LUA_CHECK_REF
#	define LUA_CHECK_REF(Y) static inline void check(lua_State* L, Y::Ptr* &ret, Index idx = Index(1), const char* type = #Y) { check<>(L, ret, idx, type); }
#endif /* LUA_CHECK_REF */
#ifndef LUA_READ_REF
#	define LUA_READ_REF(Y) static inline void read(lua_State* L, Y::Ptr* &ret, Index idx = Index(1), const char* type = #Y) { read<>(L, ret, idx, type); }
#endif /* LUA_READ_REF */
#ifndef LUA_WRITE_REF
#	define LUA_WRITE_REF(Y) template<> inline int write(lua_State* L, Y::Ptr &val) { return writeClass(L, &val, #Y); }
#endif /* LUA_WRITE_REF */
#ifndef LUA_WRITE_REF_CONST
#	define LUA_WRITE_REF_CONST(Y) template<> inline int write(lua_State* L, const Y::Ptr &val) { return writeClass(L, &val, #Y); }
#endif /* LUA_WRITE_REF_CONST */

#ifndef LUA_LIB
#	define LUA_LIB(R) [] (lua_State* L) -> int { Lua::lib(L, R); return 1; }
#endif /* LUA_LIB */

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

namespace Lua {

/**< Structures. */

struct TableOptions {
	bool viewable = false;
	bool includeMetaTable = false;
	int maxLevelCount = 100; // 100 levels.

	TableOptions();
};

/**< Common. */

bool isPlugin(lua_State* L);

/**< Variant. */

void check(lua_State* L, class Variant* ret, Index idx = Index(1), TableOptions options = TableOptions());
template<int Idx = 1> void check(lua_State* L, class Variant* ret) {
	check(L, ret, Index(Idx));
}
void read(lua_State* L, class Variant* ret, Index idx = Index(1), TableOptions options = TableOptions());
template<int Idx = 1> void read(lua_State* L, class Variant* ret) {
	read(L, ret, Index(Idx));
}
template<> int write(lua_State* L, const class Variant* val);
template<> int write(lua_State* L, class Variant* val);

int call(lua_State* L, const Function &func, int argc, const class Variant* argv);
int call(class Variant* ret, lua_State* L, const Function &func);
int call(class Variant* ret, lua_State* L, const Function &func, int argc, const class Variant* argv);
int call(int retc, class Variant* retv, lua_State* L, const Function &func);
int call(int retc, class Variant* retv, lua_State* L, const Function &func, int argc, const class Variant* argv);

}

/* ===========================================================================} */

/*
** {===========================================================================
** Standard
*/

namespace Lua {

namespace Standard {

void open(class Executable* exec);

}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Libraries
*/

namespace Lua {

namespace Libs {

void open(class Executable* exec);

}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Engine
*/

namespace Lua {

namespace Engine {

void open(class Executable* exec);

}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Application
*/

namespace Lua {

namespace Application {

void open(class Executable* exec);

}

}

/* ===========================================================================} */

#endif /* __SCRIPTING_LUA_API_H__ */
