:: Usage:
:: call env-msys.bat [reinstall|clean]

:: Initialize environment
call "%~dp0env.bat"
if errorlevel 1 goto error_env

set EnvMSYSPath=%EnvRootPath%\msys

call "%~dp0tools\prepare-msys.bat" %1
if errorlevel 1 exit /B %ERRORLEVEL%

set EnvMSYSSH=%EnvMSYSPath%\msys\1.0\bin\sh.exe
if not exist "%EnvMSYSSH%" if errorlevel 1 goto error_env

set EnvMSYSCmd="%EnvMSYSSH%" --login -i -c

exit /B 0

:error_env
echo Failed to initialize environment.
exit /B 1
