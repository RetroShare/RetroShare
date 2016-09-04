ZLIB_VERSION=1.2.3
BZIP2_VERSION=1.0.6
MINIUPNPC_VERSION=2.0
OPENSSL_VERSION=1.0.2h
SPEEX_VERSION=1.2rc2
SPEEXDSP_VERSION=1.2rc3
OPENCV_VERSION=2.4.13
LIBXML2_VERSION=2.9.3
LIBXSLT_VERSION=1.1.28
CURL_VERSION=7.46.0
TCL_VERSION=8.6.2
SQLCIPHER_VERSION=2.2.1
LIBMICROHTTPD_VERSION=0.9.46
FFMPEG_VERSION=3.1.2

all: dirs zlib bzip2 miniupnpc openssl speex speexdsp opencv libxml2 libxslt curl sqlcipher libmicrohttpd ffmpeg copylibs

download: \
	download/zlib-$(ZLIB_VERSION).tar.gz \
	download/bzip2-$(BZIP2_VERSION).tar.gz \
	download/miniupnpc-$(MINIUPNPC_VERSION).tar.gz \
	download/openssl-$(OPENSSL_VERSION).tar.gz \
	download/speex-$(SPEEX_VERSION).tar.gz \
	download/speexdsp-$(SPEEXDSP_VERSION).tar.gz \
	download/opencv-$(OPENCV_VERSION).tar.gz \
	download/libxml2-$(LIBXML2_VERSION).tar.gz \
	download/libxslt-$(LIBXSLT_VERSION).tar.gz \
	download/curl-$(CURL_VERSION).tar.gz \
	download/tcl$(TCL_VERSION)-src.tar.gz \
	download/sqlcipher-$(SQLCIPHER_VERSION).tar.gz \
	download/libmicrohttpd-$(LIBMICROHTTPD_VERSION).tar.gz \
	download/ffmpeg-$(FFMPEG_VERSION).tar.gz

dirs:
	mkdir -p download
	mkdir -p libs/include
	mkdir -p libs/lib
	mkdir -p libs/bin

download/zlib-$(ZLIB_VERSION).tar.gz:
	wget --no-check-certificate http://sourceforge.net/projects/libpng/files/zlib/$(ZLIB_VERSION)/zlib-$(ZLIB_VERSION).tar.gz/download -O download/zlib-$(ZLIB_VERSION).tar.gz

zlib: download/zlib-$(ZLIB_VERSION).tar.gz
	tar xvf download/zlib-$(ZLIB_VERSION).tar.gz
	cd zlib-$(ZLIB_VERSION) && ./configure
	#cd zlib-$(ZLIB_VERSION) && make install prefix="`pwd`/../libs"
	cd zlib-$(ZLIB_VERSION) && make
	cp zlib-$(ZLIB_VERSION)/zlib.h libs/include/
	cp zlib-$(ZLIB_VERSION)/zconf.h libs/include/
	cp zlib-$(ZLIB_VERSION)/libz.a libs/lib/
	rm -r -f zlib-$(ZLIB_VERSION)
	touch zlib

download/bzip2-$(BZIP2_VERSION).tar.gz:
	wget http://bzip.org/$(BZIP2_VERSION)/bzip2-$(BZIP2_VERSION).tar.gz -O download/bzip2-$(BZIP2_VERSION).tar.gz

bzip2: download/bzip2-$(BZIP2_VERSION).tar.gz
	tar xvf download/bzip2-$(BZIP2_VERSION).tar.gz
	#cd bzip2-$(BZIP2_VERSION) && make install PREFIX="`pwd`/../libs"
	cd bzip2-$(BZIP2_VERSION) && make
	cp bzip2-$(BZIP2_VERSION)/bzlib.h libs/include/
	cp bzip2-$(BZIP2_VERSION)/libbz2.a libs/lib/
	rm -r -f bzip2-$(BZIP2_VERSION)
	touch bzip2

download/miniupnpc-$(MINIUPNPC_VERSION).tar.gz:
	wget http://miniupnp.free.fr/files/download.php?file=miniupnpc-$(MINIUPNPC_VERSION).tar.gz -O download/miniupnpc-$(MINIUPNPC_VERSION).tar.gz

miniupnpc: download/miniupnpc-$(MINIUPNPC_VERSION).tar.gz
	tar xvf download/miniupnpc-$(MINIUPNPC_VERSION).tar.gz
	cd miniupnpc-$(MINIUPNPC_VERSION) && CC=gcc && export CC && make -f Makefile.mingw init libminiupnpc.a miniupnpc.dll
	mkdir -p libs/include/miniupnpc && cp miniupnpc-$(MINIUPNPC_VERSION)/*.h libs/include/miniupnpc/
	cp miniupnpc-$(MINIUPNPC_VERSION)/miniupnpc.lib libs/lib/
	cp miniupnpc-$(MINIUPNPC_VERSION)/miniupnpc.dll libs/bin/
	rm -r -f miniupnpc-$(MINIUPNPC_VERSION)
	touch miniupnpc

download/openssl-$(OPENSSL_VERSION).tar.gz:
	wget --no-check-certificate https://www.openssl.org/source/openssl-$(OPENSSL_VERSION).tar.gz -O download/openssl-$(OPENSSL_VERSION).tar.gz

openssl: download/openssl-$(OPENSSL_VERSION).tar.gz
	tar xvf download/openssl-$(OPENSSL_VERSION).tar.gz
	#cd openssl-$(OPENSSL_VERSION) && ./config --prefix="`pwd`/../libs"
	#cd openssl-$(OPENSSL_VERSION) && make install
	cd openssl-$(OPENSSL_VERSION) && ./config shared
	cd openssl-$(OPENSSL_VERSION) && make
	mkdir -p libs/include/openssl && cp openssl-$(OPENSSL_VERSION)/include/openssl/*.h libs/include/openssl/
	cp openssl-$(OPENSSL_VERSION)/libeay32.dll libs/bin/
	cp openssl-$(OPENSSL_VERSION)/ssleay32.dll libs/bin/
	cp openssl-$(OPENSSL_VERSION)/libcrypto.dll.a libs/lib/
	cp openssl-$(OPENSSL_VERSION)/libssl.dll.a libs/lib/
	rm -r -f openssl-$(OPENSSL_VERSION)
	touch openssl

download/speex-$(SPEEX_VERSION).tar.gz:
	wget http://downloads.xiph.org/releases/speex/speex-$(SPEEX_VERSION).tar.gz -O download/speex-$(SPEEX_VERSION).tar.gz

speex: download/speex-$(SPEEX_VERSION).tar.gz
	tar xvf download/speex-$(SPEEX_VERSION).tar.gz
	cd speex-$(SPEEX_VERSION) && ./configure
	#cd speex-$(SPEEX_VERSION) && make install exec_prefix="`pwd`/../libs"
	cd speex-$(SPEEX_VERSION) && make
	mkdir -p libs/include/speex && cp speex-$(SPEEX_VERSION)/include/speex/*.h libs/include/speex/
	cp speex-$(SPEEX_VERSION)/libspeex/.libs/libspeex.a libs/lib
	rm -r -f speex-$(SPEEX_VERSION)
	touch speex

download/speexdsp-$(SPEEXDSP_VERSION).tar.gz:
	wget http://downloads.xiph.org/releases/speex/speexdsp-$(SPEEXDSP_VERSION).tar.gz -O download/speexdsp-$(SPEEXDSP_VERSION).tar.gz

speexdsp: download/speexdsp-$(SPEEXDSP_VERSION).tar.gz
	tar xvf download/speexdsp-$(SPEEXDSP_VERSION).tar.gz
	cd speexdsp-$(SPEEXDSP_VERSION) && ./configure
	cd speexdsp-$(SPEEXDSP_VERSION) && make
	mkdir -p libs/include/speex && cp speexdsp-$(SPEEXDSP_VERSION)/include/speex/*.h libs/include/speex/
	cp speexdsp-$(SPEEXDSP_VERSION)/libspeexdsp/.libs/libspeexdsp.a libs/lib
	rm -r -f speexdsp-$(SPEEXDSP_VERSION)
	touch speexdsp

download/opencv-$(OPENCV_VERSION).tar.gz:
	wget --no-check-certificate https://github.com/Itseez/opencv/archive/$(OPENCV_VERSION).tar.gz -O download/opencv-$(OPENCV_VERSION).tar.gz

opencv: download/opencv-$(OPENCV_VERSION).tar.gz
	tar xvf download/opencv-$(OPENCV_VERSION).tar.gz
	mkdir -p opencv-$(OPENCV_VERSION)/build
	#cd opencv-$(OPENCV_VERSION)/build && cmake .. -G"MSYS Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_PERF_TESTS=OFF -DBUILD_TESTS=OFF -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX="`pwd`/../../libs"
	cd opencv-$(OPENCV_VERSION)/build && cmake .. -G"MSYS Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_PERF_TESTS=OFF -DBUILD_TESTS=OFF -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX="`pwd`/install"
	cd opencv-$(OPENCV_VERSION)/build && make install
	cp -r opencv-$(OPENCV_VERSION)/build/install/include/* libs/include/
	mkdir -p libs/lib/opencv && cp -r opencv-$(OPENCV_VERSION)/build/install/x86/mingw/staticlib/* libs/lib/opencv/
	rm -r -f opencv-$(OPENCV_VERSION)
	touch opencv

download/libxml2-$(LIBXML2_VERSION).tar.gz:
	wget ftp://xmlsoft.org/libxml2/libxml2-$(LIBXML2_VERSION).tar.gz -O download/libxml2-$(LIBXML2_VERSION).tar.gz

libxml2: download/libxml2-$(LIBXML2_VERSION).tar.gz
	tar xvf download/libxml2-$(LIBXML2_VERSION).tar.gz
	cd libxml2-$(LIBXML2_VERSION) && ./configure --without-iconv -enable-shared=no
	#cd libxml2-$(LIBXML2_VERSION) && make install exec_prefix="`pwd`/../libs"
	cd libxml2-$(LIBXML2_VERSION) && make
	mkdir -p libs/include/libxml && cp libxml2-$(LIBXML2_VERSION)/include/libxml/*.h libs/include/libxml/
	cp libxml2-$(LIBXML2_VERSION)/.libs/libxml2.a libs/lib/
	touch libxml2

download/libxslt-$(LIBXSLT_VERSION).tar.gz:
	wget ftp://xmlsoft.org/libxml2/libxslt-$(LIBXSLT_VERSION).tar.gz -O download/libxslt-$(LIBXSLT_VERSION).tar.gz

libxslt: download/libxml2-$(LIBXML2_VERSION).tar.gz download/libxslt-$(LIBXSLT_VERSION).tar.gz
	tar xvf download/libxml2-$(LIBXML2_VERSION).tar.gz
	tar xvf download/libxslt-$(LIBXSLT_VERSION).tar.gz
	tar xvf libxslt-$(LIBXSLT_VERSION)-fix.tar.gz
	cd libxslt-$(LIBXSLT_VERSION) && ./configure --with-libxml-src=../libxml2-$(LIBXML2_VERSION) -enable-shared=no CFLAGS=-DLIBXML_STATIC
	cd libxslt-$(LIBXSLT_VERSION) && make
	mkdir -p libs/include/libxslt && cp libxslt-$(LIBXSLT_VERSION)/libxslt/*.h libs/include/libxslt/
	cp libxslt-$(LIBXSLT_VERSION)/libxslt/.libs/libxslt.a libs/lib/
	cp libxslt-$(LIBXSLT_VERSION)/libexslt/.libs/libexslt.a libs/lib/
	rm -r -f libxml2-$(LIBXML2_VERSION)
	rm -r -f libxslt-$(LIBXSLT_VERSION)
	touch libxslt

download/curl-$(CURL_VERSION).tar.gz:
	wget --no-check-certificate http://curl.haxx.se/download/curl-$(CURL_VERSION).tar.gz -O download/curl-$(CURL_VERSION).tar.gz

curl: download/curl-$(CURL_VERSION).tar.gz
	tar xvf download/curl-$(CURL_VERSION).tar.gz
	cd curl-$(CURL_VERSION) && ./configure --disable-shared --with-ssl="`pwd`/../libs"
	#cd curl-$(CURL_VERSION) && make install exec_prefix="`pwd`/../libs"
	cd curl-$(CURL_VERSION) && make
	mkdir -p libs/include/curl && cp curl-$(CURL_VERSION)/include/curl/*.h libs/include/curl/
	cp curl-$(CURL_VERSION)/lib/.libs/libcurl.a libs/lib/
	rm -r -f curl-$(CURL_VERSION)
	touch curl

download/tcl$(TCL_VERSION)-src.tar.gz:
	wget http://prdownloads.sourceforge.net/tcl/tcl$(TCL_VERSION)-src.tar.gz -O download/tcl$(TCL_VERSION)-src.tar.gz

download/sqlcipher-$(SQLCIPHER_VERSION).tar.gz:
	wget --no-check-certificate https://github.com/sqlcipher/sqlcipher/archive/v$(SQLCIPHER_VERSION).tar.gz -O download/sqlcipher-$(SQLCIPHER_VERSION).tar.gz

sqlcipher: download/tcl$(TCL_VERSION)-src.tar.gz download/sqlcipher-$(SQLCIPHER_VERSION).tar.gz
	# tcl
	tar xvf download/tcl$(TCL_VERSION)-src.tar.gz
	mkdir -p tcl$(TCL_VERSION)/build
	cd tcl$(TCL_VERSION)/build && ../win/configure
	cd tcl$(TCL_VERSION)/build && make
	#sqlcipher
	tar xvf download/sqlcipher-$(SQLCIPHER_VERSION).tar.gz
	cd sqlcipher-$(SQLCIPHER_VERSION) && ln -s ../tcl$(TCL_VERSION)/build/tclsh86.exe tclsh
	mkdir -p tcl$(TCL_VERSION)/lib
	ln -s `pwd`/tcl$(TCL_VERSION)/library `pwd`/tcl$(TCL_VERSION)/lib/tcl8.6
	cd sqlcipher-$(SQLCIPHER_VERSION) && PATH=.:$$PATH:`pwd`/../tcl$(TCL_VERSION)/build && LIBS="-L`pwd`/../libs/lib -lgdi32 $$LIBS" && export LIBS && ./configure --disable-shared --enable-static --enable-tempstore=yes CFLAGS="-DSQLITE_HAS_CODEC -I`pwd`/../libs/include -I`pwd`/../tcl$(TCL_VERSION)/generic" LDFLAGS="-L`pwd`/../libs/lib -lcrypto -lgdi32" --with-tcl="`pwd`/../tcl$(TCL_VERSION)/build" && make install prefix="`pwd`/install"
	cp -r sqlcipher-$(SQLCIPHER_VERSION)/install/include/* libs/include/
	cp sqlcipher-$(SQLCIPHER_VERSION)/install/lib/libsqlcipher.a libs/lib/
	cp sqlcipher-$(SQLCIPHER_VERSION)/install/bin/sqlcipher.exe libs/bin/
	rm -r -f sqlcipher-$(SQLCIPHER_VERSION)
	rm -r -f tcl$(TCL_VERSION)
	touch sqlcipher

download/libmicrohttpd-$(LIBMICROHTTPD_VERSION).tar.gz:
	wget --no-check-certificate http://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-$(LIBMICROHTTPD_VERSION).tar.gz -O download/libmicrohttpd-$(LIBMICROHTTPD_VERSION).tar.gz

libmicrohttpd: download/libmicrohttpd-$(LIBMICROHTTPD_VERSION).tar.gz
	tar xvf download/libmicrohttpd-$(LIBMICROHTTPD_VERSION).tar.gz
	cd libmicrohttpd-$(LIBMICROHTTPD_VERSION) && ./configure --disable-shared --enable-static --prefix="`pwd`/../libs"
	cd libmicrohttpd-$(LIBMICROHTTPD_VERSION) && make install
	rm -r -f libmicrohttpd-$(LIBMICROHTTPD_VERSION)
	touch libmicrohttpd

download/ffmpeg-$(FFMPEG_VERSION).tar.gz:
	wget --no-check-certificate https://ffmpeg.org/releases/ffmpeg-$(FFMPEG_VERSION).tar.gz -O download/ffmpeg-$(FFMPEG_VERSION).tar.gz

ffmpeg: download/ffmpeg-$(FFMPEG_VERSION).tar.gz
	tar xvf download/ffmpeg-$(FFMPEG_VERSION).tar.gz
	cd ffmpeg-$(FFMPEG_VERSION) && ./configure --disable-shared --enable-static --disable-programs --disable-ffmpeg --disable-ffplay --disable-ffprobe --disable-ffserver --disable-doc --disable-htmlpages --disable-manpages --disable-podpages --disable-txtpages --disable-yasm --disable-everything --enable-encoder=mpeg4 --enable-decoder=mpeg4 --prefix="`pwd`/../libs"
	cd ffmpeg-$(FFMPEG_VERSION) && make install
	rm -r -f ffmpeg-$(FFMPEG_VERSION)
	touch ffmpeg


copylibs: 
	read -p "Do you want to copy libs to retroshare? (yes|no)" answer; \
	if [ "$$answer" = "yes" ] ; then  \
	  cp -r libs ../../../../ ; \
	fi

