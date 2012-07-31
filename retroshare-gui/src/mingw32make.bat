set QTDIR=D:\qt\2010.01
set MINGW=%QTDIR%\mingw

set PATH=%QTDIR%\qt\bin;%QTDIR%\bin;%MINGW%\bin;%PATH%


qmake retroshare-gui.pro

mingw32-make

pause

