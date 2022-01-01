![](docs/imgs/logo.png)

[Home](https://paladin-t.github.io/bitty/index.html) | [Steam](https://store.steampowered.com/app/1386180/) | [Itch](https://tonywang.itch.io/bitty) | [Documents](https://paladin-t.github.io/bitty/documents.html) | [About](https://paladin-t.github.io/bitty/about.html)

![](docs/imgs/floppy.gif) **An itty bitty game engine.**

* [About Bitty Engine](#about-bitty-engine)
	* [Why Bitty Engine?](#why-bitty-engine)
	* [Features](#features)
	* [Technical specifications](#technical-specifications)
* [Glance](#glance)
	* [Code](#code)
	* [Games](#games)
* [Redistributing](#redistributing)
* [Getting Bitty Engine](#getting-bitty-engine)
* [Compiling from source](#compiling-from-source)
	* [Startup project](#startup-project)
	* [Windows](#windows)
	* [MacOS](#macos)
	* [Linux](#linux)
	* [Other platforms](#other-platforms)
* [Community and contributing](#community-and-contributing)
* [Documentation](#documentation)

## About Bitty Engine

![](docs/imgs/banner_coding.png)

Bitty Engine is a cross-platform itty bitty **Game Engine** and open-source **Framework**. The full featured engine is programmable in Lua and integrated with built-in editors. It keeps the nature of both engine's productivity, and fantasy computer/console's ease to iterate. It boosts your commercial/non-commercial projects, prototypes, game jams, or just thought experiments.

### Why Bitty Engine?

Bitty Engine has everything built-in for coding, graphics composing, etc; it has a full featured debugger for breakpoint, variable inspecting, stepping, and call-stack traversing; it offers a set of well-designed API with full documentation; it builds fast binaries with code and asset obfuscating, moreover its package size is small (around 10MB with empty project, other engine outputs more than 10 times bigger).

It is supposed to be your ultimate 2D game creating software.

### Features

Bitty Engine offers a set of orthogonal features that makes game development comfortable and enjoyable.

* `Resources` API
	* `Resources.load(...)`, `Resources.unload(...)`
	* `Resources.wait(...)`, `Resources.collect(...)`
	* Etc.
* Graphics API
	* Shapes: `plot(...)`, `line(...)`, `circ(...)`, `ellipse(...)`, `rect(...)`, `tri(...)`
	* Text: `text(...)`, `font(...)`, `measure(...)`
	* Texture: `tex(...)`
	* Sprite: `spr(...)`
	* Map: `map(...)`
	* `clip(...)`, `camera(...)`
	* Etc.
* Input API
	* Gamepad
	* Keyboard
	* Mouse and touch
* Audio API
	* Supports MP3, OGG, WAV, FLAC, etc.
* Libraries
	* Algorithms, `Archive`, `Bytes`, `Color`, `DateTime`, Encoding, `File`, Filesystem, `Image`, `Json`, Math, `Network`, `Platform`, `Web`
* Application interfaces
	* `Application`, `Canvas`, `Project`, `Debug`

These features are quite stable. Things just change, but it is very important that there won't be unthoughtful breaking changes in future development.

### Technical specifications

* Display: configurable resolution
* Code: Lua, supports multiple source files
* Image: either true-color (PNG, JPG, BMP, TGA) or paletted, up to 1024x1024 pixels per file
* Palette: 256 colors with transparency support
* Sprite: up to 1024x1024 pixels per frame, up to 1024 frames per sprite
* Map: up to 4096x4096 tiles per page
* Font: supports Bitmap and TrueType
* Audio: 1 BGM channel, 4 SFX channels; supports MP3, OGG, WAV, FLAC, etc.
* Gamepad: 6 buttons for each pad (D-Pad + A/B), up to 2 players
* Keyboard and mouse: supported

## Glance

### Code

```lua
local obj = nil

function setup()
  obj = Resources.load('hero.spr')
  obj:play('move', true, true)
end

function update(delta)
  spr(obj, 208, 128, 64, 64)
end
```

See more in [examples](examples).

### Games

![](docs/imgs/game1_2048.png)

![](docs/imgs/game2_reversi.png)

![](docs/imgs/game3_boing.png)

![](docs/imgs/game4_sweeper.png)

![](docs/imgs/game5_platformer.png)

![](docs/imgs/game6_rpg.png)

![](docs/imgs/game7_racing.png)

## Redistributing

![](docs/imgs/banner_platforms.png)

Bitty Engine helps you to make standalone binaries for Windows, MacOS, Linux, and HTML (WebAssembly). It is redistributable for both non-commercial and commercial use without extra fee, your project is totally yours.

Everything is done with Bitty Engine (pro) by nothing more than a few mouse clicks.

Put an image at "../icon.png" relative to executable to customize the icon dynamically. Put an image at "../splash.png" as well to customize the splash, the image could be transparent.

## Getting Bitty Engine

The pro version is available on [Steam](https://store.steampowered.com/app/1386180/) and [Itch](https://tonywang.itch.io/bitty).

[![](docs/imgs/steam.png)](https://store.steampowered.com/app/1386180/) [![](docs/imgs/itch.png)](https://tonywang.itch.io/bitty)

See [version comparison](versions.md).

## Compiling from source

A C++14 capable compiler is required, any compiler came after 2014 should be ok. You need to get this repository and its submodules before build:

```sh
git clone https://github.com/paladin-t/bitty.git
cd bitty
git submodule init
git submodule update
```

<details>
<summary>Details</summary>

See the [README](src/README.md) under the "src" directory for source code architecture.

There are some customizable macros:

* `BITTY_MULTITHREAD_ENABLED[=1]`: indicates whether project code executes on a thread separately from graphics
* `BITTY_DEBUG_ENABLED[=1]`: indicates whether project code debug is enabled; requires `BITTY_MULTITHREAD_ENABLED==1`; disable this to build a pure runner
* `BITTY_NETWORK_ENABLED[=1]`: indicates whether the `Network` API is enabled
* `BITTY_WEB_ENABLED[=1]`: indicates whether the `Web` API is enabled; disable this if you don't need web accessibility
* `BITTY_SPLASH_ENABLED[=1]`: indicates whether the splash is enabled
* `BITTY_EFFECTS_ENABLED[=0]`: whether full screen effects is enabled
* `BITTY_PROJECT_STRATEGY_MAP_BATCH_ENABLED[=0]`: indicates whether map batch is preferred; it might speed up map rendering if enabled, but requires more memory and could be slow with `mset(...)`

</details>

### Startup project

Once you have setup and built a binary, it loads from "../data" or "../data.bit" automatically as startup project. Otherwise it shows a programmable workspace.

### Windows

Dependencies: Visual Studio (with C++ development tools installed).

<details>
<summary>Steps</summary>

1. Build SDL2
	1. Compile from "lib/sdl/VisualC/SDL.sln"
	2. Execute `lib/sdl/copy_win.cmd`
2. Build SDL2_mixer
	1. Compile from "lib/sdl_mixer/VisualC/SDL_mixer.sln" (need to setup SDL2 including and linking paths from previous step manually)
	2. Execute `lib/sdl_mixer/copy_win.cmd`
3. Build Bitty Engine
	1. Compile from "bitty.sln"

</details>

### MacOS

Dependencies: Xcode.

<details>
<summary>Steps</summary>

1. Build cURL
	1. Execute:
		```sh
		cd lib/curl
		./MacOSX-Framework
		cd ../..
		```
2. Build SDL2
	1. Compile framework from "lib/sdl/Xcode/SDL/SDL.xcodeproj"
	2. Reveal "SDL2.framework" in Finder
	3. Copy "SDL2.framework" to both "lib/sdl/lib/" and "/Library/Frameworks/" (used in following step)
3. Build SDL2_mixer
	1. Compile framework from "lib/sdl_mixer/Xcode/SDL_mixer.xcodeproj"
	2. Reveal "SDL2_mixer.framework" in Finder
	3. Copy "SDL2_mixer.framework" to "lib/sdl_mixer/lib/"
4. Build Bitty Engine
	1. Compile from "bitty.xcodeproj"

</details>

### Linux

Dependencies: CMake, GCC.

<details>
<summary>Steps</summary>

1. Build cURL
	1. Execute:
		```sh
		cd lib/curl
		./configure
		make
		cd ../..
		```
	2. Execute `lib/curl/copy_linux.sh`
2. Build SDL2
	1. Execute:
		```sh
		cd lib/sdl
		./configure
		make
		sudo make install
		cd ../..
		```
	2. Execute `lib/sdl/copy_linux.sh`
3. Build SDL2_mixer
	1. Install necessary dependencies to enable extra audio format support, eg. for Ubuntu execute:
		```sh
		sudo apt install libflac-dev libfluidsynth-dev libmodplug-dev libmpg123-dev libopus-dev libopusfile-dev libvorbis-dev
		```
	2. Execute:
		```sh
		cd lib/sdl_mixer
		./configure
		make
		cd ../..
		```
	3. Execute `lib/sdl_mixer/copy_linux.sh`
4. Build Bitty Engine
	1. Execute:
		```sh
		cd bitty.linux
		cmake . && make
		cd ..
		```

</details>

### Other platforms

<details>
<summary>Details</summary>

You can also setup your own build pipeline for other platforms. The "lib" and "src" directories are almost what you need. See the [README](src/README.md) under the "src" directory for code architecture.

The "platform_*" files contain most platform dependent code, you'll probably make a specific port.

Note the file dialog library is only usable on desktop builds, make your own adaption if you need it on other platforms.

</details>

## Community and contributing

* [Discord](https://discord.gg/372vb8ct2H)
* [GitHub Discussions](https://github.com/paladin-t/bitty/discussions)
* [Issues](https://github.com/paladin-t/bitty/issues)
* [Pull Requests](https://github.com/paladin-t/bitty/pulls)

## Documentation

* [Reference Manual](https://paladin-t.github.io/bitty/manual.html)
* [Operations](https://paladin-t.github.io/bitty/operations.html)
* [Keycodes](https://paladin-t.github.io/bitty/keycodes.html)
* [Formats](https://paladin-t.github.io/bitty/formats.html)
* [Changelog](https://paladin-t.github.io/bitty/changelog.html)
