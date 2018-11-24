:: Usage:
:: call get-qt-version.bat variable

setlocal

set Var=%~1
if "%Var%"=="" (
	echo.
	echo Parameter error.
	exit /B 1
)

set QtVersion=

%EnvMSYS2Cmd% "qmake -version" >"%~dp0qtversion.tmp"
for /F "tokens=1,2,3,4" %%A in (%~sdp0qtversion.tmp) do (
	if "%%A"=="Using" (
		set QtVersion=%%D
		goto exit
	)
)

:exit
if exist "%~dp0qtversion.tmp" del /Q "%~dp0qtversion.tmp"

endlocal & set %Var%=%QtVersion%
exit /B 0