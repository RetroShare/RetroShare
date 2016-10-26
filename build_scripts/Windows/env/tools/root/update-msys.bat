@echo off

setlocal

set MSYSPath=%~dp0msys

if not exist "%MSYSPath%\bin\mingw-get.exe" echo MSYS is not installed& exit /B 0

echo Update MSYS
pushd "%MSYSPath%\bin"
mingw-get.exe update
mingw-get.exe upgrade
popd

exit /B %ERRORLEVEL%
