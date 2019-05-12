################################################################################
# retroshare-nogui.pri                                                         #
# Copyright (C) 2018, Retroshare team <retroshare.team@gmailcom>               #
#                                                                              #
# This program is free software: you can redistribute it and/or modify         #
# it under the terms of the GNU Affero General Public License as               #
# published by the Free Software Foundation, either version 3 of the           #
# License, or (at your option) any later version.                              #
#                                                                              #
# This program is distributed in the hope that it will be useful,              #
# but WITHOUT ANY WARRANTY; without even the implied warranty of               #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                #
# GNU Lesser General Public License for more details.                          #
#                                                                              #
# You should have received a copy of the GNU Lesser General Public License     #
# along with this program.  If not, see <https://www.gnu.org/licenses/>.       #
################################################################################

!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

TEMPLATE = app
TARGET = retroshare-nogui
CONFIG -= qt xml gui

DEPENDPATH  *= $${PWD} $${RS_INCLUDE_DIR}
INCLUDEPATH *= $${PWD}

libresapihttpserver {
    !include("../../libresapi/src/use_libresapi.pri"):error("Including")

    HEADERS += TerminalApiClient.h
    SOURCES += TerminalApiClient.cpp
} else {
    !include("../../libretroshare/src/use_libretroshare.pri"):error("Including")
}



################################# Linux ##########################################
linux-* {
        CONFIG += link_pkgconfig
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

rs_sanitize {
	LIBS *= -lasan -lubsan
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

    ## solve linker warnings because of the order of the libraries
    #QMAKE_LFLAGS += -Wl,--start-group

    CONFIG(debug, debug|release) {
    } else {
		# Tell linker to use ASLR protection
		QMAKE_LFLAGS += -Wl,-dynamicbase
		# Tell linker to use DEP protection
		QMAKE_LFLAGS += -Wl,-nxcompat
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
## Introserver is broken (doesn't compile) should be either fixed or removed

	HEADERS += introserver.h
	SOURCES += introserver.cc
	DEFINES *= RS_INTRO_SERVER
}
