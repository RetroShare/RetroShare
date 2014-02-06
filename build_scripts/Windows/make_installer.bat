@echo off

setlocal

:: Modify variable when makensis.exe doesn't exist in PATH
set NSIS_EXE=makensis.exe

:: Set needed environment variables
if "%SourceDir%"==""  set SourceDir=
if "%ReleaseDir%"=="" set ReleaseDir=
if "%QtDir%"==""      set QtDir=
if "%MinGWDir%"==""   set MinGWDir=

:: Check environment variables
if "%SourceDir%"==""  call :error_environment & goto :exit
if "%ReleaseDir%"=="" call :error_environment & goto :exit
if "%QtDir%"==""      call :error_environment & goto :exit
if "%MinGWDir%"==""   call :error_environment & goto :exit

:: Scan version from source
set Version=
set VersionFile="%SourceDir%\retroshare-gui\src\util\rsguiversion.h"

if not exist "%VersionFile%" (
	echo.
	echo Version file doesn't exist.
	echo %VersionFile%
	goto :exit
)

for /F "usebackq tokens=1,2,*" %%A in (%VersionFile%) do (
	if "%%A"=="#define" (
		if "%%B"=="GUI_VERSION" (
			set Version=%%~C
		)
	)
)

if "%Version%"=="" (
	echo.
	echo Version not found in
	echo %VersionFile%
	goto :exit
)

:: Create installer
"%NSIS_EXE%" "%~dp0retroshare.nsi"

:exit
endlocal

goto :EOF

:error_environment
echo.
echo Please set the needed environment variables.
