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
		QMAKE_LFLAGS = -Wl,-enable-stdcall-fixup -Wl,-enable-auto-import -Wl,-enable-runtime-pseudo-reloc
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

	LIBS_DIR = $$PWD/../../../libs

	INCLUDEPATH += . $$LIBS_DIR/include $$LIBS_DIR/include/miniupnpc

	PRE_TARGETDEPS += ../../retroshare-gui/src/lib/libretroshare-gui.a
	LIBS += -L"../../retroshare-gui/src/lib" -lretroshare-gui

	LIBS += -L"$$LIBS_DIR/lib"
	LIBS += -lpthread
}
