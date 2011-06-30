set QTDIR=d:\qt\2010.01
set MINGW=%QTDIR%\mingw

set PATH=%QTDIR%\qt\bin;%QTDIR%\bin;%MINGW%\bin;%PATH%

"D:\Programme\TortoiseSVN\bin\SubWCRev" . util\rsversion.in util\rsversion.h

mingw32-make clean 

qmake libretroshare.pro

mingw32-make

pause

