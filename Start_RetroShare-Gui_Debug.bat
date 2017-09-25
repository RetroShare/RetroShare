set QTDIR=C:\Qt\5.5\mingw492_32
set LIBS=..\libs
set RSPATH=.\retroshare-gui\src\debug

set PATH=%QTDIR%\bin;%LIBS%\bin;%PATH%

If not exist %RSPATH%\retroshare.exe (
  build-all-mingw32make.bat
)

%RSPATH%\retroshare.exe
