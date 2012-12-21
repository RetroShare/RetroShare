set QTDIR=d:\qt\2010.05
set MINGW=%QTDIR%\mingw

set PATH=%QTDIR%\qt\bin;%QTDIR%\bin;%MINGW%\bin;%PATH%

"c:\Programme\TortoiseSVN\bin\SubWCRev" . libretroshare\src\util\rsversion.in libretroshare\src\util\rsversion.h
"c:\Programme\TortoiseSVN\bin\SubWCRev" . retroshare-gui\src\util\rsguiversion.in retroshare-gui\src\util\rsguiversion.h
"c:\Programme\TortoiseSVN\bin\SubWCRev" . retroshare-gui\src\retroshare.in retroshare-gui\src\retroshare.nsi


@echo off
rem emptying used variables in case the script was aborted and tempfile
set pack=
set clean=
if exist tmp.txt del tmp.txt


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
if /i %1==pack (
    set pack=pack
	shift
    goto :loop1 
)
echo.%1>>tmp.txt
shift
goto :loop1

:end1
if %clean%x==cleanx (
    if not exist tmp.txt echo %clean% >>tmp.txt
)

if not exist tmp.txt (
	if not %pack%x==packx (
		echo debug >>tmp.txt
		set clean=clean
	)
)

for /f %%a in (tmp.txt) do (
@echo on

cd libbitdht\src

if not %clean%x==x mingw32-make clean 

qmake libbitdht.pro

mingw32-make %%a


cd ..\..\openpgpsdk\src

if not %clean%x==x mingw32-make clean 

qmake openpgpsdk.pro

mingw32-make


cd ..\..\libretroshare\src

if not %clean%x==x mingw32-make clean 

qmake libretroshare.pro

mingw32-make %%a


cd ..\..\supportlibs\pegmarkdown

if not %clean%x==x mingw32-make clean 

qmake pegmarkdown.pro

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
@echo off
)

@echo off
if %pack%x==packx call packaging.bat
rem clean up
set clean=
del tmp.txt
set pack=
pause

