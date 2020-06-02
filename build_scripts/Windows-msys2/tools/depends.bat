:: Usage:
:: call depends.bat file

if "%1"=="" (
	echo Usage: %~nx0 File
	exit /B 1
)

setlocal
pushd %~dp1

%EnvMSYS2Cmd% "ntldd --recursive $0 | cut -f1 -d"=" | awk '{$1=$1};1'" %~nx1 > %~sdp0depends.tmp

for /F %%A in (%~sdp0depends.tmp) do (
	echo %%~A
)

if exist "%~dp0depends.tmp" del /Q "%~dp0depends.tmp"

popd
endlocal
exit /B 0