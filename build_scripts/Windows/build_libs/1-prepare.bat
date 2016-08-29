@setlocal

@echo off

:: Initialize environment
call "%~dp0_env.bat"

set SevenZipUrl=http://7-zip.org/a/7z1602.msi
set SevenZipInstall=7z1602.msi
set CurlUrl=https://bintray.com/artifact/download/vszakats/generic/curl-7.50.1-win32-mingw.7z
set CurlInstall=curl-7.50.1-win32-mingw.7z

if not exist "%DownloadPath%" mkdir "%DownloadPath%"

call :remove_dir "%TempPath%"

echo Download installation files
if not exist "%DownloadPath%\%SevenZipInstall%" call "%ToolsPath%\winhttpjs.bat" %SevenZipUrl% -saveTo "%DownloadPath%\%SevenZipInstall%"
if not exist "%DownloadPath%\%SevenZipInstall%" echo Cannot download 7z& goto :exit

if not exist "%DownloadPath%\%CurlInstall%" call "%ToolsPath%\winhttpjs.bat" %CurlUrl% -saveTo "%DownloadPath%\%CurlInstall%"
if not exist "%DownloadPath%\%CurlInstall%" echo Cannot download Curl& goto :exit

echo Unpack 7z
msiexec /a "%DownloadPath%\%SevenZipInstall%" /qb TARGETDIR="%TempPath%"
copy "%TempPath%\Files\7-Zip\7z.dll" "%ToolsPath%"
copy "%TempPath%\Files\7-Zip\7z.exe" "%ToolsPath%"
call :remove_dir "%TempPath%"

echo Unpack Curl
"%SevenZipExe%" x -o"%TempPath%" "%DownloadPath%\%CurlInstall%"
copy "%TempPath%\curl-7.50.1-win32-mingw\bin\curl.exe" "%ToolsPath%"
call :remove_dir "%TempPath%"

:exit
endlocal
exit /B 0

:remove_dir
if not exist %1 goto :EOF
del /s /f /q %1 >nul
rmdir /s /q %1
goto :EOF
