@echo off
setlocal enabledelayedexpansion

if "%~2"=="" (
	echo.
	echo Parameter error.
	echo Usage %~n0 sourcepath outputfile
	exit /B 1
)

:: Check git executable
set GitPath=
call "%~dp0find-in-path.bat" GitPath git.exe
if "%GitPath%"=="" echo Git executable not found in PATH.& exit /B 1

set logfile=%~2
copy nul %logfile% > nul

pushd %~1

set last=HEAD
for /f %%t in ('git tag --sort=-taggerdate --merged ^| findstr v') do (
	echo generating changelog for !last!..%%t	
	echo ----------------------------------------------- >> %logfile%
	if !last! neq HEAD (
		git tag -n !last! >> %logfile%
	) else (
		echo HEAD >> %logfile%
	)
	rem echo !last! ---^> %%t >> %logfile%
	echo ----------------------------------------------- >> %logfile%
	echo. >> %logfile%
	git log %%t..!last! --no-merges "--pretty=format:%%h %%ai %%<(10,trunc)%%an %%s" >> %logfile%
	echo. >> %logfile%
	echo. >> %logfile%
	set last=%%t
)

echo generating changelog for %last%
echo ----------------------------------------------- >> %logfile%
git tag -n %last% >> %logfile%
echo ----------------------------------------------- >> %logfile%
echo. >> %logfile%
git log %last% --no-merges "--pretty=format:%%h %%ai %%<(10,trunc)%%an %%s" >> %logfile%

popd

endlocal enabledelayedexpansion

exit /B 0
