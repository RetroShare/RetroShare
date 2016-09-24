@echo off
REM create webfiles from sources at compile time (works without npm/node.js)

set publicdest=%1\webui
set src=%1\webui-src

if "%1" == "" set publicdest=..\..\webui&&set src=..

if exist "%publicdest%"	echo remove existing %publicdest%&&rd %publicdest% /S /Q

echo mkdir %publicdest%
md %publicdest%

echo building app.js
echo - copy template.js ...
copy %src%\make-src\template.js %publicdest%\app.js

for %%F in (%src%\app\*.js) DO (set "fname=%%~nF" && CALL :addfile)

echo building app.css
type %src%\app\green-black.scss >> %publicdest%\app.css
type %src%\make-src\main.css >> %publicdest%\app.css
type %src%\make-src\chat.css >> %publicdest%\app.css

echo copy index.html
copy %src%\app\assets\index.html %publicdest%\index.html

echo build.bat complete

goto :EOF

:addfile
echo - adding %fname% ...
echo require.register("%fname%", function(exports, require, module) { >> %publicdest%\app.js
echo %src%\app\%fname%.js
type %src%\app\%fname%.js >> %publicdest%\app.js
echo. >> %publicdest%\app.js
echo }); >> %publicdest%\app.js


:EOF
