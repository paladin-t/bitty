cmake_minimum_required(VERSION 3.3)
message(STATUS "Build Bitty Engine for Linux...")

# Projects.
project(bitty)

# Architectures.
if (NOT DEFINED BITTY_SPEC)
  set(BITTY_SPEC ${CMAKE_SYSTEM_PROCESSOR})
endif ()
if (${BITTY_SPEC} MATCHES "x86_64|AMD64")
  message(STATUS "Target arch: x64")
  set(BITTY_ARCH "x64")
  set(BITTY_BITS 64)
elseif (${BITTY_SPEC} MATCHES "i[3456]86|X86")
  message(STATUS "Target arch: x86")
  set(BITTY_ARCH "x86")
  set(BITTY_BITS 32)
elseif (${BITTY_SPEC} MATCHES "^(aarch64|arm64|ARM64)$")
  message(STATUS "Target arch: arm64")
  set(BITTY_ARCH "arm64")
  set(BITTY_BITS 64)
elseif (${BITTY_SPEC} MATCHES "^(arm|ARM)")
  message(STATUS "Target arch: arm32")
  set(BITTY_ARCH "arm32")
  set(BITTY_BITS 32)
elseif (${BITTY_SPEC} MATCHES "mips64")
  message(STATUS "Target arch: mips64")
  set(BITTY_ARCH "mips64")
  set(BITTY_BITS 64)
elseif (${BITTY_SPEC} MATCHES "mips")
  message(STATUS "Target arch: mips32")
  set(BITTY_ARCH "mips32")
  set(BITTY_BITS 32)
elseif (${BITTY_SPEC} MATCHES "riscv64")
  message(STATUS "Target arch: riscv64")
  set(BITTY_ARCH "riscv64")
  set(BITTY_BITS 64)
elseif (${BITTY_SPEC} MATCHES "riscv32")
  message(STATUS "Target arch: riscv32")
  set(BITTY_ARCH "riscv32")
  set(BITTY_BITS 32)
elseif (${BITTY_SPEC} MATCHES "ppc64le|ppc64|powerpc64")
  message(STATUS "Target arch: ppc64")
  set(BITTY_ARCH "ppc64")
  set(BITTY_BITS 64)
else ()
  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "Target arch: unknown64")
    set(BITTY_ARCH "unknown64")
    set(BITTY_BITS 64)
  elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(STATUS "Target arch: unknown32")
    set(BITTY_ARCH "unknown32")
    set(BITTY_BITS 32)
  else ()
    message(FATAL_ERROR 
      "Target arch: unknown\n"
      "Unsupported architecture detected! "
      "CMAKE_SIZEOF_VOID_P = ${CMAKE_SIZEOF_VOID_P}. "
      "Only 32-bit and 64-bit architectures are supported."
    )
  endif ()
endif ()
if (DEFINED BITTY_ARCH_FLAG)
  set(CMAKE_C_FLAGS "${BITTY_ARCH_FLAG}")
  set(CMAKE_CXX_FLAGS "${BITTY_ARCH_FLAG}")
else ()
  if (${BITTY_ARCH} MATCHES "^x86|x64$")
    if (${BITTY_BITS} EQUAL 64)
      set(CMAKE_C_FLAGS "-m64")
      set(CMAKE_CXX_FLAGS "-m64")
    elseif (${BITTY_BITS} EQUAL 32)
      set(CMAKE_C_FLAGS "-m32")
      set(CMAKE_CXX_FLAGS "-m32")
    endif ()
  endif ()
endif ()
set(BITTY_TEMP ./)

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

# Libs.
if (NOT DEFINED BITTY_LIB_CURL_USE_LOCAL)
  set(BITTY_LIB_CURL_USE_LOCAL "TRUE")
endif ()
if (${BITTY_LIB_CURL_USE_LOCAL} STREQUAL TRUE)
  if (EXISTS "../lib/curl/${BITTY_ARCH}/libcurl.a" AND EXISTS "../lib/curl/${BITTY_ARCH}/libcurl.so")
    message(STATUS "Use local cURL")
  else ()
    message(STATUS "Use global cURL")
    set(BITTY_LIB_CURL_USE_LOCAL "FALSE")
  endif ()
else ()
  message(STATUS "Use global cURL")
endif ()

if (NOT DEFINED BITTY_LIB_LIBUV_USE_LOCAL)
  set(BITTY_LIB_LIBUV_USE_LOCAL "TRUE")
endif ()
if (${BITTY_LIB_LIBUV_USE_LOCAL} STREQUAL TRUE)
  if (EXISTS "../lib/libuv/lib/${BITTY_ARCH}/libuv.so.1")
    message(STATUS "Use local libuv")
  else ()
    message(STATUS "Use global libuv")
    set(BITTY_LIB_LIBUV_USE_LOCAL "FALSE")
  endif ()
else ()
  message(STATUS "Use global libuv")
endif ()

if (NOT DEFINED BITTY_LIB_SDL_USE_LOCAL)
  set(BITTY_LIB_SDL_USE_LOCAL "TRUE")
endif ()
if (${BITTY_LIB_SDL_USE_LOCAL} STREQUAL TRUE)
  if (EXISTS "../lib/sdl/lib/${BITTY_ARCH}/libSDL2.a" AND EXISTS "../lib/sdl/lib/${BITTY_ARCH}/libSDL2-2.0.so.0" AND EXISTS "../lib/sdl_mixer/lib/${BITTY_ARCH}/libSDL2_mixer-2.0.so.0")
    message(STATUS "Use local SDL")
  else ()
    message(STATUS "Use global SDL")
    set(BITTY_LIB_SDL_USE_LOCAL "FALSE")
  endif ()
else ()
  message(STATUS "Use global SDL")
endif ()

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
  APPEND BITTY_LIB_CIVETWEB
  "../lib/civetweb/src/civetweb.c"
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
  APPEND BITTY_LIB_KISSFFT
  "../lib/kissfft/kfc.c"
  "../lib/kissfft/kiss_fft.c"
  "../lib/kissfft/kiss_fftnd.c"
  "../lib/kissfft/kiss_fftndr.c"
  "../lib/kissfft/kiss_fftr.c"
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
  "../src/scripting_lua_api_physics.cpp"
  "../src/scripting_lua_api_plugin.cpp"
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
  "../src/network_libuv.cpp"
  "../src/platform_linux.cpp"
  "../src/web_civetweb.cpp"
  "../src/web_curl.cpp"
  "../src/web_html.cpp"
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
  "../src/text_box.cpp"
  "../src/web.cpp"
)
list(
  APPEND BITTY_SRC_WORKSPACE
  "../src/command.cpp"
  "../src/commands_image.cpp"
  "../src/commands_map.cpp"
  "../src/commands_paintable.cpp"
  "../src/commands_palette.cpp"
  "../src/commands_sprite.cpp"
  "../src/editing.cpp"
  "../src/editor.cpp"
  "../src/editor_bytes.cpp"
  "../src/editor_code.cpp"
  "../src/editor_font.cpp"
  "../src/editor_image.cpp"
  "../src/editor_json.cpp"
  "../src/editor_map.cpp"
  "../src/editor_palette.cpp"
  "../src/editor_plugin.cpp"
  "../src/editor_sound.cpp"
  "../src/editor_sprite.cpp"
  "../src/editor_text.cpp"
  "../src/resource/inline_resource.cpp"
  "../src/editable.cpp"
  "../src/analyser.cpp"
  "../src/document.cpp"
  "../src/entry.cpp"
  "../src/generic.cpp"
  "../src/recorder.cpp"
  "../src/shapes.cpp"
  "../src/asset.cpp"
  "../src/loader.cpp"
  "../src/operations.cpp"
  "../src/project.cpp"
  "../src/theme.cpp"
  "../src/widgets.cpp"
  "../src/workspace.cpp"
)
list(
  APPEND BITTY_SRC_STUDIO
  "../studio/loader_studio.cpp"
  "../studio/scripting_lua_api_studio.cpp"
  "../studio/theme_studio.cpp"
  "../studio/widgets_studio.cpp"
  "../studio/workspace_studio.cpp"
)
add_executable(
  bitty
  ${BITTY_LIB_B64}
  ${BITTY_LIB_CHIPMUNK}
  ${BITTY_LIB_CIVETWEB}
  ${BITTY_LIB_GL3W}
  ${BITTY_LIB_IMGUI}
  ${BITTY_LIB_IMGUI_CODE_EDITOR}
  ${BITTY_LIB_IMGUI_SDL}
  ${BITTY_LIB_JO_GIF}
  ${BITTY_LIB_KISSFFT}
  ${BITTY_LIB_LUA}
  ${BITTY_LIB_LZ4}
  ${BITTY_LIB_MD4C}
  ${BITTY_LIB_MICROPATHER}
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
  ${BITTY_SRC_STUDIO}
  "../studio/main.cpp"
)
list(
  APPEND BITTY_INC
  "../lib/chipmunk2d/include"
  "../lib/civetweb/include"
  "../lib/civetweb/src"
  "../lib/imgui"
  "../lib/kissfft"
  "../lib/zlib"
)
target_include_directories(bitty PRIVATE ${BITTY_INC})

# Compiling.
execute_process(
  COMMAND grep -o "Raspberry Pi" /proc/cpuinfo
  OUTPUT_VARIABLE RPI_DETECT
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
if (RPI_DETECT STREQUAL "Raspberry Pi")
  message(STATUS "Raspberry Pi detected")
  list(
    APPEND BITTY_DEF
    __RASPBERRYPI__
  )
endif ()
list(
  APPEND BITTY_DEF
  BITTY_TRIAL_ENABLED=0
  NO_SSL
)
target_compile_definitions(bitty PRIVATE ${BITTY_DEF})

# Flags.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNDEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-format-security -Wno-unused-result -Wno-stringop-overflow")
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
if (${BITTY_LIB_CURL_USE_LOCAL} STREQUAL TRUE)
  include_directories("../lib/curl/include")
  add_library(libcurl SHARED IMPORTED)
  set_target_properties(libcurl PROPERTIES IMPORTED_LOCATION "./libcurl.so")
  target_link_libraries(bitty libcurl)
else ()
  find_package(CURL REQUIRED)
  if (TARGET CURL::libcurl)
    target_link_libraries(bitty CURL::libcurl)
  else ()
    include_directories(${CURL_INCLUDE_DIRS})
    target_link_libraries(bitty ${CURL_LIBRARIES})
  endif ()
endif ()

# libuv.
if (${BITTY_LIB_LIBUV_USE_LOCAL} STREQUAL TRUE)
  include_directories("../lib/libuv/include")
  add_library(libuv SHARED IMPORTED)
  set_target_properties(libuv PROPERTIES IMPORTED_LOCATION "./libuv.so.1")
  target_link_libraries(bitty libuv)
else ()
  pkg_check_modules(LIBUV REQUIRED IMPORTED_TARGET libuv)
  if (TARGET PkgConfig::LIBUV)
    target_link_libraries(bitty PkgConfig::LIBUV)
  else ()
    include_directories(${LIBUV_INCLUDE_DIRS})
    target_link_libraries(bitty ${LIBUV_LIBRARIES})
  endif ()
endif ()

# SDL.
if (${BITTY_LIB_SDL_USE_LOCAL} STREQUAL TRUE)
  include_directories("../lib/sdl/include")
  include_directories("../lib/sdl_mixer/include")
  add_library(libsdl SHARED IMPORTED)
  set_target_properties(libsdl PROPERTIES IMPORTED_LOCATION "./libSDL2-2.0.so.0")
  target_link_libraries(bitty libsdl)
  add_library(libsdlmixer SHARED IMPORTED)
  set_target_properties(libsdlmixer PROPERTIES IMPORTED_LOCATION "./libSDL2_mixer-2.0.so.0")
  target_link_libraries(bitty libsdlmixer)
else ()
  find_package(SDL2 REQUIRED)
  find_package(SDL2_mixer)
  find_library(SNDIO_LIBRARY NAMES sndio)
  if (NOT SNDIO_LIBRARY)
    message(WARNING "sndio library not found, audio may not work properly")
  endif ()
  if (TARGET SDL2::SDL2)
    target_link_libraries(bitty SDL2::SDL2)
  else ()
    include_directories(${SDL2_INCLUDE_DIRS})
    target_link_libraries(bitty ${SDL2_LIBRARIES})
  endif ()
  if (TARGET SDL2_mixer::SDL2_mixer)
    target_link_libraries(bitty SDL2_mixer::SDL2_mixer)
  else ()
    if (NOT SDL2_MIXER_INCLUDE_DIRS)
      set(SDL2_MIXER_INCLUDE_DIRS "/usr/include/SDL2")
    endif ()
    if (NOT SDL2_MIXER_LIBRARIES)
      find_library(SDL2_MIXER_PATH
        NAMES SDL2_mixer
        PATHS
          /usr/lib/x86_64-linux-gnu
          /usr/lib/i386-linux-gnu
          /usr/lib/aarch64-linux-gnu
          /usr/lib/arm-linux-gnueabihf
          /usr/lib/arm-linux-gnueabi
          /usr/lib64
          /usr/lib
      )
      set(SDL2_MIXER_LIBRARIES ${SDL2_MIXER_PATH})
    endif ()
    include_directories(${SDL2_MIXER_INCLUDE_DIRS})
    target_link_libraries(bitty ${SDL2_MIXER_LIBRARIES})
  endif ()
  if (SNDIO_LIBRARY)
    target_link_libraries(bitty ${SNDIO_LIBRARY})
  endif ()
endif ()

# Prev event.
if (${BITTY_LIB_CURL_USE_LOCAL} STREQUAL TRUE)
  list(
    APPEND BITTY_PREV_LIB
    COMMAND cp "../lib/curl/${BITTY_ARCH}/libcurl.a" ${BITTY_TEMP}
    COMMAND cp "../lib/curl/${BITTY_ARCH}/libcurl.so" ${BITTY_TEMP}
  )
endif ()
if (${BITTY_LIB_LIBUV_USE_LOCAL} STREQUAL TRUE)
  list(
    APPEND BITTY_PREV_LIB
    COMMAND cp "../lib/libuv/lib/${BITTY_ARCH}/libuv.so.1" ${BITTY_TEMP}
  )
endif ()
if (${BITTY_LIB_SDL_USE_LOCAL} STREQUAL TRUE)
  list(
    APPEND BITTY_PREV_LIB
    COMMAND cp "../lib/sdl/lib/${BITTY_ARCH}/libSDL2.a" ${BITTY_TEMP}
    COMMAND cp "../lib/sdl/lib/${BITTY_ARCH}/libSDL2-2.0.so.0" ${BITTY_TEMP}
    COMMAND cp "../lib/sdl/lib/${BITTY_ARCH}/libsndio.so.6.1" ${BITTY_TEMP}
    COMMAND cp "../lib/sdl_mixer/lib/${BITTY_ARCH}/libSDL2_mixer-2.0.so.0" ${BITTY_TEMP}
  )
endif ()
if (${BITTY_LIB_CURL_USE_LOCAL} STREQUAL TRUE OR ${BITTY_LIB_LIBUV_USE_LOCAL} STREQUAL TRUE OR ${BITTY_LIB_SDL_USE_LOCAL} STREQUAL TRUE)
  add_custom_target(
    bitty_prev_lib
    ${BITTY_PREV_LIB}
  )
  add_dependencies(bitty bitty_prev_lib)
endif ()

# Post event.
add_custom_command(
  TARGET bitty POST_BUILD
  COMMAND mkdir -p "../bitty/${BITTY_ARCH}_release/"
  COMMAND file "${BITTY_TEMP}bitty"
  COMMAND cp "${BITTY_TEMP}bitty" "../bitty/${BITTY_ARCH}_release/"
)
if (${BITTY_LIB_CURL_USE_LOCAL} STREQUAL TRUE)
  add_custom_command(
    TARGET bitty POST_BUILD
    COMMAND file "${BITTY_TEMP}libcurl.so"
    COMMAND cp "${BITTY_TEMP}libcurl.so" "../bitty/${BITTY_ARCH}_release/"
  )
endif ()
if (${BITTY_LIB_LIBUV_USE_LOCAL} STREQUAL TRUE)
  add_custom_command(
    TARGET bitty POST_BUILD
    COMMAND file "${BITTY_TEMP}libuv.so.1"
    COMMAND cp "${BITTY_TEMP}libuv.so.1" "../bitty/${BITTY_ARCH}_release/"
  )
endif ()
if (${BITTY_LIB_SDL_USE_LOCAL} STREQUAL TRUE)
  add_custom_command(
    TARGET bitty POST_BUILD
    COMMAND file "${BITTY_TEMP}libSDL2-2.0.so.0"
    COMMAND file "${BITTY_TEMP}libSDL2_mixer-2.0.so.0"
    COMMAND cp "${BITTY_TEMP}libSDL2-2.0.so.0" "../bitty/${BITTY_ARCH}_release/"
    COMMAND cp "${BITTY_TEMP}libSDL2_mixer-2.0.so.0" "../bitty/${BITTY_ARCH}_release/"
    COMMAND cp "${BITTY_TEMP}libsndio.so.6.1" "../bitty/${BITTY_ARCH}_release/"
  )
endif ()
