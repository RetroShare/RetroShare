set QTDIR=C:\Qt\4.8.6
set MINGW=C:\MinGW

set PATH=%QTDIR%\bin;%MINGW%\bin;%PATH%


qmake retroshare-gui.pro

mingw32-make

pause

