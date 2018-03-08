@echo off

setlocal

:: Initialize environment
call "%~dp0..\env.bat"
if errorlevel 1 goto error_env
call "%EnvPath%\env.bat"
if errorlevel 1 goto error_env

:: Get gcc versions
call "%ToolsPath%\get-gcc-version.bat" GCCVersion
if "%GCCVersion%"=="" echo Cannot get gcc version.& exit /B 1

:: Check external libraries
if not exist "%RootPath%\libs" echo Please build external libraries first.& exit /B 1

:: Check gcc version of external libraries
if not exist "%RootPath%\libs\gcc-version" echo Cannot get gcc version of external libraries.& exit /B 1
set /P LibsGCCVersion=<"%RootPath%\libs\gcc-version"
if "%LibsGCCVersion%" NEQ "%GCCVersion%" echo Please use correct version of external libraries. (gcc %GCCVersion% ^<^> libs %LibsGCCVersion%).& exit /B 1

:: Initialize environment
call "%~dp0env.bat" %*
if errorlevel 1 goto error_env

:: Check git executable
set GitPath=
call "%ToolsPath%\find-in-path.bat" GitPath git.exe
if "%GitPath%" NEQ "" goto found_git
choice /M "Git not found in PATH. Version information cannot be calculated. Do you want to proceed?"
if %errorlevel%==2 exit /B 1
:found_git

echo.
echo === Version
echo.

title Build - %SourceName%%RsType%-%RsBuildConfig% [Version]

pushd "%SourcePath%\retroshare-gui\src\gui\images"
:: Touch resource file
copy /b retroshare_win.rc +,,
popd

if not exist "%RsBuildPath%" mkdir "%RsBuildPath%"
pushd "%RsBuildPath%"

echo.
echo === qmake
echo.

title Build - %SourceName%%RsType%-%RsBuildConfig% [qmake]

set RS_QMAKE_CONFIG=%RsBuildConfig% version_detail_bash_script rs_autologin retroshare_plugins
if "%RsRetroTor%"=="1" set RS_QMAKE_CONFIG=%RS_QMAKE_CONFIG% retrotor

qmake "%SourcePath%\RetroShare.pro" -r "CONFIG+=%RS_QMAKE_CONFIG%"
if errorlevel 1 goto error

echo.
echo === make
echo.

title Build - %SourceName%%RsType%-%RsBuildConfig% [make]

if exist "%EnvJomExe%" (
	"%EnvJomExe%"
) else (
	mingw32-make
)

:error
popd

title %COMSPEC%

if errorlevel 1 echo.& echo Build failed& echo.
exit /B %ERRORLEVEL%

:error_env
echo Failed to initialize environment.
endlocal
exit /B 1
