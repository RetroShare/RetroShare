@echo off

setlocal

:: Initialize environment
call "%~dp0..\env.bat"
if errorlevel 1 goto error_env
call "%EnvPath%\env.bat"
if errorlevel 1 goto error_env

:: Initialize environment
call "%~dp0env.bat" %*
if errorlevel 2 exit /B 2
if errorlevel 1 goto error_env

:: Check external libraries
if not exist "%BuildLibsPath%\libs" %cecho% error "Please build external libraries first." & exit /B 1

:: Check gcc version of external libraries
if not exist "%BuildLibsPath%\libs\gcc-version" %cecho% error "Cannot get gcc version of external libraries." & exit /B 1
set /P LibsGCCVersion=<"%BuildLibsPath%\libs\gcc-version"
if "%LibsGCCVersion%" NEQ "%GCCVersion%" %cecho% error "Please use correct version of external libraries. (gcc %GCCVersion% ^<^> libs %LibsGCCVersion%)." & exit /B 1

:: Check git executable
set GitPath=
call "%ToolsPath%\find-in-path.bat" GitPath git.exe
if "%GitPath%"=="" (
	%cecho% error "Git not found in PATH. Version information cannot be determined."
	exit /B 1
)

echo.
echo === Version
echo.

title Build - %SourceName%-%RsBuildConfig% [Version]

pushd "%SourcePath%\retroshare-gui\src\gui\images"
:: Touch resource file
copy /b retroshare_win.rc +,,
popd

if not exist "%RsBuildPath%" mkdir "%RsBuildPath%"
pushd "%RsBuildPath%"

echo.
echo === qmake
echo.

title Build - %SourceName%-%RsBuildConfig% [qmake]

set RS_QMAKE_CONFIG=%RsBuildConfig%
if "%ParamAutologin%"=="1" set RS_QMAKE_CONFIG=%RS_QMAKE_CONFIG% rs_autologin
if "%ParamJsonApi%"=="1" set RS_QMAKE_CONFIG=%RS_QMAKE_CONFIG% rs_jsonapi
if "%ParamPlugins%"=="1" set RS_QMAKE_CONFIG=%RS_QMAKE_CONFIG% retroshare_plugins

qmake "%SourcePath%\RetroShare.pro" -r -spec win32-g++ "CONFIG+=%RS_QMAKE_CONFIG%" "EXTERNAL_LIB_DIR=%BuildLibsPath%\libs"
if errorlevel 1 goto error

echo.
echo === make
echo.

title Build - %SourceName%-%RsBuildConfig% [make]

if exist "%EnvJomExe%" (
	"%EnvJomExe%"
) else (
	mingw32-make
)
if errorlevel 1 goto error

echo.
echo === Changelog
echo.

title Build - %SourceName%-%RsBuildConfig% [changelog]
call "%ToolsPath%\generate-changelog.bat" "%SourcePath%" "%RsBuildPath%\changelog.txt"

:error
popd

title %COMSPEC%

if errorlevel 1 %cecho% error "\nBuild failed\n"
exit /B %ERRORLEVEL%

:error_env
echo Failed to initialize environment.
endlocal
exit /B 1
