set QTDIR=C:\Qt\5.5.0\5.5\mingw492_32
set MINGW=C:\Qt\Tools\mingw492_32

set GIT=C:\Program Files\Git

set PATH=%QTDIR%\bin;%MINGW%\bin;%GIT%\bin;%PATH%


qmake retroshare-gui.pro "CONFIG+=version_detail_bash_script" 


mingw32-make

pause

