@echo off
setlocal
rem Creates dist\retroshare-profile-<date>.zip with everything needed to run
rem retroshare-profile.exe on Windows. Build the tool first (release).
rem
rem MSYS2 is looked for in C:\msys64, or set MSYS2_ROOT to another folder.
rem Works for Qt5 and Qt6 builds (detected from the exe).

if "%MSYS2_ROOT%"=="" set "MSYS2_ROOT=C:\msys64"

if not exist "%MSYS2_ROOT%\usr\bin\bash.exe" (
	echo MSYS2 not found in "%MSYS2_ROOT%".
	echo Set MSYS2_ROOT to your MSYS2 folder, for example:  set MSYS2_ROOT=D:\msys64
	pause
	exit /b 1
)

rem MSYS2 environment the tool was built in: MINGW64 by default, or set
rem MSYSTEM=UCRT64 (or CLANG64) before running this file.
if "%MSYSTEM%"=="" set "MSYSTEM=MINGW64"
set "CHERE_INVOKING=1"
cd /d "%~dp0"

"%MSYS2_ROOT%\usr\bin\bash.exe" -lc "./make-zip.sh"
set ERR=%ERRORLEVEL%

echo.
pause
exit /b %ERR%
