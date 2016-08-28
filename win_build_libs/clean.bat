@setlocal

@echo off

:: Initialize environment
call "%~dp0_env.bat"

call :remove_dir "%LibsPath%"

call :remove_file "%CurPath%bzip2"
call :remove_file "%CurPath%curl"
call :remove_file "%CurPath%ffmpeg"
call :remove_file "%CurPath%libmicrohttpd"
call :remove_file "%CurPath%libxml2"
call :remove_file "%CurPath%libxslt"
call :remove_file "%CurPath%miniupnpc"
call :remove_file "%CurPath%opencv"
call :remove_file "%CurPath%openssl"
call :remove_file "%CurPath%speex"
call :remove_file "%CurPath%speexdsp"
call :remove_file "%CurPath%sqlcipher"
call :remove_file "%CurPath%zlib"

endlocal
exit /B 0

:remove_dir
if not exist %1 goto :EOF
del /s /f /q %1 >nul
rmdir /s /q %1
goto :EOF

:remove_file
if not exist %1 goto :EOF
del /q %1 >nul
goto :EOF
