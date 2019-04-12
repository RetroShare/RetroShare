RetroShare
==============================
RetroShare is a decentralized, private and secure commmunication and sharing platform. RetroShare provides filesharing, chat, messages, forums and channels. 

Build Status
------------

| Platform  | Build Status |
| :------------- | :------------- |
| GNU/Linux, MacOS, (via travis-ci)  | [![Build Status](https://travis-ci.org/RetroShare/RetroShare.svg?branch=master)](https://travis-ci.org/RetroShare/RetroShare)  |
| Windows, `MSys2` (via appveyor)  | [![Build status](https://ci.appveyor.com/api/projects/status/github/RetroShare/RetroShare?svg=true)](https://ci.appveyor.com/project/RetroShare58622/retroshare)  |

Compilation on Windows
----------------------------
Follow this file : [WindowsMSys2_InstallGuide.md](https://github.com/RetroShare/RetroShare/blob/master/WindowsMSys2_InstallGuide.md)

Compilation on MacOSX
----------------------------
Follow this file : [MacOS_X_InstallGuide](https://github.com/RetroShare/RetroShare/blob/master/MacOS_X_InstallGuide.md)

Compilation for Android
---------------------------
Follow this file : [README-Android](https://github.com/RetroShare/RetroShare/blob/master/README-Android.asciidoc)

Compilation on Linux
----------------------------

1. Install package dependencies:
   * Debian/Ubuntu
   ```bash
   sudo apt-get install libglib2.0-dev libupnp-dev qt4-dev-tools \
       libqt4-dev libssl-dev libxss-dev libgnome-keyring-dev libbz2-dev \
       libqt4-opengl-dev libqtmultimediakit1 qtmobility-dev libsqlcipher-dev \
       libspeex-dev libspeexdsp-dev libxslt1-dev libcurl4-openssl-dev \
       libopencv-dev tcl8.5 libmicrohttpd-dev rapidjson-dev
   ```
   * openSUSE
   ```bash
   sudo zypper install gcc-c++ libqt4-devel libgnome-keyring-devel \
       glib2-devel speex-devel libssh-devel protobuf-devel libcurl-devel \
       libxml2-devel libxslt-devel sqlcipher-devel libmicrohttpd-devel \
       opencv-devel speexdsp-devel libupnp-devel libavcodec-devel rapidjson
   ```
   * Arch Linux
   ```bash
   pacman -S base-devel libgnome-keyring libmicrohttpd libupnp libxslt \
       libxss opencv qt4 speex speexdsp sqlcipher rapidjson
   ```

2. Checkout the source code
   ```bash
   mkdir ~/retroshare
   cd ~/retroshare 
   git clone https://github.com/RetroShare/RetroShare.git trunk
   ```

3. Compile
   ```bash
   cd trunk
   qmake CONFIG+=debug
   make
   ```

4. Install
   ```bash
   sudo make install
   ```

   The executables produced will be:

         /usr/bin/retroshare
         /usr/bin/retroshare-nogui

5. Uninstall:
   ```bash
   sudo make uninstall
   ```

Compile only retroshare-nogui
-----------------------------
If you want to run RetroShare on a server and don’t need the gui and plugins,
you can run the following commands to only compile/install the nogui version:

```bash
qmake
make retroshare-nogui
sudo make retroshare-nogui-install_subtargets
```

For packagers
-------------
Packagers can use PREFIX and LIB\_DIR to customize the installation paths:
```bash
qmake PREFIX=/usr LIB_DIR=/usr/lib64 "CONFIG-=debug" "CONFIG+=release"
make
make INSTALL_ROOT=${PKGDIR} install
```

If libsqlcipher is not available as a package
---------------------------------------------

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

Using retroshare-nogui & webUI
------------------------------

The webUI needs to be enabled as a parameter option in retroshare-nogui:

```bash
./retroshare-nogui --webinterface 9090 --docroot /usr/share/retroshare/webui/
```

The webUI is only accessible on localhost:9090. It is advised to keep it that way so that your RS
cannot be controlled using an untrusted connection.

To access your web UI from a distance, just open a SSH tunnel on it:

```bash
distant_machine:~/ >  ssh rs_host -L 9090:localhost:9090 -N
```

"rs_host" is the machine running retroshare-nogui. Then on the distant machine, access your webUI on 


      http://localhost:9090

That also works with a retroshare GUI of course.

Compile and run tests
---------------------

      qmake CONFIG+=tests
      make
      tests/unittests/unittests
