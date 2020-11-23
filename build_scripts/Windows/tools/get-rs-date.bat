REM Usage:
REM call get-rs-date.bat SourcePath Variable

setlocal

set SourcePath=%~1
set Variable=%~2
if "%Variable%"=="" (
	echo.
	echo Parameter error
	exit /B 1
)

:: Check git executable
set GitPath=
call "%~dp0find-in-path.bat" GitPath git.exe
if "%GitPath%"=="" (
	echo.
	echo Git executable not found in PATH.
	exit /B 1
)

set Date=

pushd "%SourcePath%"
rem This doesn't work: git log -1 --date=format:"%Y%m%d" --format="%ad"
for /F "tokens=1,2,3* delims=-" %%A in ('git log -1 --date^=short --format^="%%ad"') do set Date=%%A%%B%%C
popd

:exit
endlocal & set %Variable%=%Date%
exit /B 0
