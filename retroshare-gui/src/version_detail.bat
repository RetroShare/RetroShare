@echo off

setlocal enabledelayedexpansion

:: Search git in PATH
set GitPath=
for %%P in ("%PATH:;=" "%") do (
	if exist "%%~P.\git.exe" (
		set GitPath=%%~P
		goto found_git
	)
)

:found_git
if "%GitPath%"=="" (
	echo git not found in PATH. Version update cancelled.
	endlocal
	exit /B 0
)

echo Update version

:: Retrieve git information
set RsBranch=
set RsHash=

pushd "%~dp0"
for /f "tokens=1*" %%A in ('"git log --pretty=format:"%%H" --max-count=1"') do set RsHash=%%A
for /f "tokens=*" %%A in ('git rev-parse --abbrev-ref HEAD') do set RsBranch=%%A
popd

if "%RsBranch%"=="" (
	echo Git branch not found.
	endlocal
	exit /B 1
)
if "%RsHash%"=="" (
	echo Git hash not found.
	endlocal
	exit /B 1
)

set RsDate=%date% %TIME%

:: Create file
set InFile=%~dp0gui\help\version.html.in
set OutFile=%~dp0gui\help\version.html
if exist "%OutFile%" del /Q "%OutFile%"

for /f "tokens=* delims= " %%a in (%InFile%) do (
	set line=%%a
	set line=!line:$Hash$=%RsHash%!
	set line=!line:$Branch$=%RsBranch%!
	set line=!line:$Date$=%RsDate%!
	echo !line!>>"%OutFile%"
)

endlocal
exit /B 0
