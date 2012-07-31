set QTDIR=d:\qt\2010.01
set MINGW=%QTDIR%\mingw

set PATH=%QTDIR%\qt\bin;%QTDIR%\bin;%MINGW%\bin;%PATH%

"c:\Program Files\TortoiseSVN\bin\SubWCRev" . libretroshare\src\util\rsversion.in libretroshare\src\util\rsversion.h

@echo off
:loop1
if %1x == x (
    rem if not exist tmp.txt echo debug >>tmp.txt
    goto :end1
)
if /i %1==clean (
    set clean=clean
	shift
    goto :loop1 
)
echo.%1>>tmp.txt
shift
goto :loop1

:end1
if not exist tmp.txt (
    echo debug >>tmp.txt
	set clean=clean
)
if %clean%x==cleanx (
    if not exist tmp.txt echo %clean% >>tmp.txt
    
)
for /f %%a in (tmp.txt) do (
@echo on

cd libbitdht\src

if not %clean%x==x mingw32-make clean 

qmake libbitdht.pro

mingw32-make %%a

cd ..\..\libretroshare\src

if not %clean%x==x mingw32-make clean 

qmake libretroshare.pro

mingw32-make %%a

cd ..\..\openpgpsdk\src

if not %clean%x==x mingw32-make clean 

qmake openpgpsdk.pro

mingw32-make %%a

cd ..\..\retroshare-nogui\src

if not %clean%x==x mingw32-make clean 

qmake retroshare-nogui.pro

mingw32-make %%a

cd ..\..\retroshare-gui\src

if not %clean%x==x mingw32-make clean

qmake retroshare-gui.pro

mingw32-make %%a

cd ..\..

)

@echo off
rem clean up
set clean=
del tmp.txt

pause

