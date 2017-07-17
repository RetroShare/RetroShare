!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

TEMPLATE = lib
CONFIG *= plugin

DEPENDPATH += $$PWD/../../libretroshare/src/ $$PWD/../../retroshare-gui/src/
INCLUDEPATH += $$PWD/../../libretroshare/src/ $$PWD/../../retroshare-gui/src/

unix {
	target.path = "$${PLUGIN_DIR}"
	INSTALLS += target
}

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

	CONFIG(debug, debug|release) {
	} else {
		# Tell linker to use ASLR protection
		QMAKE_LFLAGS += -Wl,-dynamicbase
		# Tell linker to use DEP protection
		QMAKE_LFLAGS += -Wl,-nxcompat
	}

	# solve linker warnings because of the order of the libraries
	QMAKE_LFLAGS += -Wl,--start-group

	OBJECTS_DIR = temp/obj
	MOC_DIR = temp/moc
	RCC_DIR = temp/qrc
	UI_DIR  = temp/ui

	DEFINES += WINDOWS_SYS WIN32 STATICLIB MINGW WIN32_LEAN_AND_MEAN _USE_32BIT_TIME_T
	#DEFINES += MINIUPNPC_VERSION=13
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
	#HEADERS += upnp/upnputil.h
	#SOURCES += upnp/upnputil.c

	DEPENDPATH += . $$INC_DIR
	INCLUDEPATH += . $$INC_DIR

	PRE_TARGETDEPS += $$OUT_PWD/../../retroshare-gui/src/lib/libretroshare-gui.a
	LIBS += -L"$$OUT_PWD/../../retroshare-gui/src/lib" -lretroshare-gui

	for(lib, LIB_DIR):LIBS += -L"$$lib"
	for(bin, BIN_DIR):LIBS += -L"$$bin"
	LIBS += -lpthread
}

macx {
	#You can found some information here:
	#https://developer.apple.com/library/mac/documentation/Porting/Conceptual/PortingUnix/compiling/compiling.html
	QMAKE_LFLAGS_PLUGIN -= -dynamiclib
	QMAKE_LFLAGS_PLUGIN += -bundle
	QMAKE_LFLAGS_PLUGIN += -bundle_loader "../../retroshare-gui/src/retroshare.app/Contents/MacOS/retroshare"

	OBJECTS_DIR = temp/obj
	MOC_DIR = temp/moc
	RCC_DIR = temp/qrc
	UI_DIR  = temp/ui

	DEPENDPATH += . $$INC_DIR
	INCLUDEPATH += . $$INC_DIR

	for(lib, LIB_DIR):LIBS += -L"$$lib"
	for(bin, BIN_DIR):LIBS += -L"$$bin"
}
