@echo off

setlocal

:: Initialize environment
call "%~dp0..\env.bat"
if errorlevel 1 goto error_env
call "%EnvPath%\env.bat"
if errorlevel 1 goto error_env
call "%EnvPath%\env.bat"
if errorlevel 1 goto error_env
call "%EnvPath%\env-msys2.bat"
if errorlevel 1 goto error_env

:: Initialize base environment
call "%~dp0env-base.bat" %*
if errorlevel 2 exit /B 2
if errorlevel 1 goto error_env

:: Install needed things
%EnvMSYS2Cmd% "pacman --noconfirm --needed -S make git mingw-w64-%RsMSYS2Architecture%-toolchain mingw-w64-%RsMSYS2Architecture%-qt5 mingw-w64-%RsMSYS2Architecture%-miniupnpc mingw-w64-%RsMSYS2Architecture%-sqlcipher mingw-w64-%RsMSYS2Architecture%-xapian-core mingw-w64-%RsMSYS2Architecture%-cmake mingw-w64-%RsMSYS2Architecture%-rapidjson"

:: Plugins
if "%ParamPlugins%"=="1" %EnvMSYS2Cmd% "pacman --noconfirm --needed -S mingw-w64-%RsMSYS2Architecture%-speex mingw-w64-%RsMSYS2Architecture%-speexdsp mingw-w64-%RsMSYS2Architecture%-curl mingw-w64-%RsMSYS2Architecture%-libxslt mingw-w64-%RsMSYS2Architecture%-opencv mingw-w64-%RsMSYS2Architecture%-ffmpeg"

:: Initialize environment
call "%~dp0env.bat" %*
if errorlevel 2 exit /B 2
if errorlevel 1 goto error_env

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

set RS_QMAKE_CONFIG=%RS_QMAKE_CONFIG% "CONFIG+=%RsBuildConfig%"
if "%ParamAutologin%"=="1" set RS_QMAKE_CONFIG=%RS_QMAKE_CONFIG% "CONFIG+=rs_autologin"
if "%ParamPlugins%"=="1" set RS_QMAKE_CONFIG=%RS_QMAKE_CONFIG% "CONFIG+=retroshare_plugins"

:: Dump the active build config into a file
echo %RS_QMAKE_CONFIG% > buildinfo.txt
echo %RsBuildConfig% >> buildinfo.txt
echo %RsArchitecture% >> buildinfo.txt
echo Qt %QtVersion% >> buildinfo.txt

call "%ToolsPath%\msys2-path.bat" "%SourcePath%" MSYS2SourcePath
call "%ToolsPath%\msys2-path.bat" "%EnvMSYS2Path%" MSYS2EnvMSYS2Path
%EnvMSYS2Cmd% "qmake "%MSYS2SourcePath%/RetroShare.pro" -r -spec win32-g++ %RS_QMAKE_CONFIG%"
if errorlevel 1 goto error

echo.
echo === make
echo.

title Build - %SourceName%-%RsBuildConfig% [make]

%EnvMSYS2Cmd% "make -j %CoreCount%"

:: Webui
if "%ParamWebui%"=="1" (
	call "%~dp0..\tools\webui.bat"
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
