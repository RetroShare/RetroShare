:: Usage:
:: call env-msys2.bat [reinstall|clean]

:: Initialize environment
call "%~dp0env.bat"
if errorlevel 1 goto error_env

IF DEFINED ProgramFiles(x86) (
	:: x64
	set MSYS2Architecture=x86_64
	set MSYS2Base=64
) else (
	:: x86
	set MSYS2Architecture=i686
	set MSYS2Base=32
)

set CHERE_INVOKING=1 

set EnvMSYS2Path=%EnvRootPath%\msys2
set EnvMSYS2BasePath=%EnvMSYS2Path%\msys%MSYS2Base%

call "%~dp0tools\prepare-msys2.bat" %1
if errorlevel 1 exit /B %ERRORLEVEL%

set EnvMSYS2SH=%EnvMSYS2BasePath%\usr\bin\sh.exe
if not exist "%EnvMSYS2SH%" if errorlevel 1 goto error_env

set EnvMSYS2Cmd="%EnvMSYS2SH%" -lc

set PATH=%EnvMSYS2BasePath%\usr\bin;%PATH%

exit /B 0

:error_env
echo Failed to initialize environment.
exit /B 1
