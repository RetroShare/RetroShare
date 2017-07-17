!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

TEMPLATE = app
TARGET = retroshare-nogui
CONFIG += bitdht
#CONFIG += introserver
CONFIG -= qt xml gui
CONFIG += link_prl

#CONFIG += debug
debug {
        QMAKE_CFLAGS -= -O2
        QMAKE_CFLAGS += -O0
        QMAKE_CFLAGS += -g

        QMAKE_CXXFLAGS -= -O2
        QMAKE_CXXFLAGS += -O0
        QMAKE_CXXFLAGS += -g
}

################################# Linux ##########################################
linux-* {
	#CONFIG += version_detail_bash_script
	QMAKE_CXXFLAGS *= -D_FILE_OFFSET_BITS=64

	LIBS *= -rdynamic
}

unix {
	target.path = "$${BIN_DIR}"
	INSTALLS += target
}

linux-g++ {
	OBJECTS_DIR = temp/linux-g++/obj
}

linux-g++-64 {
	OBJECTS_DIR = temp/linux-g++-64/obj
}

#################### Cross compilation for windows under Linux ###################

win32-x-g++ {
	OBJECTS_DIR = temp/win32-x-g++/obj

	LIBS += ../../../../lib/win32-x-g++/libssl.a 
	LIBS += ../../../../lib/win32-x-g++/libcrypto.a 
	LIBS += ../../../../lib/win32-x-g++/libminiupnpc.a 
	LIBS += ../../../../lib/win32-x-g++/libz.a 
	LIBS += -L${HOME}/.wine/drive_c/pthreads/lib -lpthreadGCE2
	LIBS += -lws2_32 -luuid -lole32 -liphlpapi -lcrypt32 -gdi32
	LIBS += -lole32 -lwinmm

	RC_FILE = gui/images/retroshare_win.rc

	DEFINES *= WIN32
}

#################################### Windows #####################################

win32 {
	CONFIG += console
	OBJECTS_DIR = temp/obj
	RCC_DIR = temp/qrc
	UI_DIR  = temp/ui
	MOC_DIR = temp/moc

	# solve linker warnings because of the order of the libraries
	QMAKE_LFLAGS += -Wl,--start-group

	CONFIG(debug, debug|release) {
	} else {
		# Tell linker to use ASLR protection
		QMAKE_LFLAGS += -Wl,-dynamicbase
		# Tell linker to use DEP protection
		QMAKE_LFLAGS += -Wl,-nxcompat
	}

	for(lib, LIB_DIR):LIBS += -L"$$lib"
	LIBS += -lssl -lcrypto -lpthread -lminiupnpc -lz
	LIBS += -lcrypto -lws2_32 -lgdi32
	LIBS += -luuid -lole32 -liphlpapi -lcrypt32
	LIBS += -lole32 -lwinmm

	RC_FILE = resources/retroshare_win.rc

	DEFINES *= WINDOWS_SYS _USE_32BIT_TIME_T

	DEPENDPATH += . $$INC_DIR
	INCLUDEPATH += . $$INC_DIR
}

##################################### MacOS ######################################

macx {
	# ENABLE THIS OPTION FOR Univeral Binary BUILD.
	# CONFIG += ppc x86

	LIBS += -Wl,-search_paths_first
	LIBS += -lssl -lcrypto -lz
	for(lib, LIB_DIR):exists($$lib/libminiupnpc.a){ LIBS += $$lib/libminiupnpc.a}
	LIBS += -framework CoreFoundation
	LIBS += -framework Security
	for(lib, LIB_DIR):LIBS += -L"$$lib"
	for(bin, BIN_DIR):LIBS += -L"$$bin"

	DEPENDPATH += . $$INC_DIR
	INCLUDEPATH += . $$INC_DIR

	QMAKE_CXXFLAGS *= -Dfseeko64=fseeko -Dftello64=ftello -Dstat64=stat -Dstatvfs64=statvfs -Dfopen64=fopen
}

##################################### FreeBSD ######################################

freebsd-* {
	INCLUDEPATH *= /usr/local/include/gpgme
	LIBS *= -lssl
	LIBS *= -lgpgme
	LIBS *= -lupnp
	LIBS *= -lgnome-keyring
}

##################################### OpenBSD  ######################################

openbsd-* {
	INCLUDEPATH *= /usr/local/include
	QMAKE_CXXFLAGS *= -Dfseeko64=fseeko -Dftello64=ftello -Dstat64=stat -Dstatvfs64=statvfs -Dfopen64=fopen
	LIBS *= -lssl -lcrypto
	LIBS *= -lgpgme
	LIBS *= -lupnp
	LIBS *= -lgnome-keyring
	LIBS *= -rdynamic
}

##################################### Haiku ######################################

haiku-* {
	QMAKE_CXXFLAGS *= -D_BSD_SOURCE

	PRE_TARGETDEPS *= ../../libretroshare/src/lib/libretroshare.a
	PRE_TARGETDEPS *= ../../openpgpsdk/src/lib/libops.a

	LIBS *= ../../libretroshare/src/lib/libretroshare.a
	LIBS *= ../../openpgpsdk/src/lib/libops.a -lbz2 -lbsd
	LIBS *= -lssl -lcrypto -lnetwork
	LIBS *= -lgpgme
	LIBS *= -lupnp
	LIBS *= -lz
	LIBS *= -lixml

	LIBS += ../../supportlibs/pegmarkdown/lib/libpegmarkdown.a
	LIBS += -lsqlite3
	
}

############################## Common stuff ######################################

DEPENDPATH += . $$PWD/../../libretroshare/src
INCLUDEPATH += . $$PWD/../../libretroshare/src

PRE_TARGETDEPS *= $$OUT_PWD/../../libretroshare/src/lib/libretroshare.a
LIBS *= $$OUT_PWD/../../libretroshare/src/lib/libretroshare.a

# Input
HEADERS +=  notifytxt.h
SOURCES +=  notifytxt.cc \
            retroshare.cc

introserver {
	HEADERS += introserver.h
	SOURCES += introserver.cc
	DEFINES *= RS_INTRO_SERVER
}

libresapihttpserver {
	DEFINES *= ENABLE_WEBUI
        PRE_TARGETDEPS *= $$OUT_PWD/../../libresapi/src/lib/libresapi.a
	LIBS += $$OUT_PWD/../../libresapi/src/lib/libresapi.a
        DEPENDPATH += $$PWD/../../libresapi/src
	INCLUDEPATH += $$PWD/../../libresapi/src
        HEADERS += \
            TerminalApiClient.h
        SOURCES +=  \
            TerminalApiClient.cpp
}
