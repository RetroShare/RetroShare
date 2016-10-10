:: Usage:
:: call get-rs-version.bat Define Variable

setlocal

set Define=%~1
set Variable=%~2
if "%Variable%"=="" (
	echo.
	echo Parameter error.
	exit /B 1
)

set Result=
set VersionFile="%~dp0..\..\..\libretroshare\src\retroshare\rsversion.h"

if not exist "%VersionFile%" (
	echo.
	echo Version file doesn't exist.
	echo %VersionFile%
	exit /B1
)

for /F "usebackq tokens=1,2,3" %%A in (%VersionFile%) do (
	if "%%A"=="#define" (
		if "%%B"=="%Define%" (
			set Result=%%~C
		)
	)
)

endlocal & set %Variable%=%Result%
exit /B 0