:: Usage:
:: call download-file-wildcard.bat url file-wildcard download-path variable

if "%~4"=="" (
	echo.
	echo Parameter error.
	exit /B 1
)

if "%EnvTempPath%"=="" (
	echo.
	echo Environment error.
	exit /B 1
)

setlocal

set Url=%~1
set FileWildcard=%~2
set DownloadPath=%~3
set Var=%~4
set File=

call "%~dp0remove-dir.bat" "%EnvTempPath%"
mkdir "%EnvTempPath%"

"%EnvWgetExe%" --recursive --continue --no-directories --no-parent -A "%FileWildcard%" --directory-prefix="%EnvTempPath%" "%Url%"

if errorlevel 1 (
	call "%~dp0remove-dir.bat" "%EnvTempPath%"
	endlocal & set %Var%=
	exit /B %ERRORLEVEL%
)

for %%A in (%EnvTempPath%\%FileWildcard%) do set File=%%~nxA
if "%File%"=="" (
	call "%~dp0remove-dir.bat" "%EnvTempPath%"
	endlocal & set %Var%=
	exit /B %ERRORLEVEL%
)

move "%EnvTempPath%\%File%" "%DownloadPath%"
call "%~dp0remove-dir.bat" "%EnvTempPath%"

endlocal & set %Var%=%File%
exit /B 0
