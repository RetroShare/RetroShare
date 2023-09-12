:: Usage:
:: call prepare-msys2.bat [reinstall|clean]

setlocal enabledelayedexpansion

if "%EnvMSYS2Path%"=="" exit /B 1
if "%MSYS2Architecture%"=="" exit /B 1
if "%MSYS2Base%"=="" exit /B 1
if not exist "%EnvRootPath%"=="" exit /B 1

copy "%~dp0root\update-msys2.bat" "%EnvRootPath%" >nul

if "%~1"=="clean" (
	%cecho% info "Clean MSYS2"
	call "%ToolsPath%\remove-dir.bat" "%EnvMSYS2Path%"
	goto exit
)

set MSYS2Version=20230318

set MSYS2Install=msys2-base-x86_64-%MSYS2Version%.sfx.exe
set MSYS2Url=https://github.com/msys2/msys2-installer/releases/download/%MSYS2Version:~0,4%-%MSYS2Version:~4,2%-%MSYS2Version:~6,2%/%MSYS2Install%
set CMakeInstall=cmake-3.19.0-win32-x86.zip
set CMakeUrl=https://github.com/Kitware/CMake/releases/download/v3.19.0/%CMakeInstall%
set CMakeUnpackPath=%EnvMSYS2Path%\msys64

if exist "%CMakeUnpackPath%\usr\bin\pacman.exe" (
	if "%~1"=="reinstall" (
		choice /M "Found existing MSYS2 version. Do you want to proceed?"
		if !ERRORLEVEL!==2 goto exit
	) else (
		goto exit
	)
)


if exist "%CMakeUnpackPath%" (
	%cecho% info "Remove previous MSYS2 version"
	call "%ToolsPath%\remove-dir.bat" "%CMakeUnpackPath%"
)

%cecho% info "Download installation files"
if not exist "%EnvDownloadPath%\%MSYS2Install%" call "%ToolsPath%\download-file.bat" "%MSYS2Url%" "%EnvDownloadPath%\%MSYS2Install%"
if not exist "%EnvDownloadPath%\%MSYS2Install%" %cecho% error "Cannot download MSYS" & goto error

if not exist "%EnvDownloadPath%\%CMakeInstall%" call "%ToolsPath%\download-file.bat" "%CMakeUrl%" "%EnvDownloadPath%\%CMakeInstall%"
if not exist "%EnvDownloadPath%\%CMakeInstall%" %cecho% error "Cannot download CMake" & goto error

%cecho% info "Unpack MSYS2"
"%EnvDownloadPath%\%MSYS2Install%" -y -o"%EnvMSYS2Path%"

%cecho% info "Unpack CMake"
"%EnvSevenZipExe%" x -o"%CMakeUnpackPath%" "%EnvDownloadPath%\%CMakeInstall%"

%cecho% info "Install CMake"
set CMakeVersion=
for /D %%F in (%CMakeUnpackPath%\cmake*) do set CMakeVersion=%%~nxF
if "%CMakeVersion%"=="" %cecho% error "CMake version not found." & goto :exit
%cecho% info "Found CMake version %CMakeVersion%"

set FoundProfile=
for /f "tokens=3" %%F in ('find /c /i "%CMakeVersion%" "%CMakeUnpackPath%\etc\profile"') do set FoundProfile=%%F

if "%FoundProfile%"=="0" (
	echo export PATH="${PATH}:/%CMakeVersion%/bin">>"%CMakeUnpackPath%\etc\profile"
)

set MSYS2SH=%CMakeUnpackPath%\usr\bin\sh

%cecho% info "Initialize MSYS2"
"%MSYS2SH%" -lc "yes | pacman --noconfirm -Syuu msys2-keyring"
"%MSYS2SH%" -lc "pacman --noconfirm -Sy"
"%MSYS2SH%" -lc "pacman --noconfirm -Su"

call "%CMakeUnpackPath%\autorebase.bat"

:exit
endlocal
exit /B 0

:error
endlocal
exit /B 1
