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
set Revision=
set BuildAdd=
call "%~dp0GetRsVersion.bat" RS_REVISION_STRING Revision
if errorlevel 1 goto exit
call "%~dp0GetRsVersion.bat" RS_BUILD_NUMBER_ADD BuildAdd
if errorlevel 1 goto exit

if "%Revision%"=="" (
	echo.
	echo Version not found
	goto exit
)

set NSIS_PARAM=%NSIS_PARAM% /DREVISION=%Revision% /DBUILDADD=%BuildAdd%

:: Create installer
"%NSIS_EXE%" %NSIS_PARAM% "%~dp0retroshare.nsi"

:exit
endlocal
