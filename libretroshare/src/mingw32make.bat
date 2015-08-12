set QTDIR=C:\Qt\4.8.6
set MINGW=C:\MinGW
set GIT=C:\Program Files\Git

set PATH=%QTDIR%\bin;%MINGW%\bin;;%GIT%\bin;%PATH% 

mingw32-make clean 

qmake libretroshare.pro "CONFIG+=version_detail_bash_script" 

mingw32-make

pause

