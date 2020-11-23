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

if exist "%EnvMSYS2Path%\msys%MSYS2Base%\usr\bin\pacman.exe" (
	if "%~1"=="reinstall" (
		choice /M "Found existing MSYS2 version. Do you want to proceed?"
		if !ERRORLEVEL!==2 goto exit
	) else (
		goto exit
	)
)

set MSYS2Install=msys2-base-%MSYS2Architecture%-20190524.tar.xz
set MSYS2Url=http://sourceforge.net/projects/msys2/files/Base/%MSYS2Architecture%/%MSYS2Install%/download
set CMakeInstall=cmake-3.1.0-win32-x86.zip
set CMakeUrl=http://www.cmake.org/files/v3.1/%CMakeInstall%
set CMakeUnpackPath=%EnvMSYS2Path%\msys%MSYS2Base%

if exist "%EnvMSYS2Path%\msys%MSYS2Base%" (
	%cecho% info "Remove previous MSYS2 version"
	call "%ToolsPath%\remove-dir.bat" "%EnvMSYS2Path%\msys%MSYS2Base%"
)

%cecho% info "Download installation files"
if not exist "%EnvDownloadPath%\%MSYS2Install%" call "%ToolsPath%\download-file.bat" "%MSYS2Url%" "%EnvDownloadPath%\%MSYS2Install%"
if not exist "%EnvDownloadPath%\%MSYS2Install%" %cecho% error "Cannot download MSYS" & goto error

if not exist "%EnvDownloadPath%\%CMakeInstall%" call "%ToolsPath%\download-file.bat" "%CMakeUrl%" "%EnvDownloadPath%\%CMakeInstall%"
if not exist "%EnvDownloadPath%\%CMakeInstall%" %cecho% error "Cannot download CMake" & goto error

%cecho% info "Unpack MSYS2"
"%EnvSevenZipExe%" x -so "%EnvDownloadPath%\%MSYS2Install%" | "%EnvSevenZipExe%" x -y -si -ttar -o"%EnvMSYS2Path%"

%cecho% info "Unpack CMake"
"%EnvSevenZipExe%" x -o"%CMakeUnpackPath%" "%EnvDownloadPath%\%CMakeInstall%"

%cecho% info "Install CMake"
set CMakeVersion=
for /D %%F in (%CMakeUnpackPath%\cmake*) do set CMakeVersion=%%~nxF
if "%CMakeVersion%"=="" %cecho% error "CMake version not found." & goto :exit
%cecho% info "Found CMake version %CMakeVersion%"

set FoundProfile=
for /f "tokens=3" %%F in ('find /c /i "%CMakeVersion%" "%EnvMSYS2Path%\msys%MSYS2Base%\etc\profile"') do set FoundProfile=%%F

if "%FoundProfile%"=="0" (
	echo export PATH="${PATH}:/%CMakeVersion%/bin">>"%EnvMSYS2Path%\msys%MSYS2Base%\etc\profile"
)

set MSYS2SH=%EnvMSYS2Path%\msys%MSYS2Base%\usr\bin\sh

%cecho% info "Update keyring"
"%MSYS2SH%" -lc "curl -O http://repo.msys2.org/msys/x86_64/msys2-keyring-r21.b39fb11-1-any.pkg.tar.xz"
"%MSYS2SH%" -lc "pacman --noconfirm -U msys2-keyring-r21.b39fb11-1-any.pkg.tar.xz"

%cecho% info "Initialize MSYS2"
"%MSYS2SH%" -lc "pacman -Sy"
"%MSYS2SH%" -lc "pacman --noconfirm --needed -S bash pacman pacman-mirrors msys2-runtime"

call "%EnvMSYS2Path%\msys%MSYS2Base%\autorebase.bat"
call "%EnvRootPath%\update-msys2.bat"
call "%EnvRootPath%\update-msys2.bat"

:exit
endlocal
exit /B 0

:error
endlocal
exit /B 1
