@setlocal

@echo off

:: Initialize environment
call "%~dp0_env.bat"

set MSYSInstall=mingw-get-0.6.2-mingw32-beta-20131004-1-bin.zip
set CMakeInstall=cmake-3.1.0-win32-x86.zip
set CMakeUnpackPath=%MSYSPath%\msys\1.0

if not exist "%DownloadPath%" mkdir "%DownloadPath%"

echo Check existing installation
if not exist "%MSYSPath%\bin\mingw-get.exe" goto proceed
choice /M "Found existing MSYS version. Do you want to proceed?"
if %ERRORLEVEL%==2 goto exit

:proceed
echo Remove previous MSYS version
call :remove_dir "%MSYSPath%"

echo Download installation files
if not exist "%DownloadPath%\%MSYSInstall%" "%CurlExe%" -L -k http://sourceforge.net/projects/mingw/files/Installer/mingw-get/mingw-get-0.6.2-beta-20131004-1/%MSYSInstall%/download -o "%DownloadPath%\%MSYSInstall%"
if not exist "%DownloadPath%%\MSYSInstall%" echo Cannot download MSYS& goto :exit

if not exist "%DownloadPath%\%CMakeInstall%" "%CurlExe%" -L -k http://www.cmake.org/files/v3.1/cmake-3.1.0-win32-x86.zip -o "%DownloadPath%\%CMakeInstall%"
if not exist "%DownloadPath%\%CMakeInstall%" echo Cannot download CMake& goto :exit

echo Unpack MSYS
"%SevenZipExe%" x -o"%MSYSPath%" "%DownloadPath%\%MSYSInstall%"

echo Install MSYS
if not exist "%MSYSPath%\var\lib\mingw-get\data\profile.xml" copy "%MSYSPath%\var\lib\mingw-get\data\defaults.xml" "%MSYSPath%\var\lib\mingw-get\data\profile.xml"
pushd "%MSYSPath%\bin"
mingw-get.exe install mingw32-mingw-get
mingw-get.exe install msys-coreutils
mingw-get.exe install msys-base
mingw-get.exe install msys-autoconf
mingw-get.exe install msys-automake
mingw-get.exe install msys-autogen
mingw-get.exe install msys-mktemp
mingw-get.exe install msys-wget
popd

echo Unpack CMake
"%SevenZipExe%" x -o"%CMakeUnpackPath%" "%DownloadPath%\%CMakeInstall%"

echo Install CMake
set CMakeVersion=
for /D %%F in (%CMakeUnpackPath%\cmake*) do set CMakeVersion=%%~nxF
if "%CMakeVersion%"=="" echo CMake version not found.& goto :exit
echo Found CMake version %CMakeVersion%

set FoundProfile=
for /f "tokens=3" %%F in ('find /c /i "%CMakeVersion%" "%MSYSPath%\msys\1.0\etc\profile"') do set FoundProfile=%%F

if "%FoundProfile%"=="0" (
	echo export PATH="${PATH}:/%CMakeVersion%/bin">>"%MSYSPath%\msys\1.0\etc\profile"
)

:exit
endlocal
exit /B 0

:remove_dir
if not exist %1 goto :EOF
del /s /f /q %1 >nul
rmdir /s /q %1
goto :EOF
