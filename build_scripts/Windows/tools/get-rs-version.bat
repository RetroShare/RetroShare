:: Usage:
:: call get-rs-version.bat Executable Variable
::
:: Variable.Major
:: Variable.Minor
:: Variable.Mini
:: Variable.Extra

setlocal

set Executable=%~1
set Variable=%~2
if "%Variable%"=="" (
	echo.
	echo Parameter error.
	exit /B 1
)

if not exist "%Executable%" (
	echo.
	echo File %Executable% doesn't exist.
	exit /B1
)

set VersionMajor=
set VersionMinor=
set VersionMini=
set VersionExtra=

for /F "USEBACKQ tokens=1,2,3,* delims=.-" %%A in (`powershell -NoLogo -NoProfile -Command ^(Get-Item "%Executable%"^).VersionInfo.FileVersion`) do (
	set VersionMajor=%%A
	set VersionMinor=%%B
	set VersionMini=%%C
	set VersionExtra=%%D
)

endlocal & set %Variable%.Major=%VersionMajor%& set %Variable%.Minor=%VersionMinor%& set %Variable%.Mini=%VersionMini%& set %Variable%.Extra=%VersionExtra%&
exit /B 0