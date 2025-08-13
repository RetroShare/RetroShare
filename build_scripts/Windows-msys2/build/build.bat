@echo off

setlocal

:: Initialize environment
call "%~dp0..\env.bat"
if errorlevel 1 goto error_env
call "%EnvPath%\env.bat"
if errorlevel 1 goto error_env
call "%EnvPath%\env-msys2.bat"
if errorlevel 1 goto error_env

:: Initialize base environment
call "%~dp0env-base.bat" %*
if errorlevel 2 exit /B 2
if errorlevel 1 goto error_env

title Build - %SourceName%-%RsBuildConfig% Qt-%QtVersion% %RsToolchain% [Prerequisites]

if not "%ParamNoupdate%"=="1" (
	:: Install needed things
	%EnvMSYS2Cmd% "pacman --noconfirm --needed -S make git mingw-w64-%RsMSYS2Architecture%-toolchain mingw-w64-%RsMSYS2Architecture%-qt%ParamQtVersion% mingw-w64-%RsMSYS2Architecture%-miniupnpc mingw-w64-%RsMSYS2Architecture%-sqlcipher mingw-w64-%RsMSYS2Architecture%-cmake mingw-w64-%RsMSYS2Architecture%-rapidjson"
	:: rnp
	%EnvMSYS2Cmd% "pacman --noconfirm --needed -S mingw-w64-%RsMSYS2Architecture%-json-c mingw-w64-%RsMSYS2Architecture%-libbotan"

	:: Webui
	if "%ParamWebui%"=="1" %EnvMSYS2Cmd% "pacman --noconfirm --needed -S mingw-w64-%RsMSYS2Architecture%-doxygen mingw-w64-%RsMSYS2Architecture%-asio"

	:: Plugins
	if "%ParamPlugins%"=="1" %EnvMSYS2Cmd% "pacman --noconfirm --needed -S mingw-w64-%RsMSYS2Architecture%-speex mingw-w64-%RsMSYS2Architecture%-speexdsp mingw-w64-%RsMSYS2Architecture%-curl mingw-w64-%RsMSYS2Architecture%-libxslt mingw-w64-%RsMSYS2Architecture%-opencv mingw-w64-%RsMSYS2Architecture%-ffmpeg"

	:: Clang
	if "%ClangCompiler%"=="1" %EnvMSYS2Cmd% "pacman --noconfirm --needed -S mingw-w64-%RsMSYS2Architecture%-clang"

	:: Indexing
	if "%ParamIndexing%"=="1" %EnvMSYS2Cmd% "pacman --noconfirm --needed -S mingw-w64-%RsMSYS2Architecture%-xapian-core mingw-w64-%RsMSYS2Architecture%-libvorbis mingw-w64-%RsMSYS2Architecture%-flac mingw-w64-%RsMSYS2Architecture%-taglib"
)

:: Fix webui compilation (TODO: remove when whole RS switched to cmake)
if "%ParamWebui%"=="1" (
	pushd "%SourcePath%"
	copy "%SourcePath%\libretroshare\src\jsonapi\jsonapi-generator-doxygen.conf" "%SourcePath%\jsonapi-generator\src\jsonapi-generator-doxygen.conf" %Quite%
	copy "%SourcePath%\libretroshare\src\jsonapi\async-method-wrapper-template.cpp.tmpl" "%SourcePath%\jsonapi-generator\src\async-method-wrapper-template.cpp.tmpl" %Quite%
	copy "%SourcePath%\libretroshare\src\jsonapi\method-wrapper-template.cpp.tmpl" "%SourcePath%\jsonapi-generator\src\method-wrapper-template.cpp.tmpl" %Quite%
	git update-index --assume-unchanged "jsonapi-generator\src\jsonapi-generator-doxygen.conf" "jsonapi-generator\src\async-method-wrapper-template.cpp.tmpl" "jsonapi-generator\src\method-wrapper-template.cpp.tmpl"
	popd
)

:: Initialize environment
call "%~dp0env.bat" %*
if errorlevel 2 exit /B 2
if errorlevel 1 goto error_env

echo.
echo === Version
echo.

title Build - %SourceName%-%RsBuildConfig% Qt-%QtVersion% %RsToolchain% [Version]

pushd "%SourcePath%\retroshare-gui\src\gui\images"
:: Touch resource file
copy /b retroshare_win.rc +,,
popd

if not exist "%RsBuildPath%" mkdir "%RsBuildPath%"
pushd "%RsBuildPath%"

echo.
echo === qmake
echo.

title Build - %SourceName%-%RsBuildConfig% Qt-%QtVersion% %RsToolchain% [qmake]

set RS_QMAKE_CONFIG=%RS_QMAKE_CONFIG% "CONFIG+=%RsBuildConfig%"
if "%ParamAutologin%"=="1" set RS_QMAKE_CONFIG=%RS_QMAKE_CONFIG% "CONFIG+=rs_autologin"
if "%ParamPlugins%"=="1" set RS_QMAKE_CONFIG=%RS_QMAKE_CONFIG% "CONFIG+=retroshare_plugins"

:: Dump the active build config into a file
echo %RS_QMAKE_CONFIG% > buildinfo.txt
echo %RsBuildConfig% >> buildinfo.txt
echo %RsArchitecture% >> buildinfo.txt
echo Qt %QtVersion% >> buildinfo.txt
echo %RsToolchain% >> buildinfo.txt

call "%ToolsPath%\msys2-path.bat" "%SourcePath%" MSYS2SourcePath
call "%ToolsPath%\msys2-path.bat" "%EnvMSYS2Path%" MSYS2EnvMSYS2Path
if "%ClangCompiler%"=="1" (
	%EnvMSYS2Cmd% "%QMakeCmd% "%MSYS2SourcePath%/RetroShare.pro" -r -spec win32-clang-g++ %RS_QMAKE_CONFIG%"
) else (
	%EnvMSYS2Cmd% "%QMakeCmd% "%MSYS2SourcePath%/RetroShare.pro" -r -spec win32-g++ %RS_QMAKE_CONFIG%"
)
if errorlevel 1 goto error

echo.
echo === make
echo.

title Build - %SourceName%-%RsBuildConfig% Qt-%QtVersion% %RsToolchain% [make]

%EnvMSYS2Cmd% "make -j %CoreCount%"
if errorlevel 1 goto error

:error
popd

title %COMSPEC%

if errorlevel 1 echo.& echo Build failed& echo.
exit /B %ERRORLEVEL%

:error_env
echo Failed to initialize environment.
endlocal
exit /B 1
