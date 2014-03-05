set QTDIR=C:\Qt\4.8.5
set MINGW=C:\MinGW

set PATH=%QTDIR%\bin;%MINGW%\bin;%PATH%

mingw32-make clean 

qmake libbitdht.pro

mingw32-make

pause

