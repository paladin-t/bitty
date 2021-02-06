## Source file structure

The source code files are flat tiled in this directory, there are also some inline resources in the "resource" directory.

```
File                        | Group
----------------------------+-----------------------------------------------------------
application                __ Application                                 __ Source Root
main                       _|                                              |
                                                                           |
luaxx                      __ Lua     __ Executable                       _|
scripting_lua              _|          |                                   |
scripting_lua_api          _|          |                                   |
scripting_lua_api_promises _|          |                                   |
scripting_lua_dbg          _|          |                                   |
plugin                     ____________|                                   |
scripting                  ____________|                                   |
                                                                           |
primitives                 __ Graphics                                    _|
renderer                   _|                                              |
texture                    _|                                              |
window                     _|                                              |
                                                                           |
input                      __ Input                                       _|
                                                                           |
audio                      __ Resources                                   _|
code                       _|                                              |
font                       _|                                              |
image                      _|                                              |
map                        _|                                              |
palette                    _|                                              |
resources                  _|                                              |
sprite                     _|                                              |
                                                                           |
noiser                     __ Algorithms                  __ Shared       _|
pathfinder                 _|                              |               |
randomizer                 _|                              |               |
raycaster                  _|                              |               |
walker                     _|                              |               |
archive_txt                __ Archive Implementations     _|               |
archive_zip                _|                              |               |
cloneable                  __ Interfaces                  _|               |
collectible                _|                              |               |
dispatchable               _|                              |               |
executable                 _|                              |               |
stream                     _|                              |               |
updatable                  _|                              |               |
network_mongoose           __ Network Implementation      _|               |
platform_html              __ Platform Implementations    _|               |
platform_linux             _|                              |               |
platform_macos             _|                              |               |
platform_windows           _|                              |               |
web_curl                   __ Web Implementation          _|               |
web_html                   _|                              |               |
web_mongoose               _|                              |               |
archive                    ________________________________|               |
bytes                      ________________________________|               |
color                      ________________________________|               |
datetime                   ________________________________|               |
either                     ________________________________|               |
encoding                   ________________________________|               |
file_handle                ________________________________|               |
filesystem                 ________________________________|               |
hacks                      ________________________________|               |
json                       ________________________________|               |
mathematics                ________________________________|               |
network                    ________________________________|               |
object                     ________________________________|               |
platform                   ________________________________|               |
plus                       ________________________________|               |
promise                    ________________________________|               |
text                       ________________________________|               |
web                        ________________________________|               |
                                                                           |
editing                    __________________ Editors     __ Workspace    _|
editor                     _________________|              |               |
editor_bytes               _________________|              |               |
editor_code                _________________|              |               |
editor_font                _________________|              |               |
editor_image               _________________|              |               |
editor_json                _________________|              |               |
editor_map                 _________________|              |               |
editor_palette             _________________|              |               |
editor_plugin              _________________|              |               |
editor_polyfill            _________________|              |               |
editor_sound               _________________|              |               |
editor_sprite              _________________|              |               |
editor_text                _________________|              |               |
resource/inline_font       __ Inlines                     _|               |
resource/inline_icon       _|                              |               |
resource/inline_image      _|                              |               |
resource/inline_resource   _|                              |               |
resource/inline_slice      _|                              |               |
resource/inline_sound      _|                              |               |
resource/inline_toast      _|                              |               |
editable                   __ Interfaces                  _|               |
document                   __ Shared                      _|               |
entry                      _|                              |               |
generic                    _|                              |               |
recorder                   _|                              |               |
asset                      ________________________________|               |
loader                     ________________________________|               |
operations                 ________________________________|               |
project                    ________________________________|               |
theme                      ________________________________|               |
widgets                    ________________________________|               |
workspace                  ________________________________|               |
                                                                           |
bitty                      ________________________________________________|
```
