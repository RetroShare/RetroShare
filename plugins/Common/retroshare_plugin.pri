TEMPLATE = lib

INCLUDEPATH += ../../libretroshare/src/ ../../retroshare-gui/src/

linux-g++ {
	LIBS *= -ldl
}
linux-g++-64 {
	LIBS *= -ldl
}

win32 {
        QMAKE_CC = g++
        OBJECTS_DIR = temp/obj
        MOC_DIR = temp/moc
		RCC_DIR = temp/qrc
		UI_DIR  = temp/ui

        DEFINES *= WINDOWS_SYS WIN32 STATICLIB MINGW
        DEFINES *= MINIUPNPC_VERSION=13
#        DESTDIR = lib

        # Switch off optimization for release version
        QMAKE_CXXFLAGS_RELEASE -= -O2
        QMAKE_CXXFLAGS_RELEASE += -O0
        QMAKE_CFLAGS_RELEASE -= -O2
        QMAKE_CFLAGS_RELEASE += -O0

        # Switch on optimization for debug version
        #QMAKE_CXXFLAGS_DEBUG += -O2
        #QMAKE_CFLAGS_DEBUG += -O2

        DEFINES += USE_CMD_ARGS

        #miniupnp implementation files
        HEADERS += upnp/upnputil.h
        SOURCES += upnp/upnputil.c


        UPNPC_DIR = ../../../lib/miniupnpc-1.3
        GPG_ERROR_DIR = ../../../lib/libgpg-error-1.7
        GPGME_DIR  = ../../../lib/gpgme-1.1.8

        PTHREADS_DIR = ../../../lib/pthreads-w32-2-8-0-release
        ZLIB_DIR = ../../../lib/zlib-1.2.3
        SSL_DIR = ../../../../OpenSSL


        INCLUDEPATH += . $${SSL_DIR}/include $${UPNPC_DIR} $${PTHREADS_DIR} $${ZLIB_DIR} $${GPGME_DIR}/src $${GPG_ERROR_DIR}/src

        PRE_TARGETDEPS += ../../libretroshare/src/lib/libretroshare.a
        LIBS += ../../libretroshare/src/lib/libretroshare.a

        LIBS += ../../libbitdht/src/lib/libbitdht.a
        PRE_TARGETDEPS *= ../../libbitdht/src/lib/libbitdht.a

        LIBS += -L"../../../../lib"
        LIBS += -lssl -lcrypto -lgpgme -lpthreadGC2d -lminiupnpc -lz
# added after bitdht
        LIBS += -lws2_32
        LIBS += -luuid -lole32 -liphlpapi -lcrypt32-cygwin -lgdi32
        LIBS += -lole32 -lwinmm

        GPG_ERROR_DIR = ../../../lib/libgpg-error-1.7
        GPGME_DIR  = ../../../lib/gpgme-1.1.8
}
