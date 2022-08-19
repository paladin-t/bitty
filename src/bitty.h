/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __BITTY_H__
#define __BITTY_H__

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
** {===========================================================================
** Compiler
*/

#ifndef BITTY_CP
#	if defined __EMSCRIPTEN__
#		define BITTY_CP "Emscripten"
#		define BITTY_CP_EMSCRIPTEN
#	elif defined _MSC_VER
#		define BITTY_CP "VC++"
#		define BITTY_CP_VC _MSC_VER
#	elif defined __CYGWIN__
#		define BITTY_CP "Cygwin"
#		define BITTY_CP_CYGWIN
#	elif defined __MINGW32__
#		define BITTY_CP "MinGW"
#		define BITTY_CP_MINGW32
#	elif defined __clang__
#		define BITTY_CP "Clang"
#		define BITTY_CP_CLANG
#	elif defined __GNUC__ || defined __GNUG__
#		define BITTY_CP "GCC"
#		define BITTY_CP_GCC
#	else
#		define BITTY_CP "Unknown"
#		define BITTY_CP_UNKNOWN
#	endif /* Compiler dependent macros. */
#endif /* BITTY_CP */

/* ===========================================================================} */

/*
** {===========================================================================
** OS
*/

#ifndef BITTY_OS
#	if defined __EMSCRIPTEN__
#		if defined __asmjs__
#			define BITTY_OS "HTML [AsmJS]"
#		else
#			define BITTY_OS "HTML [Wasm]"
#		endif
#		define BITTY_OS_HTML
#	elif defined _WIN64
#		if defined _M_ARM
#			define BITTY_OS "Windows [ARM64]"
#		else
#			define BITTY_OS "Windows [x86_64]"
#		endif
#		define BITTY_OS_WIN
#		define BITTY_OS_WIN64
#	elif defined _WIN32
#		if defined _M_ARM
#			define BITTY_OS "Windows [ARM32]"
#		else
#			define BITTY_OS "Windows [x86]"
#		endif
#		define BITTY_OS_WIN
#		define BITTY_OS_WIN32
#	elif defined __APPLE__
#		include <TargetConditionals.h>
#		define BITTY_OS_APPLE
#		if defined TARGET_OS_IPHONE && TARGET_OS_IPHONE == 1
#			define BITTY_OS "iOS"
#			define BITTY_OS_IOS
#		elif defined TARGET_IPHONE_SIMULATOR && TARGET_IPHONE_SIMULATOR == 1
#			define BITTY_OS "iOS [sim]"
#			define BITTY_OS_IOS_SIM
#		elif defined TARGET_OS_MAC && TARGET_OS_MAC == 1
#			if defined __x86_64__
#				define BITTY_OS "MacOS [x86_64]"
#			elif defined __i386__
#				define BITTY_OS "MacOS [x86]"
#			elif defined __aarch64__
#				define BITTY_OS "MacOS [ARM64]"
#			elif defined __arm__
#				define BITTY_OS "MacOS [ARM32]"
#			else
#				define BITTY_OS "MacOS"
#			endif
#			define BITTY_OS_MAC
#		endif
#	elif defined __ANDROID__
#		define BITTY_OS "Android"
#		define BITTY_OS_ANDROID
#	elif defined __linux__
#		if defined __x86_64__
#			define BITTY_OS "Linux [x86_64]"
#		elif defined __i386__
#			define BITTY_OS "Linux [x86]"
#		elif defined __aarch64__
#			define BITTY_OS "Linux [ARM64]"
#		elif defined __arm__
#			define BITTY_OS "Linux [ARM32]"
#		else
#			define BITTY_OS "Linux"
#		endif
#		define BITTY_OS_LINUX
#	elif defined __unix__
#		define BITTY_OS "Unix"
#		define BITTY_OS_UNIX
#	else
#		define BITTY_OS "Unknown"
#		define BITTY_OS_UNKNOWN
#	endif /* OS dependent macros. */
#endif /* BITTY_OS */

/* ===========================================================================} */

/*
** {===========================================================================
** Version
*/

#ifndef BITTY_NAME
#	define BITTY_NAME "Bitty"
#endif /* BITTY_NAME */
#ifndef BITTY_TITLE
#	define BITTY_TITLE "Bitty Engine"
#endif /* BITTY_TITLE */

#ifndef BITTY_VERSION
#	define BITTY_VER_MAJOR 1
#	define BITTY_VER_MINOR 1
#	define BITTY_VER_REVISION 8
#	define BITTY_VER_SUFFIX
#	define BITTY_VERSION ((BITTY_VER_MAJOR * 0x01000000) + (BITTY_VER_MINOR * 0x00010000) + (BITTY_VER_REVISION))
#	define BITTY_MAKE_STRINGIZE(A) #A
#	define BITTY_STRINGIZE(A) BITTY_MAKE_STRINGIZE(A)
#	if BITTY_VER_REVISION == 0
#		define BITTY_VERSION_STRING BITTY_STRINGIZE(BITTY_VER_MAJOR.BITTY_VER_MINOR BITTY_VER_SUFFIX)
#	else /* BITTY_VER_REVISION == 0 */
#		define BITTY_VERSION_STRING BITTY_STRINGIZE(BITTY_VER_MAJOR.BITTY_VER_MINOR.BITTY_VER_REVISION BITTY_VER_SUFFIX)
#	endif /* BITTY_VER_REVISION == 0 */
#endif /* BITTY_VERSION */

/* ===========================================================================} */

/*
** {===========================================================================
** Features
*/

// Indicates whether to build trial version.
#ifndef BITTY_TRIAL_ENABLED
#	define BITTY_TRIAL_ENABLED 1
#endif /* BITTY_TRIAL_ENABLED */

// Indicates whether project code executes on a thread separately from graphics.
#ifndef BITTY_MULTITHREAD_ENABLED
#	if defined BITTY_OS_HTML
#		define BITTY_MULTITHREAD_ENABLED 0
#	else /* BITTY_OS_HTML */
#		define BITTY_MULTITHREAD_ENABLED 1
#	endif /* BITTY_OS_HTML */
#endif /* BITTY_MULTITHREAD_ENABLED */

// Indicates whether project code debug is enabled.
// Requires `BITTY_MULTITHREAD_ENABLED==1`.
#ifndef BITTY_DEBUG_ENABLED
#	if BITTY_MULTITHREAD_ENABLED
#		define BITTY_DEBUG_ENABLED 1
#	else /* BITTY_MULTITHREAD_ENABLED */
#		define BITTY_DEBUG_ENABLED 0
#	endif /* BITTY_MULTITHREAD_ENABLED */
#endif /* BITTY_DEBUG_ENABLED */

// Indicates whether the `Network` API is enabled.
#ifndef BITTY_NETWORK_ENABLED
#	define BITTY_NETWORK_ENABLED 1
#endif /* BITTY_NETWORK_ENABLED */

// Indicates whether the `Web` API is enabled.
#ifndef BITTY_WEB_ENABLED
#	define BITTY_WEB_ENABLED 1
#endif /* BITTY_WEB_ENABLED */

// Indicates whether the splash is enabled.
#ifndef BITTY_SPLASH_ENABLED
#	define BITTY_SPLASH_ENABLED 1
#endif /* BITTY_SPLASH_ENABLED */

// Indicates whether full screen effects is enabled.
#ifndef BITTY_EFFECTS_ENABLED
#	define BITTY_EFFECTS_ENABLED 0
#endif /* BITTY_EFFECTS_ENABLED */

// Indicates whether map batch is preferred.
#ifndef BITTY_PROJECT_STRATEGY_MAP_BATCH_ENABLED
#	define BITTY_PROJECT_STRATEGY_MAP_BATCH_ENABLED 0
#endif /* BITTY_PROJECT_STRATEGY_MAP_BATCH_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Configurations
*/

#ifndef BITTY_ACTIVE_FRAME_RATE
#	define BITTY_ACTIVE_FRAME_RATE 60
#endif /* BITTY_ACTIVE_FRAME_RATE */

#ifndef BITTY_CANVAS_DEFAULT_WIDTH
#	define BITTY_CANVAS_DEFAULT_WIDTH 480
#endif /* BITTY_CANVAS_DEFAULT_WIDTH */
#ifndef BITTY_CANVAS_DEFAULT_HEIGHT
#	define BITTY_CANVAS_DEFAULT_HEIGHT 320
#endif /* BITTY_CANVAS_DEFAULT_HEIGHT */
#ifndef BITTY_CANVAS_MAX_WIDTH
#	define BITTY_CANVAS_MAX_WIDTH (480 * 10)
#endif /* BITTY_CANVAS_MAX_WIDTH */
#ifndef BITTY_CANVAS_MAX_HEIGHT
#	define BITTY_CANVAS_MAX_HEIGHT (320 * 10)
#endif /* BITTY_CANVAS_MAX_HEIGHT */

#ifndef BITTY_DEBUG_TABLE_LEVEL_MAX_COUNT
#	define BITTY_DEBUG_TABLE_LEVEL_MAX_COUNT 10
#endif /* BITTY_DEBUG_TABLE_LEVEL_MAX_COUNT */
#ifndef BITTY_DEBUG_TABLE_ITEM_MAX_COUNT
#	define BITTY_DEBUG_TABLE_ITEM_MAX_COUNT 199
#endif /* BITTY_DEBUG_TABLE_ITEM_MAX_COUNT */

#ifndef BITTY_GRID_DEFAULT_SIZE
#	define BITTY_GRID_DEFAULT_SIZE 8
#endif /* BITTY_GRID_DEFAULT_SIZE */

#ifndef BITTY_IMAGE_DEFAULT_WIDTH
#	define BITTY_IMAGE_DEFAULT_WIDTH 256
#endif /* BITTY_IMAGE_DEFAULT_WIDTH */
#ifndef BITTY_IMAGE_DEFAULT_HEIGHT
#	define BITTY_IMAGE_DEFAULT_HEIGHT 256
#endif /* BITTY_IMAGE_DEFAULT_HEIGHT */
#ifndef BITTY_IMAGE_MAX_WIDTH
#	define BITTY_IMAGE_MAX_WIDTH 1024
#endif /* BITTY_IMAGE_MAX_WIDTH */
#ifndef BITTY_IMAGE_MAX_HEIGHT
#	define BITTY_IMAGE_MAX_HEIGHT 1024
#endif /* BITTY_IMAGE_MAX_HEIGHT */

#ifndef BITTY_SPRITE_DEFAULT_WIDTH
#	define BITTY_SPRITE_DEFAULT_WIDTH BITTY_GRID_DEFAULT_SIZE
#endif /* BITTY_SPRITE_DEFAULT_WIDTH */
#ifndef BITTY_SPRITE_DEFAULT_HEIGHT
#	define BITTY_SPRITE_DEFAULT_HEIGHT BITTY_GRID_DEFAULT_SIZE
#endif /* BITTY_SPRITE_DEFAULT_HEIGHT */
#ifndef BITTY_SPRITE_MAX_WIDTH
#	define BITTY_SPRITE_MAX_WIDTH 1024
#endif /* BITTY_SPRITE_MAX_WIDTH */
#ifndef BITTY_SPRITE_MAX_HEIGHT
#	define BITTY_SPRITE_MAX_HEIGHT 1024
#endif /* BITTY_SPRITE_MAX_HEIGHT */
#ifndef BITTY_SPRITE_FRAME_MAX_COUNT
#	define BITTY_SPRITE_FRAME_MAX_COUNT 1024
#endif /* BITTY_SPRITE_FRAME_MAX_COUNT */

#ifndef BITTY_MAP_TILE_DEFAULT_SIZE
#	define BITTY_MAP_TILE_DEFAULT_SIZE BITTY_GRID_DEFAULT_SIZE
#endif /* BITTY_MAP_TILE_DEFAULT_SIZE */
#ifndef BITTY_MAP_DEFAULT_WIDTH
#	define BITTY_MAP_DEFAULT_WIDTH 256
#endif /* BITTY_MAP_DEFAULT_WIDTH */
#ifndef BITTY_MAP_DEFAULT_HEIGHT
#	define BITTY_MAP_DEFAULT_HEIGHT 256
#endif /* BITTY_MAP_DEFAULT_HEIGHT */
#ifndef BITTY_MAP_MAX_WIDTH
#	define BITTY_MAP_MAX_WIDTH 4096
#endif /* BITTY_MAP_MAX_WIDTH */
#ifndef BITTY_MAP_MAX_HEIGHT
#	define BITTY_MAP_MAX_HEIGHT 4096
#endif /* BITTY_MAP_MAX_HEIGHT */

#ifndef BITTY_TEXTURE_SAFE_MAX_WIDTH
#	define BITTY_TEXTURE_SAFE_MAX_WIDTH 32768
#endif /* BITTY_TEXTURE_SAFE_MAX_WIDTH */
#ifndef BITTY_TEXTURE_SAFE_MAX_HEIGHT
#	define BITTY_TEXTURE_SAFE_MAX_HEIGHT 32768
#endif /* BITTY_TEXTURE_SAFE_MAX_HEIGHT */

#ifndef BITTY_PROJECT_EXT
#	define BITTY_PROJECT_EXT "bit"
#endif /* BITTY_PROJECT_EXT */
#ifndef BITTY_PALETTE_EXT
#	define BITTY_PALETTE_EXT "pal"
#endif /* BITTY_PALETTE_EXT */
#ifndef BITTY_IMAGE_EXT
#	define BITTY_IMAGE_EXT "img"
#endif /* BITTY_IMAGE_EXT */
#ifndef BITTY_SPRITE_EXT
#	define BITTY_SPRITE_EXT "spr"
#endif /* BITTY_SPRITE_EXT */
#ifndef BITTY_MAP_EXT
#	define BITTY_MAP_EXT "map"
#endif /* BITTY_MAP_EXT */
#ifndef BITTY_FONT_EXT
#	define BITTY_FONT_EXT "ttf"
#endif /* BITTY_FONT_EXT */
#ifndef BITTY_LUA_EXT
#	define BITTY_LUA_EXT "lua"
#endif /* BITTY_LUA_EXT */
#ifndef BITTY_JSON_EXT
#	define BITTY_JSON_EXT "json"
#endif /* BITTY_JSON_EXT */
#ifndef BITTY_TEXT_EXT
#	define BITTY_TEXT_EXT "txt"
#endif /* BITTY_TEXT_EXT */
#ifndef BITTY_ZIP_EXT
#	define BITTY_ZIP_EXT "zip"
#endif /* BITTY_ZIP_EXT */

/* ===========================================================================} */

/*
** {===========================================================================
** Constants
*/

// Defined with debug build.
#ifndef BITTY_DEBUG
#	if defined DEBUG || defined _DEBUG
#		define BITTY_DEBUG
#	endif /* DEBUG || _DEBUG */
#endif /* BITTY_DEBUG */

#ifndef BITTY_MAX_PATH
#	if defined __EMSCRIPTEN__
#		define BITTY_MAX_PATH PATH_MAX
#	elif defined _WIN64 || defined _WIN32
#		define BITTY_MAX_PATH _MAX_PATH
#	elif defined __ANDROID__
#		include <linux/limits.h>
#		define BITTY_MAX_PATH PATH_MAX
#	elif defined __APPLE__
#		define BITTY_MAX_PATH PATH_MAX
#	elif defined __linux__
#		include <linux/limits.h>
#		define BITTY_MAX_PATH PATH_MAX
#	endif /* Platform macro. */
#endif /* BITTY_MAX_PATH */

/* ===========================================================================} */

/*
** {===========================================================================
** Functions
*/

#ifndef BITTY_COUNTOF
#	define BITTY_COUNTOF(A) (sizeof(A) / sizeof(*(A)))
#endif /* BITTY_COUNTOF */

#ifndef BITTY_STRICMP
#	if defined BITTY_CP_VC
#		define BITTY_STRICMP _strcmpi
#	else /* BITTY_CP_VC */
#		define BITTY_STRICMP strcasecmp
#	endif /* BITTY_CP_VC */
#endif /* BITTY_STRICMP */

/* ===========================================================================} */

/*
** {===========================================================================
** Properties and attributes
*/

#ifndef BITTY_FIELD
#	define BITTY_FIELD(Y, V) \
	protected: \
		Y _##V; \
	public: \
		const Y &V(void) const { \
			return _##V; \
		} \
		Y &V(void) { \
			return _##V; \
		}
#endif /* BITTY_FIELD */

#ifndef BITTY_PROPERTY_READONLY
#	define BITTY_PROPERTY_READONLY(Y, V) \
	protected: \
		Y _##V; \
	public: \
		const Y &V(void) const { \
			return _##V; \
		} \
	protected: \
		void V(const Y &v) { \
			_##V = v; \
		}
#endif /* BITTY_PROPERTY_READONLY */
#ifndef BITTY_PROPERTY
#	define BITTY_PROPERTY(Y, V) \
	protected: \
		Y _##V; \
	public: \
		const Y &V(void) const { \
			return _##V; \
		} \
		Y &V(void) { \
			return _##V; \
		} \
		void V(const Y &v) { \
			_##V = v; \
		}
#endif /* BITTY_PROPERTY */

#ifndef BITTY_PROPERTY_READONLY_PTR
#	define BITTY_PROPERTY_READONLY_PTR(Y, V) \
	protected: \
		Y* _##V = nullptr; \
	public: \
		Y* V(void) const { \
			return _##V; \
		} \
	protected: \
		void V(Y* v) { \
			_##V = v; \
		} \
		void V(std::nullptr_t) { \
			_##V = nullptr; \
		}
#endif /* BITTY_PROPERTY_READONLY_PTR */
#ifndef BITTY_PROPERTY_PTR
#	define BITTY_PROPERTY_PTR(Y, V) \
	protected: \
		Y* _##V = nullptr; \
	public: \
		const Y* V(void) const { \
			return _##V; \
		} \
		Y* &V(void) { \
			return _##V; \
		} \
		void V(Y* v) { \
			_##V = v; \
		} \
		void V(std::nullptr_t) { \
			_##V = nullptr; \
		}
#endif /* BITTY_PROPERTY_PTR */

#ifndef BITTY_MISSING
#	define BITTY_MISSING do { fprintf(stderr, "Missing function \"%s\".\n", __FUNCTION__); } while (false);
#endif /* BITTY_MISSING */

#ifndef BITTY_MAKE_UINT32
#	define BITTY_MAKE_UINT32(A, B, C, D) (((A) << 24) | ((B) << 16) | ((C) << 8) | (D))
#endif /* BITTY_MAKE_UINT32 */

#ifndef BITTY_CLASS_TYPE
#	define BITTY_CLASS_TYPE(A, B, C, D) static constexpr unsigned TYPE(void) { return BITTY_MAKE_UINT32(A, B, C, D); }
#endif /* BITTY_CLASS_TYPE */

/* ===========================================================================} */

#endif /* __BITTY_H__ */
