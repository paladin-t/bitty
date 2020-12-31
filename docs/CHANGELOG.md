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
