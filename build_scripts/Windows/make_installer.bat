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

:: Build defines for script
set NSIS_PARAM=

if "%SourceDir%" NEQ ""  set NSIS_PARAM=%NSIS_PARAM% /DSOURCEDIR="%SourceDir%"
if "%ReleaseDir%" NEQ "" set NSIS_PARAM=%NSIS_PARAM% /DRELEASEDIR="%ReleaseDir%"
if "%QtDir%" NEQ ""      set NSIS_PARAM=%NSIS_PARAM% /DQTDIR="%QtDir%"
if "%MinGWDir%" NEQ ""   set NSIS_PARAM=%NSIS_PARAM% /DMINGWDIR="%MinGWDir%"
if "%OutDir%" NEQ ""     set NSIS_PARAM=%NSIS_PARAM% /DOUTDIR="%OutDir%"

:: Scan version from source
set BuildAdd=
set VersionFile="%SourceDir%\libretroshare\src\retroshare\rsversion.h"

if not exist "%VersionFile%" (
	echo.
	echo Version file doesn't exist.
	echo %VersionFile%
	goto :exit
)

for /F "usebackq tokens=1,2,3" %%A in (%VersionFile%) do (
	if "%%A"=="#define" (
		if "%%B"=="RS_BUILD_NUMBER_ADD" (
			set BuildAdd=%%~C
		)
	)
)

if "%BuildAdd%"=="" (
	echo.
	echo Version not found in
	echo %VersionFile%
	goto :exit
)

set NSIS_PARAM=%NSIS_PARAM% /DBUILDADD=%BuildAdd%

:: Create installer
"%NSIS_EXE%" %NSIS_PARAM% "%~dp0retroshare.nsi"

:exit
endlocal
