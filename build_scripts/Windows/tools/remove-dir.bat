:: Usage:
:: call remove-dir.bat path

if "%~1"=="" (
	echo.
	echo Parameter error.
	exit /B 1
)

if exist %1 (
	del /s /f /q %1 >nul
	rmdir /s /q %1
)

exit /B 0
