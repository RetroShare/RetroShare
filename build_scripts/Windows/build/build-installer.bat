@echo off

setlocal

:: Initialize environment
call "%~dp0..\env.bat"
if errorlevel 1 goto error_env
call "%EnvPath%\env.bat"
if errorlevel 1 goto error_env

:: Initialize environment
call "%~dp0env.bat" release
if errorlevel 2 exit /B 2
if errorlevel 1 goto error_env

:: Check external libraries
if not exist "%BuildLibsPath%\libs" %cecho% error "Please build external libraries first." & exit /B 1

:: Check gcc version of external libraries
if not exist "%BuildLibsPath%\libs\gcc-version" %cecho% error "Cannot get gcc version of external libraries." & exit /B 1
set /P LibsGCCVersion=<"%BuildLibsPath%\libs\gcc-version"
if "%LibsGCCVersion%" NEQ "%GCCVersion%" %cecho% error "Please use correct version of external libraries. (gcc %GCCVersion% ^<^> libs %LibsGCCVersion%)." & exit /B 1

:: Build defines for script
set NSIS_PARAM=

set NSIS_PARAM=%NSIS_PARAM% /DRELEASEDIR="%RsBuildPath%"
set NSIS_PARAM=%NSIS_PARAM% /DQTDIR="%QtPath%\.."
set NSIS_PARAM=%NSIS_PARAM% /DMINGWDIR="%MinGWPath%\.."
set NSIS_PARAM=%NSIS_PARAM% /DOUTDIR="%RsPackPath%"
set NSIS_PARAM=%NSIS_PARAM% /DINSTALLERADD="%RsArchiveAdd%"
set NSIS_PARAM=%NSIS_PARAM% /DEXTERNAL_LIB_DIR="%BuildLibsPath%\libs"

:: Get compiled version
call "%ToolsPath%\get-rs-version.bat" "%RsBuildPath%\retroshare-gui\src\%RsBuildConfig%\retroshare.exe" RsVersion
if errorlevel 1 %cecho% error "Version not found."& exit /B 1

if "%RsVersion.Extra%"=="" %cecho% error "Extra number not found".& exit /B 1

set NSIS_PARAM=%NSIS_PARAM% /DREVISION=%RsVersion.Extra%

set QtMainVersion=%QtVersion:~0,1%

:: Create installer
"%EnvMakeNSISExe%" %NSIS_PARAM% "%SourcePath%\build_scripts\Windows\installer\retroshare-Qt%QtMainVersion%.nsi"

exit /B %ERRORLEVEL%

:error_env
echo Failed to initialize environment.
endlocal
exit /B 1
