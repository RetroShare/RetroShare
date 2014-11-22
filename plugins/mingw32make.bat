set QTDIR=C:\Qt\4.8.6
set MINGW=C:\MinGW

set PATH=%QTDIR%\bin;%MINGW%\bin;%PATH%

mingw32-make clean

qmake plugins.pro

mingw32-make

pause

