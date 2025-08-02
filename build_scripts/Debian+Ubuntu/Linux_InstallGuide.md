
## Compilation on Linux


### Install package dependencies:
#### Debian/Ubuntu
```bash
   sudo apt-get install g++ cmake qt5-qmake qtmultimedia5-dev \
   libqt5x11extras5-dev libbz2-dev libjson-c-dev libssl-dev libsqlcipher-dev \
   libupnp-dev libxss-dev rapidjson-dev libbotan-2-dev libasio-dev
```

Additional dependencies for Qt6 compile
```bash
   sudo apt-get install qt6-base-dev qt6-multimedia-dev qt6-5compat-dev
```

Additional dependencies for Feedreader plugin:
```bash
   sudo apt-get install libxml2-dev libxslt1-dev
```

Additional dependencies for Voip plugin:
```bash
   sudo apt-get install libavcodec-dev libcurl4-openssl-dev \
   libqt5multimedia5-plugins libspeexdsp-dev
```

#### openSUSE
```bash
   sudo zypper install gcc-c++ cmake libqt5-qtbase-devel \
   libqt5-qtmultimedia-devel libqt5-qtx11extras-devel libbz2-devel \
   libopenssl-devel libupnp-devel libXss-devel sqlcipher-devel rapidjson-devel
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
   git submodule update --init --remote libbitdht/ libretroshare/ retroshare-webui/ 
   git submodule update --init --remote supportlibs/librnp supportlibs/rapidjson supportlibs/restbed
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
 
 - /usr/bin/RetroShare06  
 

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


### Compile and run tests  
       qmake CONFIG+=tests  
       make  
       tests/unittests/unittests  


