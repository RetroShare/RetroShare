:: Usage:
:: call build-libs.bat [make tasks]

@echo off

setlocal

:: Parameter
set MakeParam="DOWNLOAD_PATH=../../download"

set MakeTask=
:param_loop
if "%~1" NEQ "" (
		set MakeTask=%MakeTask% %1
		shift /1
		goto param_loop
)

:: Initialize environment
call "%~dp0..\env.bat"
if errorlevel 1 goto error_env
call "%EnvPath%\env-msys2.bat"
if errorlevel 1 goto error_env

:: Check MSYS environment
if not exist "%EnvMSYS2SH%" %cecho% error "Please install MSYS2 first." & exit /B 1

:: Initialize environment
call "%~dp0env.bat"
if errorlevel 1 goto error_env

call "%ToolsPath%\msys2-path.bat" "%~dp0" MSYS2CurPath
call "%ToolsPath%\msys2-path.bat" "%BuildLibsPath%" MSYS2BuildLibsPath

if not exist "%BuildLibsPath%" mkdir "%BuildLibsPath%"

set MSYSTEM=MINGW%MSYS2Base%
set MSYS2_PATH_TYPE=inherit

%EnvMSYS2Cmd% "pacman --needed --noconfirm -S diffutils perl tar make wget mingw-w64-%MSYS2Architecture%-make"
::mingw-w64-%MSYS2Architecture%-cmake
::%EnvMSYS2Cmd% "pacman --noconfirm -Rd --nodeps mingw-w64-%MSYS2Architecture%-zlib"

%EnvMSYS2Cmd% "cd "%MSYS2BuildLibsPath%" && make -f %MSYS2CurPath%/makefile %MakeParam% %MakeTask%"

exit /B %ERRORLEVEL%

:error_env
echo Failed to initialize environment.
endlocal
exit /B 1
