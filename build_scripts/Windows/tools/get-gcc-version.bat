:: Usage:
:: call get-gcc-version.bat variable

setlocal

set Var=%~1
if "%Var%"=="" (
	echo.
	echo Parameter error.
	exit /B 1
)

set GCCVersion=

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
		goto exit
	)
)

:exit
if exist "%~dp0gccversion.tmp" del /Q "%~dp0gccversion.tmp"

endlocal & set %Var%=%GCCVersion%
goto :EOF

:find_version
:loop
if "%2" NEQ "" (
	shift
	goto loop
)
set GCCVersion=%1
