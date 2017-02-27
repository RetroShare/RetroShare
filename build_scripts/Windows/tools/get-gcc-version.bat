:: Usage:
:: call get-gcc-version.bat version architecture

setlocal

set VarVersion=%~1
if "%VarVersion%"=="" (
	echo.
	echo Parameter error.
	exit /B 1
)

set VarArchitecture=%~2
if "%VarArchitecture%"=="" (
	echo.
	echo Parameter error.
	exit /B 1
)

set GCCVersion=
set GCCArchitecture=

call "%~dp0find-in-path.bat" GCCPath gcc.exe
if "%GCCPath%"=="" (
	echo.
	echo Cannot find gcc.exe in PATH.
	goto exit
)

gcc --version >"%~dp0gccversion.tmp"
for /F "tokens=1*" %%A in (%~sdp0gccversion.tmp) do (
	if "%%A"=="gcc" (
		call :find_version %%B
		call :find_architecture %%B
		goto exit
	)
)

:exit
if exist "%~dp0gccversion.tmp" del /Q "%~dp0gccversion.tmp"

endlocal & set %VarVersion%=%GCCVersion%& set %VarArchitecture%=%GCCArchitecture%
goto :EOF

:find_version
:loop_version
if "%2" NEQ "" (
	shift
	goto loop_version
)
set GCCVersion=%1
goto :EOF

:find_architecture
:loop_architecture
if "%7" NEQ "" (
	shift
	goto loop_architecture
)
set _Architecture=%1
if "%_Architecture:~1,4%"=="i686" set GCCArchitecture=x86
if "%_Architecture:~1,6%"=="x86_64" set GCCArchitecture=x64
goto :EOF
