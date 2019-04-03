:: Usage:
:: call qt-cmd.bat <Qt version> [command]

@echo off

setlocal

set QtVersion=%~1

:: Initialize environment
call "%~dp0env.bat"
if errorlevel 1 goto error_env
call "%EnvPath%\env-qt.bat" %QtVersion%
if errorlevel 1 goto error_env

if "%~2"=="" (
	"%ComSpec%"
) else (
	"%ComSpec%" /c %2 %3 %4 %5 %6 %7 %8 %9
)

exit /B %ERRORLEVEL%

:error_env
echo Failed to initialize environment.
endlocal
exit /B 1
