:: Usage:
:: call cecho.bat [info|error|std] "text"

if "%~1"=="std" echo %~2
if "%~1"=="info" "%EnvCEchoExe%" green "%~2"
if "%~1"=="error" "%EnvCEchoExe%" red "%~2"
