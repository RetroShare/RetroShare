set QTDIR=d:\qt\2010.05
set MINGW=%QTDIR%\mingw

set PATH=%QTDIR%\qt\bin;%QTDIR%\bin;%MINGW%\bin;%PATH%

mingw32-make clean 

qmake retroshare-nogui.pro

mingw32-make

pause

