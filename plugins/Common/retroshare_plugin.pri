TEMPLATE = lib

INCLUDEPATH += ../../libretroshare/src/ ../../retroshare-gui/src/

linux-g++ {
	LIBS *= -ldl
}
linux-g++-64 {
	LIBS *= -ldl
}

win32 {
	lessThan(QT_MAJOR_VERSION, 5) {
		# from Qt 4.7.4 and 4.8+ the mkspecs has changed making dyn libs unusable anymore on windows : QMAKE_LFLAGS =
		QMAKE_LFLAGS = -enable-stdcall-fixup -Wl,-enable-auto-import -Wl,-enable-runtime-pseudo-reloc
	}

	# Switch on extra warnings
	QMAKE_CFLAGS += -Wextra
	QMAKE_CXXFLAGS += -Wextra

	OBJECTS_DIR = temp/obj
	MOC_DIR = temp/moc
	RCC_DIR = temp/qrc
	UI_DIR  = temp/ui

	DEFINES += WINDOWS_SYS WIN32 STATICLIB MINGW WIN32_LEAN_AND_MEAN _USE_32BIT_TIME_T
	DEFINES += MINIUPNPC_VERSION=13
#	DESTDIR = lib

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
	ZLIB_DIR = ../../../lib/zlib-1.2.3
	SSL_DIR = ../../../openssl-1.0.1c

	INCLUDEPATH += . $${SSL_DIR}/include $${UPNPC_DIR} $${PTHREADS_DIR} $${ZLIB_DIR}

	PRE_TARGETDEPS += ../../retroshare-gui/src/lib/libretroshare-gui.a
	LIBS += -L"../../retroshare-gui/src/lib" -lretroshare-gui

	LIBS += -L"$$PWD/../../../lib"
	LIBS += -lssl -lcrypto -lpthread -lminiupnpc -lz
# added after bitdht
	LIBS += -luuid -lole32 -liphlpapi -lcrypt32-cygwin -lgdi32
	LIBS += -lole32 -lwinmm
}
