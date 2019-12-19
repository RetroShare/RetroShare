# RetroShare main qmake build script
#
# Copyright (C) 2004-2019, Retroshare Team <contact@retroshare.cc>
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the
# Free Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.
# See the GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <https://www.gnu.org/licenses/>.
#
# SPDX-FileCopyrightText: Retroshare Team <contact@retroshare.cc>
# SPDX-License-Identifier: LGPL-3.0-or-later

!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt
TARGET = bitdht
DESTDIR = lib

!include("use_libbitdht.pri"):error("Including")

QMAKE_CXXFLAGS *= -Wall -DBE_DEBUG

# treat warnings as error for better removing
#QMAKE_CFLAGS += -Werror
#QMAKE_CXXFLAGS += -Werror

################################# Linux ##########################################
linux-* {
	QMAKE_CC = $${QMAKE_CXX}
}

linux-g++ {
	OBJECTS_DIR = temp/linux-g++/obj
}

linux-g++-64 {
	OBJECTS_DIR = temp/linux-g++-64/obj
}

unix {
	data_files.path = "$${DATA_DIR}"
	data_files.files = bitdht/bdboot.txt
	INSTALLS += data_files
}

android-* {
    # see https://community.kde.org/Necessitas/Assets
    bdboot.files=bitdht/bdboot.txt
    bdboot.path=/assets/values
    INSTALLS += bdboot
}

#################### Cross compilation for windows under Linux ####################

win32-x-g++ {	
	OBJECTS_DIR = temp/win32xgcc/obj
	# These have been replaced by _WIN32 && __MINGW32__
	# DEFINES *= WINDOWS_SYS WIN32 WIN_CROSS_UBUNTU
	QMAKE_CXXFLAGS *= -Wmissing-include-dirs
	QMAKE_CC = i586-mingw32msvc-g++
	QMAKE_LIB = i586-mingw32msvc-ar
	QMAKE_AR = i586-mingw32msvc-ar
	DEFINES *= STATICLIB WIN32

	INCLUDEPATH *= /usr/i586-mingw32msvc/include ${HOME}/.wine/drive_c/pthreads/include/
}
################################# Windows ##########################################

win32 {
		QMAKE_CC = $${QMAKE_CXX}
		OBJECTS_DIR = temp/obj
		MOC_DIR = temp/moc
        DEFINES *= STATICLIB WIN32_LEAN_AND_MEAN
		# These have been replaced by _WIN32 && __MINGW32__
		#DEFINES *= WINDOWS_SYS WIN32 STATICLIB MINGW

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
}

################################# MacOSX ##########################################

mac {
		QMAKE_CC = $${QMAKE_CXX}
		OBJECTS_DIR = temp/obj
		MOC_DIR = temp/moc
}

################################# FreeBSD ##########################################

freebsd-* {
}

################################# OpenBSD ##########################################

openbsd-* {
}

################################# Haiku ##########################################

haiku-* {
		DESTDIR = lib
}

################################### COMMON stuff ##################################
################################### COMMON stuff ##################################

DEPENDPATH += .
INCLUDEPATH += .

HEADERS += \
	bitdht/bdiface.h	\
	bitdht/bencode.h	\
	bitdht/bdobj.h		\
	bitdht/bdmsgs.h		\
	bitdht/bdpeer.h		\
	bitdht/bdquery.h    	\
	bitdht/bdhash.h		\
	bitdht/bdstore.h	\
	bitdht/bdnode.h		\
	bitdht/bdmanager.h	\
	bitdht/bdstddht.h	\
	bitdht/bdhistory.h	\
	util/bdnet.h	\
	util/bdthreads.h	\
	util/bdrandom.h		\
	util/bdfile.h		\
	util/bdstring.h		\
	udp/udplayer.h   	\
	udp/udpstack.h		\
	udp/udpbitdht.h   	\
	bitdht/bdconnection.h	\
	bitdht/bdfilter.h	\
	bitdht/bdaccount.h	\
	bitdht/bdquerymgr.h	\
	util/bdbloom.h		\
	bitdht/bdfriendlist.h	\

SOURCES += \
	bitdht/bencode.c	\
	bitdht/bdobj.cc    	\
	bitdht/bdmsgs.cc	\
	bitdht/bdpeer.cc	\
	bitdht/bdquery.cc	\
	bitdht/bdhash.cc	\
	bitdht/bdstore.cc	\
	bitdht/bdnode.cc	\
	bitdht/bdmanager.cc	\
	bitdht/bdstddht.cc	\
	bitdht/bdhistory.cc	\
	util/bdnet.cc 	 	\
	util/bdthreads.cc  	\
	util/bdrandom.cc  	\
	util/bdfile.cc		\
	util/bdstring.cc	\
	udp/udplayer.cc		\
	udp/udpstack.cc		\
	udp/udpbitdht.cc  	\
	bitdht/bdconnection.cc	\
	bitdht/bdfilter.cc	\
	bitdht/bdaccount.cc	\
	bitdht/bdquerymgr.cc	\
	util/bdbloom.cc		\
	bitdht/bdfriendlist.cc	\
