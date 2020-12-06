### v0.8.2 Dec. 6, 2020

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
* Improved `network:send(...)`, `network:boardcast(...)` to support Lua table
* Improved UTF-8 support for custom font

### v0.8.1 Dec. 1, 2020

Improved usability, fixed some bugs, and finished the manual.

* Added a context menu for the assets window
* Added tooltips for some unobvious operations
* Added detailed font customization by config
* Changed `bytes:readBytes(...)`, `file:readBytes(...)`, the results' cursor will be at the end
* Fixed a variable value inspecting bug with the debugger
* Fixed a bug of `fileInfo:empty()`
* Fixed a `Bytes` filling bug with `Network`
* Fixed an unexpected key event bug eg. with LAlt+A
* Fixed some minor bugs in the examples
* Improved the behaviour when open project with pending changes
* Finished the first version of the reference manual

### v0.8 Nov. 18, 2020

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
