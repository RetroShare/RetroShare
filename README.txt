[![Build Status](https://travis-ci.org/RetroShare/RetroShare.svg?branch=master)](https://travis-ci.org/RetroShare/RetroShare)

To compile:

- install the package dependencies. On ubuntu:
   # sudo apt-get install libglib2.0-dev libupnp-dev qt4-dev-tools \
      libqt4-dev libssl-dev libxss-dev libgnome-keyring-dev libbz2-dev \
      libqt4-opengl-dev libqtmultimediakit1 qtmobility-dev \
      libspeex-dev libspeexdsp-dev libxslt1-dev libprotobuf-dev \
      protobuf-compiler cmake libcurl4-openssl-dev

- create project directory (e.g. ~/workbench) and check out the source code
   # cd ~
   # mkdir workbench
   # cd ~/workbench
   # git clone --depth 1 https://github.com/RetroShare/RetroShare.git retroshare

- create a new directory named lib
   # mkdir lib

- get source code for libssh-0.7.1 and create build directory (if needed) 
   # cd ~/workbench/lib
   # git clone -b 'libssh-0.7.1' --single-branch  --depth 1 git://git.libssh.org/projects/libssh.git 
   # cd libssh
   # mkdir build
   # cd build
   # cmake -DWITH_STATIC_LIB=ON -DWITH_GSSAPI=OFF ..
   # make
   # cd ../../..

- get source code for sqlcipher-v3.3.1, and build it  
   # cd ~/workbench/lib
   # git clone -b 'v3.3.1' --single-branch --depth 1 git://github.com/sqlcipher/sqlcipher.git
   # cd sqlcipher
   # ./configure --enable-tempstore=yes CFLAGS="-DSQLITE_HAS_CODEC" \
     LDFLAGS="-lcrypto"
   # make
   # cd ..

- go to your retroshare directory
   # cd ~/workbench/retroshare
   # qmake CONFIG=release && make clean && make -j 4

   => the executable produced will be 
         retroshare/retroshare-gui/src/Retroshare
         retroshare/retroshare-nogui/src/retroshare-nogui

- to use the SSH RS server (nogui):

   # ssh-keygen -t rsa -f rs_ssh_host_rsa_key   # this makes a RSA
   # ./retroshare-nogui -G            # generates a login+passwd hash for the RSA key used.
   # ./retroshare-nogui -S 7022 -U[SSLid] -P [Passwd hash]

- to connect from a remote place to the server by SSH:

   # ssh -T -p 7022 [user]@[host]

   and use the command line interface to control your RS instance.

List of non backward compatible changes for V0.6:
================================================

- in rscertificate.cc, enable V_06_USE_CHECKSUM
- in p3charservice, remove all usage of _deprecated items
- turn file transfer into a service. Will break item IDs, but cleanup and
  simplify lots of code.

