![](imgs/logo.png)

## Welcome to Bitty Engine

[**Manual**](https://paladin-t.github.io/bitty/manual.html) | [Operations](operations)

**An itty bitty game engine.**

As you see, this document is **draft**. I will gradually fill contents to make it a full description of the libraries, tools and other aspects for game making. For the moment, I suggest you to take a look at the examples for reference, especially the "Libraries" and "Primitives" categories that already covered most API. I'll also improve the examples during this period.

## Table of content

* [Fundamental](#fundamental)
	* [Specifications](#specifications)
	* [Project Structure](#project-structure)
		* [In Directory](#in-directory)
		* [Text-based Archive](#text-based-archive)
		* [Binary-based Archive](#binary-based-archive)
	* [Backup](#backup)
	* [Capturing](#capturing)
		* [Screenshot](#screenshot)
		* [GIF](#gif)
* [Programming](#programming)
	* [Lua](#lua)
		* [Syntax](#syntax)
		* [Standard Libraries](#standard-libraries)
	* [Program Structure](#program-structure)
	* [Libraries](#libraries)
		* [Algorithms](#algorithms)
			* [Pathfinder](#pathfinder)
			* [Randomizer](#randomizer)
			* [Walker](#walker)
		* [Archive](#archive)
		* [Bytes](#bytes)
		* [Color](#color)
		* [Date Time](#date-time)
		* [Encoding](#encoding)
			* [Base64](#base64)
		* [File](#file)
		* [Filesystem](#filesystem)
		* [Image](#image)
		* [JSON](#json)
		* [Math](#math)
		* [Network](#network)
		* [Platform](#platform)
		* [Stream](#stream)
	* [Assets and Resources](#assets-and-resources)
		* [Resources](#resources)
		* [Asset](#asset)
		* [Palette Asset](#palette-asset)
		* [Font Asset](#font-asset)
		* [Texture Asset](#texture-asset)
		* [Sprite Asset](#sprite-asset)
		* [Map Asset](#map-asset)
		* [SFX Asset](#sfx-asset)
		* [Music Asset](#music-asset)
	* [Primitives](#primitives)
		* [Basics](#basics)
		* [Shapes](#shapes)
		* [Palette](#palette)
		* [Font](#font)
		* [Texture](#texture)
		* [Sprite](#sprite)
		* [Map](#map)
		* [SFX](#sfx)
		* [Music](#music)
		* [Gamepad](#gamepad)
		* [Keyboard](#keyboard)
		* [Mouse](#mouse)
		* [Camera](#camera)
		* [Clip](#clip)
		* [Blend](#blend)
	* [Application](#application)
		* [Canvas](#canvas)
		* [Project](#project)
		* [Debug](#debug)
* [Import and Export](#import-and-export)
	* [Import](#import)
	* [Export](#export)
* [Building](#building)
	* [Building for Desktop](#building-for-desktop)

[HOME](#welcome-to-bitty-engine)

# Fundamental

## Specifications

* Display: defaults to 480x320 pixels, configurable to bigger, smaller or self-adaption
* Audio: 1 BGM channel, 4 SFX channels; supports MP3, OGG, WAV, FLAC, etc.
* Font: supports Bitmap and TrueType
* Code: Lua, supports multiple source files
* Sprite: up to 1024x1024 pixels per frame, up to 1024 frames per sprite
* Map: up to 4096x4096 tiles per page
* Image: either true-colored (PNG, JPG, BMP, TGA) or paletted, up to 1024x1024 pixels per file
* Palette: 256 colors with transparency support
* Gamepad: 6 buttons for each pad (D-Pad + A/B), up to 2 players
* Keyboard and mouse: supported

## Project Structure

A new created project consists of a meta info asset ("info.json") and an entry source ("main.lua"). A meta info indicates basic information of a project in JSON. An entry is where a project starts to execute. You can add supported existing file or create new blank assets into a project. All text-based assets use Unix LF ('\n') for line ending.

### In Directory

All assets are stored as raw files under a specific directory in this format.

### Text-based Archive

A text-based project archive is a plain text file with the "*.bit" extension, with all assets encoded in printable UTF-8 characters or Base64 string. This format uses Unix LF ('\n') for line ending.

### Binary-based Archive

A binary-based project archive is just a compressed ZIP package replaced with the "*.bit" extension.

## Backup

Bitty Engine makes backup once you save an asset or a project, the entire project will be stored to `X:/Users/YourName/AppData/Roaming/bitty/engine/backup` on Windows.

You can navigate there by clicking "Project", "Browse Data Directory...".

## Capturing

### Screenshot

* F6: take a screenshot

### GIF

* F7: start recording frames
* F8: stop recording frames

[HOME](#welcome-to-bitty-engine)

# Programming

## Lua

Bitty project is programmable in the [Lua](https://www.lua.org/) programming language.

### Syntax

Lua is widely used and validated in the software industry, there are a lot of learning materials about the language on the internet. Click to see the [official documentation](https://www.lua.org/docs.html).

### Standard Libraries

The ready to use modules, `package`, `coroutine`, `table`, `string`, `math`, `utf8`, are reserved from the original.

The trivial modules, `io`, `os`, `debug`, are disabled. Bitty Engine offers alternatives.

## Program Structure

A conventional entry program of Bitty project is made up of a `setup` function which is called once a program starts, and an `update` which is called periodically:

```lua
function setup()
end

function update(delta)
end
```

Define another `quit` function to run code on execution termination:

```lua
function quit()
end
```

Generally `setup` is used to initial game variables, states, `update` is where gameplay logic goes, and `quit` is for persist necessary data on disk. All the three entries are optional.

## Libraries

### Algorithms

#### Pathfinder

This module is used to perform a pathfinding algorithm on 2D grids:

**Constructors**

* `Pathfinder.new(w, n, e, s)`

**Object Field**

* `pathfinder.diagonalCost`

**Methods**

* `pathfinder:get(pos)`
* `pathfinder:set(pos, cost)`
* `pathfinder:clear()`
* `pathfinder:solve(beginPos, endPos, eval)`
* `pathfinder:solve(beginPos, endPos)`

#### Randomizer

**Constructors**

* `Random.new()`

**Methods**

* `random:seed([seed])`
* `random:next(low, up)`
* `random:next(up)`
* `random:next()`

#### Walker

This module is used to perform a smooth walking algorithm on 2D grids:

**Constants**

* `Walker.None`
* `Walker.Left`
* `Walker.Right`
* `Walker.Up`
* `Walker.Down`

**Constructors**

* `Walker.new()`

**Object Field**

* `walker.objectSize`
* `walker.tileSize`
* `walker.offset`

**Methods**

* `walker:solve(objPos, expDir, eval, slidable = 5)`

### Archive

**Constructors**

* `Archive.new()`

**Methods**

* `archive:open(path, access = Stream.Read)`
* `archive:close()`
* `archive:all()`
* `archive:exists(entry)`
* `archive:make(entry)`
* `archive:toBytes(entry, bytes)`
* `archive:fromBytes(entry, bytes)`
* `archive:toFile(entry, path)`
* `archive:fromFile(entry, path)`
* `archive:toDirectory(entry, path)`
* `archive:fromDirectory(entry, path)`

### Bytes

Being the same as Lua array, `Bytes` index starts from 1. Implements a `Stream` as memory buffer.

**Constructors**

* `Bytes.new()`

**Operators**

* `bytes[index]`
* `bytes:__len()`

**Methods**

* `bytes:peek()`
* `bytes:poke(index)`
* `bytes:count()`
* `bytes:empty()`
* `bytes:endOfStream()`
* `bytes:readByte()`
* `bytes:readInt16()`
* `bytes:readUInt16()`
* `bytes:readInt32()`
* `bytes:readUInt32()`
* `bytes:readInt64()`
* `bytes:readSingle()`
* `bytes:readDouble()`
* `bytes:readBytes([expSize[, buf]])`
* `bytes:readString([expSize])`
* `bytes:readLine()`
* `bytes:writeByte(val)`
* `bytes:writeInt16(val)`
* `bytes:writeUInt16(val)`
* `bytes:writeInt32(val)`
* `bytes:writeUInt32(val)`
* `bytes:writeInt64(val)`
* `bytes:writeSingle(val)`
* `bytes:writeDouble(val)`
* `bytes:writeBytes(bytes[, expSize])`
* `bytes:writeString(val)`
* `bytes:writeLine(val)`
* `bytes:get(index)`
* `bytes:set(index, val)`
* `bytes:resize(expSize)`
* `bytes:clear()`

### Color

**Constructors**

* `Color.new(r, g, b, a = 255)`
* `Color.new()`

**Operators**

* `color:__add(color_)`
* `color:__sub(color_)`
* `color:__mul(num)`
* `color:__mul(color_)`
* `color:__unm()`
* `color:__eq(color_)`

**Object Field**

* `color.r`
* `color.g`
* `color.b`
* `color.a`

**Methods**

* `color:toRGBA()`
* `color:fromRGBA(int)`

### Date Time

**Static Functions**

* `DateTime.now()`
	* returns `sec`, `min`, `hr`, `mday`, `mo`, `yr`, `wday`, `yday`, `isdst`
* `DateTime.ticks()`
* `DateTime.toMilliseconds(ticks)`
* `DateTime.fromMilliseconds(ms)`
* `DateTime.toSeconds(ticks)`
* `DateTime.fromSeconds(sec)`

### Encoding

#### Base64

**Static Functions**

* `Base64.encode(bytes)`
* `Base64.decode(txt)`

### File

Following the Stream protocol, `File` cursor starts from 1. Implements a `Stream` as file.

**Constructors**

* `File.new()`

**Operators**

* `file:__len()`

**Methods**

* `file:open(path, access = Stream.Read)`
* `file:close()`
* `file:peek()`
* `file:poke(index)`
* `file:count()`
* `file:empty()`
* `file:endOfStream()`
* `file:readByte()`
* `file:readInt16()`
* `file:readUInt16()`
* `file:readInt32()`
* `file:readUInt32()`
* `file:readInt64()`
* `file:readSingle()`
* `file:readDouble()`
* `file:readBytes([expSize[, buf]])`
* `file:readString([expSize])`
* `file:readLine()`
* `file:writeByte()`
* `file:writeInt16()`
* `file:writeUInt16()`
* `file:writeInt32()`
* `file:writeUInt32()`
* `file:writeInt64()`
* `file:writeSingle()`
* `file:writeDouble()`
* `file:writeBytes(bytes[, expSize])`
* `file:writeString(val)`
* `file:writeLine(val)`

### Filesystem

**Static Functions**

* `Path.combine(...)`
* `Path.split(full)`
	* returns `name`, `ext`, `parent`
* `Path.existsFile(path)`
* `Path.existsDirectory(path)`
* `Path.copyFile(src, dst)`
* `Path.copyDirectory(src, dst)`
* `Path.moveFile(src, dst)`
* `Path.moveDirectory(src, dst)`
* `Path.removeFile(path, toTrashBin)`
* `Path.removeDirectory(path, toTrashBin)`
* `Path.touchFile(path)`
* `Path.touchDirectory(path)`

**Static Variables**

* `Path.executableFile`: readonly
* `Path.documentDirectory`: readonly
* `Path.writableDirectory`: readonly

**Constructors**

* `FileInfo.new(path)`

* `DirectoryInfo.new(path)`

**Methods**

* `fileInfo:fullPath()`
* `fileInfo:parentPath()`
* `fileInfo:fileName()`
* `fileInfo:extName()`
* `fileInfo:empty()`
* `fileInfo:exists()`
* `fileInfo:make()`
* `fileInfo:copyTo(dst)`
* `fileInfo:moveTo(dst)`
* `fileInfo:remove(toTrashBin)`
* `fileInfo:rename(newName[, newExt])`
* `fileInfo:parent()`
* `fileInfo:readAll()`

* `directoryInfo:fullPath()`
* `directoryInfo:parentPath()`
* `directoryInfo:dirName()`
* `directoryInfo:empty()`
* `directoryInfo:exists()`
* `directoryInfo:make()`
* `directoryInfo:copyTo(dst)`
* `directoryInfo:moveTo(dst)`
* `directoryInfo:remove(toTrashBin)`
* `directoryInfo:rename(newName)`
* `directoryInfo:getFiles(pattern = "*.*", recursive = false)`
* `directoryInfo:getDirectories(recursive = false)`
* `directoryInfo:parent()`

### Image

**Constructors**

* `Image.new(palette)`
* `Image.new()`

**Object Field**

* `image.channels`: readonly
* `image.width`: readonly
* `image.height`: readonly

**Methods**

* `image:resize(w, h)`
* `image:get(x, y)`
* `image:set(x, y, val)`
* `image:fromImage(img)`
* `image:fromBlank(w, h, paletted = 0)`
* `image:toBytes(bytes, type = "png")`
* `image:fromBytes(bytes)`

### JSON

**Constructors**

* `Json.new()`

**Methods**

* `json:toString(pretty = true)`
* `json:fromString(txt)`
* `json:toTable()`
* `json:fromTable(tbl)`

### Math

**Static Functions**

* `Rect.byXYWH(x, y, w, h)`

* `Recti.byXYWH(x, y, w, h)`

**Constructors**

* `Vec2.new([x, y])`

* `Vec3.new([x, y, z])`

* `Vec4.new([x, y, z, w])`

* `Rect.new([x1, y1, x2, y2])`

* `Recti.new([x1, y1, x2, y2])`

* `Rot.new([s, c])`

**Operators**

* `vec2:__add(vec2_)`
* `vec2:__sub(vec2_)`
* `vec2:__mul(num)`
* `vec2:__mul(vec2_)`
* `vec2:__unm()`
* `vec2:__len()`
* `vec2:__eq(vec2_)`

* `vec3:__add(vec3_)`
* `vec3:__sub(vec3_)`
* `vec3:__mul(num)`
* `vec3:__mul(vec3_)`
* `vec3:__unm()`
* `vec3:__len()`
* `vec3:__eq(vec3_)`

* `vec4:__add(vec4_)`
* `vec4:__sub(vec4_)`
* `vec4:__mul(num)`
* `vec4:__mul(vec4_)`
* `vec4:__unm()`
* `vec4:__eq(vec4_)`

* `rot:__mul(vec2)`
* `rot:__mul(rot_)`
* `rot:__eq(rot_)`

* `rect:__eq(rect_)`

* `recti:__eq(recti_)`

**Object Field**

* `vec2.x`
* `vec2.y`
* `vec2.normalized`: readonly
* `vec2.length`: readonly
* `vec2.angle`: readonly

* `vec3.x`
* `vec3.y`
* `vec3.z`
* `vec3.normalized`: readonly
* `vec3.length`: readonly

* `rect4.x`
* `rect4.y`
* `rect4.z`
* `rect4.w`

* `rot.s`
* `rot.c`
* `rot.angle`

* `rect.x0`
* `rect.y0`
* `rect.x1`
* `rect.y1`

* `recti.x0`
* `recti.y0`
* `recti.x1`
* `recti.y1`

**Methods**

* `vec2:normalize()`
* `vec2:distanceTo(vec2_)`
* `vec2:dot(vec2_)`
* `vec2:cross(num)`
* `vec2:cross(vec2_)`
* `vec2:angleTo(vec2_)`
* `vec2:rotated(angle[, pivot])`
* `vec2:rotated(rot[, pivot])`

* `vec3:normalize()`
* `vec3:dot(vec3_)`

* `rect:xMin()`
* `rect:yMin()`
* `rect:xMax()`
* `rect:xMax()`
* `rect:width()`
* `rect:height()`

* `recti:xMin()`
* `recti:yMin()`
* `recti:xMax()`
* `recti:xMax()`
* `recti:width()`
* `recti:height()`

### Network

**Constants**

* `Network.None`
* `Network.Udp`
* `Network.Tcp`

**Constructors**

* `Network.new(onRecv[, onEstb[, onDisc]])`
	* `onRecv`: callback on received
	* `onEstb`: callback on connection established
	* `onDisc`: callback on connection disconnected

**Object Field**

* `network.ready`: readonly
* `network.polling`: readonly
* `network.connective`: readonly

**Methods**

* `network:getOption(key)`
* `network:setOption(key, val)`
* `network:open(addr[, protocal])`
* `network:close()`
* `network:poll([timeoutMs])`
* `network:disconnect()`
* `network:send(bytes)`
* `network:send(txt)`
* `network:send(json)`
* `network:broadcast(bytes)`
* `network:broadcast(txt)`
* `network:broadcast(json)`

### Platform

**Static Functions**

* `Platform.surf(url)`
* `Platform.browse(dir)`
* `Platform.hasClipboardText()`
* `Platform.getClipboardText()`
* `Platform.setClipboardText(txt)`
* `Platform.execute(cmd)`

**Static Variables**

* `Platform.os`: readonly
* `Platform.endian`: readonly
	* returns "little-endian" or "big-endian"

### Stream

**Constants**

* `Stream.Read`
* `Stream.Write`
* `Stream.Append`
* `Stream.ReadWrite`

## Assets and Resources

### Resources

**Static Functions**

* `Resources.load(entry, hint)`
* `Resources.load(str, hint)`
* `Resources.load(json, hint)`
* `Resources.load(table, hint)`
* `Resources.load(img, hint)`
* `Resources.wait(res)`
* `Resources.unload(res)`
* `Resources.collect()`

### Asset

Created by `Resources.load(...)`.

### Palette Asset

Created by `Resources.load(...)`.

### Font Asset

**Constructors**

* `Font.new()`

### Texture Asset

Created by `Resources.load(...)`.

**Object Field**

* `texture.width`: readonly
* `texture.height`: readonly

**Methods**

* `texture:blend(mode)`

### Sprite Asset

Created by `Resources.load(...)`.

**Object Field**

* `sprite.count`: readonly
* `sprite.width`: readonly
* `sprite.height`: readonly

* `sprite.hFlip`
* `sprite.vFlip`

**Methods**

* `sprite:play(res, beginIndex = -1, endIndex = -1, reset = true, loop = true)`
* `sprite:play(res, beginStr, reset = true, loop = true)`
* `sprite:pause()`
* `sprite:resume()`
* `sprite:stop()`

### Map Asset

Created by `Resources.load(...)`.

**Object Field**

* `map.width`: readonly
* `map.height`: readonly

### SFX Asset

Created by `Resources.load(...)`.

### Music Asset

Created by `Resources.load(...)`.

## Primitives

The coordinate definition in Bitty Engine is:

![](imgs/coordinate.png)

### Basics

**Functions**

* `cls([col])`: clears the screen with the specific color
	* `col`: optional, defaults to the previous passed value
* `color(col)`: sets the active color with a specific value
* `color()`: resets the active color to white
* `sync()`: synchronizes commands to graphics manually

### Shapes

**Functions**

* `plot(x, y[, col])`
* `line(x1, y1, x2, y2[, col])`
* `circ(x, y, r, fill = false[, col])`
* `ellipse(x, y, rx, ry, fill = false[, col])`
* `rect(x1, y1, x2, y2, fill = false[, col[, rad]])`
* `text(txt, x, y[, col, margin = 1])`
* `tri(p1, p2, p3, fill = false[, col])`

### Palette

**Functions**

* `pget(res, index)`
	* `index`: starts from 0
* `pset(res, index, col)`
	* `index`: starts from 0

### Font

**Functions**

* `font(font_)`
* `font()`
* `measure(txt, font, margin = 1)`

### Texture

**Functions**

* `tex(res, x, y[, w, h[, sx, sy[, sw, sh, [rotAngle, rotCenter = Vec2.new(0.5, 0.5), hFlip = false, vFlip = false]]]])`

### Sprite

**Functions**

* `spr(res, x, y[, w, h[, rotAngle, rotCenter = Vec2.new(0.5, 0.5)]])`

### Map

**Functions**

* `map(res, x, y)`
* `mget(res, x, y)`
	* `x`: starts from 0
	* `y`: starts from 0
* `mset(res, x, y, cel)`
	* `x`: starts from 0
	* `y`: starts from 0

### SFX

**Functions**

* `volume(sfxVol, musicVol)`
	* `sfxVol`: with range of values from 0.0 to 1.0
	* `musicVol`: with range of values from 0.0 to 1.0
* `play(sfx, loop = false[, fade[, channel]])`
* `stop(sfx[, fade])`

### Music

**Functions**

* `volume(sfxVol, musicVol)`
	* `sfxVol`: with range of values from 0.0 to 1.0
	* `musicVol`: with range of values from 0.0 to 1.0
* `play(music, loop = false[, fade])`
* `stop(music[, fade])`

### Gamepad

**Functions**

* `btn(button, index)`
	* `index`: starts from 1
* `btnp(button, index)`
	* `index`: starts from 1
* `rumble(index, lowHz = 100[, hiHz, ms = 100])`
	* `index`: starts from 1

0 for Left, 1 for Right, 2 for Up, 3 for Down, 4 for A, 5 for B.

### Keyboard

**Functions**

* `key(code)`
* `keyp(code)`

### Mouse

**Functions**

* `mouse(index)`
	* `index`: starts from 1
	* returns `x`, `y`, `b1`, `b2`, `b3`

### Camera

**Functions**

* `camera(x, y)`: sets the camera offset
* `camera()`: resets the camera offset

### Clip

**Functions**

* `clip(x, y, w, h)`: sets the clip area
* `clip()`: resets the clip area

### Blend

**Functions**

* `blend(mode)`: sets the blend state with the specific mode
* `blend()`: resets the blend state to alpha blend

## Application

### Canvas

**Constants**

* `Canvas.BlendModeNone`
* `Canvas.BlendModeBlend`
* `Canvas.BlendModeAdd`
* `Canvas.BlendModeMod`
* `Canvas.BlendModeMul`

* `Canvas.BlendFactorZero`
* `Canvas.BlendFactorOne`
* `Canvas.BlendFactorSrcColor`
* `Canvas.BlendFactorOneMinusSrcColor`
* `Canvas.BlendFactorSrcAlpha`
* `Canvas.BlendFactorOneMinusSrcAlpha`
* `Canvas.BlendFactorDstColor`
* `Canvas.BlendFactorOneMinusDstColor`
* `Canvas.BlendFactorDstAlpha`
* `Canvas.BlendFactorOneMinusDstAlpha`

* `Canvas.BlendOperationAdd`
* `Canvas.BlendOperationSub`
* `Canvas.BlendOperationRevSub`
* `Canvas.BlendOperationMin`
* `Canvas.BlendOperationMax`

**Static Functions**

* `Canvas.compose(srcColFactor, dstColFactor, colOp, srcAlphaFactor, dstAlphaFactor, alphaOp)`
	* returns composed blend option

**Static Variables**

* `Canvas.main`: readonly

**Object Field**

* `canvas.target`
* `canvas.autoCls`

**Methods**

* `canvas:size()`
* `canvas:resize()`

### Project

**Static Variables**

* `Project.main`: readonly

**Methods**

* `project:fullPath()`
	* returns the full path of the project, or `nil`
* `project:getAssets()`
	* returns a list of asset entries, or `nil`
* `project:read()`
	* returns asset content as `Bytes`, or `nil`

### Debug

**Static Functions**

* `Debug.setBreakpoint(src, ln, brk = true)`
	* returns `true` for success, otherwise `false`
* `Debug.clearBreakpoints([src])`
	* returns `true` for success, otherwise `false`
* `Debug.getTimeout()`
	* returns the active timeout value
* `Debug.setTimeout(val)`
* `Debug.setTimeout()`

[HOME](#welcome-to-bitty-engine)

# Import and Export

## Import

Click "Project", "Import..." to browse and import some assets from a "*.bit" archive. This operation doesn't overwrite conflictions.

## Export

Click "Project", "Export..." to select and export some assets to a "*.bit" archive.

[HOME](#welcome-to-bitty-engine)

# Building

## Building for Desktop

Click "Project", "Build for Desktop", "Windows" to make an executable for Windows with the current project.

[HOME](#welcome-to-bitty-engine)
