################################################################################
# librssimulator.pro                                                           #
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
# GNU Affero General Public License for more details.                          #
#                                                                              #
# You should have received a copy of the GNU Affero General Public License     #
# along with this program.  If not, see <https://www.gnu.org/licenses/>.       #
################################################################################

!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

TEMPLATE = lib
CONFIG += staticlib 
CONFIG -= qt
TARGET = rssimulator

CONFIG += gxs debug

profiling {
	QMAKE_CXXFLAGS -= -fomit-frame-pointer
	QMAKE_CXXFLAGS *= -pg -g -fno-omit-frame-pointer
}


debug {
        QMAKE_CXXFLAGS -= -O2 -fomit-frame-pointer
        QMAKE_CXXFLAGS *= -g -fno-omit-frame-pointer
}

# gxs defines.
gxs {
	DEFINES *= RS_ENABLE_GXS
	DEFINES *= SQLITE_HAS_CODEC
	DEFINES *= GXS_ENABLE_SYNC_MSGS
}

INCLUDEPATH += ../../libretroshare/src/

######################### Peer ##################################

HEADERS += peer/FakeLinkMgr.h \
		peer/FakePeerMgr.h \
		peer/FakeNetMgr.h  \
		peer/FakePublisher.h \
		peer/FakeServiceControl.h \
		peer/PeerNode.h \

SOURCES += peer/PeerNode.cc \


###################### Unit Tests ###############################

HEADERS += testing/IsolatedServiceTester.h \
	testing/SetServiceTester.h \
	testing/SetPacket.h \
	testing/SetFilter.h \

SOURCES += testing/IsolatedServiceTester.cc \
	testing/SetServiceTester.cc \
	testing/SetFilter.cc \

##################### Network Sims ##############################
# to be ported over.

#HEADERS += network/Network.h \

#SOURCES += network/Network.cc \


################################# Linux ##########################################
linux-* {
	isEmpty(PREFIX)  { PREFIX = /usr }
	isEmpty(INC_DIR) { INC_DIR = $${PREFIX}/include/retroshare/ }
	isEmpty(LIB_DIR) { LIB_DIR = $${PREFIX}/lib/ }

	# These two lines fixe compilation on ubuntu natty. Probably a ubuntu packaging error.
	INCLUDEPATH += $$system(pkg-config --cflags glib-2.0 | sed -e "s/-I//g")

	OPENPGPSDK_DIR = ../../openpgpsdk/src
	INCLUDEPATH *= $${OPENPGPSDK_DIR} ../openpgpsdk

	DESTDIR = lib
	QMAKE_CXXFLAGS *= -Wall -D_FILE_OFFSET_BITS=64
	QMAKE_CC = $${QMAKE_CXX}

	SSL_DIR = /usr/include/openssl
	UPNP_DIR = /usr/include/upnp
	INCLUDEPATH += . $${SSL_DIR} $${UPNP_DIR}

	# where to put the shared library itself
	target.path = $$LIB_DIR
	INSTALLS *= target

	# where to put the librarys interface
	include_rsiface.path = $${INC_DIR}
	include_rsiface.files = $$PUBLIC_HEADERS
	INSTALLS += include_rsiface

	#CONFIG += version_detail_bash_script


	# linux/bsd can use either - libupnp is more complete and packaged.
	#CONFIG += upnp_miniupnpc 
	CONFIG += upnp_libupnp

	# Check if the systems libupnp has been Debian-patched
	system(grep -E 'char[[:space:]]+PublisherUrl' $${UPNP_DIR}/upnp.h >/dev/null 2>&1) {
		# Normal libupnp
	} else {
		# Patched libupnp or new unreleased version
		DEFINES *= PATCHED_LIBUPNP
	}

	DEFINES *= HAS_GNOME_KEYRING
	INCLUDEPATH += /usr/include/glib-2.0/ /usr/lib/glib-2.0/include
	LIBS *= -lgnome-keyring
}

linux-g++ {
	OBJECTS_DIR = temp/linux-g++/obj
}

linux-g++-64 {
	OBJECTS_DIR = temp/linux-g++-64/obj
}

version_detail_bash_script {
    QMAKE_EXTRA_TARGETS += write_version_detail
    PRE_TARGETDEPS = write_version_detail
    write_version_detail.commands = ./version_detail.sh
}

#################### Cross compilation for windows under Linux ####################

win32-x-g++ {	
	OBJECTS_DIR = temp/win32xgcc/obj
	DESTDIR = lib.win32xgcc
	DEFINES *= WINDOWS_SYS WIN32 WIN_CROSS_UBUNTU
	QMAKE_CXXFLAGS *= -Wmissing-include-dirs
	QMAKE_CC = i586-mingw32msvc-g++
	QMAKE_LIB = i586-mingw32msvc-ar
	QMAKE_AR = i586-mingw32msvc-ar
	DEFINES *= STATICLIB WIN32

	CONFIG += upnp_miniupnpc

        SSL_DIR=../../../../openssl
	UPNPC_DIR = ../../../../miniupnpc-1.3
	GPG_ERROR_DIR = ../../../../libgpg-error-1.7
	GPGME_DIR  = ../../../../gpgme-1.1.8

	INCLUDEPATH *= /usr/i586-mingw32msvc/include ${HOME}/.wine/drive_c/pthreads/include/
}
################################# Windows ##########################################

win32 {
	QMAKE_CC = $${QMAKE_CXX}
	OBJECTS_DIR = temp/obj
	MOC_DIR = temp/moc
	DEFINES *= WINDOWS_SYS WIN32 STATICLIB MINGW WIN32_LEAN_AND_MEAN
	DEFINES *= MINIUPNPC_VERSION=13
	# This defines the platform to be WinXP or later and is needed for getaddrinfo (_WIN32_WINNT_WINXP)
	DEFINES *= WINVER=0x0501
	DESTDIR = lib

	# Switch on extra warnings
	QMAKE_CFLAGS += -Wextra
	QMAKE_CXXFLAGS += -Wextra

	# Switch off optimization for release version
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O0
	QMAKE_CFLAGS_RELEASE -= -O2
	QMAKE_CFLAGS_RELEASE += -O0

	# Switch on optimization for debug version
	#QMAKE_CXXFLAGS_DEBUG += -O2
	#QMAKE_CFLAGS_DEBUG += -O2

	DEFINES += USE_CMD_ARGS

	CONFIG += upnp_miniupnpc

	for(lib, LIB_DIR):LIBS += -L"$$lib"
	for(bin, BIN_DIR):LIBS += -L"$$bin"

	DEPENDPATH += . $$INC_DIR
	INCLUDEPATH += . $$INC_DIR
}

################################# MacOSX ##########################################

mac {
	QMAKE_CC = $${QMAKE_CXX}
	OBJECTS_DIR = temp/obj
	MOC_DIR = temp/moc
	#DEFINES = WINDOWS_SYS WIN32 STATICLIB MINGW
	#DEFINES *= MINIUPNPC_VERSION=13
	DESTDIR = lib

	CONFIG += upnp_miniupnpc

	# zeroconf disabled at the end of libretroshare.pro (but need the code)
	#CONFIG += zeroconf
	#CONFIG += zcnatassist

	# Beautiful Hack to fix 64bit file access.
	QMAKE_CXXFLAGS *= -Dfseeko64=fseeko -Dftello64=ftello -Dfopen64=fopen -Dvstatfs64=vstatfs

	#UPNPC_DIR = ../../../miniupnpc-1.0
	#GPG_ERROR_DIR = ../../../../libgpg-error-1.7
	#GPGME_DIR  = ../../../../gpgme-1.1.8
	#OPENPGPSDK_DIR = ../../openpgpsdk/src
	#INCLUDEPATH += . $${UPNPC_DIR}
	#INCLUDEPATH += $${OPENPGPSDK_DIR}

	#for(lib, LIB_DIR):exists($$lib/libminiupnpc.a){ LIBS += $$lib/libminiupnpc.a}
	for(lib, LIB_DIR):LIBS += -L"$$lib"
	for(bin, BIN_DIR):LIBS += -L"$$bin"

	DEPENDPATH += . $$INC_DIR
	INCLUDEPATH += . $$INC_DIR
	INCLUDEPATH += ../../../.

	# We need a explicit path here, to force using the home version of sqlite3 that really encrypts the database.
	LIBS += /usr/local/lib/libsqlcipher.a
	#LIBS += -lsqlite3
}

################################# FreeBSD ##########################################

freebsd-* {
	INCLUDEPATH *= /usr/local/include/gpgme
	INCLUDEPATH *= /usr/local/include/glib-2.0

	QMAKE_CXXFLAGS *= -Dfseeko64=fseeko -Dftello64=ftello -Dstat64=stat -Dstatvfs64=statvfs -Dfopen64=fopen

	# linux/bsd can use either - libupnp is more complete and packaged.
	#CONFIG += upnp_miniupnpc 
	CONFIG += upnp_libupnp

	DESTDIR = lib
}

################################# OpenBSD ##########################################

openbsd-* {
	INCLUDEPATH *= /usr/local/include
	INCLUDEPATH += $$system(pkg-config --cflags glib-2.0 | sed -e "s/-I//g")

	OPENPGPSDK_DIR = ../../openpgpsdk/src
	INCLUDEPATH *= $${OPENPGPSDK_DIR} ../openpgpsdk

	QMAKE_CXXFLAGS *= -Dfseeko64=fseeko -Dftello64=ftello -Dstat64=stat -Dstatvfs64=statvfs -Dfopen64=fopen

	CONFIG += upnp_libupnp

	DESTDIR = lib
}

################################### COMMON stuff ##################################
