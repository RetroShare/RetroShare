@setlocal

@echo off

:: Initialize environment
call "%~dp0_env.bat"

if not exist "%MSYSPath%\bin\mingw-get.exe" exit /B 0

echo Update MSYS
pushd "%MSYSPath%\bin"
mingw-get.exe update
mingw-get.exe upgrade
popd

exit /B %ERRORLEVEL%
