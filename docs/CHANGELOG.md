### v1.1.8 Aug. 20, 2022

Enhanced modules, improved usability, fixed some bugs.

* Added auto loading last project at startup
* Added a `DateTime.utc()` function to get current UTC time
* Added a wildcard pattern parameter to the `project:getAssets(...)` method
* Added a shortcut key to reload project (Ctrl+Shift+R)
* Added an option to disable pause menu on ESC pressed
* Added a launch option to specify a different frame rate
* Added a launch option to enable high-precision real number debug
* Fixed a calculation bug of the `Raycaster` algorithm
* Fixed a border issue of maximized canvas
* Fixed a few memory leaks
* Fixed a potential crash issue
* Improved the music visualizer
* Improved speed calculation of the "Games/Racing" example

### v1.1.7 Apr. 2, 2022

Enhanced modules, improved usability, fixed some bugs.

* Added theme options to change the editor color
* Added records of recent saved paths
* Added vector constants
* Fixed a control key handling bug with IME (Windows)
* Fixed a resetting bug of blend mode
* Fixed a potential crash bug when set SFX volume with an array
* Fixed a potential crash bug when network connection error occurs
* Improved the asset selection area
* Upgraded Lua from v5.4.3 to v5.4.4

### v1.1.6 Jan. 7, 2022

Enhanced modules, improved usability, fixed some bugs.

* Added a parameter to specify font scaling for the `measure(...)` and `text(...)` functions
* Added a parameter to specify map scaling for the `map(...)` function
* Added project strategy for non-pixelated canvas preference
* Added extension recognition of ".xm", ".s3m", ".669", ".it" and ".med" formats
* Added a menu entry to open recent projects
* Added an option to toggle project backup
* Removed Lua source reading from Lua (can still read other asset types)
* Fixed a bug of `Application.size()`
* Fixed a bug of `Application.resize(...)` when restored from fullscreen
* Fixed a clip issue when size is negative
* Fixed a crash bug of the image and map editors
* Fixed a directory accessing bug with Russian localization (Windows)
* Fixed a font customization bug
* Improved code asset releasing

### v1.1.5 Nov. 25, 2021

Enhanced modules, improved usability, fixed some bugs.

* Added "args.txt" to specify launch options
* Added a launch option to enable software renderer
* Added an `Application` "title" option to set the title of the application window
* Added an `Application.size()` function to get size of the application window
* Added an `Application.raise()` function to raise the application window
* Added a transparent color template to the image editor
* Added a `color:__mul(vec4)` operator
* Added encryption support for binary builders
* Fixed a fading bug of the `stop(sfx)` and `stop(music)` functions
* Fixed a duplicate execution bug of audio primitives
* Fixed a crash bug when clicked missing asset
* Fixed a size issue of the painting tools
* Improved the in-app document reader
* Upgraded ImGui from v1.84 to v1.85

### v1.1.4 Sep. 29, 2021

Enhanced modules, improved usability, fixed some bugs.

* Added an asynchronized sprite playing mode
* Added support of the standard `debug` module
* Fixed a calculation bug of the `color:__mul(color_)` operator
* Fixed a cursor moving bug of the `project:read(...)` method
* Fixed an audio type detection bug when load
* Fixed an exiting issue from code
* Improved sprite rendering with a same object for multiple times
* Improved redundant argument handling
* Improved the color filling tool

### v1.1.3 Aug. 31, 2021

Enhanced modules, improved usability, fixed some bugs.

* Added a `Path.savedGamesDirectory` static variable
* Added a `pie(...)` function
* Added an `image:blit(...)` method
* Added navigation by keypad support to text-based editors
* Fixed a stuck issue of HTML build
* Fixed a directory browsing issue (Linux)
* Fixed a table traversing issue of the debugger
* Fixed a debug panel dragging issue
* Fixed an exiting issue from code
* Fixed a sprite playing issue when all parameters omitted
* Fixed a resetting bug of `Application.setCursor(...)`
* Upgraded ImGui from v1.83 to v1.84

### v1.1.2 Jun. 29, 2021

Enhanced modules, improved usability, fixed some bugs.

* Added a `Debug.trace(...)` function
* Added a `Debug.clearConsole()` function
* Added case-insensitive support for assets filter
* Added error report for the JSON editor
* Fixed crash bugs when add invalid assets
* Fixed some string argument retrieving issues
* Fixed a renderer resetting issue after restoring the window
* Fixed a sprite sizing issue of the sprite editor with non-square assets
* Upgraded ImGui from v1.82 to v1.83

### v1.1.1 May. 27, 2021

Enhanced modules, improved usability, fixed some bugs.

* Added an `Application.setOption(...)` function to change application behaviour
* Added `rot:__add(...)`, `rot:__sub(...)`, `rot:__unm()` operators to the `Rot` object
* Added context menu to graphics-based editors
* Added a menu item to reload project
* Added a dialog box to resize helper grid of the image editor
* Fixed a render target resetting issue
* Fixed a crash bug when asset ref is unloaded
* Fixed a crash bug when error occurred
* Fixed a manual exiting issue when debug is disabled
* Fixed an infinite sleep issue (Windows)
* Fixed an ellipse drawing bug of graphics-based editors
* Improved error handling during parsing
* Improved asset lookup performance

### v1.1 Apr. 10, 2021

Enhanced modules, improved usability, fixed some bugs.

* Added an `Application.resize(...)` method
* Added support for splash customization dynamically
* Fixed a UV calculation bug
* Fixed an exiting issue of built binary
* Fixed a canvas border issue of built binary
* Fixed an unexpected deleting issue of the image editor
* Fixed an unexpected deleting issue of the sprite editor
* Improved asset creating
* Improved assets filter when no wildcard provided
* Upgraded Lua from v5.4.2 to v5.4.3

### v1.0.3 Mar. 20, 2021

Enhanced modules, improved usability, fixed some bugs.

* Added source index tooltips to the sprite and map editors
* Added a `rendererReset` entry
* Added a `Json.Null` value representing null in JSON
* Added an `allowNull` parameter to the `json:toTable(...)` method
* Improved colored text rendering
* Improved canvas sizing
* Improved performance of batched map setting
* Fixed a render target reset bug of batched map
* Fixed a map sizing issue
* Upgraded ImGui from v1.81 to v1.82

### v1.0.2 Feb. 18, 2021

Enhanced modules, fixed some bugs.

* Added a color parameter to the `tex(...)`, `spr(...)`, `map(...)` functions
* Added a `Project.new()` constructor
* Added `project:load(...)`, `project:save(...)` methods
* Added `project:exists(...)`, `project:write(...)`, `project:remove(...)` methods
* Added a customizable "window_mask_background" style option
* Fixed a resizing issue of image and map
* Fixed an execution issue of the `Platform.execute(...)` function (HTML)
* Upgraded ImGui from v1.80 to v1.81

### v1.0.1 Feb. 6, 2021

Enhanced modules, improved usability, fixed some bugs.

* Added context menu to text-based editors
* Added support to resize image asset
* Added support to resize map asset
* Added a `stretch` parameter to the `image:resize(...)` method
* Added return values to the `cls(...)` function for previous clear color
* Added return values to the `color(...)` function for previous active color
* Added return values to the `camera(...)` function for previous camera offset
* Added return values to the `clip(...)` function for previous clip area
* Added a return value to the `mouse(...)` function for mouse wheel state
* Changed modifier of shortcut combinations from Ctrl to Cmd on MacOS
* Improved plugin to support customized source editor and compiler
* Fixed a crash bug when step into with the debugger
* Fixed a freeze bug when break with the debugger
* Fixed a scope navigation bug for recursion with the debugger
* Fixed an asset closing issue
* Fixed a mouse cursor rendering issue of the code editor
* Fixed a selection issue after typing in text-based editors
* Fixed a tab ('\t') character rendering issue of text-based editors
* Upgraded ImGui from v1.79 to v1.80

### v1.0 Jan. 19, 2021

Enhanced modules, improved usability, fixed some bugs.

* Added a `Math.intersects(...)` function
* Added an `Application.setCursor(...)` function to customize mouse cursor
* Added support to set volume of SFX channels respectively
* Added support for object inspecting of the debugger
* Added support for icon customization on startup
* Added a "Libraries/Math/Intersection Detection" example
* Added tooltips for asset ref
* Improved performance of text-based project saving
* Improved completeness checking when call object methods
* Improved assets filter
* Improved the `btn(...)` and `btnp(...)` functions to support getting any button
* Improved performance when edit big map
* Fixed an audio volume resetting bug
* Fixed a sprite animation resetting issue
* Fixed return value bugs when write to `Bytes`
* Fixed return value bugs when write to `File`
* Fixed a readonly issue when open code assets during running
* Fixed a deadlock issue when break (by breakpoint) during requiring another source
* Fixed a selection issue after pasting in text-based editors
* Fixed a table splitting issue in the document viewer

### v0.9.3 beta Jan. 10, 2021

Enhanced modules, improved usability, fixed some bugs.

* Added a `Noiser` module
* Added support to load texture and audio from `Bytes`
* Added project strategy for map batch preference
* Added a `project:strategies()` method to get effective strategies
* Added error prompt for HTML build
* Added a "Libraries/Algorithms/Noiser" example
* Added a "Recipes/Tween" example
* Fixed a crash bug of the `Path`, `FileInfo`, `DirectoryInfo` modules when path is too long (Windows)
* Fixed a freeze bug of the `exit()` function
* Fixed a non-working bug of the `stop(sfx)` function
* Fixed a tab bar rendering issue of the editors
* Improved stability if ran into memory problem
* Improved completeness checking when `close()` objects
* Improved code editing and highlighting

### v0.9.2 beta Dec. 31, 2020

Added HTML builder. Enhanced modules, fixed some bugs.

* Added binary builders for HTML (WebAssembly)
* Added support to load `Font` object from file directly
* Fixed a UTF-8 rendering issue of the `text(...)` function
* Fixed a resolving issue of the `fetch(...)` function
* Fixed a popup representation issue with built binary
* Fixed a crash bug with the JSON editor (Linux)
* Fixed a crash bug of the `input(...)` function

### v0.9.1 beta Dec. 25, 2020

Enhanced modules, improved high-DPI awareness on MacOS, fixed some bugs.

* Added a `Raycaster` module
* Added alpha support for text rendering
* Added a "Libraries/Algorithms/Raycaster" example
* Added an optional parameter when a `Promise` got error
* Added a launch option to disable high-DPI (MacOS)
* Fixed a return value bug of the `mouse(...)` function
* Fixed an input value validation issue of the `Walker` module
* Fixed a touch position bug with global scale
* Fixed a rendering bug with high-DPI monitor (MacOS)

### v0.9 beta Dec. 18, 2020

Added MacOS and Linux versions. Enhanced modules, improved usability, fixed some bugs.

* Added MacOS and Linux support
* Added binary builders for MacOS and Linux
* Added `focusLost()`, `focusGained()` entries
* Added support for `Lz4` encoding
* Added `Promise.Pending`, `Promise.Resolved`, `Promise.Rejected` constant
* Added `promise.state`, `promise.value` fields
* Fixed an interaction blocking issue with minor buttons
* Fixed a text encoding bug of the `Platform.surf(...)`, `Platform.browse(...)` methods
* Fixed a browsing bug of the frame recorder
* Fixed a calculation bug of `vec3:dot(vec3_)`
* Improved responsiveness for project saving
* Upgraded Lua from v5.4.1 to v5.4.2
* Upgraded cURL from v7.73.0 to v7.74.0

### v0.8.2 beta Dec. 6, 2020

Enhanced modules, improved usability.

* Added an assets filter
* Added a `Promise` protocol
* Added a `fetch(...)` function (experimental)
* Added an `input(...)` function
* Added a `msgbox(...)` function
* Added an `exit()` function
* Added `Platform.openFile(...)`, `Platform.saveFile(...)`, `Platform.selectDirectory(...)` functions
* Added a "Basics/Updating" example
* Added a "Libraries/Web" example
* Fixed an indent/unindent bug for single line
* Fixed an abort bug for infinite loop
* Improved the `network:send(...)`, `network:boardcast(...)` methods to support Lua table
* Improved UTF-8 support for custom font

### v0.8.1 beta Dec. 1, 2020

Improved usability, fixed some bugs, and finished the manual.

* Added a context menu for the assets window
* Added tooltips for some unobvious operations
* Added detailed font customization by config
* Changed the `bytes:readBytes(...)`, `file:readBytes(...)` methods, the results' cursor will be at the end
* Fixed a variable value inspecting bug of the debugger
* Fixed a bug of the `fileInfo:empty()` method
* Fixed a `Bytes` filling bug of the `Network` module
* Fixed an unexpected key event bug eg. with LAlt+A
* Fixed some minor bugs in the examples
* Improved the behaviour when open project with pending changes
* Finished the first version of the reference manual

### v0.8 beta Nov. 18, 2020

Added Windows version. First release with features including:

* Added Lua programming interfaces
* Added a resource manager
* Added graphics primitives
* Added input functions
* Added audio functions
* Added utility functions
* Added workspace and editors
* Added binary builder for Windows
* Added various examples
