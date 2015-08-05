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
set RsHash=

pushd "%~dp0"
for /f "tokens=1*" %%A in ('"git log --pretty=format:"%%H" --max-count=1"') do set RsHash=%%A
popd

if "%RsHash%"=="" (
	echo Git hash not found.
	endlocal
	exit /B 1
)

:: Create file
set InFile=%~dp0retroshare\rsversion.in
set OutFile=%~dp0retroshare\rsversion.h
if exist "%OutFile%" del /Q "%OutFile%"

for /f "tokens=* delims= " %%a in (%InFile%) do (
	set line=%%a
	set line=!line:$REV$=%RsHash:~0,8%!
	echo !line!>>"%OutFile%"
)

endlocal
exit /B 0
