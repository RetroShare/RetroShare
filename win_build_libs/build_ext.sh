cd Build

mkdir -p libs/include
mkdir -p libs/lib
mkdir -p libs/bin

[ -s zlib-1.2.3.tar.gz ] || curl -L http://sourceforge.net/projects/libpng/files/zlib/1.2.3/zlib-1.2.3.tar.gz/download -o zlib-1.2.3.tar.gz
if [ -s zlib-1.2.3.tar.gz ]; then
  tar xvf zlib-1.2.3.tar.gz
  cd zlib-1.2.3
  ./configure
  #make install prefix="`pwd`/../libs"
  make
  cp zlib.h ../libs/include/
  cp zconf.h ../libs/include/
  cp libz.a ../libs/lib/
  cd ..
  rm -r -f zlib-1.2.3
fi

[ -s bzip2-1.0.6.tar.gz ] || curl http://bzip.org/1.0.6/bzip2-1.0.6.tar.gz -o bzip2-1.0.6.tar.gz
if [ -s bzip2-1.0.6.tar.gz ]; then
  tar xvf bzip2-1.0.6.tar.gz
  cd bzip2-1.0.6
  #make install PREFIX="`pwd`/../libs"
  make
  cp bzlib.h ../libs/include/
  cp libbz2.a ../libs/lib/
  cd ..
  rm -r -f bzip2-1.0.6
fi

[ -s miniupnpc-1.3.tar.gz ] || curl -L http://miniupnp.free.fr/files/download.php?file=miniupnpc-1.3.tar.gz -o miniupnpc-1.3.tar.gz
if [ -s miniupnpc-1.3.tar.gz ]; then
  tar xvf miniupnpc-1.3.tar.gz
  cd miniupnpc-1.3
  make -f Makefile.mingw init libminiupnpc.a miniupnpc.dll
  mkdir -p ../libs/include/miniupnpc && cp *.h ../libs/include/miniupnpc/
  cp libminiupnpc.a ../libs/lib/
  cp miniupnpc.dll ../libs/bin/
  cd ..
  rm -r -f miniupnpc-1.3
fi

[ -s openssl-1.0.1h.tar.gz ] || curl -k https://www.openssl.org/source/openssl-1.0.1h.tar.gz -o openssl-1.0.1h.tar.gz
if [ -s openssl-1.0.1h.tar.gz ]; then
  tar xvf openssl-1.0.1h.tar.gz
  cd openssl-1.0.1h
  #./config --prefix="`pwd`/../libs"
  #make install
  ./config
  make
  mkdir -p ../libs/include/openssl && cp include/openssl/*.h ../libs/include/openssl/
  cp libcrypto.a ../libs/lib/
  cp libssl.a ../libs/lib/
  cd ..
  rm -r -f openssl-1.0.1h
fi

[ -s speex-1.2rc1.tar.gz ] || curl http://downloads.xiph.org/releases/speex/speex-1.2rc1.tar.gz -o speex-1.2rc1.tar.gz
if [ -s speex-1.2rc1.tar.gz ]; then
  tar xvf speex-1.2rc1.tar.gz
  cd speex-1.2rc1
  ./configure
  #make install exec_prefix="`pwd`/../libs"
  make
  mkdir -p ../libs/include/speex && cp include/speex/*.h ../libs/include/speex/
  cp libspeex/.libs/libspeex.a ../libs/lib
  cp libspeex/.libs/libspeexdsp.a ../libs/lib
  cd ..
  rm -r -f speex-1.2rc1
fi

[ -s opencv-2.4.9.tar.gz ] || curl -L -k https://github.com/Itseez/opencv/archive/2.4.9.tar.gz -o opencv-2.4.9.tar.gz
if [ -s opencv-2.4.9.tar.gz ]; then
  tar xvf opencv-2.4.9.tar.gz
  cd opencv-2.4.9
  mkdir -p build
  cd build
  #cmake .. -G"MSYS Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_PERF_TESTS=OFF -DBUILD_TESTS=OFF -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX="`pwd`/../../libs"
  cmake .. -G"MSYS Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_PERF_TESTS=OFF -DBUILD_TESTS=OFF -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX="`pwd`/install"
  make install
  cp -r install/include/* ../../libs/include/
  mkdir -p ../../libs/lib/opencv && cp -r install/x64/mingw/staticlib/* ../../libs/lib/opencv/
  cd ../..
  rm -r -f opencv-2.4.9
fi

[ -s libxml2-2.9.1.tar.gz ] || curl ftp://xmlsoft.org/libxml2/libxml2-2.9.1.tar.gz -o libxml2-2.9.1.tar.gz
[ -s libxslt-1.1.28.tar.gz ] || curl ftp://xmlsoft.org/libxml2/libxslt-1.1.28.tar.gz -o libxslt-1.1.28.tar.gz
if [ -s libxml2-2.9.1.tar.gz -a -s libxslt-1.1.28.tar.gz ]; then
  tar xvf libxml2-2.9.1.tar.gz
  cd libxml2-2.9.1
  ./configure --without-iconv -enable-shared=no
  #make install exec_prefix="`pwd`/../libs"
  make
  mkdir -p ../libs/include/libxml && cp include/libxml/*.h ../libs/include/libxml/
  cp .libs/libxml2.a ../libs/lib/
  cd ..

  tar xvf libxslt-1.1.28.tar.gz
  tar xvf libxslt-1.1.28-fix.tar.gz
  cd libxslt-1.1.28
  ./configure --with-libxml-src=../libxml2-2.9.1 -enable-shared=no CFLAGS=-DLIBXML_STATIC
  make
  mkdir -p ../libs/include/libxslt && cp libxslt/*.h ../libs/include/libxslt/
  cp libxslt/.libs/libxslt.a ../libs/lib/
  cp libexslt/.libs/libexslt.a ../libs/lib/
  cd ..
  rm -r -f libxml2-2.9.1
  rm -r -f libxslt-1.1.28
fi

[ -s curl-7.34.0.tar.gz ] || curl http://curl.haxx.se/download/curl-7.34.0.tar.gz -o curl-7.34.0.tar.gz
if [ -s curl-7.34.0.tar.gz ]; then
  tar xvf curl-7.34.0.tar.gz
  cd curl-7.34.0
  LIBS_OLD=$LIBS
  LIBS="-L`pwd`/../libs/lib $LIBS"
  export LIBS
  ./configure --disable-shared --with-ssl="`pwd`/../libs"
  #make install exec_prefix="`pwd`/../libs"
  make
  LIBS=$LIBS_OLD
  LIBS_OLD=
  export LIBS
  mkdir -p ../libs/include/curl && cp include/curl/*.h ../libs/include/curl/
  cp lib/.libs/libcurl.a ../libs/lib/
  cd ..
  rm -r -f curl-7.34.0
fi

[ -s libssh-0.5.4.tar.gz ] || curl -k https://red.libssh.org/attachments/download/41/libssh-0.5.4.tar.gz -o libssh-0.5.4.tar.gz
if [ -s libssh-0.5.4.tar.gz ]; then
  tar xvf libssh-0.5.4.tar.gz
  tar xvf libssh-0.5.4-fix.tar.gz
  cd libssh-0.5.4
  mkdir -p build
  cd build
  cmake .. -G"MSYS Makefiles" -DWITH_STATIC_LIB:BOOL=ON -DZLIB_LIBRARY:FILEPATH="`pwd`/../../libs/lib/libz.a" -DZLIB_INCLUDE_DIR:PATH="`pwd`/../../libs/include" -DOPENSSL_LIBRARIES:FILEPATH="`pwd`/../../libs/lib/libcrypto.a" -DOPENSSL_INCLUDE_DIRS:PATH="`pwd`/../../libs/include"
  make
  cp src/libssh.a ../../libs/lib/
  cp src/threads/libssh_threads.a ../../libs/lib/
  cd ..
  rm -r -f build
  mkdir -p ../libs/include/libssh && cp include/libssh/*.h ../libs/include/libssh/
  cd ..
  rm -r -f libssh-0.5.4
fi

[ -s protobuf-2.4.1.tar.gz ] || curl -k https://protobuf.googlecode.com/files/protobuf-2.4.1.tar.gz -o protobuf-2.4.1.tar.gz
if [ -s protobuf-2.4.1.tar.gz ]; then
  tar xvf protobuf-2.4.1.tar.gz
  cd protobuf-2.4.1
  ./configure --disable-shared
  #make install exec_prefix="`pwd`/../libs"
  make
  mkdir -p ../libs/include/protobuf && cp -r src/google/ ../libs/include/protobuf/
  cp src/.libs/libprotobuf.a ../libs/lib/
  cp src/protoc.exe ../libs/bin/
  cd ..
  rm -r -f protobuf-2.4.1
fi

[ -s tcl8.6.2-src.tar.gz ] || curl -L http://prdownloads.sourceforge.net/tcl/tcl8.6.2-src.tar.gz -o tcl8.6.2-src.tar.gz
[ -s sqlcipher-2.2.1.tar.gz ] || curl -L -k https://github.com/sqlcipher/sqlcipher/archive/v2.2.1.tar.gz -o sqlcipher-2.2.1.tar.gz
if [ -s tcl8.6.2-src.tar.gz -a -s sqlcipher-2.2.1.tar.gz ]; then
  tar xvf tcl8.6.2-src.tar.gz
  cd tcl8.6.2
  mkdir -p build
  cd build
  ../win/configure
  make
  #make clean
  cd ../..

  tar xvf sqlcipher-2.2.1.tar.gz
  cd sqlcipher-2.2.1
  ln -s ../tcl8.6.2/build/tclsh86.exe tclsh
  mkdir -p `pwd`/../tcl8.6.2/lib
  ln -s `pwd`/../tcl8.6.2/library `pwd`/../tcl8.6.2/lib/tcl8.6
  PATH=$PATH:`pwd`/../tcl8.6.2/build
  LIBS_OLD=$LIBS
  LIBS="-L`pwd`/../libs/lib -lgdi32 $LIBS"
  export LIBS
  ./configure --disable-shared --enable-static --enable-tempstore=yes CFLAGS="-DSQLITE_HAS_CODEC -I`pwd`/../libs/include -I`pwd`/../tcl8.6.2/generic" LDFLAGS="-L`pwd`/../libs/lib -lcrypto -lgdi32" --with-tcl="`pwd`/../tcl8.6.2/build"
  make install prefix="`pwd`/install"
  LIBS=$LIBS_OLD
  LIBS_OLD=
  export LIBS
  cp -r install/include/* ../libs/include/
  cp install/lib/libsqlcipher.a ../libs/lib/
  cp install/bin/sqlcipher.exe ../libs/bin/
  rm -r -f `pwd`/../tcl8.6.2/lib
  rm tclsh
  cd ..
  rm -r -f sqlcipher-2.2.1
  rm -r -f tcl8.6.2
fi

cd ..
