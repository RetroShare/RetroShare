@echo off

setlocal

if not exist "%~dp0msys2\msys64" goto :EOF

set MSYS2SH=%~dp0msys2\msys64\usr\bin\sh

echo Update MSYS2
"%MSYS2SH%" -lc "yes | pacman --noconfirm -Syuu msys2-keyring"
"%MSYS2SH%" -lc "pacman --noconfirm -Su"

endlocal
goto :EOF
