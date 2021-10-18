/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "archive.h"
#include "bytes.h"
#include "code.h"
#include "datetime.h"
#include "editable.h"
#include "effects.h"
#include "encoding.h"
#include "file_handle.h"
#include "filesystem.h"
#include "network.h"
#include "noiser.h"
#include "pathfinder.h"
#include "platform.h"
#include "primitives.h"
#include "project.h"
#include "randomizer.h"
#include "raycaster.h"
#include "renderer.h"
#include "scripting_lua.h"
#include "scripting_lua_api.h"
#include "walker.h"
#include "web.h"
#include "window.h"
#include "resource/inline_resource.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#if defined BITTY_OS_HTML
#	include <emscripten.h>
#endif /* BITTY_OS_HTML */

/*
** {===========================================================================
** Macros and constants
*/

static_assert(sizeof(Int64) == sizeof(lua_Integer), "Wrong size.");
static_assert(sizeof(UInt64) == sizeof(lua_Unsigned), "Wrong size.");
static_assert(sizeof(Double) == sizeof(lua_Number), "Wrong size.");

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

namespace Lua { // Library.

/**< Algorithms. */

LUA_CHECK_OBJ(Noiser)
LUA_READ_OBJ(Noiser)
LUA_WRITE_OBJ(Noiser)
LUA_WRITE_OBJ_CONST(Noiser)

LUA_CHECK_OBJ(Pathfinder)
LUA_READ_OBJ(Pathfinder)
LUA_WRITE_OBJ(Pathfinder)
LUA_WRITE_OBJ_CONST(Pathfinder)

LUA_CHECK_ALIAS(Randomizer::Ptr, Random)
LUA_READ_ALIAS(Randomizer::Ptr, Random)
LUA_WRITE_ALIAS(Randomizer::Ptr, Random)
LUA_WRITE_ALIAS_CONST(Randomizer::Ptr, Random)

LUA_CHECK_OBJ(Raycaster)
LUA_READ_OBJ(Raycaster)
LUA_WRITE_OBJ(Raycaster)
LUA_WRITE_OBJ_CONST(Raycaster)

LUA_CHECK_OBJ(Walker)
LUA_READ_OBJ(Walker)
LUA_WRITE_OBJ(Walker)
LUA_WRITE_OBJ_CONST(Walker)

/**< Archive. */

LUA_CHECK_OBJ(Archive)
LUA_READ_OBJ(Archive)
LUA_WRITE_OBJ(Archive)
LUA_WRITE_OBJ_CONST(Archive)

/**< Bytes. */

LUA_CHECK_OBJ(Bytes)
LUA_READ_OBJ(Bytes)
LUA_WRITE_OBJ(Bytes)
LUA_WRITE_OBJ_CONST(Bytes)

/**< Color. */

LUA_CHECK(Color)
LUA_READ(Color)
LUA_WRITE(Color)
LUA_WRITE_CONST(Color)

/**< File. */

LUA_CHECK_OBJ(File)
LUA_READ_OBJ(File)
LUA_WRITE_OBJ(File)
LUA_WRITE_OBJ_CONST(File)

/**< Filesystem. */

LUA_CHECK_OBJ(FileInfo)
LUA_READ_OBJ(FileInfo)
LUA_WRITE_OBJ(FileInfo)
LUA_WRITE_OBJ_CONST(FileInfo)

LUA_CHECK_OBJ(DirectoryInfo)
LUA_READ_OBJ(DirectoryInfo)
LUA_WRITE_OBJ(DirectoryInfo)
LUA_WRITE_OBJ_CONST(DirectoryInfo)

LUA_WRITE_CAST(FileInfo::Ptr, FileInfo::Ptr, [] (const FileInfo::Ptr &ptr) -> FileInfo::Ptr { return ptr; })
LUA_WRITE_CAST_CONST(FileInfo::Ptr, FileInfo::Ptr, [] (const FileInfo::Ptr &ptr) -> FileInfo::Ptr { return ptr; })

LUA_WRITE_CAST(DirectoryInfo::Ptr, DirectoryInfo::Ptr, [] (const DirectoryInfo::Ptr &ptr) -> DirectoryInfo::Ptr { return ptr; })
LUA_WRITE_CAST_CONST(DirectoryInfo::Ptr, DirectoryInfo::Ptr, [] (const DirectoryInfo::Ptr &ptr) -> DirectoryInfo::Ptr { return ptr; })

/**< Image. */

LUA_CHECK_OBJ(Image)
LUA_READ_OBJ(Image)
LUA_WRITE_OBJ(Image)
LUA_WRITE_OBJ_CONST(Image)

/**< JSON. */

LUA_CHECK_OBJ(Json)
LUA_READ_OBJ(Json)
LUA_WRITE_OBJ(Json)
LUA_WRITE_OBJ_CONST(Json)

/**< Math. */

LUA_CHECK_ALIAS(Math::Vec2f, Vec2)
LUA_READ_ALIAS(Math::Vec2f, Vec2)
LUA_WRITE_ALIAS(Math::Vec2f, Vec2)
LUA_WRITE_ALIAS_CONST(Math::Vec2f, Vec2)

LUA_CHECK_ALIAS(Math::Vec3f, Vec3)
LUA_READ_ALIAS(Math::Vec3f, Vec3)
LUA_WRITE_ALIAS(Math::Vec3f, Vec3)
LUA_WRITE_ALIAS_CONST(Math::Vec3f, Vec3)

LUA_CHECK_ALIAS(Math::Vec4f, Vec4)
LUA_READ_ALIAS(Math::Vec4f, Vec4)
LUA_WRITE_ALIAS(Math::Vec4f, Vec4)
LUA_WRITE_ALIAS_CONST(Math::Vec4f, Vec4)

LUA_CHECK_ALIAS(Math::Rectf, Rect)
LUA_READ_ALIAS(Math::Rectf, Rect)
LUA_WRITE_ALIAS(Math::Rectf, Rect)
LUA_WRITE_ALIAS_CONST(Math::Rectf, Rect)

LUA_CHECK_ALIAS(Math::Recti, Recti)
LUA_READ_ALIAS(Math::Recti, Recti)
LUA_WRITE_ALIAS(Math::Recti, Recti)
LUA_WRITE_ALIAS_CONST(Math::Recti, Recti)

LUA_CHECK_ALIAS(Math::Rotf, Rot)
LUA_READ_ALIAS(Math::Rotf, Rot)
LUA_WRITE_ALIAS(Math::Rotf, Rot)
LUA_WRITE_ALIAS_CONST(Math::Rotf, Rot)

LUA_CHECK_CAST(Math::Vec2i, Math::Vec2f, [] (const Math::Vec2f &val) -> Math::Vec2i { return Math::Vec2i((Int)val.x, (Int)val.y); })
LUA_READ_CAST(Math::Vec2i, Math::Vec2f, [] (const Math::Vec2f &val) -> Math::Vec2i { return Math::Vec2i((Int)val.x, (Int)val.y); })
LUA_WRITE_CAST(Math::Vec2f, Math::Vec2i, [] (const Math::Vec2i &val) -> Math::Vec2f { return Math::Vec2f(val.x, val.y); })
LUA_WRITE_CAST_CONST(Math::Vec2f, Math::Vec2i, [] (const Math::Vec2i &val) -> Math::Vec2f { return Math::Vec2f(val.x, val.y); })

LUA_CHECK_CAST(Math::Vec3i, Math::Vec3f, [] (const Math::Vec3f &val) -> Math::Vec3i { return Math::Vec3i((Int)val.x, (Int)val.y, (Int)val.z); })
LUA_READ_CAST(Math::Vec3i, Math::Vec3f, [] (const Math::Vec3f &val) -> Math::Vec3i { return Math::Vec3i((Int)val.x, (Int)val.y, (Int)val.z); })
LUA_WRITE_CAST(Math::Vec3f, Math::Vec3i, [] (const Math::Vec3i &val) -> Math::Vec3f { return Math::Vec3f(val.x, val.y, val.z); })
LUA_WRITE_CAST_CONST(Math::Vec3f, Math::Vec3i, [] (const Math::Vec3i &val) -> Math::Vec3f { return Math::Vec3f(val.x, val.y, val.z); })

LUA_CHECK_CAST(Math::Vec4i, Math::Vec4f, [] (const Math::Vec4f &val) -> Math::Vec4i { return Math::Vec4i((Int)val.x, (Int)val.y, (Int)val.z, (Int)val.w); })
LUA_READ_CAST(Math::Vec4i, Math::Vec4f, [] (const Math::Vec4f &val) -> Math::Vec4i { return Math::Vec4i((Int)val.x, (Int)val.y, (Int)val.z, (Int)val.w); })
LUA_WRITE_CAST(Math::Vec4f, Math::Vec4i, [] (const Math::Vec4i &val) -> Math::Vec4f { return Math::Vec4f(val.x, val.y, val.z, val.w); })
LUA_WRITE_CAST_CONST(Math::Vec4f, Math::Vec4i, [] (const Math::Vec4i &val) -> Math::Vec4f { return Math::Vec4f(val.x, val.y, val.z, val.w); })

/**< Network. */

#if BITTY_NETWORK_ENABLED

LUA_CHECK_OBJ(Network)
LUA_READ_OBJ(Network)
LUA_WRITE_OBJ(Network)
LUA_WRITE_OBJ_CONST(Network)

#endif /* BITTY_NETWORK_ENABLED */

/**< Web. */

#if BITTY_WEB_ENABLED

LUA_CHECK_OBJ(Web)
LUA_READ_OBJ(Web)
LUA_WRITE_OBJ(Web)
LUA_WRITE_OBJ_CONST(Web)

#endif /* BITTY_WEB_ENABLED */

}

namespace Lua { // Engine.

/**< Resources. */

LUA_CHECK_ALIAS(Resources::Asset::Ptr, Asset)
LUA_READ_ALIAS(Resources::Asset::Ptr, Asset)
LUA_WRITE_ALIAS(Resources::Asset::Ptr, Asset)
LUA_WRITE_ALIAS_CONST(Resources::Asset::Ptr, Asset)

LUA_CHECK_ALIAS(Resources::Palette::Ptr, Palette)
LUA_READ_ALIAS(Resources::Palette::Ptr, Palette)
LUA_WRITE_ALIAS(Resources::Palette::Ptr, Palette)
LUA_WRITE_ALIAS_CONST(Resources::Palette::Ptr, Palette)

LUA_CHECK_ALIAS(Resources::Texture::Ptr, Texture)
LUA_READ_ALIAS(Resources::Texture::Ptr, Texture)
LUA_WRITE_ALIAS(Resources::Texture::Ptr, Texture)
LUA_WRITE_ALIAS_CONST(Resources::Texture::Ptr, Texture)

LUA_CHECK_ALIAS(Resources::Sprite::Ptr, Sprite)
LUA_READ_ALIAS(Resources::Sprite::Ptr, Sprite)
LUA_WRITE_ALIAS(Resources::Sprite::Ptr, Sprite)
LUA_WRITE_ALIAS_CONST(Resources::Sprite::Ptr, Sprite)

LUA_CHECK_ALIAS(Resources::Map::Ptr, Map)
LUA_READ_ALIAS(Resources::Map::Ptr, Map)
LUA_WRITE_ALIAS(Resources::Map::Ptr, Map)
LUA_WRITE_ALIAS_CONST(Resources::Map::Ptr, Map)

LUA_CHECK_ALIAS(Resources::Sfx::Ptr, Sfx)
LUA_READ_ALIAS(Resources::Sfx::Ptr, Sfx)
LUA_WRITE_ALIAS(Resources::Sfx::Ptr, Sfx)
LUA_WRITE_ALIAS_CONST(Resources::Sfx::Ptr, Sfx)

LUA_CHECK_ALIAS(Resources::Music::Ptr, Music)
LUA_READ_ALIAS(Resources::Music::Ptr, Music)
LUA_WRITE_ALIAS(Resources::Music::Ptr, Music)
LUA_WRITE_ALIAS_CONST(Resources::Music::Ptr, Music)

/**< Palette. */

LUA_CHECK_OBJ(Palette)
LUA_READ_OBJ(Palette)
LUA_WRITE_OBJ(Palette)
LUA_WRITE_OBJ_CONST(Palette)

/**< Font. */

LUA_CHECK_OBJ(Font)
LUA_READ_OBJ(Font)
LUA_WRITE_OBJ(Font)
LUA_WRITE_OBJ_CONST(Font)

}

namespace Lua { // Application.

/**< Canvas. */

typedef ::Primitives Canvas;
typedef std::shared_ptr<Canvas> CanvasPtr;
LUA_CHECK_ALIAS(CanvasPtr, Canvas)
LUA_READ_ALIAS(CanvasPtr, Canvas)
LUA_WRITE_ALIAS(CanvasPtr, Canvas)
LUA_WRITE_ALIAS_CONST(CanvasPtr, Canvas)

/**< Project. */

typedef std::shared_ptr<const Project> ProjectPtr;
LUA_CHECK_ALIAS(ProjectPtr, Project)
LUA_READ_ALIAS(ProjectPtr, Project)
LUA_WRITE_ALIAS(ProjectPtr, Project)
LUA_WRITE_ALIAS_CONST(ProjectPtr, Project)

}

namespace Lua { // Generic.

/**< Structures. */

TableOptions::TableOptions() {
}

/**< Common. */

class References {
private:
	typedef std::list<uintptr_t> List;

private:
	List _list;

public:
	References() {
	}

	uintptr_t back(void) const {
		if (_list.empty())
			return 0;

		return _list.back();
	}
	bool contains(uintptr_t ptr) const {
		return std::find(_list.begin(), _list.end(), ptr) != _list.end();
	}
	bool add(uintptr_t ptr) {
		if (std::find(_list.begin(), _list.end(), ptr) != _list.end())
			return false;

		_list.push_back(ptr);

		return true;
	}
	bool remove(uintptr_t ptr) {
		List::iterator it = std::find(_list.begin(), _list.end(), ptr);
		if (it == _list.end())
			return false;

		_list.erase(it);

		return true;
	}
};

bool isPlugin(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	return !!impl->editing();
}

/**< Variant. */

static void checkOrRead(lua_State* L, Variant* ret, Index idx, References &refs, bool check, int level, const TableOptions &options) {
	*ret = nullptr;

	const int type = lua_type(L, idx);
	switch (type) {
	case LUA_TNONE:
		*ret = nullptr;

		break;
	case LUA_TNIL:
		*ret = nullptr;

		break;
	case LUA_TBOOLEAN:
		*ret = !!lua_toboolean(L, idx);

		break;
	case LUA_TLIGHTUSERDATA:
		*ret = Object::Ptr();

		break;
	case LUA_TNUMBER:
		*ret = lua_tonumber(L, idx);

		break;
	case LUA_TSTRING:
		*ret = lua_tostring(L, idx);

		break;
	case LUA_TTABLE: {
			if (level > options.maxLevelCount)
				break;

			const uintptr_t ref = (uintptr_t)lua_topointer(L, idx);
			if (refs.contains(ref)) {
				if (options.viewable) {
					if (refs.back() == ref) {
						static constexpr const char* ctype = "self";
						*ret = (void*)ctype;
					} else {
						static constexpr const char* ctype = "*recursion";
						*ret = (void*)ctype;
					}

					break;
				}

				if (check)
					error(L, "Unsupported reference cycle.");

				break;
			}
			refs.add(ref);

			if (isArray(L, idx)) {
				IList::Ptr lst(List::create());
				*ret = (Object::Ptr)lst;

				const lua_Unsigned n = len(L, idx);
				for (int i = 1; i <= (int)n; ++i) { // 1-based.
					write(L, i);
					get(L, -2, i);

					Variant v = nullptr;
					checkOrRead(L, &v, Index(-1), refs, check, level + 1, options);

					lst->add(v);

					pop(L, 2);
				}
			} else {
				IDictionary::Ptr dict(Dictionary::create());
				*ret = (Object::Ptr)dict;

				int unknownIndex = 1;
				write(L, nullptr); // Before: ...table (top); after: ...table, nil (top).
				if (idx < 0)
					--idx;
				while (next(L, idx)) {
					std::string k; // Stack: table, key, value (top).
					if (options.viewable) {
						const int y = typeOf(L, -2);
						switch (y) {
						case LUA_TBOOLEAN: {
								bool val = false;
								read(L, val, Index(-2));
								if (val)
									k = "true";
								else
									k = "false";
							}

							break;
						case LUA_TLIGHTUSERDATA: {
								LightUserdata val;
								read(L, val, Index(-2));
								if (val.data == nullptr) {
									k = "null";
								} else {
									constexpr bool IS32BIT = sizeof(uintptr_t) == sizeof(UInt32);
									(void)IS32BIT;
#if IS32BIT
									k = "0x" + Text::toHex((UInt32)(uintptr_t)val.data, false);
#else /* IS32BIT */
									k = "0x" + Text::toHex((UInt64)(uintptr_t)val.data, false);
#endif /* IS32BIT */
								}
							}

							break;
						case LUA_TNUMBER:
							if (isInteger(L, -2)) {
								lua_Integer val = 0;
								read(L, val, Index(-2));
								k = Text::toString((Int64)val);
							} else {
								lua_Number val = 0;
								read(L, val, Index(-2));
								k = Text::toString(val);
							}

							break;
						case LUA_TSTRING:
							read(L, k, Index(-2));

							break;
						case LUA_TTABLE:
							k = "table (" + Text::toString(unknownIndex++) + ")";

							break;
						case LUA_TFUNCTION:
							k = "function (" + Text::toString(unknownIndex++) + ")";

							break;
						case LUA_TUSERDATA:
							k = "userdata (" + Text::toString(unknownIndex++) + ")";

							break;
						case LUA_TTHREAD:
							k = "thread (" + Text::toString(unknownIndex++) + ")";

							break;
						default:
							k = "unknown (" + Text::toString(unknownIndex++) + ")";

							break;
						}
					} else {
						read(L, k, Index(-2));
					}
					Variant v = nullptr;
					checkOrRead(L, &v, Index(-1), refs, check, level + 1, options);

					dict->set(k, v);

					pop(L); // Pop value, leaving the key.
							// Stack: table, key (top).
				} // Stack: table (top), when next returns 0 it pops the key, but does not push anything.

				if (options.includeMetaTable) {
					if (getMetaOf(L, -1)) {
						const std::string k = "(metatable)";
						Variant v = nullptr;
						checkOrRead(L, &v, Index(-1), refs, check, level + 1, options);

						dict->set(k, v);
	
						pop(L);
					}
				}
			}

			refs.remove(ref);
		}

		break;
	case LUA_TFUNCTION:
		if (options.viewable) {
			static constexpr const char* ctype = "function";
			*ret = (void*)ctype;

			break;
		}

		if (check)
			error(L, "Unsupported function.");

		break;
	case LUA_TUSERDATA:
		if (options.viewable) {
			size_t len = 0;
			const char* str = toString(L, idx, &len); // Convert it to string.
			if (str) {
				*ret = str;

				pop(L); // Pop result.
			} else {
				static constexpr const char* ctype = "userdata";
				*ret = (void*)ctype;
			}

			break;
		}

		*ret = Object::Ptr();

		break;
	case LUA_TTHREAD:
		if (options.viewable) {
			static constexpr const char* ctype = "thread";
			*ret = (void*)ctype;

			break;
		}

		if (check)
			error(L, "Unsupported thread.");

		break;
	default:
		assert(false && "Impossible.");

		break;
	}
}

void check(lua_State* L, class Variant* ret, Index idx, TableOptions options) {
	References refs;
	checkOrRead(L, ret, idx, refs, true, 1, options);
}

void read(lua_State* L, class Variant* ret, Index idx, TableOptions options) {
	References refs;
	checkOrRead(L, ret, idx, refs, false, 1, options);
}

static int write_(lua_State* L, const Variant* val, References &refs) {
	switch (val->type()) {
	case Variant::NIL:
		lua_pushnil(L);

		return 1;
	case Variant::BOOLEAN:
		lua_pushboolean(L, (bool)*val ? 1 : 0);

		return 1;
	case Variant::INTEGER:
		lua_pushinteger(L, (lua_Integer)(Int)*val);

		return 1;
	case Variant::REAL:
		lua_pushnumber(L, (lua_Number)(Real)*val);

		return 1;
	case Variant::STRING:
		lua_pushstring(L, (const char*)*val);

		return 1;
	case Variant::POINTER: // Do nothing.
		return 0;
	case Variant::OBJECT: {
			const uintptr_t ref = (uintptr_t)val;
			if (refs.contains(ref)) {
				break;
			}
			refs.add(ref);

			Object::Ptr obj = (Object::Ptr)*val;
			if (Object::is<IList::Ptr>(obj)) {
				IList::Ptr lst = Object::as<IList::Ptr>(obj);
				if (!lst)
					break;
				newTable(L, lst->count());

				for (int i = 0; i < lst->count(); ++i) {
					Variant elem = lst->at(i);

					write_(L, &elem, refs);

					setTable(L, i + 1); // 1-based.
				}
			} else if (Object::is<IDictionary::Ptr>(obj)) {
				IDictionary::Ptr dict = Object::as<IDictionary::Ptr>(obj);
				if (!dict)
					break;
				newTable(L);

				IDictionary::Keys keys = dict->keys();
				for (const std::string &key : keys) {
					Variant elem = dict->get(key);

					write_(L, &elem, refs);

					setTable(L, key);
				}
			}

			refs.remove(ref);
		}

		return 1;
	}

	lua_pushnil(L);

	return 1;
}

template<> int write(lua_State* L, const class Variant* val) {
	References refs;

	return write_(L, val, refs);
}

template<> int write(lua_State* L, class Variant* val) {
	References refs;

	return write_(L, val, refs);
}

int call(lua_State* L, const Function &func, int argc, const class Variant* argv) {
	function(L, func);
	for (int i = 0; i < argc; ++i)
		write(L, &argv[i]);
	const int result = invoke(L, argc, 0);
	if (result == LUA_OK || result == LUA_YIELD)
		end(L);

	return result;
}

int call(class Variant* ret, lua_State* L, const Function &func) {
	function(L, func);
	const int result = invoke(L, 0, 1);
	if (result == LUA_OK || result == LUA_YIELD) {
		check(L, ret, Index(-1));
		end(L);
	}

	return result;
}

int call(class Variant* ret, lua_State* L, const Function &func, int argc, const class Variant* argv) {
	function(L, func);
	for (int i = 0; i < argc; ++i)
		write(L, &argv[i]);
	const int result = invoke(L, argc, 1);
	if (result == LUA_OK || result == LUA_YIELD) {
		check(L, ret, Index(-1));
		end(L);
	}

	return result;
}

int call(int retc, class Variant* retv, lua_State* L, const Function &func) {
	function(L, func);
	const int result = invoke(L, 0, retc);
	if (result == LUA_OK || result == LUA_YIELD) {
		for (int i = 0; i < retc; ++i)
			check(L, &retv[i], Index(-retc + i));
		end(L);
	}

	return result;
}

int call(int retc, class Variant* retv, lua_State* L, const Function &func, int argc, const class Variant* argv) {
	function(L, func);
	for (int i = 0; i < argc; ++i)
		write(L, &argv[i]);
	const int result = invoke(L, argc, retc);
	if (result == LUA_OK || result == LUA_YIELD) {
		for (int i = 0; i < retc; ++i)
			check(L, &retv[i], Index(-retc + i));
		end(L);
	}

	return result;
}

/**< JSON. */

static void read_(lua_State* L, rapidjson::Value &val, Index idx, rapidjson::MemoryPoolAllocator<> &allocator, References &refs) {
	switch (typeOf(L, idx)) {
	case LUA_TNUMBER: {
			if (isInteger(L, idx)) {
				lua_Integer data = 0;
				read(L, data, idx);
				val.SetInt64(data);
			} else {
				lua_Number data = 0;
				read(L, data, idx);
				val.SetDouble(data);
			}
		}

		break;
	case LUA_TBOOLEAN: {
			bool data = true;
			read(L, data, idx);
			val.SetBool(data);
		}

		break;
	case LUA_TNIL:
		val.SetNull();

		break;
	case LUA_TSTRING: {
			const char* data = nullptr;
			read(L, data, idx);
			val.SetString(data, allocator);
		}

		break;
	case LUA_TTABLE: {
			const uintptr_t ref = (uintptr_t)lua_topointer(L, idx);
			if (refs.contains(ref)) {
				error(L, "Unsupported reference cycle.");

				break;
			}
			refs.add(ref);

			if (isArray(L, idx)) {
				val.SetArray();

				const lua_Unsigned n = len(L, idx);
				for (int i = 1; i <= (int)n; ++i) { // 1-based.
					write(L, i);
					get(L, -2, i);

					rapidjson::Value jv;
					read_(L, jv, Index(-1), allocator, refs);

					val.PushBack(jv, allocator);

					pop(L, 2);
				}
			} else {
				val.SetObject();

				write(L, nullptr); // Before: ...table (top); after: ...table, nil (top).
				if (idx < 0)
					--idx;
				while (next(L, idx)) {
					rapidjson::Value jk; // Stack: table, key, value (top).
					rapidjson::Value jv;
					std::string k;
					read(L, k, Index(-2));
					jk.SetString(k.c_str(), allocator);
					read_(L, jv, Index(-1), allocator, refs);

					val.AddMember(jk, jv, allocator);

					pop(L); // Pop value, leaving the key.
							// Stack: table, key (top).
				} // Stack: table (top), when next returns 0 it pops the key, but does not push anything.
			}

			refs.remove(ref);
		}

		break;
	case LUA_TUSERDATA: {
			LightUserdata data;
			if (isLightuserdata(L, idx)) {
				read(L, data, idx);

				if (data.data == nullptr)
					val.SetNull();
			}
		}

		break;
	}
}

static void read(lua_State* L, rapidjson::Value &val, Index idx, rapidjson::MemoryPoolAllocator<> &allocator) {
	References refs;
	read_(L, val, idx, allocator, refs);
}

static void read(lua_State* L, rapidjson::Document &doc, Index idx) {
	rapidjson::Value &val = doc;
	read(L, val, idx, doc.GetAllocator());
}

static int write_(lua_State* L, const rapidjson::Value &val, bool allowNull) {
	switch (val.GetType()) {
	case rapidjson::kNullType:
		if (allowNull) {
			const LightUserdata Null;
			write(L, Null);
		} else {
			write(L, nullptr);
		}

		return 1;
	case rapidjson::kFalseType:
		write(L, false);

		return 1;
	case rapidjson::kTrueType:
		write(L, true);

		return 1;
	case rapidjson::kObjectType: {
			rapidjson::Value::ConstObject jobj = val.GetObject();
			newTable(L);

			for (rapidjson::Value::ConstMemberIterator it = jobj.MemberBegin(); it != jobj.MemberEnd(); ++it) {
				const rapidjson::Value &jk = it->name;
				const rapidjson::Value &jv = it->value;

				write_(L, jv, allowNull);

				setTable(L, jk.GetString());
			}
		}

		return 1;
	case rapidjson::kArrayType: {
			rapidjson::Value::ConstArray jarr = val.GetArray();
			newTable(L, jarr.Size());

			for (rapidjson::SizeType i = 0; i < jarr.Size(); ++i) {
				const rapidjson::Value &ji = jarr[i];

				write_(L, ji, allowNull);

				setTable(L, i + 1); // 1-based.
			}
		}

		return 1;
	case rapidjson::kStringType:
		write(L, val.GetString());

		return 1;
	case rapidjson::kNumberType: {
			if (val.IsInt())
				write(L, val.GetInt());
			else if (val.IsInt64())
				write(L, val.GetInt64());
			else if (val.IsUint())
				write(L, val.GetUint());
			else if (val.IsUint64())
				write(L, val.GetUint64());
			else if (val.IsFloat())
				write(L, val.GetFloat());
			else
				write(L, val.GetDouble());
		}

		return 1;
	}

	return 0;
}

template<> int write(lua_State* L, const rapidjson::Value &val) {
	return write_(L, val, false);
}

template<> int write(lua_State* L, rapidjson::Value &val) {
	return write_(L, val, false);
}

template<> int write(lua_State* L, const rapidjson::Document &doc) {
	const rapidjson::Value &val = doc;

	return write_(L, val, false);
}

template<> int write(lua_State* L, rapidjson::Document &doc) {
	rapidjson::Value &val = doc;

	return write_(L, val, false);
}

/**< Walker. */

template<typename Func, typename ...Args> int call(Walker::Blocking &ret, lua_State* L, const Func &func, const Args &...args) {
	function(L, func);
	write(L, args...);
	const int result = invoke(L, sizeof...(Args), 2);
	if (result == LUA_OK || result == LUA_YIELD) {
		check(L, ret.block, Index(-2));
		read(L, ret.pass, Index(-1));
		end(L);
	}

	return result;
}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Standard
*/

namespace Lua {

namespace Standard {

/**< Builtin. */

static void open_Builtin(lua_State* L) {
	req(
		L,
		array(
			luaL_Reg{ LUA_GNAME, luaopen_base },
			luaL_Reg{ LUA_LOADLIBNAME, luaopen_package },
			luaL_Reg{ LUA_COLIBNAME, luaopen_coroutine },
			luaL_Reg{ LUA_TABLIBNAME, luaopen_table },
			// luaL_Reg{ LUA_IOLIBNAME, luaopen_io },
			// luaL_Reg{ LUA_OSLIBNAME, luaopen_os },
			luaL_Reg{ LUA_STRLIBNAME, luaopen_string },
			luaL_Reg{ LUA_MATHLIBNAME, luaopen_math },
			luaL_Reg{ LUA_UTF8LIBNAME, luaopen_utf8 },
			luaL_Reg{ LUA_DBLIBNAME, luaopen_debug },
			luaL_Reg{ nullptr, nullptr }
		)
	);
}

/**< Standard. */

enum MessageTypes {
	PRINT,
	WARN,
	ERROR
};

static int message(lua_State* L, const char* msg, MessageTypes type) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	switch (type) {
	case PRINT:
		impl->observer()->print(msg);

		break;
	case WARN:
		impl->observer()->warn(msg);

		break;
	case ERROR:
		impl->observer()->error(msg);

		break;
	}

	return 0;
}
static int message(lua_State* L, MessageTypes type) {
	std::string msg;

	const int n = getTop(L); // Number of arguments.
	for (int i = 1; i <= n; ++i) {
		size_t len = 0;
		const char* str = toString(L, i, &len); // Convert it to string.
		if (!str)
			return error(L, "`tostring` must return a string to `print`.");

		msg += str;
		if (i > 1)
			msg += "\t";

		pop(L); // Pop result.
	}

	return message(L, msg.c_str(), type);
}

static int print(lua_State* L) {
	return message(L, PRINT);
}

static int warn(lua_State* L) {
	return message(L, WARN);
}

static int collectgarbage(lua_State* L) {
	// See: `luaB_collectgarbage` in "./lib/lua/src/lbaselib.c".
	constexpr const char* const OPTIONS[] = {
		"stop", "restart", "collect",
		"count", "step", "setpause", "setstepmul",
		"isrunning", "generational", "incremental",
		nullptr
	};
	constexpr const int OPTION_VALUES[] = {
		LUA_GCSTOP, LUA_GCRESTART, LUA_GCCOLLECT,
		LUA_GCCOUNT, LUA_GCSTEP, LUA_GCSETPAUSE, LUA_GCSETSTEPMUL,
		LUA_GCISRUNNING, LUA_GCGEN, LUA_GCINC
	};
	const int opt = OPTION_VALUES[luaL_checkoption(L, 1, "collect", OPTIONS)];
	switch (opt) {
	case LUA_GCCOUNT: {
			const int k = gc(L, opt);
			const int b = gc(L, LUA_GCCOUNTB);

			return write(L, (lua_Number)k + ((lua_Number)b / 1024));
		}
	case LUA_GCSTEP: {
			const int step = (int)luaL_optinteger(L, 2, 0);
			const bool ret = !!gc(L, opt, step);

			return write(L, ret);
		}
	case LUA_GCSETPAUSE: // Fall through.
	case LUA_GCSETSTEPMUL: {
			const int p = (int)luaL_optinteger(L, 2, 0);
			const int previous = gc(L, opt, p);

			return write(L, previous);
		}
	case LUA_GCISRUNNING: {
			const bool ret = !!gc(L, opt);

			return write(L, ret);
		}
	case LUA_GCGEN: {
			const int minormul = (int)luaL_optinteger(L, 2, 0);
			const int majormul = (int)luaL_optinteger(L, 3, 0);
			const int oldmode = gc(L, opt, minormul, majormul);

			return write(L, (oldmode == LUA_GCINC) ? "incremental" : "generational");
		}
	case LUA_GCINC: {
			const int pause = (int)luaL_optinteger(L, 2, 0);
			const int stepmul = (int)luaL_optinteger(L, 3, 0);
			const int stepsize = (int)luaL_optinteger(L, 4, 0);
			const int oldmode = gc(L, opt, pause, stepmul, stepsize);

			return write(L, (oldmode == LUA_GCINC) ? "incremental" : "generational");
		}
	case LUA_GCSTOP:
		message(L, "GC stopped.", WARN);
		// Fall through.
	default: {
			const int ret = gc(L, opt);

			return write(L, ret);
		}
	}
}

static int exit(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const bool ret = impl->exit();

	return write(L, ret);
}

static void open_Standard(lua_State* L) {
	reg(
		L,
		array(
			luaL_Reg{ "print", print },
			luaL_Reg{ "warn", warn },
			luaL_Reg{ "collectgarbage", collectgarbage },
			luaL_Reg{ "exit", exit },
			luaL_Reg{ nullptr, nullptr }
		)
	);
}

/**< Categories. */

void open(class Executable* exec) {
	// Prepare.
	lua_State* L = (lua_State*)exec->pointer();

	// Builtin.
	open_Builtin(L);

	// Standard.
	open_Standard(L);
}

}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Libraries
*/

namespace Lua {

namespace Libs {

/**< Light userdata. */

static int LightUserdata_toString(lua_State* L) {
	LightUserdata data;
	if (isLightuserdata(L, 1)) {
		read(L, data, Index(1));

		std::string ret;
		if (data.data == nullptr) {
			ret = "null";
		} else {
			constexpr bool IS32BIT = sizeof(uintptr_t) == sizeof(UInt32);
			(void)IS32BIT;
#if IS32BIT
			ret = "0x" + Text::toHex((UInt32)(uintptr_t)data.data, false);
#else /* IS32BIT */
			ret = "0x" + Text::toHex((UInt64)(uintptr_t)data.data, false);
#endif /* IS32BIT */
		}

		return write(L, ret);
	} else {
		return write(L, "unknown");
	}
}

static void open_LightUserdata(lua_State* L) {
	def(
		L, "LightUserdata",
		nullptr,
		array(
			luaL_Reg{ "__tostring", LightUserdata_toString },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ nullptr, nullptr }
		),
		nullptr, nullptr
	);

	const LightUserdata lightUserdata;
	write(L, lightUserdata);
	setMetaOf(L, "LightUserdata");
	pop(L);
}

/**< Algorithms. */

static int Noiser_ctor(lua_State* L) {
	Noiser::Ptr obj(Noiser::create());
	if (!obj)
		return write(L, nullptr);

	return write(L, &obj);
}

static int Noiser_setOption(lua_State* L) {
	Noiser::Ptr* obj = nullptr;
	std::string key;
	Variant val = nullptr;
	read<>(L, obj, key);
	read<3>(L, &val);

	if (obj) {
		const bool ret = obj->get()->option(key, val);

		return write(L, ret);
	}

	return 0;
}

static int Noiser_seed(lua_State* L) {
	Noiser::Ptr* obj = nullptr;
	int seed = 0;
	read<>(L, obj, seed);

	if (obj)
		obj->get()->seed(seed);

	return 0;
}

static int Noiser_get(lua_State* L) {
	const int n = getTop(L);
	Noiser::Ptr* obj = nullptr;
	Math::Vec2f* pos2 = nullptr;
	Math::Vec3f* pos3 = nullptr;
	read<>(L, obj);
	if (n >= 2) {
		Placeholder _1;
		read<>(L, _1, pos2);
		if (!pos2)
			read<>(L, _1, pos3);
	}

	if (obj) {
		if (pos2) {
			const Real ret = obj->get()->get(*pos2);

			return write(L, ret);
		}
		if (pos3) {
			const Real ret = obj->get()->get(*pos3);

			return write(L, ret);
		}
	}

	return 0;
}

static int Noiser_domainWarp(lua_State* L) {
	const int n = getTop(L);
	Noiser::Ptr* obj = nullptr;
	Math::Vec2f* pos2 = nullptr;
	Math::Vec3f* pos3 = nullptr;
	read<>(L, obj);
	if (n >= 2) {
		Placeholder _1;
		read<>(L, _1, pos2);
		if (!pos2)
			read<>(L, _1, pos3);
	}

	if (obj) {
		if (pos2) {
			Math::Vec2f ret = *pos2;
			obj->get()->domainWarp(ret);

			return write(L, &ret);
		}
		if (pos3) {
			Math::Vec3f ret = *pos3;
			obj->get()->domainWarp(ret);

			return write(L, &ret);
		}
	}

	return 0;
}

static void open_Noiser(lua_State* L) {
	def(
		L, "Noiser",
		LUA_LIB(
			array(
				luaL_Reg{ "new", Noiser_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Noiser::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Noiser::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "setOption", Noiser_setOption },
			luaL_Reg{ "seed", Noiser_seed },
			luaL_Reg{ "get", Noiser_get },
			luaL_Reg{ "domainWarp", Noiser_domainWarp },
			luaL_Reg{ nullptr, nullptr }
		),
		nullptr, nullptr
	);
}

static int Pathfinder_ctor(lua_State* L) {
	int w = 0, n = 0, e = 0, s = 0;
	read<>(L, w, n, e, s);

	Pathfinder::Ptr obj(Pathfinder::create(w, n, e, s));
	if (!obj)
		return write(L, nullptr);

	return write(L, &obj);
}

static int Pathfinder_get(lua_State* L) {
	Pathfinder::Ptr* obj = nullptr;
	Math::Vec2i pos;
	read<>(L, obj, pos);

	if (obj) {
		float cost = 0;
		if (!obj->get()->get(pos, &cost))
			return write(L, nullptr);

		return write(L, cost);
	}

	return 0;
}

static int Pathfinder_set(lua_State* L) {
	Pathfinder::Ptr* obj = nullptr;
	Math::Vec2i pos;
	float cost = 0;
	read<>(L, obj, pos, cost);

	if (obj) {
		const bool ret = obj->get()->set(pos, cost);

		return write(L, ret);
	}

	return 0;
}

static int Pathfinder_clear(lua_State* L) {
	Pathfinder::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj)
		obj->get()->clear();

	return 0;
}

static int Pathfinder_solve(lua_State* L) {
	const int n = getTop(L);
	Pathfinder::Ptr* obj = nullptr;
	Math::Vec2i begin, end;
	Function::Ptr eval = nullptr;
	if (n >= 4)
		read<>(L, obj, begin, end, eval);
	else
		read<>(L, obj, begin, end);

	if (obj) {
		Math::Vec2i::List path;
		float cost = 0;
		if (begin.x == end.x && begin.y == end.y) {
			path.push_back(begin);

			return write(L, path, cost);
		}

		Pathfinder::EvaluationHandler eval_ = nullptr;
		if (eval) {
			eval_ = [L, eval] (const Math::Vec2i &pos) -> float {
				float ret = -1;
				ScriptingLua::check(L, call(ret, L, *eval, pos));

				return ret;
			};
		}

		if (!obj->get()->solve(begin, end, eval_, path, &cost))
			return write(L, path, cost);

		return write(L, path, cost); // Undocumented: secondary value.
	}

	return 0;
}

static int Pathfinder___index(lua_State* L) {
	Pathfinder::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "diagonalCost") == 0) {
		const float ret = obj->get()->diagonalCost();
		if (!ret)
			return write(L, 0);

		return write(L, ret);
	} else {
		return __index(L, field);
	}
}

static int Pathfinder___newindex(lua_State* L) {
	Pathfinder::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "diagonalCost") == 0) {
		float val = 1.414f;
		read<3>(L, val);
		obj->get()->diagonalCost(val);
	}

	return 0;
}

static void open_Pathfinder(lua_State* L) {
	def(
		L, "Pathfinder",
		LUA_LIB(
			array(
				luaL_Reg{ "new", Pathfinder_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Pathfinder::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Pathfinder::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "get", Pathfinder_get },
			luaL_Reg{ "set", Pathfinder_set },
			luaL_Reg{ "clear", Pathfinder_clear },
			luaL_Reg{ "solve", Pathfinder_solve },
			luaL_Reg{ nullptr, nullptr }
		),
		Pathfinder___index, Pathfinder___newindex
	);
}

static int Random_ctor(lua_State* L) {
	Randomizer::Ptr obj(Randomizer::create());
	if (!obj)
		return write(L, nullptr);

	return write(L, &obj);
}

static int Random_seed(lua_State* L) {
	const int n = getTop(L);
	Randomizer::Ptr* obj = nullptr;
	Int64 first = 0;
	Int64 second = 0;
	if (n >= 3)
		read<>(L, obj, first, second);
	else if (n == 2)
		read<>(L, obj, first);
	else
		read<>(L, obj);

	if (obj) {
		Randomizer::Seed ret;
		if (n >= 3)
			ret = obj->get()->seed(first, second);
		else if (n == 2)
			ret = obj->get()->seed(first);
		else
			ret = obj->get()->seed();

		return write(L, ret.first, ret.second);
	}

	return 0;
}

static int Random_next(lua_State* L) {
	const int n = getTop(L);
	Randomizer::Ptr* obj = nullptr;
	Int64 low = 0;
	Int64 up = 0;
	if (n >= 3)
		read<>(L, obj, low, up);
	else if (n == 2)
		read<>(L, obj, up);
	else
		read<>(L, obj);

	if (obj) {
		if (n >= 3) {
			const Int64 ret = obj->get()->next(low, up);

			return write(L, ret);
		} else if (n == 2) {
			const Int64 ret = obj->get()->next(up);

			return write(L, ret);
		} else {
			const Double ret = obj->get()->next();

			return write(L, ret);
		}
	}

	return 0;
}

static void open_Random(lua_State* L) {
	def(
		L, "Random",
		LUA_LIB(
			array(
				luaL_Reg{ "new", Random_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Randomizer::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Randomizer::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "seed", Random_seed },
			luaL_Reg{ "next", Random_next },
			luaL_Reg{ nullptr, nullptr }
		),
		nullptr, nullptr
	);
}

static int Raycaster_ctor(lua_State* L) {
	Raycaster::Ptr obj(Raycaster::create());
	if (!obj)
		return write(L, nullptr);

	obj->tileSize(Math::Vec2i(BITTY_MAP_TILE_DEFAULT_SIZE, BITTY_MAP_TILE_DEFAULT_SIZE));

	return write(L, &obj);
}

static int Raycaster_solve(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	Raycaster::Ptr* obj = nullptr;
	Math::Vec2f* rayPos = nullptr;
	Math::Vec2f* rayDir = nullptr;
	Placeholder _4;
	read<>(L, obj, rayPos, rayDir, _4);

	Function::Ptr block = nullptr;
	Resources::Map::Ptr* map = nullptr;
	if (isFunction(L, 4))
		read<4>(L, block);
	else
		read<4>(L, map);

	if (!obj)
		return 0;

	if (!rayPos || !rayDir)
		return 0;

	if (!block && !map) {
		error(L, "Function or map resource argument(4) expected.");

		return 0;
	}

	Raycaster::BlockingHandler block_ = nullptr;
	if (block) {
		block_ = [L, block] (const Math::Vec2i &pos) -> bool {
			bool ret = false;
			ScriptingLua::check(L, call(ret, L, *block, pos));

			return ret;
		};
	}
	Raycaster::EvaluationHandler eval = nullptr;
	if (map) {
		eval = std::bind(
			[impl] (Resources::Map::Ptr map, const Math::Vec2i &pos) -> int {
				int cel = -1;
				impl->primitives()->mget(map, (int)pos.x, (int)pos.y, cel);

				return cel;
			},
			*map, std::placeholders::_1
		);
	}
	const Raycaster::AccessHandler access = block_ ? Raycaster::AccessHandler(block_) : Raycaster::AccessHandler(eval);

	Math::Vec2f intersectionPos;
	Math::Vec2i intersectionIndex;
	const int ret = obj->get()->solve(
		*rayPos, *rayDir,
		access,
		intersectionPos,
		intersectionIndex
	);

	if (!ret)
		return write(L, nullptr, nullptr);

	return write(L, &intersectionPos, intersectionIndex);
}

static int Raycaster___index(lua_State* L) {
	Raycaster::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "tileSize") == 0) {
		const Math::Vec2i ret = obj->get()->tileSize();

		return write(L, ret);
	} else if (strcmp(field, "offset") == 0) {
		const Math::Vec2f ret = obj->get()->offset();

		return write(L, &ret);
	} else {
		return __index(L, field);
	}
}

static int Raycaster___newindex(lua_State* L) {
	Raycaster::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "tileSize") == 0) {
		Math::Vec2i val;
		read<3>(L, val);

		obj->get()->tileSize(val);
	} else if (strcmp(field, "offset") == 0) {
		Math::Vec2f* val = nullptr;
		read<3>(L, val);

		if (val)
			obj->get()->offset(*val);
		else
			obj->get()->offset(Math::Vec2f(0, 0));
	}

	return 0;
}

static void open_Raycaster(lua_State* L) {
	def(
		L, "Raycaster",
		LUA_LIB(
			array(
				luaL_Reg{ "new", Raycaster_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Raycaster::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Raycaster::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "solve", Raycaster_solve },
			luaL_Reg{ nullptr, nullptr }
		),
		Raycaster___index, Raycaster___newindex
	);
}

static int Walker_ctor(lua_State* L) {
	Walker::Ptr obj(Walker::create());
	if (!obj)
		return write(L, nullptr);

	obj->objectSize(Math::Vec2i(BITTY_GRID_DEFAULT_SIZE, BITTY_GRID_DEFAULT_SIZE));

	obj->tileSize(Math::Vec2i(BITTY_MAP_TILE_DEFAULT_SIZE, BITTY_MAP_TILE_DEFAULT_SIZE));

	return write(L, &obj);
}

static int Walker_solve(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	Walker::Ptr* obj = nullptr;
	Math::Vec2f* objPos = nullptr;
	Math::Vec2f* expDir = nullptr;
	Placeholder _4;
	int slidable = 5;
	if (n >= 5)
		read<>(L, obj, objPos, expDir, _4, slidable);
	else
		read<>(L, obj, objPos, expDir, _4);

	Function::Ptr block = nullptr;
	Resources::Map::Ptr* map = nullptr;
	if (isFunction(L, 4))
		read<4>(L, block);
	else
		read<4>(L, map);

	if (!obj)
		return 0;

	if (!objPos || !expDir)
		return 0;

	if (!block && !map) {
		error(L, "Function or map resource argument(4) expected.");

		return 0;
	}

	Walker::BlockingHandler block_ = nullptr;
	if (block) {
		block_ = [L, block] (const Math::Vec2i &pos) -> Walker::Blocking {
			Walker::Blocking ret;
			ScriptingLua::check(L, call(ret, L, *block, pos));

			return ret;
		};
	}
	Walker::EvaluationHandler eval = nullptr;
	if (map) {
		eval = std::bind(
			[impl] (Resources::Map::Ptr map, const Math::Vec2i &pos) -> int {
				int cel = -1;
				impl->primitives()->mget(map, (int)pos.x, (int)pos.y, cel);

				return cel;
			},
			*map, std::placeholders::_1
		);
	}
	const Walker::AccessHandler access = block_ ? Walker::AccessHandler(block_) : Walker::AccessHandler(eval);

	Math::Vec2f newDir;
	const int ret = obj->get()->solve(
		*objPos, *expDir,
		access,
		newDir,
		slidable
	);

	return write(L, &newDir, !!ret); // Undocumented: secondary value.
}

static int Walker___index(lua_State* L) {
	Walker::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "objectSize") == 0) {
		const Math::Vec2i ret = obj->get()->objectSize();

		return write(L, ret);
	} else if (strcmp(field, "tileSize") == 0) {
		const Math::Vec2i ret = obj->get()->tileSize();

		return write(L, ret);
	} else if (strcmp(field, "offset") == 0) {
		const Math::Vec2f ret = obj->get()->offset();

		return write(L, &ret);
	} else {
		return __index(L, field);
	}
}

static int Walker___newindex(lua_State* L) {
	Walker::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "objectSize") == 0) {
		Math::Vec2i val;
		read<3>(L, val);

		obj->get()->objectSize(val);
	} else if (strcmp(field, "tileSize") == 0) {
		Math::Vec2i val;
		read<3>(L, val);

		obj->get()->tileSize(val);
	} else if (strcmp(field, "offset") == 0) {
		Math::Vec2f* val = nullptr;
		read<3>(L, val);

		if (val)
			obj->get()->offset(*val);
		else
			obj->get()->offset(Math::Vec2f(0, 0));
	}

	return 0;
}

static void open_Walker(lua_State* L) {
	def(
		L, "Walker",
		LUA_LIB(
			array(
				luaL_Reg{ "new", Walker_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Walker::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Walker::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "solve", Walker_solve },
			luaL_Reg{ nullptr, nullptr }
		),
		Walker___index, Walker___newindex
	);

	getGlobal(L, "Walker");
	setTable(
		L,
		"None", (Enum)Walker::NONE,
		"Left", (Enum)Walker::LEFT,
		"Right", (Enum)Walker::RIGHT,
		"Up", (Enum)Walker::UP,
		"Down", (Enum)Walker::DOWN
	);
	pop(L);
}

/**< Archive. */

static int Archive_ctor(lua_State* L) {
	Archive::Ptr obj(Archive::create(Archive::ZIP));
	if (!obj)
		return write(L, nullptr);

	return write(L, &obj);
}

static int Archive_open(lua_State* L) {
	const int n = getTop(L);
	Archive::Ptr* obj = nullptr;
	const char* path = nullptr;
	Enum access = Stream::READ;
	const char* password = nullptr;
	if (n >= 4)
		read<>(L, obj, path, access, password);
	else if (n == 3)
		read<>(L, obj, path, access);
	else if (n == 2)
		read<>(L, obj, path);

	if (obj) {
		const bool ret = obj->get()->open(path, (Stream::Accesses)access);
		if (ret && password)
			obj->get()->password(password); // Undocumented.

		return write(L, ret);
	}

	return 0;
}

static int Archive_close(lua_State* L) {
	Archive::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const bool ret = obj->get()->close();

		return write(L, ret);
	} else {
		error(L, "Archive expected, did you use \".\" rather than \":\".");
	}

	return 0;
}

static int Archive_all(lua_State* L) {
	Archive::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		Text::Array entries;
		if (!obj->get()->all(entries))
			return 0;

		return write(L, entries);
	}

	return 0;
}

static int Archive_exists(lua_State* L) {
	Archive::Ptr* obj = nullptr;
	const char* nameInArchive = nullptr;
	read<>(L, obj, nameInArchive);

	if (obj) {
		const bool ret = obj->get()->exists(nameInArchive);

		return write(L, ret);
	}

	return 0;
}

static int Archive_make(lua_State* L) {
	Archive::Ptr* obj = nullptr;
	const char* nameInArchive = nullptr;
	read<>(L, obj, nameInArchive);

	if (obj) {
		const bool ret = obj->get()->make(nameInArchive);

		return write(L, ret);
	}

	return 0;
}

static int Archive_toBytes(lua_State* L) {
	Archive::Ptr* obj = nullptr;
	const char* nameInArchive = nullptr;
	Bytes::Ptr* bytes = nullptr;
	read<>(L, obj, nameInArchive, bytes);

	if (obj) {
		const bool created = !bytes;
		Bytes::Ptr ptr = nullptr;
		if (created) {
			ptr = Bytes::Ptr(Bytes::create());
			bytes = &ptr;
		}

		if (bytes && bytes->get() && obj->get()->toBytes(bytes->get(), nameInArchive)) {
			if (created)
				return write(L, bytes);
			else
				return write(L, Index(3));
		} else {
			return write(L, nullptr);
		}
	}

	return 0;
}

static int Archive_fromBytes(lua_State* L) {
	Archive::Ptr* obj = nullptr;
	const char* nameInArchive = nullptr;
	Bytes::Ptr* bytes = nullptr;
	read<>(L, obj, nameInArchive, bytes);

	if (obj && bytes) {
		const bool ret = obj->get()->fromBytes(bytes->get(), nameInArchive);

		return write(L, ret);
	}

	return 0;
}

static int Archive_toFile(lua_State* L) {
	Archive::Ptr* obj = nullptr;
	const char* nameInArchive = nullptr;
	const char* path = nullptr;
	read<>(L, obj, nameInArchive, path);

	if (obj && path) {
		const bool ret = obj->get()->toFile(path, nameInArchive);

		return write(L, ret);
	}

	return 0;
}

static int Archive_fromFile(lua_State* L) {
	Archive::Ptr* obj = nullptr;
	const char* nameInArchive = nullptr;
	const char* path = nullptr;
	read<>(L, obj, nameInArchive, path);

	if (obj && path) {
		const bool ret = obj->get()->fromFile(path, nameInArchive);

		return write(L, ret);
	}

	return 0;
}

static int Archive_toDirectory(lua_State* L) {
	Archive::Ptr* obj = nullptr;
	const char* dir = nullptr;
	read<>(L, obj, dir);

	if (obj && dir) {
		const bool ret = obj->get()->toDirectory(dir);

		return write(L, ret);
	}

	return 0;
}

static int Archive_fromDirectory(lua_State* L) {
	Archive::Ptr* obj = nullptr;
	const char* dir = nullptr;
	read<>(L, obj, dir);

	if (obj && dir) {
		const bool ret = obj->get()->fromDirectory(dir);

		return write(L, ret);
	}

	return 0;
}

static int Archive___index(lua_State* L) {
	Archive::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "password") == 0) { // Undocumented.
		const char* ret = obj->get()->password();
		if (!ret)
			return write(L, nullptr);

		return write(L, ret);
	} else {
		return __index(L, field);
	}
}

static int Archive___newindex(lua_State* L) {
	Archive::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "password") == 0) { // Undocumented.
		const char* val = nullptr;
		read<3>(L, val);
		obj->get()->password(val);
	}

	return 0;
}

static void open_Archive(lua_State* L) {
	def(
		L, "Archive",
		LUA_LIB(
			array(
				luaL_Reg{ "new", Archive_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Archive::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Archive::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "open", Archive_open },
			luaL_Reg{ "close", Archive_close },
			luaL_Reg{ "all", Archive_all },
			luaL_Reg{ "exists", Archive_exists },
			luaL_Reg{ "make", Archive_make },
			luaL_Reg{ "toBytes", Archive_toBytes },
			luaL_Reg{ "fromBytes", Archive_fromBytes },
			luaL_Reg{ "toFile", Archive_toFile },
			luaL_Reg{ "fromFile", Archive_fromFile },
			luaL_Reg{ "toDirectory", Archive_toDirectory },
			luaL_Reg{ "fromDirectory", Archive_fromDirectory },
			luaL_Reg{ nullptr, nullptr }
		),
		Archive___index, Archive___newindex
	);
}

/**< Bytes. */

static int Bytes_ctor(lua_State* L) {
	Bytes::Ptr obj(Bytes::create());
	if (!obj)
		return write(L, nullptr);

	return write(L, &obj);
}

static int Bytes___len(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	check<>(L, obj);

	if (obj) {
		const size_t ret = obj->get()->count();

		return write(L, ret);
	} else {
		error(L, "Bytes expected.");
	}

	return 0;
}

static int Bytes_peek(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const int ret = (int)obj->get()->peek() + 1; // 1-based.

		return write(L, ret);
	} else {
		error(L, "Bytes expected.");
	}

	return 0;
}

static int Bytes_poke(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	int p = 0;
	read<>(L, obj, p);

	--p; // 1-based.
	if (obj) {
		const bool ret = obj->get()->poke((size_t)p);

		return write(L, ret);
	} else {
		error(L, "Bytes expected.");
	}

	return 0;
}

static int Bytes_count(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const size_t ret = obj->get()->count();

		return write(L, ret);
	} else {
		error(L, "Bytes expected.");
	}

	return 0;
}

static int Bytes_empty(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const bool ret = obj->get()->empty();

		return write(L, ret);
	} else {
		error(L, "Bytes expected.");
	}

	return 0;
}

static int Bytes_endOfStream(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const bool ret = obj->get()->endOfStream();

		return write(L, ret);
	} else {
		error(L, "Bytes expected.");
	}

	return 0;
}

static int Bytes_readByte(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Byte ret = obj->get()->readByte();

		return write(L, ret);
	} else {
		error(L, "Bytes expected.");
	}

	return 0;
}

static int Bytes_readInt16(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Int16 ret = obj->get()->readInt16();

		return write(L, ret);
	} else {
		error(L, "Bytes expected.");
	}

	return 0;
}

static int Bytes_readUInt16(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const UInt16 ret = obj->get()->readUInt16();

		return write(L, ret);
	} else {
		error(L, "Bytes expected.");
	}

	return 0;
}

static int Bytes_readInt32(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Int32 ret = obj->get()->readInt32();

		return write(L, ret);
	} else {
		error(L, "Bytes expected.");
	}

	return 0;
}

static int Bytes_readUInt32(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const UInt32 ret = obj->get()->readUInt32();

		return write(L, ret);
	} else {
		error(L, "Bytes expected.");
	}

	return 0;
}

static int Bytes_readInt64(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Int64 ret = obj->get()->readInt64();

		return write(L, ret);
	} else {
		error(L, "Bytes expected.");
	}

	return 0;
}

static int Bytes_readSingle(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Single ret = obj->get()->readSingle();

		return write(L, ret);
	} else {
		error(L, "Bytes expected.");
	}

	return 0;
}

static int Bytes_readDouble(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Double ret = obj->get()->readDouble();

		return write(L, ret);
	} else {
		error(L, "Bytes expected.");
	}

	return 0;
}

static int Bytes_readBytes(lua_State* L) {
	const int n = getTop(L);
	Bytes::Ptr* obj = nullptr;
	size_t expSize = 0;
	Bytes::Ptr* buf = nullptr;
	if (n >= 3)
		read<>(L, obj, expSize, buf);
	else
		read<>(L, obj, expSize);

	if (obj) {
		const bool created = !buf;
		Bytes::Ptr ptr = nullptr;
		if (created) {
			ptr = Bytes::Ptr(Bytes::create());
			buf = &ptr;
		}

		if (buf && buf->get()) {
			if (obj->get() == buf->get()) {
				error(L, "Cannot read from self.");

				return 0;
			}

			if (expSize)
				obj->get()->readBytes(buf->get(), expSize);
			else
				obj->get()->readBytes(buf->get());

			buf->get()->poke(buf->get()->count());

			if (created)
				return write(L, buf);
			else
				return write(L, Index(3));
		}
	} else {
		error(L, "Bytes expected.");
	}

	return 0;
}

static int Bytes_readString(lua_State* L) {
	const int n = getTop(L);
	Bytes::Ptr* obj = nullptr;
	size_t expSize = 0;
	if (n >= 2)
		read<>(L, obj, expSize);
	else
		read<>(L, obj);

	if (obj) {
		if (expSize) {
			char* buf = new char[expSize + 1];
			if (!buf)
				return 0;
			obj->get()->readString(buf, expSize);
			buf[expSize] = '\0';
			const std::string str = buf;
			delete[] buf;

			return write(L, str);
		} else {
			std::string str;
			obj->get()->readString(str);

			return write(L, str);
		}
	} else {
		error(L, "Bytes expected.");
	}

	return 0;
}

static int Bytes_readLine(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		std::string str;
		obj->get()->readLine(str);

		return write(L, str);
	} else {
		error(L, "Bytes expected.");
	}

	return 0;
}

static int Bytes_writeByte(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	Byte val = 0;
	read<>(L, obj, val);

	if (obj) {
		const int ret = obj->get()->writeByte(val);

		return write(L, ret);
	} else {
		error(L, "Bytes expected.");
	}

	return write(L, 0);
}

static int Bytes_writeInt16(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	Int16 val = 0;
	read<>(L, obj, val);

	if (obj) {
		const int ret = obj->get()->writeInt16(val);

		return write(L, ret);
	} else {
		error(L, "Bytes expected.");
	}

	return write(L, 0);
}

static int Bytes_writeUInt16(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	UInt16 val = 0;
	read<>(L, obj, val);

	if (obj) {
		const int ret = obj->get()->writeUInt16(val);

		return write(L, ret);
	} else {
		error(L, "Bytes expected.");
	}

	return write(L, 0);
}

static int Bytes_writeInt32(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	Int32 val = 0;
	read<>(L, obj, val);

	if (obj) {
		const int ret = obj->get()->writeInt32(val);

		return write(L, ret);
	} else {
		error(L, "Bytes expected.");
	}

	return write(L, 0);
}

static int Bytes_writeUInt32(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	UInt32 val = 0;
	read<>(L, obj, val);

	if (obj) {
		const int ret = obj->get()->writeUInt32(val);

		return write(L, ret);
	} else {
		error(L, "Bytes expected.");
	}

	return write(L, 0);
}

static int Bytes_writeInt64(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	Int64 val = 0;
	read<>(L, obj, val);

	if (obj) {
		const int ret = obj->get()->writeInt64(val);

		return write(L, ret);
	} else {
		error(L, "Bytes expected.");
	}

	return write(L, 0);
}

static int Bytes_writeSingle(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	Single val = 0;
	read<>(L, obj, val);

	if (obj) {
		const int ret = obj->get()->writeSingle(val);

		return write(L, ret);
	} else {
		error(L, "Bytes expected.");
	}

	return write(L, 0);
}

static int Bytes_writeDouble(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	Double val = 0;
	read<>(L, obj, val);

	if (obj) {
		const int ret = obj->get()->writeDouble(val);

		return write(L, ret);
	} else {
		error(L, "Bytes expected.");
	}

	return write(L, 0);
}

static int Bytes_writeBytes(lua_State* L) {
	const int n = getTop(L);
	Bytes::Ptr* obj = nullptr;
	Bytes::Ptr* buf = nullptr;
	size_t expSize = 0;
	if (n >= 3)
		read<>(L, obj, buf, expSize);
	else
		read<>(L, obj, buf);

	int ret = 0;
	if (obj) {
		if (buf) {
			if (obj->get() == buf->get()) {
				error(L, "Cannot write to self.");

				return 0;
			}

			if (expSize)
				ret = obj->get()->writeBytes(buf->get(), expSize);
			else
				ret = obj->get()->writeBytes(buf->get());
		}
	} else {
		error(L, "Bytes expected.");
	}

	return write(L, 0);
}

static int Bytes_writeString(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	std::string val;
	read<>(L, obj, val);

	if (obj) {
		const int ret = obj->get()->writeString(val);

		return write(L, ret);
	} else {
		error(L, "Bytes expected.");
	}

	return write(L, 0);
}

static int Bytes_writeLine(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	std::string val;
	read<>(L, obj, val);

	if (obj) {
		const int ret = obj->get()->writeLine(val);

		return write(L, ret);
	} else {
		error(L, "Bytes expected.");
	}

	return write(L, 0);
}

static int Bytes_get(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	int index = 1;
	read<>(L, obj, index);

	--index; // 1-based.
	if (obj) {
		if (index >= 0 && index < (int)obj->get()->count()) {
			const Byte ret = obj->get()->get((size_t)index);

			return write(L, ret);
		}
	} else {
		error(L, "Bytes expected.");
	}

	return 0;
}

static int Bytes_set(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	int index = 1;
	Byte val = 0;
	read<>(L, obj, index, val);

	--index; // 1-based.
	if (obj) {
		if (index >= 0 && index < (int)obj->get()->count())
			obj->get()->set((size_t)index, val);
	} else {
		error(L, "Bytes expected.");
	}

	return 0;
}

static int Bytes_resize(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	size_t expSize = 0;
	read<>(L, obj, expSize);

	constexpr const size_t MAX_SIZE = (size_t)std::numeric_limits<UInt32>::max(); // Limited in 4GB.
	if (expSize > MAX_SIZE) {
		error(L, "Cannot resize to the specific size.");

		return 0;
	}

	if (obj)
		obj->get()->resize(expSize);
	else
		error(L, "Bytes expected.");

	return 0;
}

static int Bytes_clear(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj)
		obj->get()->clear();
	else
		error(L, "Bytes expected.");

	return 0;
}

static int Bytes___index(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj) {
		error(L, "Bytes expected.");

		return 0;
	}

	if (isNumber(L, 2)) {
		int index = 1;
		read<2>(L, index);

		--index; // 1-based.
		if (index >= 0 && index < (int)obj->get()->count()) {
			const Byte ret = obj->get()->get((size_t)index);

			return write(L, ret);
		}

		return 0;
	}

	return __index(L, field);
}

static int Bytes___newindex(lua_State* L) {
	Bytes::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj) {
		error(L, "Bytes expected.");

		return 0;
	}

	if (isNumber(L, 2)) {
		int index = 1;
		Byte val = 0;
		read<2>(L, index, val);

		--index; // 1-based.
		if (index >= 0 && index < (int)obj->get()->count())
			obj->get()->set((size_t)index, val);

		return 0;
	}

	return 0;
}

static void open_Bytes(lua_State* L) {
	def(
		L, "Bytes",
		LUA_LIB(
			array(
				luaL_Reg{ "new", Bytes_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Bytes::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Bytes::Ptr> },
			luaL_Reg{ "__len", Bytes___len },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "peek", Bytes_peek },
			luaL_Reg{ "poke", Bytes_poke },
			luaL_Reg{ "count", Bytes_count },
			luaL_Reg{ "empty", Bytes_empty },
			luaL_Reg{ "endOfStream", Bytes_endOfStream },
			luaL_Reg{ "readByte", Bytes_readByte },
			luaL_Reg{ "readInt16", Bytes_readInt16 },
			luaL_Reg{ "readUInt16", Bytes_readUInt16 },
			luaL_Reg{ "readInt32", Bytes_readInt32 },
			luaL_Reg{ "readUInt32", Bytes_readUInt32 },
			luaL_Reg{ "readInt64", Bytes_readInt64 },
			luaL_Reg{ "readSingle", Bytes_readSingle },
			luaL_Reg{ "readDouble", Bytes_readDouble },
			luaL_Reg{ "readBytes", Bytes_readBytes },
			luaL_Reg{ "readString", Bytes_readString },
			luaL_Reg{ "readLine", Bytes_readLine },
			luaL_Reg{ "writeByte", Bytes_writeByte },
			luaL_Reg{ "writeInt16", Bytes_writeInt16 },
			luaL_Reg{ "writeUInt16", Bytes_writeUInt16 },
			luaL_Reg{ "writeInt32", Bytes_writeInt32 },
			luaL_Reg{ "writeUInt32", Bytes_writeUInt32 },
			luaL_Reg{ "writeInt64", Bytes_writeInt64 },
			luaL_Reg{ "writeSingle", Bytes_writeSingle },
			luaL_Reg{ "writeDouble", Bytes_writeDouble },
			luaL_Reg{ "writeBytes", Bytes_writeBytes },
			luaL_Reg{ "writeString", Bytes_writeString },
			luaL_Reg{ "writeLine", Bytes_writeLine },
			luaL_Reg{ "get", Bytes_get },
			luaL_Reg{ "set", Bytes_set },
			luaL_Reg{ "resize", Bytes_resize },
			luaL_Reg{ "clear", Bytes_clear },
			luaL_Reg{ nullptr, nullptr }
		),
		Bytes___index, Bytes___newindex
	);
}

/**< Color. */

static int Color_ctor(lua_State* L) {
	const int n = getTop(L);
	Color obj;
	switch (n) {
	case 3: {
			Byte r = 255, g = 255, b = 255;
			read<>(L, r, g, b);

			obj = Color(r, g, b);
		}

		break;
	case 4: {
			Byte r = 255, g = 255, b = 255, a = 255;
			read<>(L, r, g, b, a);

			obj = Color(r, g, b, a);
		}

		break;
	}

	return write(L, &obj);
}

static int Color___tostring(lua_State* L) {
	Color* obj = nullptr;
	check<>(L, obj);

	std::string str;

	str += "Color[0x";
	str += Text::toHex(obj->toRGBA());
	str += "]";

	return write(L, str);
}

static int Color___add(lua_State* L) {
	Color* obj = nullptr;
	Color* other = nullptr;
	check<>(L, obj, other);

	if (obj && other) {
		const Color ret = *obj + *other;

		return write(L, &ret);
	}

	return 0;
}

static int Color___sub(lua_State* L) {
	Color* obj = nullptr;
	Color* other = nullptr;
	check<>(L, obj, other);

	if (obj && other) {
		const Color ret = *obj - *other;

		return write(L, &ret);
	}

	return 0;
}

static int Color___mul(lua_State* L) {
	Color* obj = nullptr;
	Color* other = nullptr;
	Math::Vec4f* vec = nullptr;
	Real num = 0;
	check<>(L, obj);

	if (obj) {
		if (isNumber(L, 2)) {
			check<2>(L, num);

			const Color ret = *obj * num;

			return write(L, &ret);
		} else {
			check<2>(L, other);
			if (other) {
				const Color ret = *obj * *other;

				return write(L, &ret);
			}

			check<2>(L, vec);
			if (vec) {
				const Color ret(
					obj->r * vec->x,
					obj->g * vec->y,
					obj->b * vec->z,
					obj->a * vec->w
				);

				return write(L, &ret);
			}

			return 0;
		}
	}

	return 0;
}

static int Color___unm(lua_State* L) {
	Color* obj = nullptr;
	check<>(L, obj);

	if (obj) {
		const Color ret = -*obj;

		return write(L, &ret);
	}

	return 0;
}

static int Color___eq(lua_State* L) {
	Color* obj = nullptr;
	Color* other = nullptr;
	check<>(L, obj, other);

	if (obj && other) {
		const bool ret = *obj == *other;

		return write(L, ret);
	}

	return write(L, false);
}

static int Color_toRGBA(lua_State* L) {
	Color* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const UInt32 ret = obj->toRGBA();

		return write(L, ret);
	}

	return 0;
}

static int Color_fromRGBA(lua_State* L) {
	Color* obj = nullptr;
	UInt32 rgba = 0xffffffff;
	read<>(L, obj, rgba);

	if (obj)
		obj->fromRGBA(rgba);

	return 0;
}

static int Color___index(lua_State* L) {
	Color* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "r") == 0) {
		return write(L, obj->r);
	} else if (strcmp(field, "g") == 0) {
		return write(L, obj->g);
	} else if (strcmp(field, "b") == 0) {
		return write(L, obj->b);
	} else if (strcmp(field, "a") == 0) {
		return write(L, obj->a);
	} else {
		return __index(L, field);
	}
}

static int Color___newindex(lua_State* L) {
	Color* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "r") == 0) {
		Byte val = 255;
		read<3>(L, val);
		obj->r = val;
	} else if (strcmp(field, "g") == 0) {
		Byte val = 255;
		read<3>(L, val);
		obj->g = val;
	} else if (strcmp(field, "b") == 0) {
		Byte val = 255;
		read<3>(L, val);
		obj->b = val;
	} else if (strcmp(field, "a") == 0) {
		Byte val = 255;
		read<3>(L, val);
		obj->a = val;
	}

	return 0;
}

static void open_Color(lua_State* L) {
	def(
		L, "Color",
		LUA_LIB(
			array(
				luaL_Reg{ "new", Color_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Color> },
			luaL_Reg{ "__tostring", Color___tostring },
			luaL_Reg{ "__add", Color___add },
			luaL_Reg{ "__sub", Color___sub },
			luaL_Reg{ "__mul", Color___mul },
			luaL_Reg{ "__unm", Color___unm },
			luaL_Reg{ "__eq", Color___eq },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "toRGBA", Color_toRGBA },
			luaL_Reg{ "fromRGBA", Color_fromRGBA },
			luaL_Reg{ nullptr, nullptr }
		),
		Color___index, Color___newindex
	);
}

/**< Date time. */

static int DateTime_now(lua_State* L) {
	int sec = 0, mi = 0, hr = 0;
	int mday = 0, mo = 0, yr = 0;
	int wday = 0, yday = 0, isdst = 0;
	const long long ticks = DateTime::now(
		&sec, &mi, &hr,
		&mday, &mo, &yr,
		&wday, &yday, &isdst
	);

	return write(
		L,
		sec, mi, hr,
		mday, mo + 1, yr + 1900,
		wday + 1, yday + 1, !!isdst,
		ticks
	);
}

static int DateTime_ticks(lua_State* L) {
	const long long ret = DateTime::ticks();

	return write(L, ret);
}

static int DateTime_toMilliseconds(lua_State* L) {
	long long t = 0;
	read<>(L, t);

	const int ret = DateTime::toMilliseconds(t);

	return write(L, ret);
}

static int DateTime_fromMilliseconds(lua_State* L) {
	int t = 0;
	read<>(L, t);

	const long long ret = DateTime::fromMilliseconds(t);

	return write(L, ret);
}

static int DateTime_toSeconds(lua_State* L) {
	long long t = 0;
	read<>(L, t);

	const double ret = DateTime::toSeconds(t);

	return write(L, ret);
}

static int DateTime_fromSeconds(lua_State* L) {
	double t = 0;
	read<>(L, t);

	const long long ret = DateTime::fromSeconds(t);

	return write(L, ret);
}

static void open_DateTime(lua_State* L) {
	req(
		L,
		array(
			luaL_Reg{
				"DateTime",
				LUA_LIB(
					array(
						luaL_Reg{ "now", DateTime_now },
						luaL_Reg{ "ticks", DateTime_ticks },
						luaL_Reg{ "toMilliseconds", DateTime_toMilliseconds },
						luaL_Reg{ "fromMilliseconds", DateTime_fromMilliseconds },
						luaL_Reg{ "toSeconds", DateTime_toSeconds },
						luaL_Reg{ "fromSeconds", DateTime_fromSeconds },
						luaL_Reg{ nullptr, nullptr }
					)
				)
			},
			luaL_Reg{ nullptr, nullptr }
		)
	);
}

/**< Encoding. */

static int Base64_encode(lua_State* L) {
	Bytes::Ptr* bytes = nullptr;
	read<>(L, bytes);

	if (bytes) {
		std::string ret;
		if (Base64::fromBytes(ret, bytes->get()))
			return write(L, ret);
	}
	
	return write(L, nullptr);
}

static int Base64_decode(lua_State* L) {
	const char* str = nullptr;
	read<>(L, str);

	if (str) {
		Bytes::Ptr ret(Bytes::create());
		if (Base64::toBytes(ret.get(), str))
			return write(L, &ret);
	}

	return write(L, nullptr);
}

static void open_Base64(lua_State* L) {
	req(
		L,
		array(
			luaL_Reg{
				"Base64",
				LUA_LIB(
					array(
						luaL_Reg{ "encode", Base64_encode },
						luaL_Reg{ "decode", Base64_decode },
						luaL_Reg{ nullptr, nullptr }
					)
				)
			},
			luaL_Reg{ nullptr, nullptr }
		)
	);
}

static int Lz4_encode(lua_State* L) {
	Bytes::Ptr* bytes = nullptr;
	read<>(L, bytes);

	if (bytes) {
		Bytes::Ptr ret(Bytes::create());
		if (Lz4::fromBytes(ret.get(), bytes->get()))
			return write(L, &ret);
	}
	
	return write(L, nullptr);
}

static int Lz4_decode(lua_State* L) {
	Bytes::Ptr* bytes = nullptr;
	read<>(L, bytes);

	if (bytes) {
		Bytes::Ptr ret(Bytes::create());
		if (Lz4::toBytes(ret.get(), bytes->get()))
			return write(L, &ret);
	}

	return write(L, nullptr);
}

static void open_Lz4(lua_State* L) {
	req(
		L,
		array(
			luaL_Reg{
				"Lz4",
				LUA_LIB(
					array(
						luaL_Reg{ "encode", Lz4_encode },
						luaL_Reg{ "decode", Lz4_decode },
						luaL_Reg{ nullptr, nullptr }
					)
				)
			},
			luaL_Reg{ nullptr, nullptr }
		)
	);
}

/**< File. */

static int File_ctor(lua_State* L) {
	File::Ptr obj(File::create());
	if (!obj)
		return write(L, nullptr);

	return write(L, &obj);
}

static int File___len(lua_State* L) {
	File::Ptr* obj = nullptr;
	check<>(L, obj);

	if (obj) {
		const size_t ret = obj->get()->count();

		return write(L, ret);
	} else {
		error(L, "File expected.");
	}

	return 0;
}

static int File_open(lua_State* L) {
	const int n = getTop(L);
	File::Ptr* obj = nullptr;
	const char* path = nullptr;
	Enum access = Stream::READ;
	if (n >= 3)
		read<>(L, obj, path, access);
	else if (n == 2)
		read<>(L, obj, path);

	if (obj) {
		const bool ret = obj->get()->open(path, (Stream::Accesses)access);

		return write(L, ret);
	} else {
		error(L, "File expected.");
	}

	return 0;
}

static int File_close(lua_State* L) {
	File::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const bool ret = obj->get()->close();

#if defined BITTY_OS_HTML
		EM_ASM(
			FS.syncfs(
				function (err) {
					if (err)
						Module.printErr(err);

					Module.print("Filesystem synced.");
				}
			);
		);
#endif /* BITTY_OS_HTML */

		return write(L, ret);
	} else {
		error(L, "File expected, did you use \".\" rather than \":\".");
	}

	return 0;
}

static int File_peek(lua_State* L) {
	File::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const int ret = (int)obj->get()->peek() + 1; // 1-based.

		return write(L, ret);
	} else {
		error(L, "File expected.");
	}

	return 0;
}

static int File_poke(lua_State* L) {
	File::Ptr* obj = nullptr;
	int p = 0;
	read<>(L, obj, p);

	--p; // 1-based.
	if (obj)
		obj->get()->poke((size_t)p);
	else
		error(L, "File expected.");

	return 0;
}

static int File_count(lua_State* L) {
	File::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const size_t ret = obj->get()->count();

		return write(L, ret);
	} else {
		error(L, "File expected.");
	}

	return 0;
}

static int File_empty(lua_State* L) {
	File::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const bool ret = obj->get()->empty();

		return write(L, ret);
	} else {
		error(L, "File expected.");
	}

	return 0;
}

static int File_endOfStream(lua_State* L) {
	File::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const bool ret = obj->get()->endOfStream();

		return write(L, ret);
	} else {
		error(L, "File expected.");
	}

	return 0;
}

static int File_readByte(lua_State* L) {
	File::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Byte ret = obj->get()->readByte();

		return write(L, ret);
	} else {
		error(L, "File expected.");
	}

	return 0;
}

static int File_readInt16(lua_State* L) {
	File::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Int16 ret = obj->get()->readInt16();

		return write(L, ret);
	} else {
		error(L, "File expected.");
	}

	return 0;
}

static int File_readUInt16(lua_State* L) {
	File::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const UInt16 ret = obj->get()->readUInt16();

		return write(L, ret);
	} else {
		error(L, "File expected.");
	}

	return 0;
}

static int File_readInt32(lua_State* L) {
	File::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Int32 ret = obj->get()->readInt32();

		return write(L, ret);
	} else {
		error(L, "File expected.");
	}

	return 0;
}

static int File_readUInt32(lua_State* L) {
	File::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const UInt32 ret = obj->get()->readUInt32();

		return write(L, ret);
	} else {
		error(L, "File expected.");
	}

	return 0;
}

static int File_readInt64(lua_State* L) {
	File::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Int64 ret = obj->get()->readInt64();

		return write(L, ret);
	} else {
		error(L, "File expected.");
	}

	return 0;
}

static int File_readSingle(lua_State* L) {
	File::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Single ret = obj->get()->readSingle();

		return write(L, ret);
	} else {
		error(L, "File expected.");
	}

	return 0;
}

static int File_readDouble(lua_State* L) {
	File::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Double ret = obj->get()->readDouble();

		return write(L, ret);
	} else {
		error(L, "File expected.");
	}

	return 0;
}

static int File_readBytes(lua_State* L) {
	const int n = getTop(L);
	File::Ptr* obj = nullptr;
	size_t expSize = 0;
	Bytes::Ptr* buf = nullptr;
	if (n >= 3)
		read<>(L, obj, expSize, buf);
	else
		read<>(L, obj, expSize);

	if (obj) {
		const bool created = !buf;
		Bytes::Ptr ptr = nullptr;
		if (created) {
			ptr = Bytes::Ptr(Bytes::create());
			buf = &ptr;
		}

		if (buf && buf->get()) {
			if (expSize)
				obj->get()->readBytes(buf->get(), expSize);
			else
				obj->get()->readBytes(buf->get());

			buf->get()->poke(buf->get()->count());

			if (created)
				return write(L, buf);
			else
				return write(L, Index(3));
		}
	} else {
		error(L, "File expected.");
	}

	return 0;
}

static int File_readString(lua_State* L) {
	const int n = getTop(L);
	File::Ptr* obj = nullptr;
	size_t expSize = 0;
	if (n >= 2)
		read<>(L, obj, expSize);
	else
		read<>(L, obj);

	if (obj) {
		if (expSize) {
			char* buf = new char[expSize + 1];
			if (!buf)
				return 0;
			obj->get()->readString(buf, expSize);
			buf[expSize] = '\0';
			const std::string str = buf;
			delete[] buf;

			return write(L, str);
		} else {
			std::string str;
			obj->get()->readString(str);

			return write(L, str);
		}
	} else {
		error(L, "File expected.");
	}

	return 0;
}

static int File_readLine(lua_State* L) {
	File::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		std::string str;
		obj->get()->readLine(str);

		return write(L, str);
	} else {
		error(L, "File expected.");
	}

	return 0;
}

static int File_writeByte(lua_State* L) {
	File::Ptr* obj = nullptr;
	Byte val = 0;
	read<>(L, obj, val);

	if (obj) {
		const int ret = obj->get()->writeByte(val);

		return write(L, ret);
	} else {
		error(L, "File expected.");
	}

	return write(L, 0);
}

static int File_writeInt16(lua_State* L) {
	File::Ptr* obj = nullptr;
	Int16 val = 0;
	read<>(L, obj, val);

	if (obj) {
		const int ret = obj->get()->writeInt16(val);

		return write(L, ret);
	} else {
		error(L, "File expected.");
	}

	return write(L, 0);
}

static int File_writeUInt16(lua_State* L) {
	File::Ptr* obj = nullptr;
	UInt16 val = 0;
	read<>(L, obj, val);

	if (obj) {
		const int ret = obj->get()->writeUInt16(val);

		return write(L, ret);
	} else {
		error(L, "File expected.");
	}

	return write(L, 0);
}

static int File_writeInt32(lua_State* L) {
	File::Ptr* obj = nullptr;
	Int32 val = 0;
	read<>(L, obj, val);

	if (obj) {
		const int ret = obj->get()->writeInt32(val);

		return write(L, ret);
	} else {
		error(L, "File expected.");
	}

	return write(L, 0);
}

static int File_writeUInt32(lua_State* L) {
	File::Ptr* obj = nullptr;
	UInt32 val = 0;
	read<>(L, obj, val);

	if (obj) {
		const int ret = obj->get()->writeUInt32(val);

		return write(L, ret);
	} else {
		error(L, "File expected.");
	}

	return write(L, 0);
}

static int File_writeInt64(lua_State* L) {
	File::Ptr* obj = nullptr;
	Int64 val = 0;
	read<>(L, obj, val);

	if (obj) {
		const int ret = obj->get()->writeInt64(val);

		return write(L, ret);
	} else {
		error(L, "File expected.");
	}

	return write(L, 0);
}

static int File_writeSingle(lua_State* L) {
	File::Ptr* obj = nullptr;
	Single val = 0;
	read<>(L, obj, val);

	if (obj) {
		const int ret = obj->get()->writeSingle(val);

		return write(L, ret);
	} else {
		error(L, "File expected.");
	}

	return write(L, 0);
}

static int File_writeDouble(lua_State* L) {
	File::Ptr* obj = nullptr;
	Double val = 0;
	read<>(L, obj, val);

	if (obj) {
		const int ret = obj->get()->writeDouble(val);

		return write(L, ret);
	} else {
		error(L, "File expected.");
	}

	return write(L, 0);
}

static int File_writeBytes(lua_State* L) {
	const int n = getTop(L);
	File::Ptr* obj = nullptr;
	Bytes::Ptr* buf = nullptr;
	size_t expSize = 0;
	if (n >= 3)
		read<>(L, obj, buf, expSize);
	else
		read<>(L, obj, buf);

	int ret = 0;
	if (obj) {
		if (buf) {
			if (expSize)
				ret = obj->get()->writeBytes(buf->get(), expSize);
			else
				ret = obj->get()->writeBytes(buf->get());
		}
	} else {
		error(L, "File expected.");
	}

	return write(L, ret);
}

static int File_writeString(lua_State* L) {
	File::Ptr* obj = nullptr;
	std::string val;
	read<>(L, obj, val);

	if (obj) {
		const int ret = obj->get()->writeString(val);

		return write(L, ret);
	} else {
		error(L, "File expected.");
	}

	return write(L, 0);
}

static int File_writeLine(lua_State* L) {
	File::Ptr* obj = nullptr;
	std::string val;
	read<>(L, obj, val);

	if (obj) {
		const int ret = obj->get()->writeLine(val);

		return write(L, ret);
	} else {
		error(L, "File expected.");
	}

	return write(L, 0);
}

static void open_File(lua_State* L) {
	def(
		L, "File",
		LUA_LIB(
			array(
				luaL_Reg{ "new", File_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<File::Ptr> },
			luaL_Reg{ "__tostring", __tostring<File::Ptr> },
			luaL_Reg{ "__len", File___len },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "open", File_open },
			luaL_Reg{ "close", File_close },
			luaL_Reg{ "peek", File_peek },
			luaL_Reg{ "poke", File_poke },
			luaL_Reg{ "count", File_count },
			luaL_Reg{ "empty", File_empty },
			luaL_Reg{ "endOfStream", File_endOfStream },
			luaL_Reg{ "readByte", File_readByte },
			luaL_Reg{ "readInt16", File_readInt16 },
			luaL_Reg{ "readUInt16", File_readUInt16 },
			luaL_Reg{ "readInt32", File_readInt32 },
			luaL_Reg{ "readUInt32", File_readUInt32 },
			luaL_Reg{ "readInt64", File_readInt64 },
			luaL_Reg{ "readSingle", File_readSingle },
			luaL_Reg{ "readDouble", File_readDouble },
			luaL_Reg{ "readBytes", File_readBytes },
			luaL_Reg{ "readString", File_readString },
			luaL_Reg{ "readLine", File_readLine },
			luaL_Reg{ "writeByte", File_writeByte },
			luaL_Reg{ "writeInt16", File_writeInt16 },
			luaL_Reg{ "writeUInt16", File_writeUInt16 },
			luaL_Reg{ "writeInt32", File_writeInt32 },
			luaL_Reg{ "writeUInt32", File_writeUInt32 },
			luaL_Reg{ "writeInt64", File_writeInt64 },
			luaL_Reg{ "writeSingle", File_writeSingle },
			luaL_Reg{ "writeDouble", File_writeDouble },
			luaL_Reg{ "writeBytes", File_writeBytes },
			luaL_Reg{ "writeString", File_writeString },
			luaL_Reg{ "writeLine", File_writeLine },
			luaL_Reg{ nullptr, nullptr }
		),
		nullptr, nullptr
	);
}

/**< Filesystem. */

static std::string Path_executableFile(lua_State*) {
	return Path::executableFile();
}

static std::string Path_documentDirectory(lua_State*) {
	return Path::documentDirectory();
}

static std::string Path_writableDirectory(lua_State*) {
	return Path::writableDirectory();
}

static std::string Path_savedGamesDirectory(lua_State*) {
	return Path::savedGamesDirectory();
}

static int Path_combine(lua_State* L) {
	const int n = getTop(L);
	std::string ret;
	for (int i = 1; i <= n; ++i) {
		const char* part = nullptr;
		read(L, part, Index(i));
		if (i == 1)
			ret = part;
		else
			ret = Path::combine(ret.c_str(), part);
	}

	return write(L, ret);
}

static int Path_split(lua_State* L) {
	std::string full;
	read<>(L, full);

	std::string self, ext, parent;
	Path::split(full, &self, &ext, &parent);

	return write(L, self, ext, parent);
}

static int Path_existsFile(lua_State* L) {
	const char* path = nullptr;
	read<>(L, path);

	const bool ret = Path::existsFile(path);

	return write(L, ret);
}

static int Path_existsDirectory(lua_State* L) {
	const char* path = nullptr;
	read<>(L, path);

	const bool ret = Path::existsDirectory(path);

	return write(L, ret);
}

static int Path_copyFile(lua_State* L) {
	const char* src = nullptr;
	const char* dst = nullptr;
	read<>(L, src, dst);

	const bool ret = Path::copyFile(src, dst);

	return write(L, ret);
}

static int Path_copyDirectory(lua_State* L) {
	const char* src = nullptr;
	const char* dst = nullptr;
	read<>(L, src, dst);

	const bool ret = Path::copyDirectory(src, dst);

	return write(L, ret);
}

static int Path_moveFile(lua_State* L) {
	const char* src = nullptr;
	const char* dst = nullptr;
	read<>(L, src, dst);

	const bool ret = Path::moveFile(src, dst);

	return write(L, ret);
}

static int Path_moveDirectory(lua_State* L) {
	const char* src = nullptr;
	const char* dst = nullptr;
	read<>(L, src, dst);
	
	const bool ret = Path::moveDirectory(src, dst);

	return write(L, ret);
}

static int Path_removeFile(lua_State* L) {
	const int n = getTop(L);
	const char* path = nullptr;
	bool toTrashBin = true;
	if (n >= 2)
		read<>(L, path, toTrashBin);
	else
		read<>(L, path);

	const bool ret = Path::removeFile(path, toTrashBin);

	return write(L, ret);
}

static int Path_removeDirectory(lua_State* L) {
	const int n = getTop(L);
	const char* path = nullptr;
	bool toTrashBin = true;
	if (n >= 2)
		read<>(L, path, toTrashBin);
	else
		read<>(L, path);

	const bool ret = Path::removeDirectory(path, toTrashBin);

	return write(L, ret);
}

static int Path_touchFile(lua_State* L) {
	const char* path = nullptr;
	read<>(L, path);

	const bool ret = Path::touchFile(path);

	return write(L, ret);
}

static int Path_touchDirectory(lua_State* L) {
	const char* path = nullptr;
	read<>(L, path);

	const bool ret = Path::touchDirectory(path);

	return write(L, ret);
}

static void open_Path(lua_State* L) {
	req(
		L,
		array(
			luaL_Reg{
				"Path",
				LUA_LIB(
					array(
						luaL_Reg{ "combine", Path_combine },
						luaL_Reg{ "split", Path_split },
						luaL_Reg{ "existsFile", Path_existsFile },
						luaL_Reg{ "existsDirectory", Path_existsDirectory },
						luaL_Reg{ "copyFile", Path_copyFile },
						luaL_Reg{ "copyDirectory", Path_copyDirectory },
						luaL_Reg{ "moveFile", Path_moveFile },
						luaL_Reg{ "moveDirectory", Path_moveDirectory },
						luaL_Reg{ "removeFile", Path_removeFile },
						luaL_Reg{ "removeDirectory", Path_removeDirectory },
						luaL_Reg{ "touchFile", Path_touchFile },
						luaL_Reg{ "touchDirectory", Path_touchDirectory },
						luaL_Reg{ nullptr, nullptr }
					)
				)
			},
			luaL_Reg{ nullptr, nullptr }
		)
	);

	getGlobal(L, "Path");
	setTable(
		L,
		"executableFile", Path_executableFile(L),
		"documentDirectory", Path_documentDirectory(L),
		"writableDirectory", Path_writableDirectory(L),
		"savedGamesDirectory", Path_savedGamesDirectory(L)
	);
	pop(L);
}

static int FileInfo_ctor(lua_State* L) {
	const char* path = nullptr;
	read<>(L, path);

	if (path) {
		FileInfo::Ptr obj = FileInfo::make(path);
		if (!obj)
			return write(L, nullptr);

		return write(L, &obj);
	}

	return 0;
}

static int FileInfo_fullPath(lua_State* L) {
	FileInfo::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const std::string &ret = obj->get()->fullPath();

		return write(L, ret);
	}

	return 0;
}

static int FileInfo_parentPath(lua_State* L) {
	FileInfo::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const std::string &ret = obj->get()->parentPath();

		return write(L, ret);
	}

	return 0;
}

static int FileInfo_fileName(lua_State* L) {
	FileInfo::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const std::string &ret = obj->get()->fileName();

		return write(L, ret);
	}

	return 0;
}

static int FileInfo_extName(lua_State* L) {
	FileInfo::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const std::string &ret = obj->get()->extName();

		return write(L, ret);
	}

	return 0;
}

static int FileInfo_empty(lua_State* L) {
	FileInfo::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const bool ret = obj->get()->empty();

		return write(L, ret);
	}

	return 0;
}

static int FileInfo_exists(lua_State* L) {
	FileInfo::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const bool ret = obj->get()->exists();

		return write(L, ret);
	}

	return 0;
}

static int FileInfo_make(lua_State* L) {
	FileInfo::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const bool ret = obj->get()->make();

		return write(L, ret);
	}

	return 0;
}

static int FileInfo_copyTo(lua_State* L) {
	FileInfo::Ptr* obj = nullptr;
	const char* newPath = nullptr;
	read<>(L, obj, newPath);

	if (obj) {
		const bool ret = obj->get()->copyTo(newPath);

		return write(L, ret);
	}

	return 0;
}

static int FileInfo_moveTo(lua_State* L) {
	FileInfo::Ptr* obj = nullptr;
	const char* newPath = nullptr;
	read<>(L, obj, newPath);

	if (obj) {
		const bool ret = obj->get()->moveTo(newPath);

		return write(L, ret);
	}

	return 0;
}

static int FileInfo_remove(lua_State* L) {
	const int n = getTop(L);
	FileInfo::Ptr* obj = nullptr;
	bool toTrashBin = true;
	if (n >= 2)
		read<>(L, obj, toTrashBin);
	else
		read<>(L, obj);

	if (obj) {
		const bool ret = obj->get()->remove(toTrashBin);

		return write(L, ret);
	}

	return 0;
}

static int FileInfo_rename(lua_State* L) {
	const int n = getTop(L);
	FileInfo::Ptr* obj = nullptr;
	const char* newName = nullptr;
	const char* newExt = nullptr;
	if (n >= 3)
		read<>(L, obj, newName, newExt);
	else
		read<>(L, obj, newName);

	if (obj && newName) {
		const bool ret = obj->get()->rename(newName, newExt);

		return write(L, ret);
	}

	return 0;
}

static int FileInfo_parent(lua_State* L) {
	FileInfo::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		DirectoryInfo::Ptr parent = obj->get()->parent();

		return write(L, &parent);
	}

	return 0;
}

static int FileInfo_readAll(lua_State* L) {
	FileInfo::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const std::string ret = obj->get()->readAll();

		return write(L, ret);
	}

	return 0;
}

static void open_FileInfo(lua_State* L) {
	def(
		L, "FileInfo",
		LUA_LIB(
			array(
				luaL_Reg{ "new", FileInfo_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<FileInfo::Ptr> },
			luaL_Reg{ "__tostring", __tostring<FileInfo::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "fullPath", FileInfo_fullPath },
			luaL_Reg{ "parentPath", FileInfo_parentPath },
			luaL_Reg{ "fileName", FileInfo_fileName },
			luaL_Reg{ "extName", FileInfo_extName },
			luaL_Reg{ "empty", FileInfo_empty },
			luaL_Reg{ "exists", FileInfo_exists },
			luaL_Reg{ "make", FileInfo_make },
			luaL_Reg{ "copyTo", FileInfo_copyTo },
			luaL_Reg{ "moveTo", FileInfo_moveTo },
			luaL_Reg{ "remove", FileInfo_remove },
			luaL_Reg{ "rename", FileInfo_rename },
			luaL_Reg{ "parent", FileInfo_parent },
			luaL_Reg{ "readAll", FileInfo_readAll },
			luaL_Reg{ nullptr, nullptr }
		),
		nullptr, nullptr
	);
}

static int DirectoryInfo_ctor(lua_State* L) {
	const char* path = nullptr;
	read<>(L, path);

	if (path) {
		DirectoryInfo::Ptr obj = DirectoryInfo::make(path);
		if (!obj)
			return write(L, nullptr);

		return write(L, &obj);
	}

	return 0;
}

static int DirectoryInfo_fullPath(lua_State* L) {
	DirectoryInfo::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const std::string &ret = obj->get()->fullPath();

		return write(L, ret);
	}

	return 0;
}

static int DirectoryInfo_parentPath(lua_State* L) {
	DirectoryInfo::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const std::string &ret = obj->get()->parentPath();

		return write(L, ret);
	}

	return 0;
}

static int DirectoryInfo_dirName(lua_State* L) {
	DirectoryInfo::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const std::string &ret = obj->get()->dirName();

		return write(L, ret);
	}

	return 0;
}

static int DirectoryInfo_empty(lua_State* L) {
	DirectoryInfo::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const bool ret = obj->get()->empty();

		return write(L, ret);
	}

	return 0;
}

static int DirectoryInfo_exists(lua_State* L) {
	DirectoryInfo::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const bool ret = obj->get()->exists();

		return write(L, ret);
	}

	return 0;
}

static int DirectoryInfo_make(lua_State* L) {
	DirectoryInfo::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const bool ret = obj->get()->make();

		return write(L, ret);
	}

	return 0;
}

static int DirectoryInfo_copyTo(lua_State* L) {
	DirectoryInfo::Ptr* obj = nullptr;
	const char* newPath = nullptr;
	read<>(L, obj, newPath);

	if (obj) {
		const bool ret = obj->get()->copyTo(newPath);

		return write(L, ret);
	}

	return 0;
}

static int DirectoryInfo_moveTo(lua_State* L) {
	DirectoryInfo::Ptr* obj = nullptr;
	const char* newPath = nullptr;
	read<>(L, obj, newPath);

	if (obj) {
		const bool ret = obj->get()->moveTo(newPath);

		return write(L, ret);
	}

	return 0;
}

static int DirectoryInfo_remove(lua_State* L) {
	const int n = getTop(L);
	DirectoryInfo::Ptr* obj = nullptr;
	bool toTrashBin = true;
	if (n >= 2)
		read<>(L, obj, toTrashBin);
	else
		read<>(L, obj);

	if (obj) {
		const bool ret = obj->get()->remove(toTrashBin);

		return write(L, ret);
	}

	return 0;
}

static int DirectoryInfo_rename(lua_State* L) {
	DirectoryInfo::Ptr* obj = nullptr;
	const char* newName = nullptr;
	read<>(L, obj, newName);

	if (obj && newName) {
		const bool ret = obj->get()->rename(newName);

		return write(L, ret);
	}

	return 0;
}

static int DirectoryInfo_getFiles(lua_State* L) {
	const int n = getTop(L);
	DirectoryInfo::Ptr* obj = nullptr;
	const char* pattern = "*;*.*";
	bool recursive = false;
	if (n >= 3)
		read<>(L, obj, pattern, recursive);
	else if (n == 2)
		read<>(L, obj, pattern);
	else
		read<>(L, obj);

	if (obj) {
		FileInfos::Ptr subs = obj->get()->getFiles(pattern, recursive);
		const int count = subs->count();
		std::list<FileInfo::Ptr> lst;
		for (int i = 0; i < count; ++i)
			lst.push_back(subs->get(i));

		return write(L, lst);
	}

	return 0;
}

static int DirectoryInfo_getDirectories(lua_State* L) {
	const int n = getTop(L);
	DirectoryInfo::Ptr* obj = nullptr;
	bool recursive = false;
	if (n >= 2)
		read<>(L, obj, recursive);
	else
		read<>(L, obj);

	if (obj) {
		DirectoryInfos::Ptr subs = obj->get()->getDirectories(recursive);
		const int count = subs->count();
		std::list<DirectoryInfo::Ptr> lst;
		for (int i = 0; i < count; ++i)
			lst.push_back(subs->get(i));

		return write(L, lst);
	}

	return 0;
}

static int DirectoryInfo_parent(lua_State* L) {
	DirectoryInfo::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		DirectoryInfo::Ptr parent = obj->get()->parent();

		return write(L, &parent);
	}

	return 0;
}

static void open_DirectoryInfo(lua_State* L) {
	def(
		L, "DirectoryInfo",
		LUA_LIB(
			array(
				luaL_Reg{ "new", DirectoryInfo_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<DirectoryInfo::Ptr> },
			luaL_Reg{ "__tostring", __tostring<DirectoryInfo::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "fullPath", DirectoryInfo_fullPath },
			luaL_Reg{ "parentPath", DirectoryInfo_parentPath },
			luaL_Reg{ "dirName", DirectoryInfo_dirName },
			luaL_Reg{ "empty", DirectoryInfo_empty },
			luaL_Reg{ "exists", DirectoryInfo_exists },
			luaL_Reg{ "make", DirectoryInfo_make },
			luaL_Reg{ "copyTo", DirectoryInfo_copyTo },
			luaL_Reg{ "moveTo", DirectoryInfo_moveTo },
			luaL_Reg{ "remove", DirectoryInfo_remove },
			luaL_Reg{ "rename", DirectoryInfo_rename },
			luaL_Reg{ "getFiles", DirectoryInfo_getFiles },
			luaL_Reg{ "getDirectories", DirectoryInfo_getDirectories },
			luaL_Reg{ "parent", DirectoryInfo_parent },
			luaL_Reg{ nullptr, nullptr }
		),
		nullptr, nullptr
	);
}

/**< Image. */

static int Image_ctor(lua_State* L) {
	const int n = getTop(L);
	Palette::Ptr* palette = nullptr;
	if (n >= 1)
		read<>(L, palette);

	Image::Ptr obj(Image::create(palette ? *palette : nullptr));
	if (!obj)
		return write(L, nullptr);

	return write(L, &obj);
}

static int Image_resize(lua_State* L) {
	const int n = getTop(L);
	Image::Ptr* obj = nullptr;
	int width = 0, height = 0;
	bool stretch = true;
	if (n >= 4)
		read<>(L, obj, width, height, stretch);
	else
		read<>(L, obj, width, height);

	if (obj) {
		const bool ret = obj->get()->resize(width, height, stretch);

		return write(L, ret);
	}

	return 0;
}

static int Image_get(lua_State* L) {
	Image::Ptr* obj = nullptr;
	int x = 0, y = 0;
	Color col;
	int index = 0;
	read<>(L, obj, x, y);

	if (obj) {
		if (obj->get()->paletted()) {
			obj->get()->get(x, y, index);

			return write(L, index);
		} else {
			obj->get()->get(x, y, col);

			return write(L, &col);
		}
	}

	return 0;
}

static int Image_set(lua_State* L) {
	Image::Ptr* obj = nullptr;
	int x = 0, y = 0;
	read<>(L, obj, x, y);

	if (obj) {
		if (obj->get()->paletted()) {
			int index = 0;
			read<4>(L, index);

			const bool ret = obj->get()->set(x, y, index);

			return write(L, ret);
		} else {
			Color* col = nullptr;
			read<4>(L, col);

			if (col) {
				const bool ret = obj->get()->set(x, y, *col);

				return write(L, ret);
			}

			return 0;
		}
	}

	return 0;
}

static int Image_blit(lua_State* L) {
	const int n = getTop(L);
	Image::Ptr* obj = nullptr;
	Image::Ptr* other = nullptr;
	int x = 0, y = 0, w = 0, h = 0;
	int sx = 0, sy = 0;
	if (n >= 8)
		read<>(L, obj, other, x, y, w, h, sx, sy);
	else if (n == 6)
		read<>(L, obj, other, x, y, w, h);
	else
		read<>(L, obj, other, x, y);

	if (obj && other) {
		const bool ret = obj->get()->blit(other->get(), x, y, w, h, sx, sy);

		return write(L, ret);
	}

	return 0;
}

static int Image_fromImage(lua_State* L) {
	Image::Ptr* obj = nullptr;
	Image::Ptr* other = nullptr;
	read<>(L, obj, other);

	if (obj && other) {
		const bool ret = obj->get()->fromImage(other->get());

		return write(L, ret);
	}

	return 0;
}

static int Image_fromBlank(lua_State* L) {
	const int n = getTop(L);
	Image::Ptr* obj = nullptr;
	int width = 0, height = 0;
	int paletted = 0;
	if (n >= 4)
		read<>(L, obj, width, height, paletted);
	else
		read<>(L, obj, width, height);

	if (obj) {
		const bool ret = obj->get()->fromBlank(width, height, paletted);

		return write(L, ret);
	}

	return 0;
}

static int Image_toBytes(lua_State* L) {
	const int n = getTop(L);
	Image::Ptr* obj = nullptr;
	Bytes::Ptr* val = nullptr;
	const char* type = "png";
	if (n >= 3)
		read<>(L, obj, val, type);
	else if (n == 2)
		read<>(L, obj, val);
	else
		read<>(L, obj);

	if (obj) {
		Bytes::Ptr ptr = nullptr;
		if (!val) {
			ptr = Bytes::Ptr(Bytes::create());
			val = &ptr;
		}

		if (val && val->get() && obj->get()->toBytes(val->get(), type))
			return write(L, val);
		else
			return write(L, nullptr);
	}

	return 0;
}

static int Image_fromBytes(lua_State* L) {
	Image::Ptr* obj = nullptr;
	Bytes::Ptr* val = nullptr;
	read<>(L, obj, val);

	if (obj && val) {
		const bool ret = obj->get()->fromBytes(val->get());

		return write(L, ret);
	}

	return 0;
}

static int Image___index(lua_State* L) {
	Image::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "width") == 0) {
		const int ret = obj->get()->width();

		return write(L, ret);
	} else if (strcmp(field, "height") == 0) {
		const int ret = obj->get()->height();

		return write(L, ret);
	} else if (strcmp(field, "channels") == 0) { // Undocumented.
		const int ret = obj->get()->channels();

		return write(L, ret);
	} else {
		return __index(L, field);
	}
}

static int Image___newindex(lua_State* L) {
	Image::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	return 0;
}

static void open_Image(lua_State* L) {
	def(
		L, "Image",
		LUA_LIB(
			array(
				luaL_Reg{ "new", Image_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Image::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Image::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "resize", Image_resize },
			luaL_Reg{ "get", Image_get },
			luaL_Reg{ "set", Image_set },
			luaL_Reg{ "blit", Image_blit },
			luaL_Reg{ "fromImage", Image_fromImage },
			luaL_Reg{ "fromBlank", Image_fromBlank },
			luaL_Reg{ "toBytes", Image_toBytes },
			luaL_Reg{ "fromBytes", Image_fromBytes },
			luaL_Reg{ nullptr, nullptr }
		),
		Image___index, Image___newindex
	);
}

/**< JSON. */

static int Json_ctor(lua_State* L) {
	Json::Ptr obj(Json::create());
	if (!obj)
		return write(L, nullptr);

	return write(L, &obj);
}

static int Json_toString(lua_State* L) {
	const int n = getTop(L);
	Json::Ptr* obj = nullptr;
	bool pretty = true;
	if (n >= 2)
		read<>(L, obj, pretty);
	else
		read<>(L, obj);

	if (obj) {
		std::string val;
		if (obj->get()->toString(val, pretty))
			return write(L, val);

		return 0;
	}

	return 0;
}

static int Json_fromString(lua_State* L) {
	Json::Ptr* obj = nullptr;
	std::string val;
	read<>(L, obj, val);

	if (obj) {
		const bool ret = obj->get()->fromString(val);

		return write(L, ret);
	}

	return 0;
}

static int Json_toTable(lua_State* L) {
	const int n = getTop(L);
	Json::Ptr* obj = nullptr;
	bool allowNull = false;
	if (n >= 2)
		read<>(L, obj, allowNull);
	else
		read<>(L, obj);

	if (obj) {
		rapidjson::Document doc;
		if (obj->get()->toJson(doc))
			return write_(L, doc, allowNull);

		return 0;
	}

	return 0;
}

static int Json_fromTable(lua_State* L) {
	Json::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		rapidjson::Document doc;
		read(L, doc, Index(2));

		const bool ret = obj->get()->fromJson(doc);

		return write(L, ret);
	}

	return 0;
}

static void open_Json(lua_State* L) {
	def(
		L, "Json",
		LUA_LIB(
			array(
				luaL_Reg{ "new", Json_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Json::Ptr> },
			luaL_Reg{ "__tostring", Json_toString },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "toString", Json_toString },
			luaL_Reg{ "fromString", Json_fromString },
			luaL_Reg{ "toTable", Json_toTable },
			luaL_Reg{ "fromTable", Json_fromTable },
			luaL_Reg{ nullptr, nullptr }
		),
		nullptr, nullptr
	);

	getGlobal(L, "Json");
	const LightUserdata Null;
	setTable(
		L,
		"Null", Null
	);
	pop(L);
}

/**< Math. */

static int Vec2_ctor(lua_State* L) {
	const int n = getTop(L);
	Math::Vec2f obj;
	if (n >= 2) {
		Math::Vec2f::ValueType x = 0, y = 0;
		read<>(L, x, y);

		obj = Math::Vec2f(x, y);
	}

	return write(L, &obj);
}

static int Vec2___tostring(lua_State* L) {
	Math::Vec2f* obj = nullptr;
	check<>(L, obj);

	std::string str;

	str += "Vec2[";
	str += Text::toString(obj->x);
	str += ", ";
	str += Text::toString(obj->y);
	str += "]";

	return write(L, str);
}

static int Vec2___add(lua_State* L) {
	Math::Vec2f* obj = nullptr;
	Math::Vec2f* other = nullptr;
	check<>(L, obj, other);

	if (obj && other) {
		const Math::Vec2f ret = *obj + *other;

		return write(L, &ret);
	}

	return 0;
}

static int Vec2___sub(lua_State* L) {
	Math::Vec2f* obj = nullptr;
	Math::Vec2f* other = nullptr;
	check<>(L, obj, other);

	if (obj && other) {
		const Math::Vec2f ret = *obj - *other;

		return write(L, &ret);
	}

	return 0;
}

static int Vec2___mul(lua_State* L) {
	Math::Vec2f* obj = nullptr;
	Math::Vec2f* other = nullptr;
	Math::Vec2f::ValueType num = 0;
	check<>(L, obj);

	if (obj) {
		if (isNumber(L, 2)) {
			check<2>(L, num);

			const Math::Vec2f ret = *obj * num;

			return write(L, &ret);
		} else {
			check<2>(L, other);
			if (!other)
				return 0;

			const Math::Vec2f ret = *obj * *other;

			return write(L, &ret);
		}
	}

	return 0;
}

static int Vec2___unm(lua_State* L) {
	Math::Vec2f* obj = nullptr;
	check<>(L, obj);

	if (obj) {
		const Math::Vec2f ret = -*obj;

		return write(L, &ret);
	}

	return 0;
}

static int Vec2___len(lua_State* L) {
	Math::Vec2f* obj = nullptr;
	check<>(L, obj);

	if (obj) {
		const Real ret = obj->length();

		return write(L, ret);
	}

	return 0;
}

static int Vec2___eq(lua_State* L) {
	Math::Vec2f* obj = nullptr;
	Math::Vec2f* other = nullptr;
	check<>(L, obj, other);

	if (obj && other) {
		const bool ret = *obj == *other;

		return write(L, ret);
	}

	return write(L, false);
}

static int Vec2_normalize(lua_State* L) {
	Math::Vec2f* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Real ret = obj->normalize();

		return write(L, ret);
	}

	return 0;
}

static int Vec2_distanceTo(lua_State* L) {
	Math::Vec2f* obj = nullptr;
	Math::Vec2f* other = nullptr;
	read<>(L, obj, other);

	if (obj && other) {
		const Real ret = obj->distanceTo(*other);

		return write(L, ret);
	}

	return 0;
}

static int Vec2_dot(lua_State* L) {
	Math::Vec2f* obj = nullptr;
	Math::Vec2f* other = nullptr;
	read<>(L, obj, other);

	if (obj && other) {
		const Real ret = obj->dot(*other);

		return write(L, ret);
	}

	return 0;
}

static int Vec2_cross(lua_State* L) {
	Math::Vec2f* obj = nullptr;
	Math::Vec2f* other = nullptr;
	Math::Vec2f::ValueType num = 0;
	read<>(L, obj);

	if (obj) {
		if (isNumber(L, 2)) {
			read<2>(L, num);

			const Math::Vec2f ret = obj->cross(num);

			return write(L, &ret);
		} else {
			read<2>(L, other);
			if (!other)
				return 0;

			const Real ret = obj->cross(*other);

			return write(L, ret);
		}
	}

	return 0;
}

static int Vec2_angleTo(lua_State* L) {
	Math::Vec2f* obj = nullptr;
	Math::Vec2f* other = nullptr;
	read<>(L, obj, other);

	if (obj && other) {
		const Real ret = obj->angleTo(*other);

		return write(L, ret);
	}

	return 0;
}

static int Vec2_rotated(lua_State* L) {
	const int n = getTop(L);
	Math::Vec2f* obj = nullptr;
	Math::Vec2f::ValueType angle = 0;
	Math::Rotf* rot = nullptr;
	Math::Vec2f* pivot = nullptr;
	read<>(L, obj);
	if (isNumber(L, 2)) {
		read<2>(L, angle);
	} else {
		read<2>(L, rot);
		angle = rot->angle();
	}
	if (n >= 3)
		read<3>(L, pivot);

	if (obj) {
		if (pivot) {
			const Math::Vec2f ret = obj->rotated(angle, *pivot);

			return write(L, &ret);
		} else {
			const Math::Vec2f ret = obj->rotated(angle);

			return write(L, &ret);
		}
	}

	return 0;
}

static int Vec2___index(lua_State* L) {
	Math::Vec2f* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "x") == 0) {
		return write(L, obj->x);
	} else if (strcmp(field, "y") == 0) {
		return write(L, obj->y);
	} else if (strcmp(field, "normalized") == 0) {
		const Math::Vec2f ret = obj->normalized();

		return write(L, &ret);
	} else if (strcmp(field, "length") == 0) {
		const Real ret = obj->length();

		return write(L, ret);
	} else if (strcmp(field, "angle") == 0) {
		const Real ret = obj->angle();

		return write(L, ret);
	} else {
		return __index(L, field);
	}
}

static int Vec2___newindex(lua_State* L) {
	Math::Vec2f* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "x") == 0) {
		Math::Vec2f::ValueType val = 0;
		read<3>(L, val);
		obj->x = val;
	} else if (strcmp(field, "y") == 0) {
		Math::Vec2f::ValueType val = 0;
		read<3>(L, val);
		obj->y = val;
	}

	return 0;
}

static void open_Vec2(lua_State* L) {
	def(
		L, "Vec2",
		LUA_LIB(
			array(
				luaL_Reg{ "new", Vec2_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Math::Vec2f> },
			luaL_Reg{ "__tostring", Vec2___tostring },
			luaL_Reg{ "__add", Vec2___add },
			luaL_Reg{ "__sub", Vec2___sub },
			luaL_Reg{ "__mul", Vec2___mul },
			luaL_Reg{ "__unm", Vec2___unm },
			luaL_Reg{ "__len", Vec2___len },
			luaL_Reg{ "__eq", Vec2___eq },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "normalize", Vec2_normalize },
			luaL_Reg{ "distanceTo", Vec2_distanceTo },
			luaL_Reg{ "dot", Vec2_dot },
			luaL_Reg{ "cross", Vec2_cross },
			luaL_Reg{ "angleTo", Vec2_angleTo },
			luaL_Reg{ "rotated", Vec2_rotated },
			luaL_Reg{ nullptr, nullptr }
		),
		Vec2___index, Vec2___newindex
	);
}

static int Vec3_ctor(lua_State* L) {
	const int n = getTop(L);
	Math::Vec3f obj;
	if (n >= 3) {
		Math::Vec3f::ValueType x = 0, y = 0, z = 0;
		read<>(L, x, y, z);

		obj = Math::Vec3f(x, y, z);
	}

	return write(L, &obj);
}

static int Vec3___tostring(lua_State* L) {
	Math::Vec3f* obj = nullptr;
	check<>(L, obj);

	std::string str;

	str += "Vec3[";
	str += Text::toString(obj->x);
	str += ", ";
	str += Text::toString(obj->y);
	str += ", ";
	str += Text::toString(obj->z);
	str += "]";

	return write(L, str);
}

static int Vec3___add(lua_State* L) {
	Math::Vec3f* obj = nullptr;
	Math::Vec3f* other = nullptr;
	check<>(L, obj, other);

	if (obj && other) {
		const Math::Vec3f ret = *obj + *other;

		return write(L, &ret);
	}

	return 0;
}

static int Vec3___sub(lua_State* L) {
	Math::Vec3f* obj = nullptr;
	Math::Vec3f* other = nullptr;
	check<>(L, obj, other);

	if (obj && other) {
		const Math::Vec3f ret = *obj - *other;

		return write(L, &ret);
	}

	return 0;
}

static int Vec3___mul(lua_State* L) {
	Math::Vec3f* obj = nullptr;
	Math::Vec3f* other = nullptr;
	Math::Vec3f::ValueType num = 0;
	check<>(L, obj);

	if (obj) {
		if (isNumber(L, 2)) {
			check<2>(L, num);

			const Math::Vec3f ret = *obj * num;

			return write(L, &ret);
		} else {
			check<2>(L, other);
			if (!other)
				return 0;

			const Math::Vec3f ret = *obj * *other;

			return write(L, &ret);
		}
	}

	return 0;
}

static int Vec3___unm(lua_State* L) {
	Math::Vec3f* obj = nullptr;
	check<>(L, obj);

	if (obj) {
		const Math::Vec3f ret = -*obj;

		return write(L, &ret);
	}

	return 0;
}

static int Vec3___len(lua_State* L) {
	Math::Vec3f* obj = nullptr;
	check<>(L, obj);

	if (obj) {
		const Real ret = obj->length();

		return write(L, ret);
	}

	return 0;
}

static int Vec3___eq(lua_State* L) {
	Math::Vec3f* obj = nullptr;
	Math::Vec3f* other = nullptr;
	check<>(L, obj, other);

	if (obj && other) {
		const bool ret = *obj == *other;

		return write(L, ret);
	}

	return write(L, false);
}

static int Vec3_normalize(lua_State* L) {
	Math::Vec3f* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Real ret = obj->normalize();

		return write(L, ret);
	}

	return 0;
}

static int Vec3_dot(lua_State* L) {
	Math::Vec3f* obj = nullptr;
	Math::Vec3f* other = nullptr;
	read<>(L, obj, other);

	if (obj && other) {
		const Real ret = obj->dot(*other);

		return write(L, ret);
	}

	return 0;
}

static int Vec3___index(lua_State* L) {
	Math::Vec3f* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "x") == 0) {
		return write(L, obj->x);
	} else if (strcmp(field, "y") == 0) {
		return write(L, obj->y);
	} else if (strcmp(field, "z") == 0) {
		return write(L, obj->z);
	} else if (strcmp(field, "normalized") == 0) {
		const Math::Vec3f ret = obj->normalized();

		return write(L, &ret);
	} else if (strcmp(field, "length") == 0) {
		const Real ret = obj->length();

		return write(L, ret);
	} else {
		return __index(L, field);
	}
}

static int Vec3___newindex(lua_State* L) {
	Math::Vec3f* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "x") == 0) {
		Math::Vec3f::ValueType val = 0;
		read<3>(L, val);
		obj->x = val;
	} else if (strcmp(field, "y") == 0) {
		Math::Vec3f::ValueType val = 0;
		read<3>(L, val);
		obj->y = val;
	} else if (strcmp(field, "z") == 0) {
		Math::Vec3f::ValueType val = 0;
		read<3>(L, val);
		obj->z = val;
	}

	return 0;
}

static void open_Vec3(lua_State* L) {
	def(
		L, "Vec3",
		LUA_LIB(
			array(
				luaL_Reg{ "new", Vec3_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Math::Vec3f> },
			luaL_Reg{ "__tostring", Vec3___tostring },
			luaL_Reg{ "__add", Vec3___add },
			luaL_Reg{ "__sub", Vec3___sub },
			luaL_Reg{ "__mul", Vec3___mul },
			luaL_Reg{ "__unm", Vec3___unm },
			luaL_Reg{ "__len", Vec3___len },
			luaL_Reg{ "__eq", Vec3___eq },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "normalize", Vec3_normalize },
			luaL_Reg{ "dot", Vec3_dot },
			luaL_Reg{ nullptr, nullptr }
		),
		Vec3___index, Vec3___newindex
	);
}

static int Vec4_ctor(lua_State* L) {
	const int n = getTop(L);
	Math::Vec4f obj;
	if (n >= 4) {
		Math::Vec4f::ValueType x = 0, y = 0, z = 0, w = 0;
		read<>(L, x, y, z, w);

		obj = Math::Vec4f(x, y, z, w);
	}

	return write(L, &obj);
}

static int Vec4___tostring(lua_State* L) {
	Math::Vec4f* obj = nullptr;
	check<>(L, obj);

	std::string str;

	str += "Vec4[";
	str += Text::toString(obj->x);
	str += ", ";
	str += Text::toString(obj->y);
	str += ", ";
	str += Text::toString(obj->z);
	str += ", ";
	str += Text::toString(obj->w);
	str += "]";

	return write(L, str);
}

static int Vec4___add(lua_State* L) {
	Math::Vec4f* obj = nullptr;
	Math::Vec4f* other = nullptr;
	check<>(L, obj, other);

	if (obj && other) {
		const Math::Vec4f ret = *obj + *other;

		return write(L, &ret);
	}

	return 0;
}

static int Vec4___sub(lua_State* L) {
	Math::Vec4f* obj = nullptr;
	Math::Vec4f* other = nullptr;
	check<>(L, obj, other);

	if (obj && other) {
		const Math::Vec4f ret = *obj - *other;

		return write(L, &ret);
	}

	return 0;
}

static int Vec4___mul(lua_State* L) {
	Math::Vec4f* obj = nullptr;
	Math::Vec4f* other = nullptr;
	Math::Vec4f::ValueType num = 0;
	check<>(L, obj);

	if (obj) {
		if (isNumber(L, 2)) {
			check<2>(L, num);

			const Math::Vec4f ret = *obj * num;

			return write(L, &ret);
		} else {
			check<2>(L, other);
			if (!other)
				return 0;

			const Math::Vec4f ret = *obj * *other;

			return write(L, &ret);
		}
	}

	return 0;
}

static int Vec4___unm(lua_State* L) {
	Math::Vec4f* obj = nullptr;
	check<>(L, obj);

	if (obj) {
		const Math::Vec4f ret = -*obj;

		return write(L, &ret);
	}

	return 0;
}

static int Vec4___eq(lua_State* L) {
	Math::Vec4f* obj = nullptr;
	Math::Vec4f* other = nullptr;
	check<>(L, obj, other);

	if (obj && other) {
		const bool ret = *obj == *other;

		return write(L, ret);
	}

	return write(L, false);
}

static int Vec4___index(lua_State* L) {
	Math::Vec4f* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "x") == 0) {
		return write(L, obj->x);
	} else if (strcmp(field, "y") == 0) {
		return write(L, obj->y);
	} else if (strcmp(field, "z") == 0) {
		return write(L, obj->z);
	} else if (strcmp(field, "w") == 0) {
		return write(L, obj->w);
	} else {
		return __index(L, field);
	}
}

static int Vec4___newindex(lua_State* L) {
	Math::Vec4f* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "x") == 0) {
		Math::Vec4f::ValueType val = 0;
		read<3>(L, val);
		obj->x = val;
	} else if (strcmp(field, "y") == 0) {
		Math::Vec4f::ValueType val = 0;
		read<3>(L, val);
		obj->y = val;
	} else if (strcmp(field, "z") == 0) {
		Math::Vec4f::ValueType val = 0;
		read<3>(L, val);
		obj->z = val;
	} else if (strcmp(field, "w") == 0) {
		Math::Vec4f::ValueType val = 0;
		read<3>(L, val);
		obj->w = val;
	}

	return 0;
}

static void open_Vec4(lua_State* L) {
	def(
		L, "Vec4",
		LUA_LIB(
			array(
				luaL_Reg{ "new", Vec4_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Math::Vec4f> },
			luaL_Reg{ "__tostring", Vec4___tostring },
			luaL_Reg{ "__add", Vec4___add },
			luaL_Reg{ "__sub", Vec4___sub },
			luaL_Reg{ "__mul", Vec4___mul },
			luaL_Reg{ "__unm", Vec4___unm },
			luaL_Reg{ "__eq", Vec4___eq },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ nullptr, nullptr }
		),
		Vec4___index, Vec4___newindex
	);
}

static int Rect_ctor(lua_State* L) {
	const int n = getTop(L);
	Math::Rectf obj;
	if (n >= 4) {
		Math::Rectf::ValueType x0 = 0, y0 = 0, x1 = 0, y1 = 0;
		read<>(L, x0, y0, x1, y1);

		obj = Math::Rectf(x0, y0, x1, y1);
	}

	return write(L, &obj);
}

static int Rect_byXYWH(lua_State* L) {
	const int n = getTop(L);
	Math::Rectf obj;
	if (n >= 4) {
		Math::Rectf::ValueType x = 0, y = 0, w = 0, h = 0;
		read<>(L, x, y, w, h);

		obj = Math::Rectf::byXYWH(x, y, w, h);
	}

	return write(L, &obj);
}

static int Rect___tostring(lua_State* L) {
	Math::Rectf* obj = nullptr;
	check<>(L, obj);

	std::string str;

	str += "Rect[";
	str += Text::toString(obj->x0);
	str += ", ";
	str += Text::toString(obj->y0);
	str += ", ";
	str += Text::toString(obj->x1);
	str += ", ";
	str += Text::toString(obj->y1);
	str += "]";

	return write(L, str);
}

static int Rect___eq(lua_State* L) {
	Math::Rectf* obj = nullptr;
	Math::Rectf* other = nullptr;
	check<>(L, obj, other);

	if (obj && other) {
		const bool ret = *obj == *other;

		return write(L, ret);
	}

	return write(L, false);
}

static int Rect_xMin(lua_State* L) {
	Math::Rectf* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Real ret = obj->xMin();

		return write(L, ret);
	}

	return 0;
}

static int Rect_yMin(lua_State* L) {
	Math::Rectf* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Real ret = obj->yMin();

		return write(L, ret);
	}

	return 0;
}

static int Rect_xMax(lua_State* L) {
	Math::Rectf* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Real ret = obj->xMax();

		return write(L, ret);
	}

	return 0;
}

static int Rect_yMax(lua_State* L) {
	Math::Rectf* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Real ret = obj->yMax();

		return write(L, ret);
	}

	return 0;
}

static int Rect_width(lua_State* L) {
	Math::Rectf* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Real ret = obj->width();

		return write(L, ret);
	}

	return 0;
}

static int Rect_height(lua_State* L) {
	Math::Rectf* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Real ret = obj->height();

		return write(L, ret);
	}

	return 0;
}

static int Rect___index(lua_State* L) {
	Math::Rectf* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "x0") == 0) {
		return write(L, obj->x0);
	} else if (strcmp(field, "y0") == 0) {
		return write(L, obj->y0);
	} else if (strcmp(field, "x1") == 0) {
		return write(L, obj->x1);
	} else if (strcmp(field, "y1") == 0) {
		return write(L, obj->y1);
	} else {
		return __index(L, field);
	}
}

static int Rect___newindex(lua_State* L) {
	Math::Rectf* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "x0") == 0) {
		Math::Rectf::ValueType val = 0;
		read<3>(L, val);
		obj->x0 = val;
	} else if (strcmp(field, "y0") == 0) {
		Math::Rectf::ValueType val = 0;
		read<3>(L, val);
		obj->y0 = val;
	} else if (strcmp(field, "x1") == 0) {
		Math::Rectf::ValueType val = 0;
		read<3>(L, val);
		obj->x1 = val;
	} else if (strcmp(field, "y1") == 0) {
		Math::Rectf::ValueType val = 0;
		read<3>(L, val);
		obj->y1 = val;
	}

	return 0;
}

static void open_Rect(lua_State* L) {
	def(
		L, "Rect",
		LUA_LIB(
			array(
				luaL_Reg{ "new", Rect_ctor },
				luaL_Reg{ "byXYWH", Rect_byXYWH },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Math::Rectf> },
			luaL_Reg{ "__tostring", Rect___tostring },
			luaL_Reg{ "__eq", Rect___eq },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "xMin", Rect_xMin },
			luaL_Reg{ "yMin", Rect_yMin },
			luaL_Reg{ "xMax", Rect_xMax },
			luaL_Reg{ "yMax", Rect_yMax },
			luaL_Reg{ "width", Rect_width },
			luaL_Reg{ "height", Rect_height },
			luaL_Reg{ nullptr, nullptr }
		),
		Rect___index, Rect___newindex
	);
}

static int Recti_ctor(lua_State* L) {
	const int n = getTop(L);
	Math::Recti obj;
	if (n >= 4) {
		Math::Recti::ValueType x0 = 0, y0 = 0, x1 = 0, y1 = 0;
		read<>(L, x0, y0, x1, y1);

		obj = Math::Recti(x0, y0, x1, y1);
	}

	return write(L, &obj);
}

static int Recti_byXYWH(lua_State* L) {
	const int n = getTop(L);
	Math::Recti obj;
	if (n >= 4) {
		Math::Recti::ValueType x = 0, y = 0, w = 0, h = 0;
		read<>(L, x, y, w, h);

		obj = Math::Recti::byXYWH(x, y, w, h);
	}

	return write(L, &obj);
}

static int Recti___tostring(lua_State* L) {
	Math::Recti* obj = nullptr;
	check<>(L, obj);

	std::string str;

	str += "Recti[";
	str += Text::toString(obj->x0);
	str += ", ";
	str += Text::toString(obj->y0);
	str += ", ";
	str += Text::toString(obj->x1);
	str += ", ";
	str += Text::toString(obj->y1);
	str += "]";

	return write(L, str);
}

static int Recti___eq(lua_State* L) {
	Math::Recti* obj = nullptr;
	Math::Recti* other = nullptr;
	check<>(L, obj, other);

	if (obj && other) {
		const bool ret = *obj == *other;

		return write(L, ret);
	}

	return write(L, false);
}

static int Recti_xMin(lua_State* L) {
	Math::Recti* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Int ret = obj->xMin();

		return write(L, ret);
	}

	return 0;
}

static int Recti_yMin(lua_State* L) {
	Math::Recti* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Int ret = obj->yMin();

		return write(L, ret);
	}

	return 0;
}

static int Recti_xMax(lua_State* L) {
	Math::Recti* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Int ret = obj->xMax();

		return write(L, ret);
	}

	return 0;
}

static int Recti_yMax(lua_State* L) {
	Math::Recti* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Int ret = obj->yMax();

		return write(L, ret);
	}

	return 0;
}

static int Recti_width(lua_State* L) {
	Math::Recti* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Int ret = obj->width();

		return write(L, ret);
	}

	return 0;
}

static int Recti_height(lua_State* L) {
	Math::Recti* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const Int ret = obj->height();

		return write(L, ret);
	}

	return 0;
}

static int Recti___index(lua_State* L) {
	Math::Recti* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "x0") == 0) {
		return write(L, obj->x0);
	} else if (strcmp(field, "y0") == 0) {
		return write(L, obj->y0);
	} else if (strcmp(field, "x1") == 0) {
		return write(L, obj->x1);
	} else if (strcmp(field, "y1") == 0) {
		return write(L, obj->y1);
	} else {
		return __index(L, field);
	}
}

static int Recti___newindex(lua_State* L) {
	Math::Recti* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "x0") == 0) {
		Math::Recti::ValueType val = 0;
		read<3>(L, val);
		obj->x0 = val;
	} else if (strcmp(field, "y0") == 0) {
		Math::Recti::ValueType val = 0;
		read<3>(L, val);
		obj->y0 = val;
	} else if (strcmp(field, "x1") == 0) {
		Math::Recti::ValueType val = 0;
		read<3>(L, val);
		obj->x1 = val;
	} else if (strcmp(field, "y1") == 0) {
		Math::Recti::ValueType val = 0;
		read<3>(L, val);
		obj->y1 = val;
	}

	return 0;
}

static void open_Recti(lua_State* L) {
	def(
		L, "Recti",
		LUA_LIB(
			array(
				luaL_Reg{ "new", Recti_ctor },
				luaL_Reg{ "byXYWH", Recti_byXYWH },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Math::Recti> },
			luaL_Reg{ "__tostring", Recti___tostring },
			luaL_Reg{ "__eq", Recti___eq },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "xMin", Recti_xMin },
			luaL_Reg{ "yMin", Recti_yMin },
			luaL_Reg{ "xMax", Recti_xMax },
			luaL_Reg{ "yMax", Recti_yMax },
			luaL_Reg{ "width", Recti_width },
			luaL_Reg{ "height", Recti_height },
			luaL_Reg{ nullptr, nullptr }
		),
		Recti___index, Recti___newindex
	);
}

static int Rot_ctor(lua_State* L) {
	const int n = getTop(L);
	Math::Rotf obj;
	if (n >= 2) {
		Math::Rotf::ValueType s = 0, c = 0;
		read<>(L, s, c);

		obj = Math::Rotf(s, c);
	}

	return write(L, &obj);
}

static int Rot___tostring(lua_State* L) {
	Math::Rotf* obj = nullptr;
	check<>(L, obj);

	std::string str;

	str += "Rot[";
	str += Text::toString(obj->s);
	str += ", ";
	str += Text::toString(obj->c);
	str += "]";

	return write(L, str);
}

static int Rot___add(lua_State* L) {
	Math::Rotf* obj = nullptr;
	Math::Rotf* other = nullptr;
	check<>(L, obj, other);

	if (obj && other) {
		const Math::Rotf ret(obj->angle() + other->angle());

		return write(L, &ret);
	}

	return 0;
}

static int Rot___sub(lua_State* L) {
	Math::Rotf* obj = nullptr;
	Math::Rotf* other = nullptr;
	check<>(L, obj, other);

	if (obj && other) {
		const Math::Rotf ret(obj->angle() - other->angle());

		return write(L, &ret);
	}

	return 0;
}

static int Rot___mul(lua_State* L) {
	Math::Rotf* obj = nullptr;
	Math::Rotf* other = nullptr;
	Math::Vec2f* vec2 = nullptr;
	check<>(L, obj);

	if (obj) {
		read<2>(L, other);
		read<2>(L, vec2);
		if (other) {
			const Math::Rotf ret = *obj * *other;

			return write(L, &ret);
		} else if (vec2) {
			const Math::Vec2f ret = *obj * *vec2;

			return write(L, &ret);
		}
	}

	return 0;
}

static int Rot___unm(lua_State* L) {
	Math::Rotf* obj = nullptr;
	check<>(L, obj);

	if (obj) {
		const Math::Rotf ret(-obj->angle());

		return write(L, &ret);
	}

	return 0;
}

static int Rot___eq(lua_State* L) {
	Math::Rotf* obj = nullptr;
	Math::Rotf* other = nullptr;
	check<>(L, obj, other);

	if (obj && other) {
		const bool ret = *obj == *other;

		return write(L, ret);
	}

	return write(L, false);
}

static int Rot___index(lua_State* L) {
	Math::Rotf* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "s") == 0) {
		return write(L, obj->s);
	} else if (strcmp(field, "c") == 0) {
		return write(L, obj->c);
	} else if (strcmp(field, "angle") == 0) {
		const Real ret = obj->angle();

		return write(L, ret);
	} else {
		return __index(L, field);
	}
}

static int Rot___newindex(lua_State* L) {
	Math::Rotf* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "s") == 0) {
		Math::Rotf::ValueType val = 0;
		read<3>(L, val);
		obj->s = val;
	} else if (strcmp(field, "c") == 0) {
		Math::Rotf::ValueType val = 0;
		read<3>(L, val);
		obj->c = val;
	} else if (strcmp(field, "angle") == 0) {
		Math::Rotf::ValueType val = 0;
		read<3>(L, val);
		obj->angle(val);
	}

	return 0;
}

static void open_Rot(lua_State* L) {
	def(
		L, "Rot",
		LUA_LIB(
			array(
				luaL_Reg{ "new", Rot_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Math::Rotf> },
			luaL_Reg{ "__tostring", Rot___tostring },
			luaL_Reg{ "__add", Rot___add },
			luaL_Reg{ "__sub", Rot___sub },
			luaL_Reg{ "__mul", Rot___mul },
			luaL_Reg{ "__unm", Rot___unm },
			luaL_Reg{ "__eq", Rot___eq },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ nullptr, nullptr }
		),
		Rot___index, Rot___newindex
	);
}

static int Math_intersects(lua_State* L) {
	Math::Vec2f* point0 = nullptr;
	Math::Vec4f* line0 = nullptr;
	Math::Vec3f* circ0 = nullptr;
	Math::Rectf* rect0 = nullptr;
	Math::Recti* recti0 = nullptr;
	Math::Vec2f* point1 = nullptr;
	Math::Vec4f* line1 = nullptr;
	Math::Vec3f* circ1 = nullptr;
	Math::Rectf* rect1 = nullptr;
	Math::Recti* recti1 = nullptr;
	Placeholder _1, _2;
	do {
		read<>(L, point0, _2);
		if (point0)
			break;
		read<>(L, line0, _2);
		if (line0)
			break;
		read<>(L, circ0, _2);
		if (circ0)
			break;
		read<>(L, rect0, _2);
		if (rect0)
			break;
		read<>(L, recti0, _2);
		if (recti0)
			break;

		if (isNil(L, 1))
			return write(L, false);

		error(L, "Invalid shape.");
	} while (false);
	do {
		read<>(L, _1, point1);
		if (point1)
			break;
		read<>(L, _1, line1);
		if (line1)
			break;
		read<>(L, _1, circ1);
		if (circ1)
			break;
		read<>(L, _1, rect1);
		if (rect1)
			break;
		read<>(L, _1, recti1);
		if (recti1)
			break;

		if (isNil(L, 2))
			return write(L, false);

		error(L, "Invalid shape.");
	} while (false);

	Math::Rectf rect0_, rect1_;
	if (recti0) {
		rect0_ = Math::Rectf::byXYWH(
			recti0->xMin(), recti0->yMin(),
			recti0->width(), recti0->height()
		);
		rect0 = &rect0_;
	}
	if (recti1) {
		rect1_ = Math::Rectf::byXYWH(
			recti1->xMin(), recti1->yMin(),
			recti1->width(), recti1->height()
		);
		rect1 = &rect1_;
	}

	bool ret = false;
	if (point0 && point1) {
		ret = Math::intersects(*point0, *point1);
	} else if (point0 && line1) {
		ret = Math::intersects(
			*point0,
			Math::Line<Math::Vec2f>(Math::Vec2f(line1->x, line1->y), Math::Vec2f(line1->z, line1->w))
		);
	} else if (point0 && circ1) {
		ret = Math::intersects(
			*point0,
			Math::Circle<Math::Vec2f>(Math::Vec2f(circ1->x, circ1->y), circ1->z)
		);
	} else if (point0 && rect1) {
		ret = Math::intersects(*point0, *rect1);
	} else if (line0 && line1) {
		ret = Math::intersects(
			Math::Line<Math::Vec2f>(Math::Vec2f(line0->x, line0->y), Math::Vec2f(line0->z, line0->w)),
			Math::Line<Math::Vec2f>(Math::Vec2f(line1->x, line1->y), Math::Vec2f(line1->z, line1->w))
		);
	} else if (line0 && circ1) {
		ret = Math::intersects(
			Math::Line<Math::Vec2f>(Math::Vec2f(line0->x, line0->y), Math::Vec2f(line0->z, line0->w)),
			Math::Circle<Math::Vec2f>(Math::Vec2f(circ1->x, circ1->y), circ1->z)
		);
	} else if (line0 && rect1) {
		ret = Math::intersects(
			Math::Line<Math::Vec2f>(Math::Vec2f(line0->x, line0->y), Math::Vec2f(line0->z, line0->w)),
			*rect1
		);
	} else if (circ0 && circ1) {
		ret = Math::intersects(
			Math::Circle<Math::Vec2f>(Math::Vec2f(circ0->x, circ0->y), circ0->z),
			Math::Circle<Math::Vec2f>(Math::Vec2f(circ1->x, circ1->y), circ1->z)
		);
	} else if (circ0 && rect1) {
		ret = Math::intersects(
			Math::Circle<Math::Vec2f>(Math::Vec2f(circ0->x, circ0->y), circ0->z),
			*rect1
		);
	} else if (rect0 && rect1 && !recti0 && !recti1) {
		ret = Math::intersects(*rect0, *rect1);
	} else if (recti0 && recti1) {
		ret = Math::intersects(*recti0, *recti1);
	} else {
		if (line0 && point1) {
			ret = Math::intersects(
				*point1,
				Math::Line<Math::Vec2f>(Math::Vec2f(line0->x, line0->y), Math::Vec2f(line0->z, line0->w))
			);
		} else if (circ0 && point1) {
			ret = Math::intersects(
				*point1,
				Math::Circle<Math::Vec2f>(Math::Vec2f(circ0->x, circ0->y), circ0->z)
			);
		} else if (rect0 && point1) {
			ret = Math::intersects(*point1, *rect0);
		} else if (circ0 && line1) {
			ret = Math::intersects(
				Math::Line<Math::Vec2f>(Math::Vec2f(line1->x, line1->y), Math::Vec2f(line1->z, line1->w)),
				Math::Circle<Math::Vec2f>(Math::Vec2f(circ0->x, circ0->y), circ0->z)
			);
		} else if (rect0 && line1) {
			ret = Math::intersects(
				Math::Line<Math::Vec2f>(Math::Vec2f(line1->x, line1->y), Math::Vec2f(line1->z, line1->w)),
				*rect0
			);
		} else if (rect0 && circ1) {
			ret = Math::intersects(
				Math::Circle<Math::Vec2f>(Math::Vec2f(circ1->x, circ1->y), circ1->z),
				*rect0
			);
		}
	}

	return write(L, ret);
}

static void open_Math(lua_State* L) {
	req(
		L,
		array(
			luaL_Reg{
				"Math",
				LUA_LIB(
					array(
						luaL_Reg{ "intersects", Math_intersects },
						luaL_Reg{ nullptr, nullptr }
					)
				)
			},
			luaL_Reg{ nullptr, nullptr }
		)
	);
}

/**< Network. */

#if BITTY_NETWORK_ENABLED

static int Network_getOnReceived(lua_State* L, Network::Ptr &obj) {
	const Network::ReceivedHandler &handler = obj->receivedCallback();
	if (!handler.empty()) {
		Function::Ptr* val = (Function::Ptr*)handler.userdata().get();

		return write(L, **val);
	}

	return 0;
}

static void Network_setOnReceived(lua_State* L, Network::Ptr &obj, const Function::Ptr &callback) {
	Network::ReceivedHandler::Callback func = std::bind(
		[] (lua_State* L, Network* obj, Network::ReceivedHandler* self, void* data, size_t size, const char* addr) -> void {
			Function::Ptr* ptr = (Function::Ptr*)self->userdata().get();

			Network::DataTypes y = obj->dataType();
			switch (y) {
			case Network::STREAM: {
					Bytes* bytes = (Bytes*)data;
					Bytes::Ptr val(Bytes::create());
					val->writeBytes(bytes);
					val->poke(0);
					ScriptingLua::check(L, call(L, **ptr, &val, size, addr));
				}

				break;
			case Network::BYTES: {
					Bytes* bytes = (Bytes*)data;
					Bytes::Ptr val(Bytes::create());
					val->writeBytes(bytes);
					val->poke(0);
					ScriptingLua::check(L, call(L, **ptr, &val, size, addr));
				}

				break;
			case Network::STRING: {
					const char* val = (const char*)data;
					ScriptingLua::check(L, call(L, **ptr, val, size, addr));
				}

				break;
			case Network::JSON: {
					Json* json = (Json*)data;
					Json::Ptr val(Json::create());
					rapidjson::Document doc;
					json->toJson(doc);
					val->fromJson(doc);
					size_t sz = 1;
					if (doc.IsObject())
						sz = doc.MemberCount();
					else if (doc.IsArray())
						sz = doc.Capacity();
					ScriptingLua::check(L, call(L, **ptr, &val, sz, addr));
				}

				break;
			default:
				assert(false && "Impossible.");

				break;
			}
		},
		L, obj.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4
	);
	Any ud(
		new Function::Ptr(callback),
		[] (void* ptr) -> void {
			Function::Ptr* func = (Function::Ptr*)ptr;
			delete func;
		}
	);
	Network::ReceivedHandler cb(func, ud);

	obj->callback(cb);
}

static int Network_getOnEstablished(lua_State* L, Network::Ptr &obj) {
	const Network::EstablishedHandler &handler = obj->establishedCallback();
	if (!handler.empty()) {
		Function::Ptr* val = (Function::Ptr*)handler.userdata().get();

		return write(L, **val);
	}

	return 0;
}

static void Network_setOnEstablished(lua_State* L, Network::Ptr &obj, const Function::Ptr &callback) {
	Network::EstablishedHandler::Callback func = std::bind(
		[] (lua_State* L, Network::EstablishedHandler* self, const char* addr) -> void {
			Function::Ptr* ptr = (Function::Ptr*)self->userdata().get();

			ScriptingLua::check(L, call(L, **ptr, addr));
		},
		L, std::placeholders::_1, std::placeholders::_2
	);
	Any ud(
		new Function::Ptr(callback),
		[] (void* ptr) -> void {
			Function::Ptr* func = (Function::Ptr*)ptr;
			delete func;
		}
	);
	Network::EstablishedHandler cb(func, ud);

	obj->callback(cb);
}

static int Network_getOnDisconnected(lua_State* L, Network::Ptr &obj) {
	const Network::DisconnectedHandler &handler = obj->disconnectedCallback();
	if (!handler.empty()) {
		Function::Ptr* val = (Function::Ptr*)handler.userdata().get();

		return write(L, **val);
	}

	return 0;
}

static void Network_setOnDisconnected(lua_State* L, Network::Ptr &obj, const Function::Ptr &callback) {
	Network::DisconnectedHandler::Callback func = std::bind(
		[] (lua_State* L, Network::DisconnectedHandler* self, const char* addr) -> void {
			Function::Ptr* ptr = (Function::Ptr*)self->userdata().get();

			ScriptingLua::check(L, call(L, **ptr, addr));
		},
		L, std::placeholders::_1, std::placeholders::_2
	);
	Any ud(
		new Function::Ptr(callback),
		[] (void* ptr) -> void {
			Function::Ptr* func = (Function::Ptr*)ptr;
			delete func;
		}
	);
	Network::DisconnectedHandler cb(func, ud);

	obj->callback(cb);
}

static int Network_ctor(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	Function::Ptr recv = nullptr;
	Function::Ptr estb = nullptr;
	Function::Ptr disc = nullptr;
	if (n >= 3)
		read<>(L, recv, estb, disc);
	else if (n == 2)
		read<>(L, recv, estb);
	else if (n == 1)
		read<>(L, recv);

	Network::Ptr obj(Network::create());
	if (!obj)
		return write(L, nullptr);

	if (recv)
		Network_setOnReceived(L, obj, recv);
	if (estb)
		Network_setOnEstablished(L, obj, estb);
	if (disc)
		Network_setOnDisconnected(L, obj, disc);

	impl->addUpdatable(obj.get());

	return write(L, &obj);
}

static int Network___gc(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	Network::Ptr* obj = nullptr;
	check<>(L, obj);
	if (!obj)
		return 0;

	impl->removeUpdatable(obj->get());

	obj->get()->disconnect();

	obj->~shared_ptr();

	return 0;
}

static int Network_getOption(lua_State* L) {
	Network::Ptr* obj = nullptr;
	std::string key;
	read<>(L, obj, key);

	if (obj) {
		const std::string ret = obj->get()->option(key); // Undocumented: "interfaces" for desktops.

		return write(L, ret);
	}

	return 0;
}

static int Network_setOption(lua_State* L) {
	Network::Ptr* obj = nullptr;
	std::string key, val;
	read<>(L, obj, key, val);

	if (obj)
		obj->get()->option(key, val);

	return 0;
}

static int Network_open(lua_State* L) {
	const int n = getTop(L);
	Network::Ptr* obj = nullptr;
	const char* addr = nullptr;
	Enum protocal = Network::ALL;
	if (n >= 3)
		read<>(L, obj, addr, protocal);
	else
		read<>(L, obj, addr);

	if (obj && addr) {
		bool toconn = false, tobind = false;
		const bool ret = obj->get()->open(
			addr, (Network::Protocols)protocal,
			&toconn, &tobind
		);

		if (toconn && obj->get()->connective())
			obj->get()->establish();

		return write(L, ret);
	}

	return 0;
}

static int Network_close(lua_State* L) {
	Network::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		obj->get()->disconnect();

		const bool ret = obj->get()->close();

		return write(L, ret);
	} else {
		error(L, "Network expected, did you use \".\" rather than \":\".");
	}

	return 0;
}

static int Network_poll(lua_State* L) {
	const int n = getTop(L);
	Network::Ptr* obj = nullptr;
	int timeoutMs = 0;
	if (n >= 2)
		read<>(L, obj, timeoutMs);
	else
		read<>(L, obj);

	if (obj)
		 obj->get()->poll(timeoutMs);

	return 0;
}

static int Network_disconnect(lua_State* L) {
	Network::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj)
		 obj->get()->disconnect();

	return 0;
}

static int Network_send(lua_State* L) {
	Network::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		if (isUserdata(L, 2)) {
			// Bytes.
			Bytes::Ptr* bytes = nullptr;
			read<2>(L, bytes);

			if (bytes) {
				const bool ret = obj->get()->send((void*)bytes->get(), bytes->get()->count(), Network::BYTES);

				return write(L, ret);
			}

			// JSON.
			Json::Ptr* json = nullptr;
			read<2>(L, json);

			if (json) {
				const bool ret = obj->get()->send((void*)json->get(), 0, Network::JSON);

				return write(L, ret);
			}
		} else if (isTable(L, 2)) {
			// Table.
			Variant tbl = nullptr;
			read<2>(L, &tbl);

			if (tbl.type() == Variant::OBJECT) {
				Json::Ptr json(Json::create());
				if (json->fromAny(tbl)) {
					const bool ret = obj->get()->send((void*)json.get(), 0, Network::JSON);

					return write(L, ret);
				}
			}
		} else if (isString(L, 2)) {
			// String.
			std::string str;
			read<2>(L, str);

			const bool ret = obj->get()->send((void*)str.c_str(), str.length(), Network::STRING);

			return write(L, ret);
		}
	}

	return 0;
}

static int Network_broadcast(lua_State* L) {
	const int n = getTop(L);
	Network::Ptr* obj = nullptr;
	Placeholder _2;
	bool filterPolling = true;
	if (n >= 3)
		read<>(L, obj, _2, filterPolling);
	else
		read<>(L, obj, _2);

	if (obj) {
		if (isUserdata(L, 2)) {
			// Bytes.
			Bytes::Ptr* bytes = nullptr;
			read<2>(L, bytes);

			if (bytes) {
				const bool ret = obj->get()->broadcast((void*)bytes->get(), bytes->get()->count(), Network::BYTES, filterPolling);

				return write(L, ret);
			}

			// JSON.
			Json::Ptr* json = nullptr;
			read<2>(L, json);

			if (json) {
				const bool ret = obj->get()->broadcast((void*)json->get(), 0, Network::JSON, filterPolling);

				return write(L, ret);
			}
		} else if (isTable(L, 2)) {
			// Table.
			Variant tbl = nullptr;
			read<2>(L, &tbl);

			if (tbl.type() == Variant::OBJECT) {
				Json::Ptr json(Json::create());
				if (json->fromAny(tbl)) {
					const bool ret = obj->get()->broadcast((void*)json.get(), 0, Network::JSON, filterPolling);

					return write(L, ret);
				}
			}
		} else if (isString(L, 2)) {
			// String.
			std::string str;
			read<2>(L, str);

			const bool ret = obj->get()->broadcast((void*)str.c_str(), str.length(), Network::STRING, filterPolling);

			return write(L, ret);
		}
	}

	return 0;
}

static int Network___index(lua_State* L) {
	Network::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "ready") == 0) {
		const bool ret = obj->get()->ready();

		return write(L, ret);
	} else if (strcmp(field, "connective") == 0) { // Undocumented.
		const bool ret = obj->get()->connective();

		return write(L, ret);
	} else if (strcmp(field, "onReceived") == 0) { // Undocumented.
		return Network_getOnReceived(L, *obj);
	} else if (strcmp(field, "onEstablished") == 0) { // Undocumented.
		return Network_getOnEstablished(L, *obj);
	} else if (strcmp(field, "onDisconnected") == 0) { // Undocumented.
		return Network_getOnDisconnected(L, *obj);
	} else {
		return __index(L, field);
	}
}

static int Network___newindex(lua_State* L) {
	Network::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "onReceived") == 0) { // Undocumented.
		Function::Ptr val = nullptr;
		read<3>(L, val);

		Network_setOnReceived(L, *obj, val);
	} else if (strcmp(field, "onEstablished") == 0) { // Undocumented.
		Function::Ptr val = nullptr;
		read<3>(L, val);

		Network_setOnEstablished(L, *obj, val);
	} else if (strcmp(field, "onDisconnected") == 0) { // Undocumented.
		Function::Ptr val = nullptr;
		read<3>(L, val);

		Network_setOnDisconnected(L, *obj, val);
	}

	return 0;
}

static void open_Network(lua_State* L) {
	def(
		L, "Network",
		LUA_LIB(
			array(
				luaL_Reg{ "new", Network_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", Network___gc },
			luaL_Reg{ "__tostring", __tostring<Network::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "getOption", Network_getOption },
			luaL_Reg{ "setOption", Network_setOption },
			luaL_Reg{ "open", Network_open },
			luaL_Reg{ "close", Network_close },
			luaL_Reg{ "poll", Network_poll },
			luaL_Reg{ "disconnect", Network_disconnect },
			luaL_Reg{ "send", Network_send },
			luaL_Reg{ "broadcast", Network_broadcast },
			luaL_Reg{ nullptr, nullptr }
		),
		Network___index, Network___newindex
	);

	getGlobal(L, "Network");
	setTable(
		L,
		"None", (Enum)Network::NONE,
		"Udp", (Enum)Network::UDP,
		"Tcp", (Enum)Network::TCP,
		"WebSocket", (Enum)Network::WEBSOCKET // Undocumented.
	);
	pop(L);
}

#else /* BITTY_NETWORK_ENABLED */

static void open_Network(lua_State*) {
	// Do nothing.
}

#endif /* BITTY_NETWORK_ENABLED */

/**< Platform. */

static int Platform_surf(lua_State* L) {
	const char* url = nullptr;
	read<>(L, url);

	if (url && *url) {
		const std::string osstr = Unicode::toOs(url);

		Platform::surf(osstr.c_str());
	}

	return 0;
}

static int Platform_browse(lua_State* L) {
	const char* dir = nullptr;
	read<>(L, dir);

	if (dir && *dir) {
		const std::string osstr = Unicode::toOs(dir);

		Platform::browse(osstr.c_str());
	}

	return 0;
}

static int Platform_hasClipboardText(lua_State* L) {
	const bool ret = Platform::hasClipboardText();

	return write(L, ret);
}

static int Platform_getClipboardText(lua_State* L) {
	const std::string osstr = Platform::clipboardText();
	const std::string ret = Unicode::fromOs(osstr);

	return write(L, ret);
}

static int Platform_setClipboardText(lua_State* L) {
	const char* txt = nullptr;
	read<>(L, txt);

	if (!txt || !*txt)
		txt = "";
	const std::string osstr = Unicode::toOs(txt);

	Platform::clipboardText(osstr.c_str());

	return 0;
}

static int Platform_execute(lua_State* L) {
	const char* cmd = nullptr;
	read<>(L, cmd);

	if (cmd &&*cmd) {
		const std::string osstr = Unicode::toOs(cmd);

		Platform::execute(osstr.c_str());
	}

	return 0;
}

static const char* Platform_os(lua_State*) {
	return Platform::os();
}

static const char* Platform_endian(lua_State*) {
	return Platform::isLittleEndian() ? "little-endian" : "big-endian";
}

static void open_Platform(lua_State* L) {
	req(
		L,
		array(
			luaL_Reg{
				"Platform",
				LUA_LIB(
					array(
						luaL_Reg{ "surf", Platform_surf },
						luaL_Reg{ "browse", Platform_browse },
						luaL_Reg{ "hasClipboardText", Platform_hasClipboardText },
						luaL_Reg{ "getClipboardText", Platform_getClipboardText },
						luaL_Reg{ "setClipboardText", Platform_setClipboardText },
						luaL_Reg{ "execute", Platform_execute },
						luaL_Reg{ nullptr, nullptr }
					)
				)
			},
			luaL_Reg{ nullptr, nullptr }
		)
	);

	getGlobal(L, "Platform");
	setTable(
		L,
		"os", Platform_os(L),
		"endian", Platform_endian(L)
	);
	pop(L);
}

/**< Stream. */

static void open_Stream(lua_State* L) {
	def(
		L, "Stream",
		LUA_LIB(
			array<luaL_Reg>()
		),
		array<luaL_Reg>(),
		array<luaL_Reg>(),
		nullptr, nullptr
	);

	getGlobal(L, "Stream");
	setTable(
		L,
		"Read", (Enum)Stream::READ,
		"Write", (Enum)Stream::WRITE,
		"Append", (Enum)Stream::APPEND,
		"ReadWrite", (Enum)Stream::READ_WRITE
	);
	pop(L);
}

/**< Web. */

#if BITTY_WEB_ENABLED

static int Web_getOnRequested(lua_State* L, Web::Ptr &obj) {
	const Web::RequestedHandler &handler = obj->requestedCallback();
	if (!handler.empty()) {
		Function::Ptr* val = (Function::Ptr*)handler.userdata().get();

		return write(L, **val);
	}

	return 0;
}

static void Web_setOnRequested(lua_State* L, Web::Ptr &obj, const Function::Ptr &callback) {
	Web::RequestedHandler::Callback func = std::bind(
		[] (lua_State* L, Web* /* obj */, Web::RequestedHandler* self, const char* method, const char* uri, const char* query, const char* body, const char* /* message */) -> bool {
			Function::Ptr* ptr = (Function::Ptr*)self->userdata().get();

			bool ret = true;
			ScriptingLua::check(L, call(ret, L, **ptr, method, uri, query, body));

			return ret;
		},
		L, obj.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6
	);
	Any ud(
		new Function::Ptr(callback),
		[] (void* ptr) -> void {
			Function::Ptr* func = (Function::Ptr*)ptr;
			delete func;
		}
	);
	Web::RequestedHandler cb(func, ud);

	obj->callback(cb);
}

static int Web_ctor(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	Function::Ptr rspd = nullptr;
	if (n >= 1)
		read<>(L, rspd);

	Web::Ptr obj(Web::create());
	if (!obj)
		return write(L, nullptr);

	if (rspd)
		Web_setOnRequested(L, obj, rspd);

	impl->addUpdatable(obj.get());

	return write(L, &obj);
}

static int Web___gc(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	Web::Ptr* obj = nullptr;
	check<>(L, obj);
	if (!obj)
		return 0;

	impl->removeUpdatable(obj->get());

	obj->~shared_ptr();

	return 0;
}

static int Web_open(lua_State* L) {
	const int n = getTop(L);
	Web::Ptr* obj = nullptr;
	unsigned short port = 8080;
	const char* root = nullptr;
	if (n >= 3)
		read<>(L, obj, port, root);
	else
		read<>(L, obj, port);

	if (obj) {
		const bool ret = obj->get()->open(port, root);

		return write(L, ret);
	}

	return 0;
}

static int Web_close(lua_State* L) {
	Web::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj) {
		const bool ret = obj->get()->close();

		return write(L, ret);
	} else {
		error(L, "Web expected, did you use \".\" rather than \":\".");
	}

	return 0;
}

static int Web_poll(lua_State* L) {
	const int n = getTop(L);
	Web::Ptr* obj = nullptr;
	int timeoutMs = 0;
	if (n >= 2)
		read<>(L, obj, timeoutMs);
	else
		read<>(L, obj);

	if (obj)
		 obj->get()->poll(timeoutMs);

	return 0;
}

static int Web___index(lua_State* L) {
	Web::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "ready") == 0) {
		const bool ret = obj->get()->ready();

		return write(L, ret);
	} else if (strcmp(field, "onRequested") == 0) { // Undocumented.
		return Web_getOnRequested(L, *obj);
	} else {
		return __index(L, field);
	}
}

static int Web___newindex(lua_State* L) {
	Web::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "onRequested") == 0) { // Undocumented.
		Function::Ptr val = nullptr;
		read<3>(L, val);

		Web_setOnRequested(L, *obj, val);
	}

	return 0;
}

static int Web_respond(lua_State* L) {
	const int n = getTop(L);
	Web::Ptr* obj = nullptr;
	Placeholder _2;
	const char* mimeType = nullptr;
	if (n >= 3)
		read<>(L, obj, _2, mimeType);
	else
		read<>(L, obj, _2);

	if (obj) {
		if (isUserdata(L, 2)) {
			// Bytes.
			Bytes::Ptr* bytes = nullptr;
			read<2>(L, bytes);

			if (bytes) {
				const bool ret = obj->get()->respond(bytes->get(), mimeType);

				return write(L, ret);
			}

			// JSON.
			Json::Ptr* json = nullptr;
			read<2>(L, json);

			if (json) {
				const bool ret = obj->get()->respond(json->get(), mimeType);

				return write(L, ret);
			}
		} else if (isTable(L, 2)) {
			// Table.
			Variant tbl = nullptr;
			read<2>(L, &tbl);

			if (tbl.type() == Variant::OBJECT) {
				Json::Ptr json(Json::create());
				if (json->fromAny(tbl)) {
					const bool ret = obj->get()->respond(json.get(), mimeType);

					return write(L, ret);
				}
			}
		} else if (isInteger(L, 2)) {
			// Integer.
			unsigned code = 404;
			read<2>(L, code);

			const bool ret = obj->get()->respond(code);

			return write(L, ret);
		} else if (isString(L, 2)) {
			// String.
			std::string str;
			read<2>(L, str);

			const bool ret = obj->get()->respond(str.c_str(), mimeType);

			return write(L, ret);
		}
	}

	return 0;
}

static void open_Web(lua_State* L) {
	def(
		L, "Web", // Undocumented.
		LUA_LIB(
			array(
				luaL_Reg{ "new", Web_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", Web___gc },
			luaL_Reg{ "__tostring", __tostring<Web::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "open", Web_open },
			luaL_Reg{ "close", Web_close },
			luaL_Reg{ "poll", Web_poll },
			luaL_Reg{ "respond", Web_respond },
			luaL_Reg{ nullptr, nullptr }
		),
		Web___index, Web___newindex
	);
}

#else /* BITTY_WEB_ENABLED */

static void open_Web(lua_State*) {
	// Do nothing.
}

#endif /* BITTY_WEB_ENABLED */

/**< Categories. */

void open(class Executable* exec) {
	// Prepare.
	lua_State* L = (lua_State*)exec->pointer();

	// Light userdata.
	open_LightUserdata(L);

	// Algorithms.
	open_Noiser(L);
	open_Pathfinder(L);
	open_Random(L);
	open_Raycaster(L);
	open_Walker(L);

	// Archive.
	open_Archive(L);

	// Bytes.
	open_Bytes(L);

	// Color.
	open_Color(L);

	// Encoding.
	open_Base64(L);
	open_Lz4(L);

	// Date time.
	open_DateTime(L);

	// File.
	open_File(L);

	// Filesystem.
	open_Path(L);
	open_FileInfo(L);
	open_DirectoryInfo(L);

	// Image.
	open_Image(L);

	// JSON.
	open_Json(L);

	// Math.
	open_Vec2(L);
	open_Vec3(L);
	open_Vec4(L);
	open_Rect(L);
	open_Recti(L);
	open_Rot(L);
	open_Math(L);

	// Network.
	open_Network(L);

	// Platform.
	open_Platform(L);

	// Stream.
	open_Stream(L);

	// Web.
	open_Web(L);
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

/**< Resources. */

template<typename P, typename Q, typename R> static P Resources_tryWait(Executable*, Primitives* primitives, Q &q, R r, unsigned y = P::element_type::TYPE()) {
	if (!q->pointer) {
		Resources::Asset::Ptr asset(new Resources::Asset(y, r));
		asset->from(*q);

		primitives->load(asset);
		if (!asset->await()) {
			asset->to(*q);

			return nullptr;
		}

		asset->to(*q);

		if (!q->pointer)
			return nullptr;
	}

	return q->pointer;
}

template<typename P, typename Q, typename R> static P Resources_waitUntilProcessed(Executable* exec, Primitives* primitives, Q &q, R r, unsigned y = P::element_type::TYPE()) {
	if (!q->pointer) {
		Resources::Asset::Ptr asset(new Resources::Asset(y, r));
		asset->from(*q);

		primitives->load(asset);
		while (!asset->await() && (exec->current() == Executable::RUNNING || exec->current() == Executable::PAUSED)) { // Resources synchronized.
			// Do nothing.
		}

		asset->to(*q);

		if (!q->pointer)
			return nullptr;
	}

	return q->pointer;
}

template<typename P> static void Resources_dispose(Executable*, Primitives* primitives, P* &p) {
	if (!*p)
		return;

	if ((*p)->pointer) {
		primitives->dispose((*p)->pointer);
		(*p)->pointer = nullptr;
	}

	Object::Ptr ref = (*p)->unref();
	if (ref)
		primitives->dispose(ref);

	*p = nullptr;
}

static unsigned Resources_namedTypeOf(const std::string &name) {
	unsigned type = 0;
	if (name == "Asset")
		type = Asset::TYPE();
	else if (name == "Palette")
		type = Palette::TYPE();
	else if (name == "Texture")
		type = Image::TYPE();
	else if (name == "Sprite")
		type = Sprite::TYPE();
	else if (name == "Map")
		type = Map::TYPE();
	else if (name == "Sfx")
		type = Sfx::TYPE();
	else if (name == "Music")
		type = Music::TYPE();
	else
		type = Asset::TYPE();

	return type;
}

static void Resources_contentOf(lua_State* L, int idx, std::string &asset, Either<Resources::Palette::Ptr, Resources::Texture::Ptr> &ref) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	if (isUserdata(L, idx)) {
		do {
			Json::Ptr* json = nullptr;
			read<>(L, json); // Asset content as JSON.

			if (json) {
				json->get()->toString(asset, false);

				break;
			}

			Image::Ptr* img = nullptr;
			read<>(L, img); // Asset content as Image.

			if (img) {
				rapidjson::Document doc;
				img->get()->toJson(doc);

				Json::Ptr tmp(Json::create());
				tmp->fromJson(doc);
				tmp->toString(asset);

				break;
			}

			Bytes::Ptr* bytes = nullptr;
			read<>(L, bytes); // Asset content as Bytes.

			if (bytes) {
				// Use std::string as generic buffer.
				asset.assign((const char*)bytes->get()->pointer(), bytes->get()->count());
				asset.insert(0, RESOURCES_BYTES_HEADER, BITTY_COUNTOF(RESOURCES_BYTES_HEADER));

				break;
			}
		} while (false);
	} else if (isTable(L, idx)) {
		Resources::Palette::Ptr* pal = nullptr;
		Resources::Texture::Ptr* tex = nullptr;
		do {
			readTable(L, idx, ASSET_REF_NAME);
			read<-1>(L, pal);
			pop(L);
			if (pal) {
				ref = Left<Resources::Palette::Ptr>(*pal);

				break;
			}

			readTable(L, idx, ASSET_REF_NAME);
			read<-1>(L, tex);
			pop(L);
			if (tex) {
				ref = Right<Resources::Texture::Ptr>(*tex);

				break;
			}
		} while (false);

		if (pal) {
			if (!Resources_waitUntilProcessed<Palette::Ptr>(impl, impl->primitives(), *pal, nullptr)) {
				error(L, "Invalid palette.");
			}
		}

		if (tex) {
			if (!Resources_waitUntilProcessed<Texture::Ptr>(impl, impl->primitives(), *tex, nullptr, Image::TYPE())) {
				error(L, "Invalid texture.");
			}
		}

		rapidjson::Document doc;
		read(L, doc, Index(idx)); // Asset content as Table.

		Json::toString(doc, asset, false);
	} else if (isString(L, idx)) {
		read<>(L, asset); // Asset path as string, or content.
	}
}

static int Resources_load(lua_State* L) {
	const int n = getTop(L);
	std::string asset;
	unsigned type = Asset::TYPE();
	Either<Resources::Palette::Ptr, Resources::Texture::Ptr> ref(Left<Resources::Palette::Ptr>(nullptr));
	if (n >= 1)
		Resources_contentOf(L, 1, asset, ref); // Resources synchronized.
	type = Asset::typeOf(asset, false);
	if (n >= 2) {
		if (isTable(L, 2)) {
			std::string y;
			getTable(L, "__name", y); // Asset type as table name.
			type = Resources_namedTypeOf(y);
		} else if (isString(L, 2)) {
			Placeholder _1;
			std::string y;
			read<>(L, _1, y); // Asset type as string.

			type = Asset::typeOf(y, false);
		}
	}
	if (!type)
		type = Asset::inferencedTypeOf(asset);

	switch (type) {
	case Palette::TYPE(): {
			Resources::Palette::Ptr res(new Resources::Palette(asset));

			return write(L, &res);
		}
	case Image::TYPE(): {
			Resources::Texture::Ptr res(new Resources::Texture(asset, ref.left().get(nullptr)));

			return write(L, &res);
		}
	case Sprite::TYPE(): {
			Resources::Sprite::Ptr res(new Resources::Sprite(asset, ref.right().get(nullptr)));

			return write(L, &res);
		}
	case Map::TYPE(): {
			Resources::Map::Ptr res(new Resources::Map(asset, ref.right().get(nullptr)));

			return write(L, &res);
		}
	case Sfx::TYPE(): {
			Resources::Sfx::Ptr res(new Resources::Sfx(asset));

			return write(L, &res);
		}
	case Music::TYPE(): {
			Resources::Music::Ptr res(new Resources::Music(asset));

			return write(L, &res);
		}
	default: {
			Resources::Asset::Ptr res(new Resources::Asset(Object::TYPE(), nullptr, asset));

			return write(L, &res);
		}
	}

	return 0;
}

static int Resources_wait(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	unsigned type = Asset::TYPE();
	std::string y;
	if (getMetaOf(L)) {
		getTable(L, "__name", y);
		pop(L);
	}
	type = Resources_namedTypeOf(y);

	switch (type) {
	case Palette::TYPE(): {
			Resources::Palette::Ptr* res = nullptr;
			read<>(L, res);

			if (!res || !*res)
				return write(L, false);

			Palette::Ptr ptr = Resources_tryWait<Palette::Ptr>(impl, impl->primitives(), *res, nullptr);
			if (!ptr)
				return write(L, false);

			return write(L, true);
		}
	case Image::TYPE(): {
			Resources::Texture::Ptr* res = nullptr;
			read<>(L, res);

			if (!res || !*res)
				return write(L, false);

			Texture::Ptr ptr = Resources_tryWait<Texture::Ptr>(impl, impl->primitives(), *res, (*res)->ref, Image::TYPE());
			if (!ptr)
				return write(L, false);

			return write(L, true);
		}
	case Sprite::TYPE(): {
			Resources::Sprite::Ptr* res = nullptr;
			read<>(L, res);

			if (!res || !*res)
				return write(L, false);

			Sprite::Ptr ptr = Resources_tryWait<Sprite::Ptr>(impl, impl->primitives(), *res, (*res)->ref);
			if (!ptr)
				return write(L, false);

			return write(L, true);
		}
	case Map::TYPE(): {
			Resources::Map::Ptr* res = nullptr;
			read<>(L, res);

			if (!res || !*res)
				return write(L, false);

			Map::Ptr ptr = Resources_tryWait<Map::Ptr>(impl, impl->primitives(), *res, (*res)->ref);
			if (!ptr)
				return write(L, false);

			return write(L, true);
		}
	case Sfx::TYPE(): {
			Resources::Sfx::Ptr* res = nullptr;
			read<>(L, res);

			if (!res || !*res)
				return write(L, false);

			Sfx::Ptr ptr = Resources_tryWait<Sfx::Ptr>(impl, impl->primitives(), *res, nullptr);
			if (!ptr)
				return write(L, false);

			return write(L, true);
		}
	case Music::TYPE(): {
			Resources::Music::Ptr* res = nullptr;
			read<>(L, res);

			if (!res || !*res)
				return write(L, false);

			Music::Ptr ptr = Resources_tryWait<Music::Ptr>(impl, impl->primitives(), *res, nullptr);
			if (!ptr)
				return write(L, false);

			return write(L, true);
		}
	default: {
			Resources::Asset::Ptr* res = nullptr;
			read<>(L, res);

			if (!res || !*res)
				return write(L, false);

			Object::Ptr ptr = Resources_tryWait<Object::Ptr>(impl, impl->primitives(), *res, (*res)->ref, (*res)->target());
			if (!ptr)
				return write(L, false);

			return write(L, true);
		}
	}
}

static int Resources_unload(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	unsigned type = Asset::TYPE();
	if (n >= 1) {
		std::string y;
		if (getMetaOf(L)) {
			getTable(L, "__name", y);
			pop(L);
		}
		type = Resources_namedTypeOf(y);
	} else {
		impl->primitives()->unload(nullptr);
	}

	switch (type) {
	case Palette::TYPE(): {
			Resources::Palette::Ptr* res = nullptr;
			read<>(L, res);

			if (!res || !*res)
				break;

			Resources::Asset::Ptr resource(new Resources::Asset(Palette::TYPE()));
			resource->from(*res->get());
			impl->primitives()->unload(resource);

			Resources_dispose(impl, impl->primitives(), res);
		}

		break;
	case Image::TYPE(): {
			Resources::Texture::Ptr* res = nullptr;
			read<>(L, res);

			if (!res || !*res)
				break;

			Resources::Asset::Ptr resource(new Resources::Asset(Image::TYPE()));
			resource->from(*res->get());
			impl->primitives()->unload(resource);

			Resources_dispose(impl, impl->primitives(), res);
		}

		break;
	case Sprite::TYPE(): {
			Resources::Sprite::Ptr* res = nullptr;
			read<>(L, res);

			if (!res || !*res)
				break;

			Resources::Asset::Ptr resource(new Resources::Asset(Sprite::TYPE()));
			resource->from(*res->get());
			impl->primitives()->unload(resource);

			Resources_dispose(impl, impl->primitives(), res);
		}

		break;
	case Map::TYPE(): {
			Resources::Map::Ptr* res = nullptr;
			read<>(L, res);

			if (!res || !*res)
				break;

			Resources::Asset::Ptr resource(new Resources::Asset(Map::TYPE()));
			resource->from(*res->get());
			impl->primitives()->unload(resource);

			Resources_dispose(impl, impl->primitives(), res);
		}

		break;
	case Sfx::TYPE(): {
			Resources::Sfx::Ptr* res = nullptr;
			read<>(L, res);

			if (!res || !*res)
				break;

			Resources::Asset::Ptr resource(new Resources::Asset(Sfx::TYPE()));
			resource->from(*res->get());
			impl->primitives()->unload(resource);

			Resources_dispose(impl, impl->primitives(), res);
		}

		break;
	case Music::TYPE(): {
			Resources::Music::Ptr* res = nullptr;
			read<>(L, res);

			if (!res || !*res)
				break;

			Resources::Asset::Ptr resource(new Resources::Asset(Music::TYPE()));
			resource->from(*res->get());
			impl->primitives()->unload(resource);

			Resources_dispose(impl, impl->primitives(), res);
		}

		break;
	default: {
			Resources::Asset::Ptr* res = nullptr;
			read<>(L, res);

			if (!res || !*res)
				break;

			Resources::Asset::Ptr &resource = *res;
			impl->primitives()->unload(resource);

			Resources_dispose(impl, impl->primitives(), res);
		}

		break;
	}

	return 0;
}

static int Resources_collect(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	impl->primitives()->collect();

	return 0;
}

static void open_Resources(lua_State* L) {
	req(
		L,
		array(
			luaL_Reg{
				"Resources",
				LUA_LIB(
					array(
						luaL_Reg{ "load", Resources_load },
						luaL_Reg{ "wait", Resources_wait },
						luaL_Reg{ "unload", Resources_unload },
						luaL_Reg{ "collect", Resources_collect },
						luaL_Reg{ nullptr, nullptr }
					)
				)
			},
			luaL_Reg{ nullptr, nullptr }
		)
	);
}

template<typename P> int Resource___gc(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	P* obj = nullptr;
	check(L, obj);
	if (!obj || !*obj)
		return 0;

	if (*obj)
		Resources_dispose(impl, impl->primitives(), obj);

	return 0;
}

static int ResourceAsset___index(lua_State* L) {
	Resources::Asset::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !*obj || !field)
		return 0;

	return __index(L, field);
}

static int ResourceAsset___newindex(lua_State* L) {
	Resources::Asset::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !*obj || !field)
		return 0;

	return 0;
}

static void open_ResourceAsset(lua_State* L) {
	def(
		L, "Asset",
		LUA_LIB(
			array<luaL_Reg>()
		),
		array(
			luaL_Reg{ "__gc", Resource___gc<Resources::Asset::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Resources::Asset::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array<luaL_Reg>(),
		ResourceAsset___index, ResourceAsset___newindex
	);

	getGlobal(L, "Asset");
	setTable(
		L,
		"__name", "Asset"
	);
	pop(L);
}

static int ResourcePalette___index(lua_State* L) {
	Resources::Palette::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !*obj || !field)
		return 0;

	return __index(L, field);
}

static int ResourcePalette___newindex(lua_State* L) {
	Resources::Palette::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !*obj || !field)
		return 0;

	return 0;
}

static void open_ResourcePalette(lua_State* L) {
	def(
		L, "Palette",
		LUA_LIB(
			array<luaL_Reg>()
		),
		array(
			luaL_Reg{ "__gc", Resource___gc<Resources::Palette::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Resources::Palette::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array<luaL_Reg>(),
		ResourcePalette___index, ResourcePalette___newindex
	);

	getGlobal(L, "Palette");
	setTable(
		L,
		"__name", "Palette"
	);
	pop(L);
}

static int ResourceTexture_blend(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	Resources::Texture::Ptr* obj = nullptr;
	Enum blendMode = SDL_BLENDMODE_NONE;
	read<>(L, obj, blendMode);

	if (obj && *obj) {
		Texture::Ptr ptr = Resources_waitUntilProcessed<Texture::Ptr>(impl, impl->primitives(), *obj, obj->get()->ref, Image::TYPE());
		if (!ptr) {
			error(L, "Invalid texture.");

			return write(L, false);
		}

		impl->primitives()->blend(*obj, blendMode);

		return write(L, true);
	}

	return write(L, false);
}

static int ResourceTexture___index(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	Resources::Texture::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !*obj || !field)
		return 0;

	if (strcmp(field, "width") == 0) {
		Texture::Ptr ptr = Resources_waitUntilProcessed<Texture::Ptr>(impl, impl->primitives(), *obj, obj->get()->ref, Image::TYPE());
		if (!ptr) {
			error(L, "Invalid texture.");

			return write(L, nullptr);
		}

		const int ret = ptr->width();

		return write(L, ret);
	} else if (strcmp(field, "height") == 0) {
		Texture::Ptr ptr = Resources_waitUntilProcessed<Texture::Ptr>(impl, impl->primitives(), *obj, obj->get()->ref, Image::TYPE());
		if (!ptr) {
			error(L, "Invalid texture.");

			return write(L, nullptr);
		}

		const int ret = ptr->height();

		return write(L, ret);
	} else {
		return __index(L, field);
	}
}

static int ResourceTexture___newindex(lua_State* L) {
	Resources::Texture::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !*obj || !field)
		return 0;

	return 0;
}

static void open_ResourceTexture(lua_State* L) {
	def(
		L, "Texture",
		LUA_LIB(
			array<luaL_Reg>()
		),
		array(
			luaL_Reg{ "__gc", Resource___gc<Resources::Texture::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Resources::Texture::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "blend", ResourceTexture_blend }, // Resources synchronized.
			luaL_Reg{ nullptr, nullptr }
		),
		ResourceTexture___index, ResourceTexture___newindex
	);

	getGlobal(L, "Texture");
	setTable(
		L,
		"__name", "Texture"
	);
	pop(L);
}

static int ResourceSprite___gc(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	Resources::Sprite::Ptr* obj = nullptr;
	check<>(L, obj);
	if (!obj || !*obj)
		return 0;

	Texture::Ptr tex = nullptr;
	if (obj->get()->pointer) {
		const Sprite::Ptr &ptr = obj->get()->pointer;
		if (ptr) {
			LockGuard<RecursiveMutex> guardAsset(obj->get()->lock);

			ptr->get(0, &tex, nullptr, nullptr, nullptr); // Retain the texture object to prevent disposing from the scripting thread.

			ptr->unload();
		}
	}

	if (tex)
		impl->primitives()->dispose(tex);

	Resources_dispose(impl, impl->primitives(), obj);

	return 0;
}

static int ResourceSprite_play(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	Resources::Sprite::Ptr* obj = nullptr;
	int begin = -1, end = -1;
	std::string beginStr;
	bool reset = true;
	bool loop = true;
	bool async = false;
	if (isNumber(L, 2)) {
		if (n >= 6)
			read<>(L, obj, begin, end, reset, loop, async);
		else if (n == 5)
			read<>(L, obj, begin, end, reset, loop);
		else if (n == 4)
			read<>(L, obj, begin, end, reset);
		else if (n == 3)
			read<>(L, obj, begin, end);
		else
			read<>(L, obj);
	} else if (isString(L, 2)) {
		if (n >= 5)
			read<>(L, obj, beginStr, reset, loop, async);
		else if (n == 4)
			read<>(L, obj, beginStr, reset, loop);
		else if (n == 3)
			read<>(L, obj, beginStr, reset);
		else if (n == 2)
			read<>(L, obj, beginStr);
		else
			read<>(L, obj);
	} else {
		read<>(L, obj);
	}

	if (obj && *obj) {
		if (async) {
			if (beginStr.empty())
				impl->primitives()->play(*obj, begin, end, reset, loop);
			else
				impl->primitives()->play(*obj, beginStr, reset, loop);

			return write(L, true, -1);
		} else {
			Sprite::Ptr ptr = Resources_waitUntilProcessed<Sprite::Ptr>(impl, impl->primitives(), *obj, obj->get()->ref);
			if (!ptr)
				return write(L, false);

			LockGuard<RecursiveMutex> guardAsset(obj->get()->lock);

			double duration = 0;
			if (beginStr.empty())
				ptr->play(begin, end, reset, loop, &duration);
			else
				ptr->play(beginStr, reset, loop, &duration);

			return write(L, true, duration);
		}
	}

	return write(L, false, 0);
}

static int ResourceSprite_pause(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	Resources::Sprite::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj && *obj) {
		LockGuard<RecursiveMutex> guardAsset(obj->get()->lock);

		Sprite::Ptr ptr = Resources_tryWait<Sprite::Ptr>(impl, impl->primitives(), *obj, obj->get()->ref);
		if (!ptr)
			return write(L, false);

		ptr->pause();

		return write(L, true);
	}

	return write(L, false);
}

static int ResourceSprite_resume(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	Resources::Sprite::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj && *obj) {
		LockGuard<RecursiveMutex> guardAsset(obj->get()->lock);

		Sprite::Ptr ptr = Resources_tryWait<Sprite::Ptr>(impl, impl->primitives(), *obj, obj->get()->ref);
		if (!ptr)
			return write(L, false);

		ptr->resume();

		return write(L, true);
	}

	return write(L, false);
}

static int ResourceSprite_stop(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	Resources::Sprite::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj && *obj) {
		LockGuard<RecursiveMutex> guardAsset(obj->get()->lock);

		Sprite::Ptr ptr = Resources_tryWait<Sprite::Ptr>(impl, impl->primitives(), *obj, obj->get()->ref);
		if (!ptr)
			return write(L, false);

		ptr->stop();

		return write(L, true);
	}

	return write(L, false);
}

static int ResourceSprite___index(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	Resources::Sprite::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !*obj || !field)
		return 0;

	if (strcmp(field, "width") == 0) {
		Sprite::Ptr ptr = Resources_waitUntilProcessed<Sprite::Ptr>(impl, impl->primitives(), *obj, obj->get()->ref);
		if (!ptr)
			return write(L, nullptr);

		const int ret = ptr->width();

		return write(L, ret);
	} else if (strcmp(field, "height") == 0) {
		Sprite::Ptr ptr = Resources_waitUntilProcessed<Sprite::Ptr>(impl, impl->primitives(), *obj, obj->get()->ref);
		if (!ptr)
			return write(L, nullptr);

		const int ret = ptr->height();

		return write(L, ret);
	} else if (strcmp(field, "hFlip") == 0) {
		Sprite::Ptr ptr = Resources_waitUntilProcessed<Sprite::Ptr>(impl, impl->primitives(), *obj, obj->get()->ref);
		if (!ptr)
			return write(L, nullptr);

		const bool ret = ptr->hFlip();

		return write(L, ret);
	} else if (strcmp(field, "vFlip") == 0) {
		Sprite::Ptr ptr = Resources_waitUntilProcessed<Sprite::Ptr>(impl, impl->primitives(), *obj, obj->get()->ref);
		if (!ptr)
			return write(L, nullptr);

		const bool ret = ptr->vFlip();

		return write(L, ret);
	} else if (strcmp(field, "count") == 0) {
		Sprite::Ptr ptr = Resources_waitUntilProcessed<Sprite::Ptr>(impl, impl->primitives(), *obj, obj->get()->ref);
		if (!ptr)
			return write(L, nullptr);

		const int ret = ptr->count();

		return write(L, ret);
	} else {
		return __index(L, field);
	}
}

static int ResourceSprite___newindex(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	Resources::Sprite::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !*obj || !field)
		return 0;

	if (strcmp(field, "hFlip") == 0) {
		bool val = false;
		read<3>(L, val);

		LockGuard<RecursiveMutex> guardAsset(obj->get()->lock);

		Sprite::Ptr ptr = Resources_waitUntilProcessed<Sprite::Ptr>(impl, impl->primitives(), *obj, obj->get()->ref);
		if (!ptr)
			return 0;

		ptr->hFlip(val);
	} else if (strcmp(field, "vFlip") == 0) {
		bool val = false;
		read<3>(L, val);

		LockGuard<RecursiveMutex> guardAsset(obj->get()->lock);

		Sprite::Ptr ptr = Resources_waitUntilProcessed<Sprite::Ptr>(impl, impl->primitives(), *obj, obj->get()->ref);
		if (!ptr)
			return 0;

		ptr->vFlip(val);
	}

	return 0;
}

static void open_ResourceSprite(lua_State* L) {
	def(
		L, "Sprite",
		LUA_LIB(
			array<luaL_Reg>()
		),
		array(
			luaL_Reg{ "__gc", ResourceSprite___gc },
			luaL_Reg{ "__tostring", __tostring<Resources::Sprite::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "play", ResourceSprite_play }, // Resources synchronized, or asynchronized (specified by parameter).
			luaL_Reg{ "pause", ResourceSprite_pause },
			luaL_Reg{ "resume", ResourceSprite_resume },
			luaL_Reg{ "stop", ResourceSprite_stop },
			luaL_Reg{ nullptr, nullptr }
		),
		ResourceSprite___index, ResourceSprite___newindex // Resources synchronized.
	);

	getGlobal(L, "Sprite");
	setTable(
		L,
		"__name", "Sprite"
	);
	pop(L);
}

static int ResourceMap___index(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	Resources::Map::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !*obj || !field)
		return 0;

	if (strcmp(field, "width") == 0) {
		Map::Ptr ptr = Resources_waitUntilProcessed<Map::Ptr>(impl, impl->primitives(), *obj, obj->get()->ref);
		if (!ptr)
			return write(L, nullptr);

		const int ret = ptr->width();

		return write(L, ret);
	} else if (strcmp(field, "height") == 0) {
		Map::Ptr ptr = Resources_waitUntilProcessed<Map::Ptr>(impl, impl->primitives(), *obj, obj->get()->ref);
		if (!ptr)
			return write(L, nullptr);

		const int ret = ptr->height();

		return write(L, ret);
	} else {
		return __index(L, field);
	}
}

static int ResourceMap___newindex(lua_State* L) {
	Resources::Map::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !*obj || !field)
		return 0;

	return 0;
}

static void open_ResourceMap(lua_State* L) {
	def(
		L, "Map",
		LUA_LIB(
			array<luaL_Reg>()
		),
		array(
			luaL_Reg{ "__gc", Resource___gc<Resources::Map::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Resources::Map::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array<luaL_Reg>(),
		ResourceMap___index, ResourceMap___newindex // Resources synchronized.
	);

	getGlobal(L, "Map");
	setTable(
		L,
		"__name", "Map"
	);
	pop(L);
}

static int ResourceSfx___index(lua_State* L) {
	Resources::Sfx::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !*obj || !field)
		return 0;

	return __index(L, field);
}

static int ResourceSfx___newindex(lua_State* L) {
	Resources::Sfx::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !*obj || !field)
		return 0;

	return 0;
}

static void open_ResourceSfx(lua_State* L) {
	def(
		L, "Sfx",
		LUA_LIB(
			array<luaL_Reg>()
		),
		array(
			luaL_Reg{ "__gc", Resource___gc<Resources::Sfx::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Resources::Sfx::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array<luaL_Reg>(),
		ResourceSfx___index, ResourceSfx___newindex
	);

	getGlobal(L, "Sfx");
	setTable(
		L,
		"__name", "Sfx"
	);
	pop(L);
}

static int ResourceMusic___len(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	Resources::Music::Ptr* obj = nullptr;
	check<>(L, obj);

	if (obj && *obj) {
		Music::Ptr ptr = Resources_waitUntilProcessed<Music::Ptr>(impl, impl->primitives(), *obj, nullptr);
		if (!ptr)
			return write(L, nullptr);

		LockGuard<Mutex> guard(obj->get()->lock);

		const double ret = ptr->length();

		return write(L, ret);
	}

	return 0;
}

static int ResourceMusic___index(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	Resources::Music::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !*obj || !field)
		return 0;

	if (strcmp(field, "length") == 0) { // Undocumented.
		Music::Ptr ptr = Resources_waitUntilProcessed<Music::Ptr>(impl, impl->primitives(), *obj, nullptr);
		if (!ptr)
			return write(L, nullptr);

		LockGuard<Mutex> guard(obj->get()->lock);

		const double ret = ptr->length();

		return write(L, ret);
	} else if (strcmp(field, "isPlaying") == 0) { // Undocumented.
		Music::Ptr ptr = Resources_waitUntilProcessed<Music::Ptr>(impl, impl->primitives(), *obj, nullptr);
		if (!ptr)
			return write(L, nullptr);

		LockGuard<Mutex> guard(obj->get()->lock);

		const bool ret = ptr->playing();

		return write(L, ret);
	} else {
		return __index(L, field);
	}
}

static int ResourceMusic___newindex(lua_State* L) {
	Resources::Music::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !*obj || !field)
		return 0;

	return 0;
}

static void open_ResourceMusic(lua_State* L) {
	def(
		L, "Music",
		LUA_LIB(
			array<luaL_Reg>()
		),
		array(
			luaL_Reg{ "__gc", Resource___gc<Resources::Music::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Resources::Music::Ptr> },
			luaL_Reg{ "__len", ResourceMusic___len }, // Undocumented.
			luaL_Reg{ nullptr, nullptr }
		),
		array<luaL_Reg>(),
		ResourceMusic___index, ResourceMusic___newindex
	);

	getGlobal(L, "Music");
	setTable(
		L,
		"__name", "Music"
	);
	pop(L);
}

/**< Font. */

static int Font_ctor(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	Image::Ptr* img = nullptr;
	const char* name = nullptr;
	if (n >= 1) {
		if (isString(L, 1))
			read<1>(L, name);
		else
			read<1>(L, img);
	}
	int size = RESOURCES_FONT_DEFAULT_SIZE;
	Math::Vec2i sizev(8, 8);
	if (n >= 2) {
		if (isNumber(L, 2)) {
			read<2>(L, size);

			sizev = Math::Vec2i(size, size);
		} else {
			read<2>(L, sizev);

			size = sizev.y > 0 ? sizev.y : sizev.x;
		}
	}
	int permeation = 1;
	if (n >= 3)
		read<3>(L, permeation);

	const unsigned type = name ? Asset::typeOf(name, false) : 0;

	// Loaders.
	auto fromImage = [] (Bytes::Ptr &bytes, const char* name, const Math::Vec2i &sizev, int permeation) -> Font::Ptr {
		Image::Ptr src(Image::create(nullptr));
		if (Text::endsWith(name, "." BITTY_IMAGE_EXT, true)) {
			std::string str;
			if (!bytes->readString(str))
				return nullptr;
			Json::Ptr json(Json::create());
			if (!json->fromString(str))
				return nullptr;
			rapidjson::Document doc;
			if (!json->toJson(doc))
				return nullptr;
			if (!src->fromJson(doc))
				return nullptr;
		} else {
			if (!src->fromBytes(bytes.get()))
				return nullptr;
		}

		Font::Ptr obj(Font::create());
		if (!obj->fromImage(src.get(), (int)sizev.x, (int)sizev.y, permeation))
			return nullptr;

		return obj;
	};
	auto fromFont = [] (const Bytes::Ptr &bytes, int size, int permeation) -> Font::Ptr {
		Font::Ptr result(Font::create());
		if (!result->fromBytes(bytes->pointer(), bytes->count(), size, permeation))
			return nullptr;

		return result;
	};

	// Load from the default font.
	do {
		if (img || name)
			break;

		Font::Ptr obj(Font::create());
		if (!obj)
			break;

		if (!obj->fromBytes(RES_FONT_PROGGY_CLEAN, BITTY_COUNTOF(RES_FONT_PROGGY_CLEAN), size, permeation))
			break;

		return write(L, &obj);
	} while (false);

	// Load from an image object.
	do {
		if (!img)
			break;

		Font::Ptr obj(Font::create());
		if (!obj)
			break;

		if (!obj->fromImage(img->get(), (int)sizev.x, (int)sizev.y, permeation))
			break;

		return write(L, &obj);
	} while (false);

	// Load from an image asset.
	do {
		if (type != Image::TYPE())
			break;

		if (!name)
			break;

		const Project* project = impl->project();

		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = project->acquire(acquired);
		if (!prj)
			break;

		Asset* asset = prj->get(name);
		if (!asset)
			break;

		Bytes::Ptr bytes(Bytes::create());
		bool saved = asset->toBytes(bytes.get());
		if (!saved)
			saved = asset->object(Asset::RUNNING) && asset->save(Asset::RUNNING, bytes.get());
		if (!saved)
			break;

		Font::Ptr ret = fromImage(bytes, name, sizev, permeation);
		if (!ret)
			break;

		return write(L, &ret);
	} while (false);

	// Load from an image file.
	do {
		if (type != Image::TYPE())
			break;

		if (!name)
			break;

		Bytes::Ptr bytes(Bytes::create());
		File::Ptr file(File::create());
		if (!file->open(name, Stream::READ))
			break;
		file->readBytes(bytes.get());
		file->close();

		Font::Ptr ret = fromImage(bytes, name, sizev, permeation);
		if (!ret)
			break;

		return write(L, &ret);
	} while (false);

	// Load from a font asset.
	do {
		if (type != Font::TYPE())
			break;

		const Project* project = impl->project();

		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = project->acquire(acquired);
		if (!prj)
			break;

		Asset* asset = prj->get(name);
		if (!asset)
			break;

		Font::Ptr ret = nullptr;
		const bool ready = asset->readyFor(Asset::RUNNING);
		if (ready) {
			Object::Ptr obj = asset->object(Asset::RUNNING);
			if (!obj)
				break;
			Bytes::Ptr bytes = Object::as<Bytes::Ptr>(obj);
			if (!bytes)
				break;
			ret = fromFont(bytes, size, permeation);
		} else {
			asset->prepare(Asset::RUNNING, true);

			Object::Ptr obj = asset->object(Asset::RUNNING);
			if (!obj)
				break;
			Bytes::Ptr bytes = Object::as<Bytes::Ptr>(obj);
			if (!bytes)
				break;
			ret = fromFont(bytes, size, permeation);

			asset->finish(Asset::RUNNING, true);
		}
		if (!ret)
			break;

		return write(L, &ret);
	} while (false);

	// Load from a font file.
	do {
		if (type != Font::TYPE())
			break;

		Bytes::Ptr bytes(Bytes::create());
		File::Ptr file(File::create());
		if (!file->open(name, Stream::READ))
			break;
		file->readBytes(bytes.get());
		file->close();

		Font::Ptr ret = fromFont(bytes, size, permeation);
		if (!ret)
			break;

		return write(L, &ret);
	} while (false);

	return write(L, nullptr);
}

static void open_Font(lua_State* L) {
	def(
		L, "Font",
		LUA_LIB(
			array(
				luaL_Reg{ "new", Font_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Font::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Font::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array<luaL_Reg>(),
		nullptr, nullptr
	);
}

/**< Primitives. */

static int Primitives_cls(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	Color* col = nullptr;
	if (n >= 1)
		read<>(L, col);

	Color ret;
	if (col)
		ret = impl->primitives()->cls(col);
	else
		ret = impl->primitives()->cls(nullptr);

	return write(L, &ret);
}

static int Primitives_blend(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	if (n >= 1) {
		Enum blendMode = SDL_BLENDMODE_NONE;
		read<>(L, blendMode);

		impl->primitives()->blend(blendMode);
	} else {
		impl->primitives()->blend();
	}

	return 0;
}

static int Primitives_camera(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	int x = 0, y = 0;
	if (n >= 2)
		read<>(L, x, y);

	int oldX = 0, oldY = 0;
	const bool changed = impl->primitives()->camera(&oldX, &oldY);
	if (n >= 2)
		impl->primitives()->camera(x, y);
	else
		impl->primitives()->camera();

	if (changed)
		return write(L, oldX, oldY);

	return write(L, nullptr, nullptr);
}

static int Primitives_clip(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	int x = 0, y = 0, w = 0, h = 0;
	if (n >= 4)
		read<>(L, x, y, w, h);

	int oldX = 0, oldY = 0, oldW = 0, oldH = 0;
	const bool changed = impl->primitives()->clip(&oldX, &oldY, &oldW, &oldH);
	if (n >= 4)
		impl->primitives()->clip(x, y, w, h);
	else
		impl->primitives()->clip();

	if (changed)
		return write(L, oldX, oldY, oldW, oldH);

	return write(L, nullptr, nullptr, nullptr, nullptr);
}

static int Primitives_color(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	Color* col = nullptr;
	if (n >= 1)
		read<>(L, col);

	Color ret;
	if (n >= 1)
		ret = impl->primitives()->color(col);
	else
		ret = impl->primitives()->color();

	return write(L, &ret);
}

static int Primitives_plot(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	int x = 0, y = 0;
	Color* col = nullptr;
	if (n >= 3)
		read<>(L, x, y, col);
	else
		read<>(L, x, y);

	impl->primitives()->plot(x, y, col);

	return 0;
}

static int Primitives_line(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
	Color* col = nullptr;
	if (n >= 5)
		read<>(L, x0, y0, x1, y1, col);
	else
		read<>(L, x0, y0, x1, y1);

	impl->primitives()->line(x0, y0, x1, y1, col);

	return 0;
}

static int Primitives_circ(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	int x = 0, y = 0, r = 0;
	bool fill = false;
	Color* col = nullptr;
	if (n >= 5)
		read<>(L, x, y, r, fill, col);
	else if (n == 4)
		read<>(L, x, y, r, fill);
	else
		read<>(L, x, y, r);

	impl->primitives()->circ(x, y, r, fill, col);

	return 0;
}

static int Primitives_ellipse(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	int x = 0, y = 0, rx = 0, ry = 0;
	bool fill = false;
	Color* col = nullptr;
	if (n >= 6)
		read<>(L, x, y, rx, ry, fill, col);
	else if (n == 5)
		read<>(L, x, y, rx, ry, fill);
	else
		read<>(L, x, y, rx, ry);

	impl->primitives()->ellipse(x, y, rx, ry, fill, col);

	return 0;
}

static int Primitives_pie(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	int x = 0, y = 0, r = 0;
	float startAngle = 0, endAngle = 0;
	bool fill = false;
	Color* col = nullptr;
	if (n >= 7)
		read<>(L, x, y, r, startAngle, endAngle, fill, col);
	else if (n == 6)
		read<>(L, x, y, r, startAngle, endAngle, fill);
	else
		read<>(L, x, y, r, startAngle, endAngle);

	impl->primitives()->pie(x, y, r, (int)Math::radToDeg(startAngle), (int)Math::radToDeg(endAngle), fill, col);

	return 0;
}

static int Primitives_rect(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
	bool fill = false;
	Color* col = nullptr;
	int rad = -1;
	if (n >= 7)
		read<>(L, x0, y0, x1, y1, fill, col, rad);
	else if (n == 6)
		read<>(L, x0, y0, x1, y1, fill, col);
	else if (n == 5)
		read<>(L, x0, y0, x1, y1, fill);
	else
		read<>(L, x0, y0, x1, y1);

	impl->primitives()->rect(x0, y0, x1, y1, fill, col, rad > 0 ? &rad : nullptr);

	return 0;
}

static int Primitives_font(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	Font::Ptr* font = nullptr;
	read<>(L, font);

	if (font)
		impl->primitives()->font(*font);
	else
		impl->primitives()->font();

	return 0;
}

static int Primitives_measure(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	Placeholder _1;
	Font::Ptr* font = nullptr;
	int margin = 1;
	if (n >= 3)
		read<>(L, _1, font, margin);
	else if (n == 2)
		read<>(L, _1, font);

	if (isString(L, 1) || isNumber(L, 1)) {
		const char* txt = nullptr;
		read<1>(L, txt);

		const Math::Vec2f size_ = impl->primitives()->measure(txt, font ? *font : nullptr, margin);

		return write(L, size_.x, size_.y);
	} else {
		Variant var = nullptr;
		read<1>(L, &var);

		const std::string str = var.toString();
		const Math::Vec2f size_ = impl->primitives()->measure(str.c_str(), font ? *font : nullptr, margin);

		return write(L, size_.x, size_.y);
	}
}

static int Primitives_text(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	Placeholder _1;
	int x = 0, y = 0;
	Color* col = nullptr;
	int margin = 1;
	if (n >= 5)
		read<>(L, _1, x, y, col, margin);
	else if (n == 4)
		read<>(L, _1, x, y, col);
	else
		read<>(L, _1, x, y);

	if (isString(L, 1) || isNumber(L, 1)) {
		const char* txt = nullptr;
		read<1>(L, txt);

		if (txt)
			impl->primitives()->text(txt, x, y, col, margin);
	} else {
		Variant var = nullptr;
		read<1>(L, &var);

		const std::string str = var.toString();
		impl->primitives()->text(str.c_str(), x, y, col, margin);
	}

	return 0;
}

static int Primitives_tri(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	Math::Vec2f* p0 = nullptr;
	Math::Vec2f* p1 = nullptr;
	Math::Vec2f* p2 = nullptr;
	bool fill = false;
	Color* col = nullptr;
	Resources::Texture::Ptr* res = nullptr;
	Math::Vec2f* uv0 = nullptr;
	Math::Vec2f* uv1 = nullptr;
	Math::Vec2f* uv2 = nullptr;
	if (isUserdata(L, 4)) {
		read<>(L, p0, p1, p2, res, uv0, uv1, uv2); // Undocumented.
	} else {
		if (n >= 5)
			read<>(L, p0, p1, p2, fill, col);
		else if (n == 4)
			read<>(L, p0, p1, p2, fill);
		else
			read<>(L, p0, p1, p2);
	}

	if (p0 && p1 && p2 && res && uv0 && uv1 && uv2)
		impl->primitives()->tri(*p0, *p1, *p2, *res, *uv0, *uv1, *uv2);
	else if (p0 && p1 && p2)
		impl->primitives()->tri(*p0, *p1, *p2, fill, col);

	return 0;
}

static int Primitives_tex(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	Resources::Texture::Ptr* res = nullptr;
	int x = 0, y = 0, w = 0, h = 0;
	int sx = 0, sy = 0, sw = 0, sh = 0;
	double rotAngle = 0;
	Math::Vec2f* rotCenter = nullptr;
	bool hFlip = false, vFlip = false;
	Color* col = nullptr;
	if (n >= 14)
		read<>(L, res, x, y, w, h, sx, sy, sw, sh, rotAngle, rotCenter, hFlip, vFlip, col);
	else if (n == 13)
		read<>(L, res, x, y, w, h, sx, sy, sw, sh, rotAngle, rotCenter, hFlip, vFlip);
	else if (n == 12)
		read<>(L, res, x, y, w, h, sx, sy, sw, sh, rotAngle, rotCenter, hFlip);
	else if (n == 11)
		read<>(L, res, x, y, w, h, sx, sy, sw, sh, rotAngle, rotCenter);
	else if (n == 10)
		read<>(L, res, x, y, w, h, sx, sy, sw, sh, rotAngle);
	else if (n == 9)
		read<>(L, res, x, y, w, h, sx, sy, sw, sh);
	else if (n == 7)
		read<>(L, res, x, y, w, h, sx, sy);
	else if (n == 5)
		read<>(L, res, x, y, w, h);
	else
		read<>(L, res, x, y);

	const double* rotAnglePtr = nullptr;
	if (rotAngle != 0) {
		rotAngle = Math::radToDeg(rotAngle);
		rotAnglePtr = &rotAngle;
	}
	impl->primitives()->tex(
		res ? *res : nullptr,
		x, y, w, h,
		sx, sy, sw, sh,
		rotAnglePtr, rotCenter,
		hFlip, vFlip,
		col
	);

	return 0;
}

static int Primitives_spr(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	Resources::Sprite::Ptr* res = nullptr;
	int x = 0, y = 0, w = 0, h = 0;
	double rotAngle = 0;
	Math::Vec2f* rotCenter = nullptr;
	Color* col = nullptr;
	if (n >= 8)
		read<>(L, res, x, y, w, h, rotAngle, rotCenter, col);
	else if (n == 7)
		read<>(L, res, x, y, w, h, rotAngle, rotCenter);
	else if (n == 6)
		read<>(L, res, x, y, w, h, rotAngle);
	else if (n == 5)
		read<>(L, res, x, y, w, h);
	else
		read<>(L, res, x, y);

	if (res && *res) {
		const double* rotAnglePtr = nullptr;
		if (rotAngle != 0) {
			rotAngle = Math::radToDeg(rotAngle);
			rotAnglePtr = &rotAngle;
		}
		impl->primitives()->spr(
			*res,
			x, y, w, h,
			rotAnglePtr, rotCenter,
			impl->delta(),
			col
		);
	} else {
		error(L, "Sprite resource expected.");
	}

	return 0;
}

static int Primitives_map(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	Resources::Map::Ptr* res = nullptr;
	int x = 0, y = 0;
	Color* col = nullptr;
	if (n >= 4)
		read<>(L, res, x, y, col);
	else
		read<>(L, res, x, y);

	if (res && *res)
		impl->primitives()->map(*res, x, y, impl->delta(), col);
	else
		error(L, "Map resource expected.");

	return 0;
}

static int Primitives_pget(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	Resources::Palette::Ptr* res = nullptr;
	int index = -1;
	read<>(L, res, index);

	if (res && *res) {
		Resources_waitUntilProcessed<Palette::Ptr>(impl, impl->primitives(), *res, nullptr);

		Color col;
		impl->primitives()->pget(*res, index, col);

		return write(L, &col);
	} else {
		error(L, "Palette resource expected.");
	}

	return 0;
}

static int Primitives_pset(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	Resources::Palette::Ptr* res = nullptr;
	int index = -1;
	Color* col = nullptr;
	read<>(L, res, index, col);

	if (res && *res && col) {
		Resources_waitUntilProcessed<Palette::Ptr>(impl, impl->primitives(), *res, nullptr);

		impl->primitives()->pset(*res, index, *col);
	} else {
		error(L, "Palette resource expected.");
	}

	return 0;
}

static int Primitives_mget(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	Resources::Map::Ptr* res = nullptr;
	int x = -1, y = -1;
	read<>(L, res, x, y);

	if (res && *res) {
		Resources_waitUntilProcessed<Map::Ptr>(impl, impl->primitives(), *res, (*res)->ref);

		int cel = Map::INVALID();
		impl->primitives()->mget(*res, x, y, cel);

		return write(L, cel);
	} else {
		error(L, "Map resource expected.");
	}

	return 0;
}

static int Primitives_mset(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	Resources::Map::Ptr* res = nullptr;
	int x = -1, y = -1;
	int cel = Map::INVALID();
	read<>(L, res, x, y, cel);

	if (res && *res) {
		Resources_waitUntilProcessed<Map::Ptr>(impl, impl->primitives(), *res, (*res)->ref);

		impl->primitives()->mset(*res, x, y, cel);
	} else {
		error(L, "Map resource expected.");
	}

	return 0;
}

static int Primitives_volume(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	Placeholder _1;
	Audio::SfxVolume sfxVols;
	float sfxVol = 1;
	float musicVol = -1;
	if (n >= 2)
		read<>(L, _1, musicVol);
	else
		read<>(L, _1);
	if (isNumber(L, 1)) {
		read<>(L, sfxVol);

		impl->primitives()->volume(sfxVol, musicVol);
	} else {
		std::vector<float> sfxVols_;
		read<>(L, sfxVols_);
		for (int i = 0; i < AUDIO_SFX_CHANNEL_COUNT && i < (int)sfxVols_.size(); ++i)
			sfxVols[i] = sfxVols_[i];
		for (int i = (int)sfxVols_.size(); i < AUDIO_SFX_CHANNEL_COUNT; ++i)
			sfxVols[i] = -1;

		impl->primitives()->volume(sfxVols, musicVol);
	}

	return 0;
}

static int Primitives_play(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	Resources::Sfx::Ptr* sfx = nullptr;
	read<>(L, sfx);

	if (sfx && *sfx) {
		bool loop = false;
		float fade = -1;
		int channel = -1;
		if (n >= 4)
			read<2>(L, loop, fade, channel);
		else if (n == 3)
			read<2>(L, loop, fade);
		else if (n == 2)
			read<2>(L, loop);

		const int fadeMs = fade >= 0 ? (int)fade * 1000 : -1;
		--channel; // 1-based.
		impl->primitives()->play(*sfx, loop, fadeMs > 0 ? &fadeMs : nullptr, channel);

		return 0;
	}

	Resources::Music::Ptr* mus = nullptr;
	read<>(L, mus);

	if (mus && *mus) {
		bool loop = false;
		float fade = -1;
		if (n >= 3)
			read<2>(L, loop, fade);
		else if (n == 2)
			read<2>(L, loop);

		const int fadeMs = fade >= 0 ? (int)fade * 1000 : -1;
		impl->primitives()->play(*mus, loop, fadeMs > 0 ? &fadeMs : nullptr);

		return 0;
	}

	error(L, "Sound resource expected.");

	return 0;
}

static int Primitives_stop(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	Resources::Sfx::Ptr* sfx = nullptr;
	read<>(L, sfx);

	if (sfx && *sfx) {
		float fade = -1;
		if (n >= 2)
			read<2>(L, fade);

		const int fadeMs = fade >= 0 ? (int)fade * 1000 : -1;
		impl->primitives()->stop(*sfx, fadeMs > 0 ? &fadeMs : nullptr);

		return 0;
	}

	Resources::Music::Ptr* mus = nullptr;
	read<>(L, mus);

	if (mus && *mus) {
		float fade = -1;
		if (n >= 2)
			read<2>(L, fade);

		const int fadeMs = fade >= 0 ? (int)fade * 1000 : -1;
		impl->primitives()->stop(*mus, fadeMs > 0 ? &fadeMs : nullptr);

		return 0;
	}

	error(L, "Sound resource expected.");

	return 0;
}

static int Primitives_btn(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	int btn = -1, idx = 1;
	if (n >= 2)
		read<>(L, btn, idx);
	else if (n == 1)
		read<>(L, btn);

	if (idx > 0) {
		--idx; // 1-based.
		const bool ret = !!impl->primitives()->btn(btn, idx);

		return write(L, ret);
	} else {
		// `idx` is -1-based.
		const int ret = impl->primitives()->btn(btn, idx); // Undocumented: controller button/axis.

		if (ret)
			return write(L, ret);

		if (btn >= 0)
			return write(L, false);
		else
			return write(L, 0);
	}
}

static int Primitives_btnp(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	int btn = -1, idx = 1;
	if (n >= 2)
		read<>(L, btn, idx);
	else if (n == 1)
		read<>(L, btn);

	if (idx > 0) {
		--idx; // 1-based.
		const bool ret = !!impl->primitives()->btnp(btn, idx);

		return write(L, ret);
	} else {
		// `idx` is -1-based.
		const int ret = impl->primitives()->btnp(btn, idx); // Undocumented: controller button/axis.

		if (ret)
			return write(L, ret);

		if (btn >= 0)
			return write(L, false);
		else
			return write(L, 0);
	}
}

static int Primitives_rumble(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	int idx = 1;
	int lowHz = 100, hiHz = 0;
	unsigned ms = 100;
	if (n >= 4)
		read<>(L, idx, lowHz, hiHz, ms);
	else if (n == 3)
		read<>(L, idx, lowHz, hiHz);
	else if (n == 2)
		read<>(L, idx, lowHz);
	else if (n == 1)
		read<>(L, idx);

	if (hiHz == 0)
		hiHz = lowHz;
	if (idx > 0) {
		--idx; // 1-based.
		impl->primitives()->rumble(idx, lowHz, hiHz, ms);
	} else {
		// `idx` is -1-based.
		impl->primitives()->rumble(idx, lowHz, hiHz, ms); // Undocumented.
	}

	return 0;
}

static int Primitives_key(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	int key = -1;
	if (n >= 1) {
		if (isNumber(L, 1)) {
			read<>(L, key);
		} else if (isString(L, 1)) {
			std::string str;
			read<>(L, str);
			key = str.empty() ? 0 : str.front();
		}
	}

	const bool ret = impl->primitives()->key(key);

	return write(L, ret);
}

static int Primitives_keyp(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	int key = -1;
	if (n >= 1) {
		if (isNumber(L, 1)) {
			read<>(L, key);
		} else if (isString(L, 1)) {
			std::string str;
			read<>(L, str);
			key = str.empty() ? 0 : str.front();
		}
	}

	const bool ret = impl->primitives()->keyp(key);

	return write(L, ret);
}

static int Primitives_mouse(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	int idx = 1;
	if (n >= 1)
		read<>(L, idx);

	--idx; // 1-based.
	int x = 0, y = 0;
	bool b0 = false, b1 = false, b2 = false;
	int wheelY = 0;
	if (!impl->primitives()->mouse(idx, &x, &y, &b0, &b1, &b2, nullptr, &wheelY)) {
		return write(
			L,
			std::numeric_limits<lua_Number>::quiet_NaN(),
			std::numeric_limits<lua_Number>::quiet_NaN(),
			b0, b1, b2,
			wheelY
		);
	}

	return write(L, x, y, b0, b1, b2, wheelY);
}

static int Primitives_sync(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int ret = impl->primitives()->sync();

	impl->sync(0);

	return write(L, ret);
}

static void open_Primitives(lua_State* L) {
	reg(
		L,
		array(
			luaL_Reg{ "cls", Primitives_cls }, // Frame synchronized/asynchronized.
			luaL_Reg{ "blend", Primitives_blend }, // Frame synchronized.
			luaL_Reg{ "camera", Primitives_camera },
			luaL_Reg{ "clip", Primitives_clip },
			luaL_Reg{ "color", Primitives_color },
			luaL_Reg{ "plot", Primitives_plot },
			luaL_Reg{ "line", Primitives_line },
			luaL_Reg{ "circ", Primitives_circ },
			luaL_Reg{ "ellipse", Primitives_ellipse },
			luaL_Reg{ "pie", Primitives_pie },
			luaL_Reg{ "rect", Primitives_rect },
			luaL_Reg{ "font", Primitives_font }, // Frame synchronized.
			luaL_Reg{ "measure", Primitives_measure },
			luaL_Reg{ "text", Primitives_text },
			luaL_Reg{ "tri", Primitives_tri },
			luaL_Reg{ "tex", Primitives_tex },
			luaL_Reg{ "spr", Primitives_spr },
			luaL_Reg{ "map", Primitives_map },
			luaL_Reg{ "pget", Primitives_pget }, // Resources synchronized.
			luaL_Reg{ "pset", Primitives_pset }, // Resources/frame synchronized.
			luaL_Reg{ "mget", Primitives_mget }, // Resources synchronized.
			luaL_Reg{ "mset", Primitives_mset }, // Resources/frame synchronized.
			luaL_Reg{ "volume", Primitives_volume }, // Frame synchronized.
			luaL_Reg{ "play", Primitives_play }, // Frame synchronized.
			luaL_Reg{ "stop", Primitives_stop }, // Frame synchronized.
			luaL_Reg{ "btn", Primitives_btn },
			luaL_Reg{ "btnp", Primitives_btnp },
			luaL_Reg{ "rumble", Primitives_rumble }, // Frame synchronized.
			luaL_Reg{ "key", Primitives_key },
			luaL_Reg{ "keyp", Primitives_keyp },
			luaL_Reg{ "mouse", Primitives_mouse },
			luaL_Reg{ "sync", Primitives_sync },
			luaL_Reg{ nullptr, nullptr }
		)
	);
}

/**< Categories. */

void open(class Executable* exec) {
	// Prepare.
	lua_State* L = (lua_State*)exec->pointer();

	// Resources.
	open_Resources(L);
	open_ResourceAsset(L);
	open_ResourcePalette(L);
	open_ResourceTexture(L);
	open_ResourceSprite(L);
	open_ResourceMap(L);
	open_ResourceSfx(L);
	open_ResourceMusic(L);

	// Font.
	open_Font(L);

	// Primitives.
	open_Primitives(L);
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

/**< Application. */

static int Application_setOption(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	std::string key;
	read<>(L, key);

	if (key == "title") {
		std::string title;
		read<2>(L, title);
		if (title.empty()) {
			error(L, "Invalid title.");

			return 0;
		}

		impl->primitives()->function(
			[=] (const Variant &) -> void {
				Window* wnd = impl->primitives()->window();
				wnd->title(title.c_str());
			},
			nullptr,
			true
		);
	} else if (key == "minimum_size") {
		int w = 0, h = 0;
		read<2>(L, w, h);
		if (w < 0 || h < 0) {
			error(L, "Invalid size.");

			return 0;
		}

		impl->primitives()->function(
			[=] (const Variant &) -> void {
				Window* wnd = impl->primitives()->window();
				const bool fullscreen = wnd->fullscreen();
				wnd->minimumSize(Math::Vec2i(w, h));
				if (!fullscreen)
					wnd->centralize();
			},
			nullptr,
			true
		);
	} else if (key == "maximum_size") {
		int w = 0, h = 0;
		read<2>(L, w, h);
		if (w < 0 || h < 0) {
			error(L, "Invalid size.");

			return 0;
		}

		impl->primitives()->function(
			[=] (const Variant &) -> void {
				Window* wnd = impl->primitives()->window();
				const bool fullscreen = wnd->fullscreen();
				wnd->maximumSize(Math::Vec2i(w, h));
				if (!fullscreen)
					wnd->centralize();
			},
			nullptr,
			true
		);
	} else if (key == "bordered") {
		bool b = true;
		read<2>(L, b);

		impl->primitives()->function(
			[=] (const Variant &) -> void {
				Window* wnd = impl->primitives()->window();
				wnd->bordered(b);
			},
			nullptr,
			true
		);
	} else if (key == "resizable") {
		bool r = true;
		read<2>(L, r);

		impl->primitives()->function(
			[=] (const Variant &) -> void {
				Window* wnd = impl->primitives()->window();
				wnd->resizable(r);
			},
			nullptr,
			true
		);
	} else {
		error(L, "Invalid option.");
	}

	return 0;
}

static int Application_setCursor(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	Image::Ptr* img = nullptr;
	float x = 0, y = 0;
	if (n >= 3)
		read<>(L, img, x, y);
	else
		read<>(L, img);

	if (img && *img) {
		constexpr const int MAX_SIZE = 256;
		if (img->get()->paletted()) {
			error(L, "True-color image expected.");
		} else if (img->get()->width() > MAX_SIZE || img->get()->height() > MAX_SIZE) {
			error(L, "Image too big.");
		} else {
			Image::Ptr cur(Image::create(nullptr));
			cur->fromImage(img->get());
			impl->primitives()->cursor(cur, x, y);
		}
	} else {
		impl->primitives()->cursor(nullptr, x, y);
	}

	return 0;
}

static int Application_size(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const Math::Vec2i ret = impl->observer()->applicationSize();

	return write(L, ret.x, ret.y);
}

static int Application_resize(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	int w = 0, h = 0;
	std::string s;
	if (n >= 2)
		read<>(L, w, h);
	else
		read<>(L, s);

	if (n >= 2) {
		if (w <= 0 || h <= 0) {
			error(L, "Invalid size.");

			return 0;
		}

		impl->primitives()->function(
			[=] (const Variant &) -> void {
				Window* wnd = impl->primitives()->window();
				Renderer* rnd = impl->primitives()->renderer();
				wnd->fullscreen(false);
				wnd->size(Math::Vec2i(w, h));
				wnd->centralize();
				impl->observer()->resizeApplication(
					Math::Vec2i(
						w / rnd->scale(),
						h / rnd->scale()
					)
				);
			},
			nullptr,
			true
		);
	} else if (s == "fullscreen") {
		impl->primitives()->function(
			[=] (const Variant &) -> void {
				Window* wnd = impl->primitives()->window();
				wnd->fullscreen(true);
			},
			nullptr,
			true
		);
	} else if (s == "windowed") {
		impl->primitives()->function(
			[=] (const Variant &) -> void {
				Window* wnd = impl->primitives()->window();
				wnd->fullscreen(false);
			},
			nullptr,
			true
		);
	} else {
		error(L, "Invalid size.");
	}

	return 0;
}

static int Application_raise(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	impl->primitives()->function(
		[=] (const Variant &) -> void {
			Window* wnd = impl->primitives()->window();
			wnd->raise();
		},
		nullptr,
		true
	);

	return 0;
}

#if BITTY_EFFECTS_ENABLED
static int Application_setEffect(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const char* material = nullptr;
	read<>(L, material);

	if (material) {
		const std::string material_ = material;
		impl->primitives()->function(
			[=] (const Variant &) -> void {
				impl->observer()->effect(material_.c_str());
			},
			nullptr,
			true
		);
	} else {
		impl->primitives()->function(
			[=] (const Variant &) -> void {
				impl->observer()->effect(nullptr);
			},
			nullptr,
			true
		);
	}

	return 0;
}

static int Application_setEffectUniform(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	Resources::Texture::Ptr* tex = nullptr;
	Image::Ptr* img = nullptr;
	Math::Vec4f* vec4 = nullptr;
	Math::Vec3f* vec3 = nullptr;
	Math::Vec2f* vec2 = nullptr;
	float number = 0;
	const char* name = nullptr;
	read<>(L, name);
	read<2>(L, tex);
	read<2>(L, img);
	read<2>(L, vec4);
	read<2>(L, vec3);
	read<2>(L, vec2);
	read<2>(L, number);

	if (name) {
		const std::string name_ = name;
		if (tex) {
			const Resources::Texture::Ptr data = *tex;
			impl->primitives()->function(
				[=] (const Variant &) -> void {
					Effects* effects = impl->primitives()->effects();
					if (effects)
						effects->inject(name_.c_str(), data);
				},
				nullptr,
				true
			);
		} else if (img) {
			const Image::Ptr data = *img;
			impl->primitives()->function(
				[=] (const Variant &) -> void {
					Effects* effects = impl->primitives()->effects();
					if (effects)
						effects->inject(name_.c_str(), data);
				},
				nullptr,
				true
			);
		} else if (vec4) {
			const Math::Vec4f data = *vec4;
			impl->primitives()->function(
				[=] (const Variant &) -> void {
					Effects* effects = impl->primitives()->effects();
					if (effects)
						effects->inject(name_.c_str(), data);
				},
				nullptr,
				true
			);
		} else if (vec3) {
			const Math::Vec3f data = *vec3;
			impl->primitives()->function(
				[=] (const Variant &) -> void {
					Effects* effects = impl->primitives()->effects();
					if (effects)
						effects->inject(name_.c_str(), data);
				},
				nullptr,
				true
			);
		} else if (vec2) {
			const Math::Vec2f data = *vec2;
			impl->primitives()->function(
				[=] (const Variant &) -> void {
					Effects* effects = impl->primitives()->effects();
					if (effects)
						effects->inject(name_.c_str(), data);
				},
				nullptr,
				true
			);
		} else {
			const float data = number;
			impl->primitives()->function(
				[=] (const Variant &) -> void {
					Effects* effects = impl->primitives()->effects();
					if (effects)
						effects->inject(name_.c_str(), data);
				},
				nullptr,
				true
			);
		}
	}

	return 0;
}
#endif /* BITTY_EFFECTS_ENABLED */

static void open_Application(lua_State* L) {
#if BITTY_EFFECTS_ENABLED
	ScriptingLua* impl = ScriptingLua::instanceOf(L);
	const bool effectsEnabled = impl->effectsEnabled();
#else /* BITTY_EFFECTS_ENABLED */
	const bool effectsEnabled = false;
#endif /* BITTY_EFFECTS_ENABLED */

	if (effectsEnabled) {
		req(
			L,
			array(
				luaL_Reg{
					"Application",
					[] (lua_State* L) -> int {
						lib(
							L,
							array(
								luaL_Reg{ "setOption", Application_setOption }, // Frame synchronized.
								luaL_Reg{ "setCursor", Application_setCursor }, // Frame synchronized.
								luaL_Reg{ "size", Application_size }, // Frame synchronized.
								luaL_Reg{ "resize", Application_resize }, // Frame synchronized.
								luaL_Reg{ "raise", Application_raise }, // Frame synchronized.
#if BITTY_EFFECTS_ENABLED
								luaL_Reg{ "setEffect", Application_setEffect }, // Undocumented. Frame synchronized.
								luaL_Reg{ "setEffectUniform", Application_setEffectUniform }, // Undocumented. Frame synchronized.
#endif /* BITTY_EFFECTS_ENABLED */
								luaL_Reg{ nullptr, nullptr }
							)
						);

						return 1;
					}
				},
				luaL_Reg{ nullptr, nullptr }
			)
		);
	} else {
		req(
			L,
			array(
				luaL_Reg{
					"Application",
					[] (lua_State* L) -> int {
						lib(
							L,
							array(
								luaL_Reg{ "setOption", Application_setOption }, // Frame synchronized.
								luaL_Reg{ "setCursor", Application_setCursor }, // Frame synchronized.
								luaL_Reg{ "size", Application_size }, // Frame synchronized.
								luaL_Reg{ "resize", Application_resize }, // Frame synchronized.
								luaL_Reg{ "raise", Application_raise }, // Frame synchronized.
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
}

/**< Canvas. */

static int Canvas_size(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	CanvasPtr* obj = nullptr;
	read<>(L, obj);

	if (!obj)
		return 0;

	const Canvas* canvas = obj->get();
	if (!canvas)
		return 0;

	Math::Vec2i ret;
	if (canvas == impl->primitives())
		ret = impl->observer()->canvasSize();

	return write(L, ret.x, ret.y);
}

static int Canvas_resize(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	CanvasPtr* obj = nullptr;
	int width = 0, height = 0;
	read<>(L, obj, width, height);

	if (!obj)
		return 0;

	const Canvas* canvas = obj->get();
	if (!canvas)
		return 0;

	bool ret = false;
	if (canvas == impl->primitives())
		ret = impl->observer()->resizeCanvas(Math::Vec2i(width, height));

	return write(L, ret);
}

static int Canvas___index(lua_State* L) {
	CanvasPtr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "target") == 0) {
		const Resources::Texture::Ptr ret = obj->get()->target();

		return write(L, &ret);
	} else if (strcmp(field, "autoCls") == 0) {
		const bool ret = obj->get()->autoCls();

		return write(L, ret);
	} else {
		return __index(L, field);
	}
}

static int Canvas___newindex(lua_State* L) {
	CanvasPtr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	if (strcmp(field, "target") == 0) {
		Resources::Texture::Ptr* val = nullptr;
		read<3>(L, val);

		if (val)
			obj->get()->target(*val);
		else
			obj->get()->target(nullptr);
	} else if (strcmp(field, "autoCls") == 0) {
		bool val = true;
		read<3>(L, val);

		obj->get()->autoCls(val);
	} else {
		__newindex(L, field, 3);
	}

	return 0;
}

static int Canvas_compose(lua_State* L) {
	const int n = getTop(L);
	if (n >= 6) {
		Enum srcColFactor = SDL_BLENDFACTOR_ONE;
		Enum dstColFactor = SDL_BLENDFACTOR_ZERO;
		Enum colOp = SDL_BLENDOPERATION_ADD;
		Enum srcAlphaFactor = SDL_BLENDFACTOR_ONE;
		Enum dstAlphaFactor = SDL_BLENDFACTOR_ZERO;
		Enum alphaOp = SDL_BLENDOPERATION_ADD;
		read<>(L, srcColFactor, dstColFactor, colOp, srcAlphaFactor, dstAlphaFactor, alphaOp);

		const SDL_BlendMode blendMode = SDL_ComposeCustomBlendMode(
			(SDL_BlendFactor)srcColFactor, (SDL_BlendFactor)dstColFactor, (SDL_BlendOperation)colOp,
			(SDL_BlendFactor)srcAlphaFactor, (SDL_BlendFactor)dstAlphaFactor, (SDL_BlendOperation)alphaOp
		);

		return write(L, (Enum)blendMode);
	} else {
		const SDL_BlendMode blendMode = SDL_BLENDMODE_NONE;

		return write(L, (Enum)blendMode);
	}
}

static CanvasPtr Canvas_main(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	Canvas* canvas = impl->primitives();

	return CanvasPtr(
		canvas,
		[] (Canvas*) -> void {
			// Do nothing.
		}
	);
}

static void open_Canvas(lua_State* L) {
	def(
		L, "Canvas",
		LUA_LIB(
			array<luaL_Reg>()
		),
		array(
			luaL_Reg{ "__gc", __gc<CanvasPtr> },
			luaL_Reg{ "__tostring", __tostring<CanvasPtr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "size", Canvas_size },
			luaL_Reg{ "resize", Canvas_resize },
			luaL_Reg{ nullptr, nullptr }
		),
		Canvas___index, Canvas___newindex
	);

	getGlobal(L, "Canvas");
	setTable(
		L,
		"BlendModeNone", (Enum)SDL_BLENDMODE_NONE,
		"BlendModeBlend", (Enum)SDL_BLENDMODE_BLEND,
		"BlendModeAdd", (Enum)SDL_BLENDMODE_ADD,
		"BlendModeMod", (Enum)SDL_BLENDMODE_MOD,
#if SDL_VERSION_ATLEAST(2, 0, 12)
		"BlendModeMul", (Enum)SDL_BLENDMODE_MUL,
#else /* SDL_VERSION_ATLEAST(2, 0, 12) */
		"BlendModeMul", (Enum)SDL_ComposeCustomBlendMode(
			SDL_BLENDFACTOR_DST_COLOR, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD,
			SDL_BLENDFACTOR_DST_ALPHA, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD
		),
#endif /* SDL_VERSION_ATLEAST(2, 0, 12) */

		"BlendFactorZero", (Enum)SDL_BLENDFACTOR_ZERO,
		"BlendFactorOne", (Enum)SDL_BLENDFACTOR_ONE,
		"BlendFactorSrcColor", (Enum)SDL_BLENDFACTOR_SRC_COLOR,
		"BlendFactorOneMinusSrcColor", (Enum)SDL_BLENDFACTOR_ONE_MINUS_SRC_COLOR,
		"BlendFactorSrcAlpha", (Enum)SDL_BLENDFACTOR_SRC_ALPHA,
		"BlendFactorOneMinusSrcAlpha", (Enum)SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
		"BlendFactorDstColor", (Enum)SDL_BLENDFACTOR_DST_COLOR,
		"BlendFactorOneMinusDstColor", (Enum)SDL_BLENDFACTOR_ONE_MINUS_DST_COLOR,
		"BlendFactorDstAlpha", (Enum)SDL_BLENDFACTOR_DST_ALPHA,
		"BlendFactorOneMinusDstAlpha", (Enum)SDL_BLENDFACTOR_ONE_MINUS_DST_ALPHA,

		"BlendOperationAdd", (Enum)SDL_BLENDOPERATION_ADD,
		"BlendOperationSub", (Enum)SDL_BLENDOPERATION_SUBTRACT,
		"BlendOperationRevSub", (Enum)SDL_BLENDOPERATION_REV_SUBTRACT,
		"BlendOperationMin", (Enum)SDL_BLENDOPERATION_MINIMUM,
		"BlendOperationMax", (Enum)SDL_BLENDOPERATION_MAXIMUM,

		"compose", Canvas_compose
	);
	{
		CanvasPtr main = Canvas_main(L);
		if (main)
			setTable(L, "main", &main);
	}
	pop(L);
}

/**< Project. */

static int Project_ctor(lua_State* L) {
	if (isPlugin(L)) {
		error(L, "The \"Project.new()\" constructor is not available for plugin.");

		return 0;
	}

	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const Project* project = impl->project();
	if (!project) {
		error(L, "Cannot create project.");

		return 0;
	}

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return write(L, false);

	Project* newPrj = new Project();
	if (!newPrj)
		return write(L, nullptr);

	newPrj->loader(prj->loader());
	newPrj->factory(prj->factory());
	newPrj->open(nullptr);

	ProjectPtr ret(newPrj);

	return write(L, &ret);
}

static int Project___gc(lua_State* L) {
	ProjectPtr* obj = nullptr;
	check<>(L, obj);
	if (!obj)
		return 0;

	obj->~shared_ptr();

	return 0;
}

static int Project_fullPath(lua_State* L) {
	ProjectPtr* obj = nullptr;
	read<>(L, obj);

	if (!obj)
		return write(L, nullptr);

	const Project* project = obj->get();
	if (!project)
		return write(L, nullptr);

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return write(L, nullptr);

	const std::string &path = prj->path();
	if (path.empty())
		return write(L, nullptr);

	return write(L, path);
}

static int Project_getAssets(lua_State* L) {
#if BITTY_TRIAL_ENABLED
	if (isPlugin(L)) {
		error(L, "The \"project:getAssets(...)\" method is not available for trial.");

		return 0;
	}
#endif /* BITTY_TRIAL_ENABLED */

	ProjectPtr* obj = nullptr;
	read<>(L, obj);

	if (!obj)
		return write(L, nullptr);

	const Project* project = obj->get();
	if (!project)
		return write(L, nullptr);

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return write(L, nullptr);

	Text::Array entries;
	prj->foreach(
		[&] (Asset* &asset, Asset::List::Index) -> void {
			entries.push_back(asset->entry().name());
		}
	);

	return write(L, entries);
}

static int Project_load(lua_State* L) {
	if (isPlugin(L)) {
		error(L, "The \"project:load(...)\" method is not available for plugin.");

		return 0;
	}

	// Prepare.
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	// Get arguments.
	ProjectPtr* obj = nullptr;
	std::string path;
	read<>(L, obj, path);

	// Prepare.
	if (!obj)
		return write(L, false);

	const Project* main = impl->project();
	const Project* editing = impl->editing();
	if ((uintptr_t)obj->get() == (uintptr_t)main || (uintptr_t)obj->get() == (uintptr_t)editing) {
		error(L, "Cannot load from this project.");

		return 0;
	}
	do {
		if (!main)
			break;

		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = main->acquire(acquired);
		if (prj) {
			if (Path::isParentOf(prj->path().c_str(), path.c_str())) {
				error(L, "Cannot load from this project.");

				return 0;
			}
		}
	} while (false);
	do {
		if (!editing)
			break;

		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = editing->acquire(acquired);
		if (prj) {
			if (Path::isParentOf(prj->path().c_str(), path.c_str())) {
				error(L, "Cannot load from this project.");

				return 0;
			}
		}
	} while (false);

	const Project* project = obj->get();
	if (!project)
		return write(L, false);

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return write(L, false);

	if (prj->iterating()) {
		error(L, "Cannot load project while iterating.");

		return write(L, false);
	}

	// Load.
	prj->unload();
	prj->readonly(false);
	if (!prj->load(path.c_str()))
		return write(L, false);
	prj->dirty(false);

	// Finish.
	return write(L, true);
}

static int Project_save(lua_State* L) {
	if (isPlugin(L)) {
		error(L, "The \"project:save(...)\" method is not available for plugin.");

		return 0;
	}

	// Prepare.
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	// Get arguments.
	ProjectPtr* obj = nullptr;
	std::string path;
	read<>(L, obj, path);

	// Prepare.
	if (!obj)
		return write(L, false);

	const Project* main = impl->project();
	const Project* editing = impl->editing();
	if ((uintptr_t)obj->get() == (uintptr_t)main || (uintptr_t)obj->get() == (uintptr_t)editing) {
		error(L, "Cannot save to this project.");

		return 0;
	}
	do {
		if (!main)
			break;

		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = main->acquire(acquired);
		if (prj) {
			if (Path::isParentOf(prj->path().c_str(), path.c_str())) {
				error(L, "Cannot save to this project.");

				return 0;
			}
		}
	} while (false);
	do {
		if (!editing)
			break;

		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = editing->acquire(acquired);
		if (prj) {
			if (Path::isParentOf(prj->path().c_str(), path.c_str())) {
				error(L, "Cannot save to this project.");

				return 0;
			}
		}
	} while (false);

	const Project* project = obj->get();
	if (!project)
		return write(L, false);

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return write(L, false);

	if (prj->iterating()) {
		error(L, "Cannot save project while iterating.");

		return write(L, false);
	}

	// Save.
	if (!path.empty() && (path.back() == '/' || path.back() == '\\'))
		Path::touchDirectory(path.c_str());
	else if (Text::endsWith(path, "." BITTY_ZIP_EXT, true))
		prj->preference(Archive::ZIP);
	else
		prj->preference(Archive::TXT);
	if (!prj->save(path.c_str(), true, [] (const char*) -> void { /* Do nothing. */ }))
		return write(L, false);
	prj->readonly(false);
	prj->dirty(false);

	// Finish.
	return write(L, true);
}

static int Project_exists(lua_State* L) {
#if BITTY_TRIAL_ENABLED
	if (isPlugin(L)) {
		error(L, "The \"project:exists(...)\" method is not available for trial.");

		return 0;
	}
#endif /* BITTY_TRIAL_ENABLED */

	ProjectPtr* obj = nullptr;
	std::string name;
	read<>(L, obj, name);

	if (!obj)
		return write(L, false);

	const Project* project = obj->get();
	if (!project)
		return write(L, false);

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return write(L, false);

	Asset* asset = prj->get(name.c_str());
	if (!asset)
		return write(L, false);

	return write(L, true);
}

static int Project_read(lua_State* L) {
#if BITTY_TRIAL_ENABLED
	if (isPlugin(L)) {
		error(L, "The \"project:read(...)\" method is not available for trial.");

		return 0;
	}
#endif /* BITTY_TRIAL_ENABLED */

	ProjectPtr* obj = nullptr;
	std::string name;
	read<>(L, obj, name);

	if (!obj)
		return write(L, nullptr);

	const Project* project = obj->get();
	if (!project)
		return write(L, nullptr);

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return write(L, nullptr);

	Asset* asset = prj->get(name.c_str());
	if (!asset)
		return write(L, nullptr);

	Bytes::Ptr bytes(Bytes::create());
	bool saved = asset->toBytes(bytes.get());
	if (!saved)
		saved = asset->object(Asset::RUNNING) && asset->save(Asset::RUNNING, bytes.get());
	if (!saved)
		return write(L, nullptr);

	bytes->poke(bytes->count());

	return write(L, &bytes);
}

static int Project_write(lua_State* L) {
#if BITTY_TRIAL_ENABLED
	if (isPlugin(L)) {
		error(L, "The \"project:write(...)\" method is not available for trial.");

		return 0;
	}
#endif /* BITTY_TRIAL_ENABLED */

	// Prepare.
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

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

	const Project* main = impl->project();
	if ((uintptr_t)obj->get() == (uintptr_t)main) {
		error(L, "Cannot write to this project.");

		return 0;
	}

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

static int Project_remove(lua_State* L) {
#if BITTY_TRIAL_ENABLED
	if (isPlugin(L)) {
		error(L, "The \"project:remove(...)\" method is not available for trial.");

		return 0;
	}
#endif /* BITTY_TRIAL_ENABLED */

	// Prepare.
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	// Get arguments.
	ProjectPtr* obj = nullptr;
	std::string name;
	read<>(L, obj, name);

	// Prepare.
	if (!obj)
		return write(L, false);

	const Project* main = impl->project();
	const Project* editing = impl->editing();
	if ((uintptr_t)obj->get() == (uintptr_t)main || (uintptr_t)obj->get() == (uintptr_t)editing) {
		error(L, "Cannot remove from this project.");

		return 0;
	}

	const Project* project = obj->get();
	if (!project)
		return write(L, false);

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return write(L, false);

	if (prj->iterating()) {
		error(L, "Cannot remove from project while iterating.");

		return write(L, false);
	}

	Asset* asset = prj->get(name.c_str());
	if (!asset)
		return write(L, true);

	// Remove.
	Asset::States* states = asset->states();
	states->deactivate();
	states->deselect();

	asset->finish((Asset::Usages)(Asset::RUNNING | Asset::EDITING), false);
	asset->unload();
	prj->cleanup((Asset::Usages)(Asset::RUNNING | Asset::EDITING));

	asset->remove();
	prj->remove(asset);

	prj->dirty(true);

	// Finish.
	return write(L, true);
}

static int Project_strategies(lua_State* L) {
	ProjectPtr* obj = nullptr;
	read<>(L, obj);

	if (!obj)
		return write(L, nullptr);

	const Project* project = obj->get();
	if (!project)
		return write(L, nullptr);

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return write(L, nullptr);

	const Text::Array strategies = prj->strategies();

	return write(L, strategies);
}

static int Project___index(lua_State* L) {
	ProjectPtr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	return __index(L, field);
}

static int Project___newindex(lua_State* L) {
	ProjectPtr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !field)
		return 0;

	__newindex(L, field, 3);

	return 0;
}

static ProjectPtr Project_main(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const Project* project = impl->project();
	if (!project)
		return nullptr;

	return ProjectPtr(
		project,
		[] (const Project*) -> void {
			// Do nothing.
		}
	);
}

static ProjectPtr Project_editing(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const Project* project = impl->editing();
	if (!project)
		return nullptr;

	return ProjectPtr(
		project,
		[] (const Project*) -> void {
			// Do nothing.
		}
	);
}

static void open_Project(lua_State* L) {
	def(
		L, "Project",
		LUA_LIB(
			array(
				luaL_Reg{ "new", Project_ctor }, // For game only.
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", Project___gc },
			luaL_Reg{ "__tostring", __tostring<ProjectPtr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "fullPath", Project_fullPath },
			luaL_Reg{ "getAssets", Project_getAssets },
			luaL_Reg{ "load", Project_load }, // For game only.
			luaL_Reg{ "save", Project_save }, // For game only.
			luaL_Reg{ "exists", Project_exists },
			luaL_Reg{ "read", Project_read },
			luaL_Reg{ "write", Project_write },
			luaL_Reg{ "remove", Project_remove },
			luaL_Reg{ "strategies", Project_strategies },
			luaL_Reg{ nullptr, nullptr }
		),
		Project___index, Project___newindex
	);

	getGlobal(L, "Project");
	{
		ProjectPtr main = Project_main(L);
		if (main) // For game and plugin.
			setTable(L, "main", &main);
		ProjectPtr editing = Project_editing(L);
		if (editing) // For plugin only.
			setTable(L, "editing", &editing); // Undocumented.
	}
	pop(L);
}

/**< Debug. */

static int Debug_setBreakpoint(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	std::string name;
	int ln = -1;
	bool brk = true;
	if (n >= 3)
		read<>(L, name, ln, brk);
	else
		read<>(L, name, ln);

#if BITTY_DEBUG_ENABLED
	const Project* project = impl->project();
	if (!project)
		return write(L, false);

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return write(L, false);

	Asset* asset = prj->get(name.c_str());
	if (!asset)
		return write(L, false);

	Editable* editor = asset->editor();
	if (!editor)
		return write(L, false);

	editor->post(Editable::SET_BREAKPOINT, (Variant::Int)ln - 1, brk);

	const bool ret = impl->setBreakpoint(asset->entry().c_str(), ln, brk); // 1-based.

	return write(L, ret);
#else /* BITTY_DEBUG_ENABLED */
	(void)impl;

	Standard::message(L, "Debug module disabled.", Standard::WARN);

	return write(L, false);
#endif /* BITTY_DEBUG_ENABLED */
}

static int Debug_clearBreakpoints(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	const int n = getTop(L);
	std::string name = EXECUTABLE_ANY_NAME;
	if (n >= 1)
		read<>(L, name);

#if BITTY_DEBUG_ENABLED
	const Project* project = impl->project();
	if (!project)
		return write(L, false);

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return write(L, false);

	int ret = 0;
	if (name == EXECUTABLE_ANY_NAME) {
		prj->foreach(
			[&] (Asset* &asset, Asset::List::Index) -> void {
				if (asset->type() != Code::TYPE())
					return;

				Editable* editor = asset->editor();
				if (!editor)
					return;

				editor->post(Editable::CLEAR_BREAKPOINTS);

				ret = impl->clearBreakpoints(asset->entry().c_str());
			}
		);
	} else {
		Asset* asset = prj->get(name.c_str());
		if (!asset)
			return write(L, false);

		Editable* editor = asset->editor();
		if (!editor)
			return write(L, false);

		editor->post(Editable::CLEAR_BREAKPOINTS);

		ret = impl->clearBreakpoints(asset->entry().c_str());
	}

	return write(L, ret);
#else /* BITTY_DEBUG_ENABLED */
	(void)impl;

	Standard::message(L, "Debug module disabled.", Standard::WARN);

	return write(L, false);
#endif /* BITTY_DEBUG_ENABLED */
}

static int Debug_clearConsole(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

	impl->observer()->clear();

	return 0;
}

static int Debug_getTimeout(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

#if BITTY_DEBUG_ENABLED
	const double ret = DateTime::toSeconds(impl->timeout());

	return write(L, ret);
#else /* BITTY_DEBUG_ENABLED */
	(void)impl;

	Standard::message(L, "Debug module disabled.", Standard::WARN);

	return write(L, nullptr);
#endif /* BITTY_DEBUG_ENABLED */
}

static int Debug_setTimeout(lua_State* L) {
	ScriptingLua* impl = ScriptingLua::instanceOf(L);

#if BITTY_DEBUG_ENABLED
	const int n = getTop(L);
	double val = DateTime::toSeconds(SCRIPTING_LUA_TIMEOUT);
	if (n >= 1) {
		if (isNil(L))
			val = 0;
		else
			read<>(L, val);
	}

	const long long value = DateTime::fromSeconds(val);
	impl->timeout(value);

	return 0;
#else /* BITTY_DEBUG_ENABLED */
	(void)impl;

	Standard::message(L, "Debug module disabled.", Standard::WARN);

	return 0;
#endif /* BITTY_DEBUG_ENABLED */
}

static int Debug_trace(lua_State* L) {
	auto getThread = [] (lua_State* L, int* arg) -> lua_State* {
		if (isThread(L, 1)) {
			*arg = 1;

			lua_State* ret = nullptr;
			read(L, ret, Index(1));

			return ret;
		} else {
			*arg = 0;

			return L; // Function will operate over current thread.
		}
	};

	int arg = 0;
	lua_State* L1 = getThread(L, &arg);
	const char* msg = nullptr;
	read(L, msg, Index(arg + 1));
	if (msg == nullptr && !isNoneOrNil(L, arg + 1)) { // Non-string "msg"?
		write(L, Index(arg + 1)); // Return it untouched.
	} else {
		int level = 0;
		optional(L, level, Index(arg + 2), (L == L1) ? 1 : 0);
		traceback(L, L1, msg, level);
	}

	return 1;
}

static void open_Debug(lua_State* L) {
	req(
		L,
		array(
			luaL_Reg{
				"Debug",
				LUA_LIB(
					array(
						luaL_Reg{ "setBreakpoint", Debug_setBreakpoint },
						luaL_Reg{ "clearBreakpoints", Debug_clearBreakpoints },
						luaL_Reg{ "clearConsole", Debug_clearConsole },
						luaL_Reg{ "getTimeout", Debug_getTimeout },
						luaL_Reg{ "setTimeout", Debug_setTimeout },
						luaL_Reg{ "trace", Debug_trace },
						luaL_Reg{ nullptr, nullptr }
					)
				)
			},
			luaL_Reg{ nullptr, nullptr }
		)
	);
}

/**< Categories. */

void open(class Executable* exec) {
	// Prepare.
	lua_State* L = (lua_State*)exec->pointer();

	// Application.
	open_Application(L);

	// Canvas.
	open_Canvas(L);

	// Project.
	open_Project(L);

	// Debug.
	open_Debug(L);
}

}

}

/* ===========================================================================} */
