@echo off

if exist "%~dp0\VisualC\Win32\Release\SDL2_mixer.dll" (
  xcopy /y /e "%~dp0\VisualC\Win32\Release\*.dll" "lib\x86\"
  xcopy /y /e "%~dp0\VisualC\Win32\Release\*.lib" "lib\x86\"
)
if exist "VisualC\x64\Release\SDL2_mixer.dll" (
  xcopy /y /e "%~dp0\VisualC\x64\Release\*.dll" "lib\x64\"
  xcopy /y /e "%~dp0\VisualC\x64\Release\*.lib" "lib\x64\"
)

echo Ok.
