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

DEPENDPATH  *= $${PWD} $${RS_INCLUDE_DIR}
INCLUDEPATH *= $${PWD} $${RS_INCLUDE_DIR}

INCLUDEPATH  *= $$system_path($${PWD}/../../libbitdht/src)
QMAKE_LIBDIR *= $$system_path($${OUT_PWD}/../../libbitdht/src/lib)

INCLUDEPATH  *= $$system_path($${PWD}/../../openpgpsdk/src)
QMAKE_LIBDIR *= $$system_path($${OUT_PWD}/../../openpgpsdk/src/lib)

INCLUDEPATH  *= $$system_path($${PWD}/../../libretroshare/src)
QMAKE_LIBDIR *= $$system_path($${OUT_PWD}/../../libretroshare/src/lib)

mSqlLib = sqlcipher
no_sqlcipher:mSqlLib = sqlite3


sLibs = retroshare ops bitdht
mLibs = ssl crypto pthread z bz2 $$mSqlLib
dLibs =

libresapihttpserver {
    DEFINES *= ENABLE_WEBUI

    sLibs = resapi $$sLibs

    DEPENDPATH += $$PWD/../../libresapi/src
    INCLUDEPATH  *= $$system_path($${PWD}/../../libresapi/src)
    QMAKE_LIBDIR *= $$system_path($${OUT_PWD}/../../libresapi/src/lib)

    HEADERS += TerminalApiClient.h
    SOURCES += TerminalApiClient.cpp
}

static {
    sLibs *= $$mLibs
} else {
    dLibs *= $$mLibs
}

LIBS += $$linkStaticLibs(sLibs)
PRE_TARGETDEPS += $$pretargetStaticLibs(sLibs)

LIBS += $$linkDynamicLibs(dLibs)


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
}

#################################### Windows #####################################

win32-g++ {
	CONFIG += console
	OBJECTS_DIR = temp/obj
	RCC_DIR = temp/qrc
	UI_DIR  = temp/ui
	MOC_DIR = temp/moc
    DEFINES *= _USE_32BIT_TIME_T

    ## solve linker warnings because of the order of the libraries
    #QMAKE_LFLAGS += -Wl,--start-group

    CONFIG(debug, debug|release) {}
    else {
		# Tell linker to use ASLR protection
		QMAKE_LFLAGS += -Wl,-dynamicbase
		# Tell linker to use DEP protection
		QMAKE_LFLAGS += -Wl,-nxcompat
	}

    upnpLib = miniupnpc
    static {
        LIBS *= $$linkStaticLibs(upnpLib)
        PRE_TARGETDEPS += $$pretargetStaticLibs(upnpLib)
    } else {
        LIBS *= $$linkDynamicLibs(upnpLib)
    }

    dLib = ws2_32 gdi32 uuid ole32 iphlpapi crypt32 winmm
    LIBS *= $$linkDynamicLibs(dLib)

	RC_FILE = resources/retroshare_win.rc
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


# Input
HEADERS +=  notifytxt.h
SOURCES +=  notifytxt.cc \
            retroshare.cc

introserver {
	HEADERS += introserver.h
	SOURCES += introserver.cc
	DEFINES *= RS_INTRO_SERVER
}
