set QTDIR=C:\Qt\4.8.6
set MINGW=C:\MinGW
set GIT=C:\Program Files\Git

set PATH=%QTDIR%\bin;%MINGW%\bin;%GIT%\bin;%PATH%


qmake retroshare-gui.pro "CONFIG+=version_detail_bash_script" 

mingw32-make

pause

