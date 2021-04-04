# Compilation on MacOS

## Qt Installation

Install Qt via: [Qt Download](http://www.qt.io/download/)

Use default options. And add Qt Script support.

Add to the PATH environment variable by editing your *~/.profile* file.

       export PATH="/users/$USER/Qt/5.14.1/clang_64/bin:$PATH"

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

## ***Choose if you use MacPort or HomeBrew***

### MacPort Installation

Install MacPort and XCode following this guide: [MacPort and XCode](http://guide.macports.org/#installing.xcode)

Start XCode to get it updated and to able C compiler to create executables.

#### Install libraries  

       $ sudo port -v selfupdate
       $ sudo port install openssl
       $ sudo port install miniupnpc
       $ sudo port install libmicrohttpd
       
For VOIP Plugin: 

       $ sudo port install speex-devel
       $ sudo port install opencv
       $ sudo port install ffmpeg

Get Your OSX SDK if missing: [MacOSX-SDKs](https://github.com/phracker/MacOSX-SDKs)

### HOMEBREW Installation

Install HomeBrew following this guide: [HomeBrew](http://brew.sh/)

Install XCode command line developer tools:

       $xcode-select --install

Start XCode to get it updated and to able C compiler to create executables.

#### Install libraries  

       $ brew install openssl
       $ brew install miniupnpc
       $ brew install libmicrohttpd
       $ brew install rapidjson
       $ brew install sqlcipher
       
If you have error in linking, run this:

       $sudo chown -R $(whoami) /usr/local/lib/pkgconfig

For VOIP Plugin: 

       $ brew install speex
       $ brew install speexdsp
       $ brew install opencv
       $ brew install ffmpeg

For FeedReader Plugin:

       $ brew install libxslt

Get Your OSX SDK if missing: [MacOSX-SDKs](https://github.com/phracker/MacOSX-SDKs)

## Last Settings

In QtCreator Projects -> Manage Kits > Version Control > Git:

       select "Pull with rebase"

In QtCreator Projects -> Build -> Build Settings -> Build Environment -> Add this path:

       /usr/local/bin

In QtCreator Projects -> Build -> Build Settings -> Build Steps -> Add Additional arguments:

       "CONFIG+=rs_autologin" "CONFIG+=rs_use_native_dialogs" 

## Set your Mac OS SDK version

Edit RetroShare.pro  

    CONFIG += c++14 rs_macos11.1

and then retroshare.pri

    macx:CONFIG *= rs_macos11.1
    rs_macos10.8:CONFIG -= rs_macos11.1
    rs_macos10.9:CONFIG -= rs_macos11.1
    rs_macos10.10:CONFIG -= rs_macos11.1
    rs_macos10.12:CONFIG -= rs_macos11.1
    rs_macos10.13:CONFIG -= rs_macos11.1
    rs_macos10.14:CONFIG -= rs_macos11.1
    rs_macos10.15:CONFIG -= rs_macos11.1


## Link Include & Libraries

Edit your retroshare.pri and add to macx-*  section

    INCLUDEPATH += "/usr/local/opt/openssl/include"
    QMAKE_LIBDIR += "/usr/local/opt/openssl/lib"
    QMAKE_LIBDIR += "/usr/local/opt/sqlcipher/lib"
    QMAKE_LIBDIR += "/usr/local/opt/miniupnpc/lib"

alternative via Terminal

    $ qmake INCLUDEPATH+="/usr/local/opt/openssl/include" QMAKE_LIBDIR+="/usr/local/opt/openssl/lib" QMAKE_LIBDIR+="/usr/local/opt/sqlcipher/lib" QMAKE_LIBDIR+="/usr/local/opt/miniupnpc/lib"

For FeedReader Plugin:

    INCLUDEPATH += "/usr/local/opt/libxml2/include/libxml2"

For building RetroShare with plugins:

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

       cd retroshare
       qmake; make

You can change Target and SDK in *./retroshare.pri:82* changing value of QMAKE_MACOSX_DEPLOYMENT_TARGET and QMAKE_MAC_SDK

You can find the compiled application at *./retroshare/retroshare-gui/src/retroshare.app*

## Copy Plugins

    $ cp \
    ./plugins/FeedReader/lib/libFeedReader.dylib \
    ./plugins/VOIP/lib/libVOIP.dylib \
    ./plugins/RetroChess/lib/libRetroChess.dylib \
    ./retroshare-gui/src/RetroShare.app/Contents/Resources/
