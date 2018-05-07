:: Usage:
:: call prepare-msys.bat [reinstall|clean]

setlocal enabledelayedexpansion

if "%EnvMSYSPath%"=="" exit /B 1
if not exist "%EnvRootPath%"=="" exit /B 1

copy "%~dp0root\update-msys.bat" "%EnvRootPath%" >nul

if "%~1"=="clean" (
	%cecho% info "Clean MSYS"
	call "%ToolsPath%\remove-dir.bat" "%EnvMSYSPath%"
	goto exit
)

if exist "%EnvMSYSPath%\bin\mingw-get.exe" (
	if "%~1"=="reinstall" (
		choice /M "Found existing MSYS version. Do you want to proceed?"
		if !ERRORLEVEL!==2 goto exit
	) else (
		goto exit
	)
)

set MSYSInstall=mingw-get-0.6.2-mingw32-beta-20131004-1-bin.zip
set MSYSUrl=http://sourceforge.net/projects/mingw/files/Installer/mingw-get/mingw-get-0.6.2-beta-20131004-1/%MSYSInstall%/download
set CMakeInstall=cmake-3.1.0-win32-x86.zip
set CMakeUrl=http://www.cmake.org/files/v3.1/%CMakeInstall%
set CMakeUnpackPath=%EnvMSYSPath%\msys\1.0

%cecho% info "Remove previous MSYS version"
call "%ToolsPath%\remove-dir.bat" "%EnvMSYSPath%"

%cecho% info "Download installation files"
if not exist "%EnvDownloadPath%\%MSYSInstall%" call "%ToolsPath%\download-file.bat" "%MSYSUrl%" "%EnvDownloadPath%\%MSYSInstall%"
if not exist "%EnvDownloadPath%\%MSYSInstall%" %cecho% error "Cannot download MSYS" & goto error

if not exist "%EnvDownloadPath%\%CMakeInstall%" call "%ToolsPath%\download-file.bat" "%CMakeUrl%" "%EnvDownloadPath%\%CMakeInstall%"
if not exist "%EnvDownloadPath%\%CMakeInstall%" %cecho% error "Cannot download CMake" & goto error

%cecho% info "Unpack MSYS"
"%EnvSevenZipExe%" x -o"%EnvMSYSPath%" "%EnvDownloadPath%\%MSYSInstall%"

%cecho% info "Install MSYS"
if not exist "%EnvMSYSPath%\var\lib\mingw-get\data\profile.xml" copy "%EnvMSYSPath%\var\lib\mingw-get\data\defaults.xml" "%EnvMSYSPath%\var\lib\mingw-get\data\profile.xml"
pushd "%EnvMSYSPath%\bin"
mingw-get.exe install mingw32-mingw-get
mingw-get.exe install msys-coreutils
mingw-get.exe install msys-base
mingw-get.exe install msys-autoconf
mingw-get.exe install msys-automake
mingw-get.exe install msys-autogen
mingw-get.exe install msys-mktemp
rem Use own wget binary, because MSYS version of wget is to old
rem mingw-get.exe install msys-wget
popd

%cecho% info "Unpack CMake"
"%EnvSevenZipExe%" x -o"%CMakeUnpackPath%" "%EnvDownloadPath%\%CMakeInstall%"

%cecho% info "Install CMake"
set CMakeVersion=
for /D %%F in (%CMakeUnpackPath%\cmake*) do set CMakeVersion=%%~nxF
if "%CMakeVersion%"=="" %cecho% error "CMake version not found." & goto :exit
%cecho% info "Found CMake version %CMakeVersion%"

set FoundProfile=
for /f "tokens=3" %%F in ('find /c /i "%CMakeVersion%" "%EnvMSYSPath%\msys\1.0\etc\profile"') do set FoundProfile=%%F

if "%FoundProfile%"=="0" (
	echo export PATH="${PATH}:/%CMakeVersion%/bin">>"%EnvMSYSPath%\msys\1.0\etc\profile"
)

:exit
endlocal
exit /B 0

:error
endlocal
exit /B 1
