@echo off

setlocal

:: Modify variable when makensis.exe doesn't exist in PATH
set NSIS_EXE=makensis.exe

:: Needed environment variables
set SourceDir=%~dp0..\..
::set ReleaseDir=
::set QtDir=
::set MinGWDir=

:: Optional environment variables
::set OutDir=
::set Revision=

:: Build defines for script
set NSIS_PARAM=

if "%SourceDir%" NEQ ""  set NSIS_PARAM=%NSIS_PARAM% /DSOURCEDIR="%SourceDir%"
if "%ReleaseDir%" NEQ "" set NSIS_PARAM=%NSIS_PARAM% /DRELEASEDIR="%ReleaseDir%"
if "%QtDir%" NEQ ""      set NSIS_PARAM=%NSIS_PARAM% /DQTDIR="%QtDir%"
if "%MinGWDir%" NEQ ""   set NSIS_PARAM=%NSIS_PARAM% /DMINGWDIR="%MinGWDir%"
if "%OutDir%" NEQ ""     set NSIS_PARAM=%NSIS_PARAM% /DOUTDIR="%OutDir%"
if "%Revision%" NEQ ""   set NSIS_PARAM=%NSIS_PARAM% /DREVISION="%Revision%"

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

set NSIS_PARAM=%NSIS_PARAM% /DVERSION=%Version%

:: Create installer
"%NSIS_EXE%" %NSIS_PARAM% "%~dp0retroshare.nsi"

:exit
endlocal
