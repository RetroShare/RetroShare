@echo off

setlocal

if exist "%~dp0msys2\msys32" call :update 32
if exist "%~dp0msys2\msys64" call :update 64

goto :EOF

:update
set MSYSSH=%~dp0msys2\msys%~1\usr\bin\sh

echo Update MSYS2 %~1
"%MSYSSH%" -lc "pacman -Sy"
"%MSYSSH%" -lc "pacman --noconfirm -Su"

:exit
endlocal
goto :EOF
