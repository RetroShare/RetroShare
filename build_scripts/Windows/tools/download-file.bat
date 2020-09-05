:: Usage:
:: call download-file.bat url file

if "%~2"=="" (
	echo.
	echo Parameter error.
	exit /B 1
)

powershell -NoLogo -NoProfile -Command (New-Object System.Net.WebClient).DownloadFile(\""%~1\"", \""%~2\"")

exit /B %ERRORLEVEL%
