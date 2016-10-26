:: Usage:
:: call depends.bat [list^|missing] file

setlocal

if "%2"=="" (
	echo Usage: %~nx0 [list^|missing] File
	goto :exit
)

if not exist "%EnvDependsExe%" echo depends.exe not found in %EnvToolsPath%.& goto exit
if not exist "%EnvCutExe%" echo cut.exe not found in %EnvToolsPath%.& goto exit

start /wait "" "%EnvDependsExe%" /c /oc:"%~dp0depends.tmp" %2
if "%1"=="missing" (
	"%EnvCutExe%" --delimiter=, --fields=1,2 "%~dp0depends.tmp" >"%~dp0depends1.tmp"
	for /F "tokens=1,2 delims=," %%A in (%~sdp0depends1.tmp) do (
		if "%%A"=="?" (
			echo %%~B
		)
	)
)

if "%1"=="list" (
	"%EnvCutExe%" --delimiter=, --fields=2 "%~dp0depends.tmp" >"%~dp0depends1.tmp"
	for /F "tokens=1 delims=," %%A in (%~sdp0depends1.tmp) do (
		if "%%A" NEQ "Module" (
			echo %%~A
		)
	)
)

if exist "%~dp0depends.tmp" del /Q "%~dp0depends.tmp"
if exist "%~dp0depends1.tmp" del /Q "%~dp0depends1.tmp"

:exit
endlocal
