TEMPLATE = lib

INCLUDEPATH += ../../libretroshare/src/ ../../retroshare-gui/src/

linux-g++ {
	LIBS *= -ldl
}
linux-g++-64 {
	LIBS *= -ldl
}

win32 {
	# Switch on extra warnings
	QMAKE_CFLAGS += -Wextra
	QMAKE_CXXFLAGS += -Wextra

	OBJECTS_DIR = temp/obj
	MOC_DIR = temp/moc
	RCC_DIR = temp/qrc
	UI_DIR  = temp/ui

	DEFINES += WINDOWS_SYS WIN32 STATICLIB MINGW
	DEFINES += MINIUPNPC_VERSION=13
#	DESTDIR = lib

	DEFINES += USE_CMD_ARGS

	#miniupnp implementation files
	HEADERS += upnp/upnputil.h
	SOURCES += upnp/upnputil.c

	UPNPC_DIR = ../../../lib/miniupnpc-1.3
	PTHREADS_DIR = ../../../lib/pthreads-w32-2-8-0-release
	ZLIB_DIR = ../../../lib/zlib-1.2.3
	SSL_DIR = ../../../openssl-1.0.1c

	INCLUDEPATH += . $${SSL_DIR}/include $${UPNPC_DIR} $${PTHREADS_DIR} $${ZLIB_DIR}

	PRE_TARGETDEPS += ../../retroshare-gui/src/lib/libretroshare-gui.a
	LIBS += -L"../../retroshare-gui/src/lib" -lretroshare-gui

	LIBS += -L"../../../lib"
	LIBS += -lssl -lcrypto -lpthreadGC2d -lminiupnpc -lz
# added after bitdht
	LIBS += -luuid -lole32 -liphlpapi -lcrypt32-cygwin -lgdi32
	LIBS += -lole32 -lwinmm
}
