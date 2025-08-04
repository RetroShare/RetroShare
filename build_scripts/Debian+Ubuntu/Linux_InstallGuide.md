
## Compilation on Linux


### Install package dependencies:
#### Debian/Ubuntu
```bash
   sudo apt-get install g++ cmake qt5-qmake qtmultimedia5-dev \
   libqt5x11extras5-dev libbz2-dev libjson-c-dev libssl-dev libsqlcipher-dev \
   libupnp-dev libxss-dev rapidjson-dev libbotan-2-dev libasio-dev
```

Additional packages to compile with Qt6:
```bash
   sudo apt-get install qt6-base-dev qt6-multimedia-dev qt6-5compat-dev
```

Additional dependencies for Feedreader plugin:
```bash
   sudo apt-get install libxml2-dev libxslt1-dev libcurl4-openssl-dev
```

Additional dependencies for Voip plugin:
```bash
   sudo apt-get install libavcodec-dev libcurl4-openssl-dev \
   libqt5multimedia5-plugins libspeexdsp-dev
```

Autologin:
```bash
   sudo apt install libsecret-1-dev
```

#### openSUSE
```bash
   sudo zypper install git gcc-c++ cmake libqt5-qtbase-devel \
   libqt5-qtmultimedia-devel libqt5-qtx11extras-devel libbz2-devel \
   libopenssl-devel libupnp-devel libXss-devel sqlcipher-devel rapidjson-devel \
   json-c botan bzip2
```

Additional packages to compile with Qt6:
```bash
   sudo zypper install qt6-base-devel qt6-multimedia-devel qt6-qt5compat-devel
```

Additional dependencies for plugins:
```bash
   sudo zypper install ffmpeg-4-libavcodec-devel libcurl-devel libxml2-devel \
   libxslt-devel speex-devel speexdsp-devel
```

#### Linux Mint
```bash
   sudo apt-get install git g++ cmake qt5-qmake qtmultimedia5-dev \
   libqt5x11extras5-dev libupnp-dev libxss-dev libssl-dev libsqlcipher-dev \
   rapidjson-dev doxygen libbz2-dev libjson-c-dev libbotan-2-dev libasio-dev
```

#### Arch Linux
```bash
   pacman -S base-devel libgnome-keyring cmake qt5-tools qt5-multimedia qt5-x11extras \
   rapidjson libupnp libxslt libxss sqlcipher botan2 bzip2 json-c
```

### Checkout the source code
```bash
   cd ~ 
   git clone https://github.com/RetroShare/RetroShare.git retroshare
```

### Checkout the submodules
```bash
   cd retroshare
   git submodule update --init --remote libbitdht/ libretroshare/ openpgpsdk/ retroshare-webui/ 
   git submodule update --init --remote supportlibs/librnp supportlibs/restbed supportlibs/rapidjson
```

### Compile
```bash
   qmake CONFIG+=release CONFIG+=rs_jsonapi CONFIG+=rs_webui
   make
```

### Install
```bash
   sudo make install
```

The executable produced will be:  
```bash
 ~/usr/bin/RetroShare  
```

### For packagers

Packagers can use PREFIX and LIB\_DIR to customize the installation paths:
```bash
   qmake PREFIX=/usr LIB_DIR=/usr/lib64 "CONFIG-=debug" "CONFIG+=release"
   make
   make INSTALL_ROOT=${PKGDIR} install
```
 
 
### libsqlcipher
If libsqlcipher is not available as a package

You need to place sqlcipher so that the hierarchy is:

      retroshare
          |
          +--- trunk
          |
          +--- lib
                |
                +---- sqlcipher
```bash
	mkdir lib
	cd lib
	git clone git://github.com/sqlcipher/sqlcipher.git
	cd sqlcipher
	./configure --enable-tempstore=yes CFLAGS="-DSQLITE_HAS_CODEC" LDFLAGS="-lcrypto"
	make
	cd ..
``` 

### Build infos

When you use only Qt6 the packages `qt5-qmake qtmultimedia5-dev libqt5x11extras5-dev` are not needed.

For the `FeedReader` it is required to append the config option `CONFIG+=retroshare_plugins`.
Make sure `plugins/plugins.pro` contains `FeedReader` in the list of plugins to compile. 
Do not mix plugins compiled with Qt5 with those compiled with Qt6. They work only if they are compiled
with the same Qt version as RetroShare.

Voip is outdated and is not compileable on the latest Debian.

For `Autologin` it is required to append the config option `CONFIG+=rs_autologin`.


### Build options

* Mandatory
  * release or debug:			normally you would like to use the release option
* Extra features (optional)
  * rs_autologin:				enable autologin
  * retroshare_plugins:			build plugins
  * rs_webui:					enable remoting features
  * rs_jsonapi:					enable json api interface
  * gxsthewire					enable Wire service
  * wikipoos 					enable Wiki service
  * rs_use_native_dialogs		enable native dialogs
  * rs_deep_channels_index		build with deep channel indexing support
  * rs_deep_files_index			build with deep file indexing support
  * "CONFIG+=..."				enable other extra compile time features, you can find the almost complete list in file *&lt;sourcefolder&gt;\retroshare.pri*

Example:

```batch
qmake CONFIG-=debug CONFIG+=release CONFIG+=rs_use_native_dialog CONFIG+=rs_gui_cmark
qmake CONFIG+=rs_jsonapi CONFIG+=rs_webui CONFIG+=rs_autologin
qmake CONFIG+=rs_deep_channels_index CONFIG += gxsthewire CONFIG += wikipoos
```


