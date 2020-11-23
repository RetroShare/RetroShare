@echo off

:: Very simple conversion from *.ts to *.nsh using xslt

pushd "%~dp0"

if "%1"=="" (
	for %%F in (*.ts) do if "%%F" NEQ "en.ts" call :convert %%~nF
	goto :exit
)

call :convert %1

:exit
popd

goto :EOF

:convert
if not exist "%~1.ts" (
	echo File "%~1.ts" not found.
	goto :EOF
)

echo %~1

"%~dp0xsltproc.exe" --output "%~dp0..\%~1.nsh" "%~dp0convert_from_ts.xsl" "%~1.ts"
