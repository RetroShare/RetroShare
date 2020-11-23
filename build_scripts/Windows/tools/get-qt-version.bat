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

call "%~dp0find-in-path.bat" QMakePath qmake.exe
if "%QMakePath%"=="" (
	echo.
	echo Cannot find qmake.exe in PATH.
	exit /B 1
)

for /F "tokens=1,2,3,4 delims= " %%A in ('qmake.exe -version') do if "%%A"=="Using" set QtVersion=%%D

endlocal & set %Var%=%QtVersion%
exit /B 0