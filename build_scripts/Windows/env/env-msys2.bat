:: Usage:
:: call env-msys2.bat [reinstall|clean]

:: Initialize environment
call "%~dp0env.bat"
if errorlevel 1 goto error_env

:: Get gcc versions
call "%ToolsPath%\get-gcc-version.bat" GCCVersion GCCArchitecture
if "%GCCVersion%"=="" %cecho% error "Cannot get gcc version." & exit /B 1
if "%GCCArchitecture%"=="" %cecho% error "Cannot get gcc architecture." & exit /B 1

rem IF DEFINED ProgramFiles(x86) (
if "%GCCArchitecture%"=="x64" (
	:: x64
	set MSYS2Architecture=x86_64
	set MSYS2Base=64
) else (
	:: x86
	set MSYS2Architecture=i686
	set MSYS2Base=32
)

set EnvMSYS2Path=%EnvRootPath%\msys2

call "%~dp0tools\prepare-msys2.bat" %1
if errorlevel 1 exit /B %ERRORLEVEL%

set EnvMSYS2SH=%EnvMSYS2Path%\msys64\usr\bin\sh.exe
if not exist "%EnvMSYS2SH%" if errorlevel 1 goto error_env

set EnvMSYS2Cmd="%EnvMSYS2SH%" -lc

exit /B 0

:error_env
echo Failed to initialize environment.
exit /B 1
