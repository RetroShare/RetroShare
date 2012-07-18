set QTDIR=d:\qt\2010.01
set MINGW=%QTDIR%\mingw

set PATH=%QTDIR%\qt\bin;%QTDIR%\bin;%MINGW%\bin;%PATH%

"D:\Program Files\Tortoisesvn\bin\SubWCRev" . libretroshare\src\util\rsversion.in libretroshare\src\util\rsversion.h

cd libbitdht\src

mingw32-make clean 

qmake libbitdht.pro

mingw32-make

cd ..\..\libretroshare\src

mingw32-make clean 

qmake libretroshare.pro

mingw32-make

cd ..\..\openpgpsdk\src

mingw32-make clean 

qmake openpgpsdk.pro

mingw32-make

cd ..\..\retroshare-nogui\src

mingw32-make clean 

qmake retroshare-nogui.pro

mingw32-make

cd ..\..\retroshare-gui\src

mingw32-make clean 

qmake RetroShare.pro

mingw32-make

pause

