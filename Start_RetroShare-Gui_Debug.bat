set QTDIR=C:\Qt\5.5\mingw492_32
set LIBS=..\libs
set RSPATH=.\retroshare-gui\src\debug

set PATH=%QTDIR%\bin;%LIBS%\bin;%PATH%

If not exist %RSPATH%\RetroShare06.exe (
  build-all-mingw32make.bat
)

%RSPATH%\RetroShare06.exe
