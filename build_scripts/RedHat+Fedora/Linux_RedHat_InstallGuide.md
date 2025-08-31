
## Compilation on Red Hat-based Linux


### Install package dependencies:
#### RedHat/Fedora
```bash
   sudo dnf install mesa-libGL-devel gcc cmake rapidjson-devel \
   libupnp openssl sqlcipher sqlcipher-devel \
   botan2 botan2-devel json-c-devel bzip2-devel asio-devel libsecret libXScrnSaver-devel
```

To compile with Qt5:
```bash
   sudo dnf install qt5-qtbase-devel qt5-qtmultimedia qt5-qtx11extras
```

To compile with Qt6:
```bash
   sudo dnf install qt6-qtbase-devel qt6-qtmultimedia-devel qt6-qt5compat-devel
```

Additional dependencies for Feedreader plugin:
```bash
   sudo dnf install libxml2-devel libxslt-devel libcurl-devel
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

The executable produced will be:  
```bash
   ./retroshare-gui/src/retroshare
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

### SQLCipher
If libsqlcipher is not available as a package

You need to place sqlcipher so that the hierarchy is:

        ~Home
          |
          +--- retroshare
          |
          +--- lib
                |
                +---- sqlcipher
```bash
	mkdir lib
	cd lib
	git clone https://github.com/sqlcipher/sqlcipher.git --depth=1 --branch v3.4.1
	cd sqlcipher
	./configure --enable-tempstore=yes CFLAGS="-DSQLITE_HAS_CODEC" LDFLAGS="-lcrypto"
	make
	cd ..
```

### Build infos

Note: If you installed Qt6 you need to use `qmake6` on the command line.

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
  * rs_webui:					enable Web interface
  * rs_jsonapi:					enable json api interface, required by rs_webui
  * gxsthewire:					enable Wire service (experimental)
  * wikipoos:					enable Wiki service (experimental)
  * rs_use_native_dialogs:		enable native dialogs (may cause crashes with some versions of Gtk)
  * rs_deep_channels_index:		build with deep channel indexing support
  * rs_deep_files_index:		build with deep file indexing support
  * "CONFIG+=..."				enable other extra compile time features, you can find the almost complete list in file *&lt;sourcefolder&gt;\retroshare.pri*

Example:

```batch
qmake CONFIG+=debug CONFIG+=release CONFIG+=rs_use_native_dialog CONFIG+=rs_gui_cmark
qmake CONFIG+=rs_jsonapi CONFIG+=rs_webui CONFIG+=rs_autologin
qmake CONFIG+=rs_deep_channels_index CONFIG +=gxsthewire CONFIG+=wikipoos
```