:: Usage:
:: call env-qt4.bat version [reinstall|clean]

:: Initialize environment
call "%~dp0env.bat"
if errorlevel 1 goto error_env

set EnvQtBasePath=%EnvRootPath%\qt

:: Create folders
if not exist "%EnvQtBasePath%" mkdir "%EnvQtBasePath%"

call "%~dp0tools\prepare-qt.bat" %1 %2
if errorlevel 1 exit /B %ERRORLEVEL%

if "%MinGWDir%" NEQ "" set PATH=%MinGWDir%\bin;%PATH%
if "%QtDir%" NEQ "" set PATH=%QtDir%\bin;%PATH%

exit /B 0

:error_env
echo Failed to initialize environment.
exit /B 1
