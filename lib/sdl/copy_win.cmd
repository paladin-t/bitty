@echo off

if exist "%~dp0\VisualC\Win32\Release\SDL2.dll" (
  xcopy /y /e "%~dp0\VisualC\Win32\Release\SDL2.dll" "lib\x86\"
  xcopy /y /e "%~dp0\VisualC\Win32\Release\SDL2.lib" "lib\x86\"
)
if exist "VisualC\x64\Release\SDL2.dll" (
  xcopy /y /e "%~dp0\VisualC\x64\Release\SDL2.dll" "lib\x64\"
  xcopy /y /e "%~dp0\VisualC\x64\Release\SDL2.lib" "lib\x64\"
)

echo Ok.
