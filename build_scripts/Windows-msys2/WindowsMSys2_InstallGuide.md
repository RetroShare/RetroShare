## Compilation on Windows

The preferred build method on Windows is by using MSYS2 which is a collection
of packages providing unix-like tools to build native Windows software.

Requirements: about 12 GB of free space

The resulting binary is a 32-bit build of Retroshare which will also work
fine on a 64-bit system.

**If you want to make complet solution without debugging it, prefer to use \build_scripts\Windows-msys2\build.bat**

This batch will install and build all for you.

You only have to clone this repository (with [git for windows](https://gitforwindows.org/)) to a local folder, then start it in a terminal.

At the end, you'll get at ..\\*-msys2\deploy\ the Portable 7zip file.

### MSYS2 INSTALLATION (for editing or debugging)

Download MSYS2 from [MSYS2](http://www.msys2.org/). Installing 
MSYS2 requires 64 bit Windows 10 or newer.

Run the installer and install MSYS2.

At the end of the installation, it'll automatically open an MSYS shell terminal.
You can also find it on the start menu as MSYS2 MSYS. This is the shell you'll 
use to install packages with pacman and do maintenance but NOT to build
RetroShare.

First, update your MSYS2 environment to the latest version by typing:

	pacman -Syu

Close the terminal window.

Run MSYS2 MSYS again and finish updating with:

	pacman -Su

Install the default programs needed to build:

	pacman -S base-devel git wget p7zip gcc perl ruby doxygen cmake

Install the 64-bit toolchain:

	pacman -S mingw-w64-x86_64-toolchain

Install all needed dependencies:

	pacman -S mingw-w64-x86_64-miniupnpc
	pacman -S mingw-w64-x86_64-libxslt
	pacman -S mingw-w64-x86_64-xapian-core
	pacman -S mingw-w64-x86_64-sqlcipher
	pacman -S mingw-w64-x86_64-qt5-base
	pacman -S mingw-w64-x86_64-qt5-multimedia
	pacman -S mingw-w64-x86_64-ccmake
	pacman -S mingw-w64-x86_64-rapidjson
	pacman -S mingw-w64-x86_64-json-c
	pacman -S mingw-w64-x86_64-libbotan

If you want to use QtCreator as IDE, prefer using this one publish by MSYS2 as all build Kit are already setted.

	pacman -S mingw-w64-x86_64-qt-creator
*You can start it from MSYS2 terminal.*


We're done installing MSYS2, close the shell terminal.

### BUILDING RETROSHARE

Now run the MSYS2 MinGW 64-bit shell terminal (it's in the start menu).
We will use it to checkout Retroshare and build it:

	git clone https://github.com/RetroShare/RetroShare.git

Go to the RetroShare directory and configure to your liking, for example:
	
	cd RetroShare
	qmake -r -Wall -spec win32-g++ "CONFIG+=debug" "CONFIG+=rs_autologin"

Now we're ready to build Retroshare. Use the '-j' option with the number of
cores you have for a faster build, for instance if you have 4 cores:

	mingw32-make -j4

Make sure your current Retroshare is not running. Then just run:

	retroshare-gui/src/debug/retroshare.exe

You'll get debug output in the terminal and a running Retroshare instance.
