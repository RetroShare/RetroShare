# Compilation on MacOS

## Qt Installation

Qt 5.15 is not available as install package.

Download Qt 5.15.x from here: [Qt 5.15.17](https://download.qt.io/archive/qt/5.15/5.15.17/single/qt-everywhere-opensource-src-5.15.17.tar.xz)

Instruction howto Build Qt 5.15.x on macOS: [macOS Building](https://doc.qt.io/archives/qt-5.15/macos-building.html)
## Set the Environment Variables
Add to the PATH environment variable by editing your *~/.profile* file.

       export PATH="/users/$USER/Qt/5.15.17/clang_64/bin:$PATH"

Depends on which version of Qt you use.

## Get RetroShare

In Qt Creator Projects -> New -> Import Project -> Git Clone -> Choose
Add Repository and Continoue

       Repository: https://github.com/RetroShare/RetroShare.git 

via Terminal:

       cd <your development directory>
       git clone https://github.com/RetroShare/RetroShare.git retroshare

via GitHub Desktop: [GitHub Desktop Download](https://central.github.com/deployments/desktop/desktop/latest/darwin)

In GitHub Desktop -> Clone Repository -> URL

       Add Repository URL: https://github.com/RetroShare/RetroShare.git and Clone

## ***Get XCode & MacOSX SDK***

To identify the correct version of Xcode to install, you need to know which OS you are running. Go to the [x] menu -> "About This Mac" and read the macOS version number.

If you are running macOS Ventura 13.5 or later, you can install Xcode directly from App Store using the instructions below. 

You can find older versions of Xcode at [Apple Developer Downloads](https://developer.apple.com/downloads/). Find the appropriate .xip file for your macOS version

To install from App Store:

Select [x] menu - > "App Store…".
Search for Xcode. Download and install.

Once Xcode has installed, you must drag the XCode icon into your Applications folder. After you have done this, open Xcode from the Applications folder by double-clicking on the icon and then follow the remaining instructions below. 

Install XCode command line developer tools:

       $xcode-select --install

Start XCode to get it updated and to able C compiler to create executables.

Older MacOSX SDK is available from here: [MacOSX-SDKs](https://github.com/phracker/MacOSX-SDKs)

### MacPort Installation

Install MacPort following this guide: [MacPort](http://guide.macports.org/#installing.xcode)

### HOMEBREW Installation

Install HomeBrew following this guide: [HomeBrew](http://brew.sh/)

#### Install libraries  

       $ brew install openssl
       $ brew install miniupnpc
       $ brew install rapidjson
       $ brew install sqlcipher

For RNP lib:

	   $ brew install bzip2
	   $ brew install zlib
	   $ brew install json-c
	   $ brew install botan@2

#### Install CMake 

       $ brew install cmake
       
If you have error in linking, run this:

       $sudo chown -R $(whoami) /usr/local/lib/pkgconfig

For VOIP Plugin: 

       $ brew install speex
       $ brew install speexdsp
       $ brew install opencv
       $ brew install ffmpeg

For FeedReader Plugin:

       $ brew install libxslt
       $ brew install libxml2

## Last Settings

In QtCreator Projects -> Manage Kits > Version Control > Git:

       select "Pull with rebase"

In QtCreator Projects -> Build -> Build Settings -> Build Environment -> Add this path:

       /usr/local/bin

In QtCreator Projects -> Build -> Build Settings -> Build Steps -> Add Additional arguments:

       "CONFIG+=rs_autologin" "CONFIG+=rs_use_native_dialogs" 

## Set your Mac OS SDK version


Edit retroshare.pri and set your installed sdk version example for 11.1 -> rs_macos11.1 (line 135:)

    macx:CONFIG *= rs_macos11.1
    rs_macos10.8:CONFIG -= rs_macos11.1
    rs_macos10.9:CONFIG -= rs_macos11.1
    rs_macos10.10:CONFIG -= rs_macos11.1
    rs_macos10.12:CONFIG -= rs_macos11.1
    rs_macos10.13:CONFIG -= rs_macos11.1
    rs_macos10.14:CONFIG -= rs_macos11.1
    rs_macos10.15:CONFIG -= rs_macos11.1


## Link Include & Libraries

When required edit your retroshare.pri macx-* section, check if the Include and Lib path are correct (macx-* section)

    INCLUDEPATH += "/usr/local/opt/openssl/include"
    QMAKE_LIBDIR += "/usr/local/opt/openssl/lib"
    QMAKE_LIBDIR += "/usr/local/opt/sqlcipher/lib"
    QMAKE_LIBDIR += "/usr/local/opt/miniupnpc/lib"

alternative via Terminal

    $ qmake 
    INCLUDEPATH+="/usr/local/opt/openssl/include" \
    QMAKE_LIBDIR+="/usr/local/opt/openssl/lib" \
    QMAKE_LIBDIR+="/usr/local/opt/sqlcipher/lib" \
    QMAKE_LIBDIR+="/usr/local/opt/miniupnpc/lib" \
    CONFIG+=rs_autologin \
    CONFIG+=rs_use_native_dialogs \
    CONFIG+=release \
    ..

For FeedReader Plugin:

    INCLUDEPATH += "/usr/local/opt/libxml2/include/libxml2"

With plugins:

    $ qmake \
    INCLUDEPATH+="/usr/local/opt/openssl/include" QMAKE_LIBDIR+="/usr/local/opt/openssl/lib" \
    QMAKE_LIBDIR+="/usr/local/opt/sqlcipher/lib" \
    QMAKE_LIBDIR+="/usr/local/opt/miniupnpc/lib" \
    INCLUDEPATH+="/usr/local/opt/opencv/include/opencv4" QMAKE_LIBDIR+="/usr/local/opt/opencv/lib" \
    INCLUDEPATH+="/usr/local/opt/speex/include" QMAKE_LIBDIR+="/usr/local/opt/speex/lib/" \
    INCLUDEPATH+="/usr/local/opt/speexdsp/include" QMAKE_LIBDIR+="/usr/local/opt/speexdsp/lib/" \
    INCLUDEPATH+="/usr/local/opt/libxslt/include" QMAKE_LIBDIR+="/usr/local/opt/libxslt/lib" \
    QMAKE_LIBDIR+="/usr/local/opt/ffmpeg/lib" \
    LIBS+=-lopencv_videoio \
    CONFIG+=retroshare_plugins \
    CONFIG+=rs_autologin \
    CONFIG+=rs_use_native_dialogs \
    CONFIG+=release \
    ..

## Compile RetroShare 

You can now compile RetroShare into Qt Creator or with Terminal

       $ cd /path/to/retroshare
       $ qmake ..
       $ make

You can change Target and SDK in *./retroshare.pri:82* changing value of QMAKE_MACOSX_DEPLOYMENT_TARGET and QMAKE_MAC_SDK

You can find the compiled application at *./retroshare/retroshare-gui/src/retroshare.app*

## Issues

If you have issues with openssl (Undefined symbols for architecture x86_64) try to add to *~/.profile* file this or via Terminal

       export PATH="/usr/local/opt/openssl/bin:$PATH"
       export LDFLAGS="-L/usr/local/opt/openssl/lib"
       export CPPFLAGS="-I/usr/local/opt/openssl/include"
       export PKG_CONFIG_PATH="/usr/local/opt/openssl/lib/pkgconfig"

For Qt Creator -> QtCreator Projects -> Build -> Build Settings -> Build Steps -> Add Additional arguments:

       LDFLAGS="-L/usr/local/opt/openssl/lib"
       CPPFLAGS="-I/usr/local/opt/openssl/include"



## Copy Plugins

    $ cp \
    ./plugins/FeedReader/lib/libFeedReader.dylib \
    ./plugins/VOIP/lib/libVOIP.dylib \
    ./plugins/RetroChess/lib/libRetroChess.dylib \
    ./retroshare-gui/src/RetroShare.app/Contents/Resources/

### Compile Retroshare-Service & Webui with CMake
before you can compile overwrite the file "asio/include/asio/detail/config.hpp" here is a fix for macos [
asio fix](https://github.com/chriskohlhoff/asio/commit/68df16d560c68944809bb2947360fe8035e9ae0a)   

       $ cd retroshare-service
       $ mkdir build-dir
       $ cd build-dir
       $ cmake -DRS_WEBUI=ON -DCMAKE_BUILD_TYPE=Release ..
       $ make
