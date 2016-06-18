@echo off
REM create dummy webfiles at qmake run

set publicdest=%1\webui
if "%1" == "" set publicdest=..\..\webui

if exist %publicdest% echo remove %publicdest%&&rd %publicdest% /S /Q

echo create %publicdest%
md %publicdest%

echo create %publicdest%\app.js, %publicdest%\app.css, %publicdest%\index.html
echo. > %publicdest%\app.js
echo. > %publicdest%\app.css
echo. > %publicdest%\index.html
