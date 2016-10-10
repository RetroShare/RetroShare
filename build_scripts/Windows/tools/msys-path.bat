:: Usage:
:: call msys-path.bat path variable

setlocal

set WinPath=%~1
set MSYSVar=%~2

if "%MSYSVar%"=="" (
	echo.
	echo Parameter error.
	exit /B 1
)

set MSYSPath=/%WinPath:~0,1%/%WinPath:~3%
set MSYSPath=%MSYSPath:\=/%

endlocal & set %MSYSVar%=%MSYSPath%

exit /B 0
