Compilation on Ubuntu/Debian
============================

1 - install package dependencies:

   # sudo apt-get install libglib2.0-dev libupnp-dev qt4-dev-tools \
      qt4-dev-tools libqt4-dev libssl-dev libxss-dev libgnome-keyring-dev libbz2-dev \
      libqt4-opengl-dev libqtmultimediakit1 qtmobility-dev \
      libspeex-dev libspeexdsp-dev libxslt1-dev libcurl4-openssl-dev \
      libopencv-dev, tcl8.5, libmicrohttpd-dev

2 - checkout the source code

   # mkdir ~/retroshare
   # cd ~/retroshare 
   # git clone https://github.com/RetroShare/RetroShare.git trunk

3 - compile

   # mkdir build
   # cd build
   # qmake CONFIG=debug ../trunk
   # make

   => the executables produced will be 

         build/retroshare-gui/src/Retroshare
         build/retroshare-nogui/src/retroshare-nogui

If libsqlcipher is not available as a package:
=============================================

   You need to place sqlcipher so that the hierarchy is:

      retroshare
          |
          +--- trunk
          |
          +--- lib
                |
                +---- sqlcipher

   # mkdir lib
   # cd lib
   # git clone git://github.com/sqlcipher/sqlcipher.git
   # cd sqlcipher
   # ./configure --enable-tempstore=yes CFLAGS="-DSQLITE_HAS_CODEC" LDFLAGS="-lcrypto"
   # make
   # cd ..

Using retroshare-nogui & webUI
==============================

   The webUI needs to be enabled as a parameter option in retroshare-nogui:

      ./retroshare-nogui --webinterface 9090 --docroot /usr/share/RetroShare06/webui/

   The webUI is only accessible on localhost:9090 (unless you canged that
   option in the GUI). It is advised to keep it that way so that your RS
   cannot be controlled using an untrusted connection.

   To access your web UI from a distance, just open a SSH tunnel on it:

      distant_machine:~/ >  ssh rs_host -L 9090:localhost:9090 -N

   "rs_host" is the machine running retroshare-nogui. Then on the distant machine, access your webUI on 

      http://localhost:9090

   That also works with a retroshare GUI of course.
