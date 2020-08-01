@echo off

:: Very simple conversion from *.nsh to *.ts

setlocal

set Language=en
if "%1" NEQ "" set Language=%1

set InputFile=%~dp0..\%Language%.nsh
set OutputFile=%~dp0%Language%.ts

if not exist "%InputFile%" (
	echo File %InputFile% not found.
	goto :exit
)

echo ^<?xml version="1.0" encoding="utf-8"?^> >"%OutputFile%"
echo ^<!DOCTYPE TS^> >>"%OutputFile%"
echo ^<TS version="2.0" language="en_US"^> >>"%OutputFile%"

for /F "tokens=1,2,3,*" %%A in (%InputFile%) do if "%%A"=="!insertmacro" call :context %%C %%D

echo ^</TS^> >>"%OutputFile%"

:exit
endlocal
goto :EOF

:context

setlocal EnableDelayedExpansion

:: Simple replace of & to &amp;
set Text=%2
set Text=%Text:&=^&amp;%
set Text=%Text:~1,-1%

echo !Text!

echo ^<context^> >>"%OutputFile%"
echo   ^<name^>%~1^</name^> >>"%OutputFile%"
echo   ^<message^> >>"%OutputFile%"
echo     ^<source^>!Text!^</source^> >>"%OutputFile%"
echo     ^<translation type="unfinished"^>^</translation^> >>"%OutputFile%"
echo   ^</message^> >>"%OutputFile%"
echo ^</context^> >>"%OutputFile%"

endlocal
goto :EOF
