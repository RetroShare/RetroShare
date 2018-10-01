:: Usage:
:: call get-rs-version.bat

for /F "tokens=1-5 delims=v.-" %%A in ('git describe') do (
	set RsMajorVersion=%%A
	set RsMinorVersion=%%B
	set RsBuildNumber=%%C
	set RsBuildNumberAdd=%%D
	set RsRevision=%%E
)

exit /B 0
