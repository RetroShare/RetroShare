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
set _Architecture=

call "%~dp0find-in-path.bat" GCCPath gcc.exe
if "%GCCPath%"=="" (
	echo.
	echo Cannot find gcc.exe in PATH.
	exit /B 1
)

for /F "tokens=1-8* delims= " %%A in ('gcc --version') do if "%%A"=="gcc" set _Architecture=%%B& set GCCVersion=%%G

if "%_Architecture:~1,4%"=="i686" set GCCArchitecture=x86
if "%_Architecture:~1,6%"=="x86_64" set GCCArchitecture=x64

endlocal & set %VarVersion%=%GCCVersion%& set %VarArchitecture%=%GCCArchitecture%
exit /B 0
