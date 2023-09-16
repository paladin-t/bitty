cmake_minimum_required(VERSION 2.8)
message(STATUS "Build Bitty Engine for Linux...")

# Projects.
project(bitty)

# Architectures.
set(BITTY_SPEC no)
if ("${arch}" STREQUAL "x64")
  set(BITTY_SPEC 64)
endif ()
if ("${arch}" STREQUAL "x86")
  set(BITTY_SPEC 86)
endif ()
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(STATUS "Host arch: x64")
  set(BITTY_IS_X64 TRUE)
  set(BITTY_ARCH 64)
  set(BITTY_BITS 64)
else ()
  message(STATUS "Host arch: x86")
  set(BITTY_IS_X64 FALSE)
  set(BITTY_ARCH 86)
  set(BITTY_BITS 32)
endif ()
if (${BITTY_SPEC} STREQUAL no)
  message(STATUS "Specified arch: no")
else ()
  message(STATUS "Specified arch: " ${BITTY_SPEC})
  if (${BITTY_SPEC} STREQUAL 64)
    if (NOT ${BITTY_IS_X64})
      set(BITTY_IS_X64 TRUE)
      set(BITTY_ARCH 64)
      set(BITTY_BITS 64)
      set(CMAKE_C_FLAGS "-m64")
      set(CMAKE_CXX_FLAGS "-m64")
    endif ()
  else ()
    if (${BITTY_IS_X64})
      set(BITTY_IS_X64 FALSE)
      set(BITTY_ARCH 86)
      set(BITTY_BITS 32)
      set(CMAKE_C_FLAGS "-m32")
      set(CMAKE_CXX_FLAGS "-m32")
    endif ()
  endif ()
endif ()
message(STATUS "Is x64: " ${BITTY_IS_X64})
set(BITTY_TEMP ./)
set(${arch} "")

# Profiles.
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif ()
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
if (${CMAKE_BUILD_TYPE} STREQUAL Release)
  list(
    APPEND BITTY_DEF
    NDEBUG
  )
else ()
  list(
    APPEND BITTY_DEF
    _DEBUG
  )
endif ()

# Requirements.
find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

# Sources.
list(
  APPEND BITTY_LIB_B64
  "../lib/b64/buffer.c"
  "../lib/b64/decode.c"
  "../lib/b64/encode.c"
)
list(
  APPEND BITTY_LIB_CHIPMUNK
  "../lib/chipmunk2d/src/chipmunk.c"
  "../lib/chipmunk2d/src/cpArbiter.c"
  "../lib/chipmunk2d/src/cpArray.c"
  "../lib/chipmunk2d/src/cpBBTree.c"
  "../lib/chipmunk2d/src/cpBody.c"
  "../lib/chipmunk2d/src/cpCollision.c"
  "../lib/chipmunk2d/src/cpConstraint.c"
  "../lib/chipmunk2d/src/cpDampedRotarySpring.c"
  "../lib/chipmunk2d/src/cpDampedSpring.c"
  "../lib/chipmunk2d/src/cpGearJoint.c"
  "../lib/chipmunk2d/src/cpGrooveJoint.c"
  "../lib/chipmunk2d/src/cpHashSet.c"
  "../lib/chipmunk2d/src/cpPinJoint.c"
  "../lib/chipmunk2d/src/cpPivotJoint.c"
  "../lib/chipmunk2d/src/cpPolyShape.c"
  "../lib/chipmunk2d/src/cpRatchetJoint.c"
  "../lib/chipmunk2d/src/cpRobust.c"
  "../lib/chipmunk2d/src/cpRotaryLimitJoint.c"
  "../lib/chipmunk2d/src/cpShape.c"
  "../lib/chipmunk2d/src/cpSimpleMotor.c"
  "../lib/chipmunk2d/src/cpSlideJoint.c"
  "../lib/chipmunk2d/src/cpSpace.c"
  "../lib/chipmunk2d/src/cpSpaceComponent.c"
  "../lib/chipmunk2d/src/cpSpaceDebug.c"
  "../lib/chipmunk2d/src/cpSpaceHash.c"
  "../lib/chipmunk2d/src/cpSpaceQuery.c"
  "../lib/chipmunk2d/src/cpSpaceStep.c"
  "../lib/chipmunk2d/src/cpSpatialIndex.c"
  "../lib/chipmunk2d/src/cpSweep1D.c"
)
list(
  APPEND BITTY_LIB_GL3W
  "../lib/gl3w/GL/gl3w.c"
)
list(
  APPEND BITTY_LIB_IMGUI
  "../lib/imgui/imgui.cpp"
  "../lib/imgui/imgui_demo.cpp"
  "../lib/imgui/imgui_draw.cpp"
  "../lib/imgui/imgui_tables.cpp"
  "../lib/imgui/imgui_widgets.cpp"
)
list(
  APPEND BITTY_LIB_IMGUI_CODE_EDITOR
  "../lib/imgui_code_editor/imgui_code_editor.cpp"
)
list(
  APPEND BITTY_LIB_IMGUI_SDL
  "../lib/imgui_sdl/imgui_sdl.cpp"
)
list(
  APPEND BITTY_LIB_JO_GIF
  "../lib/jo_gif/jo_gif.cpp"
)
list(
  APPEND BITTY_LIB_LUA
  "../lib/lua/src/lapi.c"
  "../lib/lua/src/lauxlib.c"
  "../lib/lua/src/lbaselib.c"
  "../lib/lua/src/lcode.c"
  "../lib/lua/src/lcorolib.c"
  "../lib/lua/src/lctype.c"
  "../lib/lua/src/ldblib.c"
  "../lib/lua/src/ldebug.c"
  "../lib/lua/src/ldo.c"
  "../lib/lua/src/ldump.c"
  "../lib/lua/src/lfunc.c"
  "../lib/lua/src/lgc.c"
  "../lib/lua/src/linit.c"
  "../lib/lua/src/liolib.c"
  "../lib/lua/src/llex.c"
  "../lib/lua/src/lmathlib.c"
  "../lib/lua/src/lmem.c"
  "../lib/lua/src/loadlib.c"
  "../lib/lua/src/lobject.c"
  "../lib/lua/src/lopcodes.c"
  "../lib/lua/src/loslib.c"
  "../lib/lua/src/lparser.c"
  "../lib/lua/src/lstate.c"
  "../lib/lua/src/lstring.c"
  "../lib/lua/src/lstrlib.c"
  "../lib/lua/src/ltable.c"
  "../lib/lua/src/ltablib.c"
  "../lib/lua/src/ltm.c"
  "../lib/lua/src/lundump.c"
  "../lib/lua/src/lutf8lib.c"
  "../lib/lua/src/lvm.c"
  "../lib/lua/src/lzio.c"
)
list(
  APPEND BITTY_LIB_LZ4
  "../lib/lz4/lib/lz4.c"
  "../lib/lz4/lib/lz4frame.c"
  "../lib/lz4/lib/lz4hc.c"
  "../lib/lz4/lib/xxhash.c"
)
list(
  APPEND BITTY_LIB_MD4C
  "../lib/md4c/src/md4c.c"
)
list(
  APPEND BITTY_LIB_MICROPATHER
  "../lib/micropather/micropather.cpp"
)
list(
  APPEND BITTY_LIB_MONGOOSE
  "../lib/mongoose/mongoose.c"
)
list(
  APPEND BITTY_LIB_NETWORK_INFO
  "../lib/network_info/cbits/network-unix.c"
)
list(
  APPEND BITTY_LIB_SDL_GFX
  "../lib/sdl_gfx/SDL2_framerate.c"
  "../lib/sdl_gfx/SDL2_gfxPrimitives.c"
  "../lib/sdl_gfx/SDL2_imageFilter.c"
  "../lib/sdl_gfx/SDL2_rotozoom.c"
)
list(
  APPEND BITTY_LIB_ZLIB
  "../lib/zlib/contrib/minizip/ioapi.c"
  "../lib/zlib/contrib/minizip/mztools.c"
  "../lib/zlib/contrib/minizip/unzip.c"
  "../lib/zlib/contrib/minizip/zip.c"
  "../lib/zlib/adler32.c"
  "../lib/zlib/compress.c"
  "../lib/zlib/crc32.c"
  "../lib/zlib/deflate.c"
  "../lib/zlib/gzclose.c"
  "../lib/zlib/gzlib.c"
  "../lib/zlib/gzread.c"
  "../lib/zlib/gzwrite.c"
  "../lib/zlib/infback.c"
  "../lib/zlib/inffast.c"
  "../lib/zlib/inflate.c"
  "../lib/zlib/inftrees.c"
  "../lib/zlib/trees.c"
  "../lib/zlib/uncompr.c"
  "../lib/zlib/zutil.c"
)
list(
  APPEND BITTY_SRC_APPLICATION
  "../src/application.cpp"
)
list(
  APPEND BITTY_SRC_EXECUTABLE
  "../src/luaxx.cpp"
  "../src/scripting_lua.cpp"
  "../src/scripting_lua_api.cpp"
  "../src/scripting_lua_api_promises.cpp"
  "../src/scripting_lua_dbg.cpp"
  "../src/plugin.cpp"
  "../src/scripting.cpp"
)
list(
  APPEND BITTY_SRC_GRAPHICS
  "../src/effects.cpp"
  "../src/primitives.cpp"
  "../src/renderer.cpp"
  "../src/texture.cpp"
  "../src/window.cpp"
)
list(
  APPEND BITTY_SRC_INPUT
  "../src/input.cpp"
)
list(
  APPEND BITTY_SRC_RESOURCES
  "../src/audio.cpp"
  "../src/code.cpp"
  "../src/font.cpp"
  "../src/image.cpp"
  "../src/map.cpp"
  "../src/palette.cpp"
  "../src/resources.cpp"
  "../src/sprite.cpp"
)
list(
  APPEND BITTY_SRC_SHARED
  "../src/noiser.cpp"
  "../src/pathfinder.cpp"
  "../src/raycaster.cpp"
  "../src/randomizer.cpp"
  "../src/walker.cpp"
  "../src/archive_txt.cpp"
  "../src/archive_zip.cpp"
  "../src/cloneable.cpp"
  "../src/collectible.cpp"
  "../src/dispatchable.cpp"
  "../src/executable.cpp"
  "../src/stream.cpp"
  "../src/updatable.cpp"
  "../src/network_mongoose.cpp"
  "../src/platform_linux.cpp"
  "../src/web_curl.cpp"
  "../src/web_html.cpp"
  "../src/web_mongoose.cpp"
  "../src/archive.cpp"
  "../src/bytes.cpp"
  "../src/color.cpp"
  "../src/datetime.cpp"
  "../src/either.cpp"
  "../src/encoding.cpp"
  "../src/file_handle.cpp"
  "../src/filesystem.cpp"
  "../src/hacks.cpp"
  "../src/json.cpp"
  "../src/mathematics.cpp"
  "../src/network.cpp"
  "../src/object.cpp"
  "../src/platform.cpp"
  "../src/plus.cpp"
  "../src/promise.cpp"
  "../src/text.cpp"
  "../src/web.cpp"
)
list(
  APPEND BITTY_SRC_WORKSPACE
  "../src/editing.cpp"
  "../src/editor.cpp"
  "../src/editor_code.cpp"
  "../src/editor_json.cpp"
  "../src/editor_polyfill.cpp"
  "../src/editor_text.cpp"
  "../src/resource/inline_resource.cpp"
  "../src/editable.cpp"
  "../src/document.cpp"
  "../src/entry.cpp"
  "../src/generic.cpp"
  "../src/recorder.cpp"
  "../src/asset.cpp"
  "../src/loader.cpp"
  "../src/operations.cpp"
  "../src/project.cpp"
  "../src/theme.cpp"
  "../src/widgets.cpp"
  "../src/workspace.cpp"
)
list(
  APPEND BITTY_SRC_SKETCHBOOK
  "../src/theme_sketchbook.cpp"
  "../src/widgets_sketchbook.cpp"
  "../src/workspace_sketchbook.cpp"
)
add_executable(
  bitty
  ${BITTY_LIB_B64}
  ${BITTY_LIB_CHIPMUNK}
  ${BITTY_LIB_GL3W}
  ${BITTY_LIB_IMGUI}
  ${BITTY_LIB_IMGUI_CODE_EDITOR}
  ${BITTY_LIB_IMGUI_SDL}
  ${BITTY_LIB_JO_GIF}
  ${BITTY_LIB_LUA}
  ${BITTY_LIB_LZ4}
  ${BITTY_LIB_MD4C}
  ${BITTY_LIB_MICROPATHER}
  ${BITTY_LIB_MONGOOSE}
  ${BITTY_LIB_NETWORK_INFO}
  ${BITTY_LIB_SDL_GFX}
  ${BITTY_LIB_ZLIB}
  ${BITTY_SRC_APPLICATION}
  ${BITTY_SRC_EXECUTABLE}
  ${BITTY_SRC_GRAPHICS}
  ${BITTY_SRC_INPUT}
  ${BITTY_SRC_RESOURCES}
  ${BITTY_SRC_SHARED}
  ${BITTY_SRC_WORKSPACE}
  ${BITTY_SRC_SKETCHBOOK}
  "../src/main.cpp"
)
list(
  APPEND BITTY_INC
  "../lib/chipmunk2d/include"
  "../lib/imgui"
  "../lib/sdl/include"
  "../lib/sdl_mixer/include"
  "../lib/zlib"
)
target_include_directories(bitty PRIVATE ${BITTY_INC})

# Compiling.
list(
  APPEND BITTY_DEF
  BITTY_TRIAL_ENABLED=0
)
target_compile_definitions(bitty PRIVATE ${BITTY_DEF})

# Flags.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNDEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-format-security -Wno-unused-result")
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")

# Linkage.
list(
  APPEND BITTY_LIB
  dl
  m
  pthread
  rt
  stdc++fs
)
target_link_libraries(bitty ${BITTY_LIB})

# OpenGL.
target_link_libraries(bitty ${OPENGL_LIBRARIES})

# GTK.
include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})
target_link_libraries(bitty ${GTK3_LIBRARIES})

# cURL.
include_directories("../lib/curl/include")
add_library(libcurl SHARED IMPORTED)
set_target_properties(libcurl PROPERTIES IMPORTED_LOCATION "./libcurl.so")
target_link_libraries(bitty libcurl)

# SDL.
include_directories("../lib/sdl/lib/x${BITTY_ARCH}")
add_library(libsdl SHARED IMPORTED)
set_target_properties(libsdl PROPERTIES IMPORTED_LOCATION "./libSDL2-2.0.so.0")
target_link_libraries(bitty libsdl)
add_library(libsdlmixer SHARED IMPORTED)
set_target_properties(libsdlmixer PROPERTIES IMPORTED_LOCATION "./libSDL2_mixer-2.0.so.0")
target_link_libraries(bitty libsdlmixer)

# Prev event.
add_custom_target(
  bitty_prev_sdl
  COMMAND cp "../lib/curl/x${BITTY_ARCH}/libcurl.a" ${BITTY_TEMP}
  COMMAND cp "../lib/curl/x${BITTY_ARCH}/libcurl.so" ${BITTY_TEMP}
  COMMAND cp "../lib/sdl/lib/x${BITTY_ARCH}/libSDL2.a" ${BITTY_TEMP}
  COMMAND cp "../lib/sdl/lib/x${BITTY_ARCH}/libSDL2-2.0.so.0" ${BITTY_TEMP}
  COMMAND cp "../lib/sdl_mixer/lib/x${BITTY_ARCH}/libSDL2_mixer-2.0.so.0" ${BITTY_TEMP}
)
add_dependencies(bitty bitty_prev_sdl)

# Post event.
add_custom_command(
  TARGET bitty POST_BUILD
  COMMAND file "${BITTY_TEMP}bitty"
  COMMAND file "${BITTY_TEMP}libcurl.so"
  COMMAND file "${BITTY_TEMP}libSDL2-2.0.so.0"
  COMMAND file "${BITTY_TEMP}libSDL2_mixer-2.0.so.0"
  COMMAND cp "${BITTY_TEMP}bitty" "../bitty/x${BITTY_ARCH}_release/"
  COMMAND cp "${BITTY_TEMP}libcurl.so" "../bitty/x${BITTY_ARCH}_release/"
  COMMAND cp "${BITTY_TEMP}libSDL2-2.0.so.0" "../bitty/x${BITTY_ARCH}_release/"
  COMMAND cp "${BITTY_TEMP}libSDL2_mixer-2.0.so.0" "../bitty/x${BITTY_ARCH}_release/"
)
