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

* Added context menu to the text-based editors
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
* Improved asset filter
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
* Fixed a crash bug with JSON editor (Linux)
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
