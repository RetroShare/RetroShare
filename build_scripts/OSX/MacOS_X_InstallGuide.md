# Compilation on MacOS

## Qt Installation

Install Qt via: [Qt Download](http://www.qt.io/download/)

Use default options. And add Qt Script support.

Add to the PATH environment variable by editing your *~/.profile* file.

       export PATH="/users/$USER/Qt/5.5/clang_64/bin:$PATH"

Depends on which version of Qt you use.

## ***Choose if you use MacPort or HomeBrew***

### MacPort Installation

Install MacPort and XCode following this guide: [MacPort and XCode](http://guide.macports.org/#installing.xcode)

Start XCode to get it updated and to able C compiler to create executables.

#### Install libraries  

       $sudo port -v selfupdate
       $sudo port install openssl
       $sudo port install miniupnpc
       $sudo port install libmicrohttpd
       
For VOIP Plugin: 

       $sudo port install speex-devel
       $sudo port install opencv

Get Your OSX SDK if missing: [MacOSX-SDKs](https://github.com/phracker/MacOSX-SDKs)

### HOMEBREW Installation

Install HomeBrew following this guide: [HomeBrew](http://brew.sh/)

Install XCode command line developer tools:

       $xcode-select --install

Start XCode to get it updated and to able C compiler to create executables.

#### Install libraries  

       $brew install openssl
       $brew install miniupnpc
       $brew install libmicrohttpd
       
If you have error in linking, run this:

       $sudo chown -R $(whoami) /usr/local/lib/pkgconfig

For VOIP Plugin: 

       $brew install speex
       $brew install speexdsp
       $brew install homebrew/science/opencv
       $brew install ffmpeg

Get Your OSX SDK if missing: [MacOSX-SDKs](https://github.com/phracker/MacOSX-SDKs)

## Last Settings

In QtCreator Option Git select "Pull" with "Rebase"

## Compil missing libraries
### SQLCipher
       
       cd <your development directory>
       git clone https://github.com/sqlcipher/sqlcipher.git
       cd sqlcipher
       ./configure --disable-shared --disable-tcl --enable-static --enable-tempstore=yes CFLAGS="-DSQLITE_HAS_CODEC -I/opt/local/include" LDFLAGS="-lcrypto"
       make 
       sudo make install

NOTE, might be necessary to *chmod 000 /usr/local/ssl* temporarily during *./configure* if 
homebrew uses newer, non-stock ssl dependencies found there. Configure might get confused.

You can now compile RS into Qt Creator or with terminal

       cd <your development directory>
       git clone https://github.com/RetroShare/RetroShare.git retroshare
       cd retroshare
       qmake; make

You can change Target and SDK in *./retroshare.pri:82* changing value of QMAKE_MACOSX_DEPLOYMENT_TARGET and QMAKE_MAC_SDK

You can find compiled application on *./retroshare/retroshare-gui/src/retroshare.app*
