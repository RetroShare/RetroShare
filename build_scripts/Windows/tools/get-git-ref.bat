REM Usage:
REM call get-git-ref.bat Variable [Branch]

setlocal

set Variable=%~1
if "%Variable%"=="" (
	echo.
	echo Parameter error
	exit /B 1
)

set Ref=

:: Check git executable
set GitPath=
call "%~dp0find-in-path.bat" GitPath git.exe
if "%GitPath%"=="" (
	echo.
	echo Git executable not found in PATH.
	exit /B 1
)

set GitParameter=
set Branch=%~2
if "%Branch%"=="" (
	set Branch=HEAD
	set GitParameter=--head
)

for /F "tokens=1*" %%A in ('git show-ref %GitParameter% %Branch%') do (
	if "%%B"=="%Branch%" (
		set Ref=%%A
	)
)

:exit
endlocal & set %Variable%=%Ref%
exit /B 0
