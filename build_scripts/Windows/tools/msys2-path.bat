:: Usage:
:: call msys2-path.bat path variable

setlocal

set WinPath=%~1
set MSYS2Var=%~2

if "%MSYS2Var%"=="" (
	echo.
	echo Parameter error.
	exit /B 1
)

set MSYS2Path=/%WinPath:~0,1%/%WinPath:~3%
set MSYS2Path=%MSYS2Path:\=/%

endlocal & set %MSYS2Var%=%MSYS2Path%

exit /B 0
