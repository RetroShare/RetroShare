## Build Retroshare with QtCreator & Qt 6.9 on Windows

### Qt 6.9 Installation

Download Qt 6.9 from: https://www.qt.io/download-dev

Run the installer and install Qt 6.9

Add GCC.exe to Path

	C:\Qt\Tools\mingw1310_64\bin

Compile the external libs start build-libs.bat:

Open Qt Command Prompt:

	cd C:\Users\User\Documents\GitHub\RetroShare\build_scripts\Windows\build-libs

Type (this will take some time to build all libs):

	build-libs.bat

Open Qt Creator

	Open project -> RetroShare.pro

After Project loaded, go to: 

	Projects->Build->Build Steps->Additional arguments: 

Add EXTERNAL_LIB_DIR:

	"EXTERNAL_LIB_DIR=%{sourceDir}\..\RetroShare-env\build-libs\gcc-13.1.0\x64\libs"

After done, Build Project