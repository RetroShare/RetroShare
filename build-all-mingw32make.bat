set QTDIR=C:\Qt\5.5\mingw492_32\
set MINGW=C:\Qt\Tools\mingw492_32

set PATH=%QTDIR%\bin;%MINGW%\bin;%PATH%
set DEBUG=1

@echo off
rem emptying used variables in case the script was aborted and tempfile
set pack=
set clean=
set errorlevel=
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
if ECHO==1 @echo on




rem TODO: Remove these lines
rem GOTO :retroshare-gui




:libbitdht
rem ###################################
rem ### libbitdht #####################
rem ###################################
cd libbitdht\src

if not %clean%x==x mingw32-make clean 

qmake libbitdht.pro
CALL :TEST_ERROR

mingw32-make %%a
CALL :TEST_ERROR
echo ###################################
echo ### libbitdht done ################
echo ###################################
cd ..\..

:openpgpsdk
rem ###################################
rem ### openpgpsdk ####################
rem ###################################
cd openpgpsdk\src

if not %clean%x==x mingw32-make clean 

qmake openpgpsdk.pro
CALL :TEST_ERROR

mingw32-make
CALL :TEST_ERROR
echo ###################################
echo ### openpgpsdk done ###############
echo ###################################
cd ..\..

:libresapi
rem ###################################
rem ### libresapi #####################
rem ###################################
cd libresapi\src

if not %clean%x==x mingw32-make clean 

qmake libresapi.pro
CALL :TEST_ERROR

mingw32-make %%a
CALL :TEST_ERROR
echo ###################################
echo ### libresapi done ################
echo ###################################
cd ..\..

:libretroshare
rem ###################################
rem ### libretroshare #################
rem ###################################
cd libretroshare\src

if not %clean%x==x mingw32-make clean 

qmake libretroshare.pro  "CONFIG+=version_detail_bash_script"
CALL :TEST_ERROR

mingw32-make %%a
CALL :TEST_ERROR
echo ###################################
echo ### libretroshare done ############
echo ###################################
cd ..\..

:pegmarkdown
rem ###################################
rem ### pegmarkdown ###################
rem ###################################
cd supportlibs\pegmarkdown

if not %clean%x==x mingw32-make clean 

qmake pegmarkdown.pro
CALL :TEST_ERROR

mingw32-make %%a
CALL :TEST_ERROR
echo ###################################
echo ### pegmarkdown done ##############
echo ###################################
cd ..\..

:retroshare-nogui
rem ###################################
rem ### retroshare-nogui ##############
rem ###################################
cd retroshare-nogui\src

if not %clean%x==x mingw32-make clean 

qmake retroshare-nogui.pro
CALL :TEST_ERROR

mingw32-make %%a
CALL :TEST_ERROR
echo ###################################
echo ### retroshare-nogui done #########
echo ###################################
cd ..\..

:retroshare-gui
rem ###################################
rem ### retroshare-gui ################
rem ###################################
cd retroshare-gui\src

if not %clean%x==x mingw32-make clean

rem qmake -r -spec ..\mkspecs\win32-g++ "CONFIG+=version_detail_bash_script" retroshare-gui.pro
qmake retroshare-gui.pro "CONFIG+=version_detail_bash_script"
CALL :TEST_ERROR

mingw32-make %%a
CALL :TEST_ERROR
echo ###################################
echo ### retroshare-gui done ###########
echo ###################################

cd ..\..
@echo off
)


@echo off
if %pack%x==packx call packaging.bat
rem ###################################
rem ### clean up ######################
rem ###################################
set clean=
del tmp.txt
set pack=
pause

rem ###################################
rem ### END ###########################
rem ###################################
GOTO :EOF


:TEST_ERROR
@echo off
if errorlevel 1 (
    pause
    set clean=
    del tmp.txt
    set pack=
    EXIT
)
if ECHO==1 @echo on
EXIT /B

:EOF