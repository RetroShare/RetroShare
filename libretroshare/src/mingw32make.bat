set QTDIR=C:\Qt\4.8.6
set MINGW=C:\MinGW

set PATH=%QTDIR%\bin;%MINGW%\bin;%PATH%

"c:\Program Files\TortoiseSVN\bin\SubWCRev" . retroshare\rsversion.in retroshare\rsversion.h

mingw32-make clean

qmake libretroshare.pro

mingw32-make

pause

