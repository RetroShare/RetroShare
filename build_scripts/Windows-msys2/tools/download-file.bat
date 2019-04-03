:: Usage:
:: call download-file.bat url file

if "%~2"=="" (
	echo.
	echo Parameter error.
	exit /B 1
)

::"%EnvCurlExe%" -L -k "%~1" -o "%~2"
"%EnvWgetExe%" --no-check-certificate --continue "%~1" --output-document="%~2"

exit /B %ERRORLEVEL%
