:: Usage:
:: call find-in-path.bat variable file

setlocal

set Var=%~1
set File=%~2

if "%File%"=="" (
	echo.
	echo Parameter error.
	exit /B 1
)

set FoundPath=

SET PathTemp="%Path:;=";"%"
FOR %%P IN (%PathTemp%) DO (
	IF EXIST "%%~P.\%File%" (
		set FoundPath=%%~P
		goto :found
	)
)

:found
endlocal & set %Var%=%FoundPath%
