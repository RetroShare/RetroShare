# RetroShare profile qmake build script
#
# Standalone tool to back up, restore and upgrade RetroShare nodes while
# RetroShare itself is not running. Self-contained: does not use any file of
# retroshare-gui.
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU Affero General Public License as published by the
# Free Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.
# See the GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.
#
# SPDX-FileCopyrightText: Retroshare Team <contact@retroshare.cc>
# SPDX-License-Identifier: AGPL-3.0-or-later

!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

TEMPLATE = app
TARGET = retroshare-profile

QT += core gui widgets svg

!include("../../libretroshare/src/use_libretroshare.pri"):error("Including")

INCLUDEPATH += $$PWD
DEPENDPATH  += $$PWD

HEADERS += \
    ProfileToolWindow.h \
    NodeInfo.h \
    ProfileArchive.h \
    ProfileUpgrader.h \
    ProfileUpgradeDialog.h

SOURCES += \
    main.cpp \
    ProfileToolWindow.cpp \
    NodeInfo.cpp \
    ProfileArchive.cpp \
    ProfileUpgrader.cpp \
    ProfileUpgradeDialog.cpp

FORMS += \
    ProfileToolWindow.ui \
    ProfileUpgradeDialog.ui

RESOURCES += profile.qrc

# NodeInfo reads the node certificates directly
LIBS *= -lcrypto

MOC_DIR = temp/moc
UI_DIR  = temp/ui
RCC_DIR = temp/qrc

################################# Linux ##########################################

unix {
    target.path = "$${RS_BIN_DIR}"
    INSTALLS += target
}

################################# MacOSX ##########################################

macx {
	LIBS += -lz

	for(lib, LIB_DIR):exists($$lib/libminiupnpc.a){ LIBS += $$lib/libminiupnpc.a}

	LIBS += -framework CoreFoundation
	LIBS += -framework Security
	LIBS += -framework Carbon

	# Explicitly add RNP libs (same as retroshare-service)
	LIBRNP_BUILD_PATH = $$clean_path($${OUT_PWD}/../../supportlibs/librnp/Build)
	LIBS += -L$$clean_path($${LIBRNP_BUILD_PATH}/src/lib) -lrnp
	LIBS += -L$$clean_path($${LIBRNP_BUILD_PATH}/src/libsexpp) -lsexpp
	LIBS += -lbz2 -lz -ljson-c -lbotan-2

	for(lib, LIB_DIR):LIBS += -L"$$lib"
	for(bin, BIN_DIR):LIBS += -L"$$bin"

	DEPENDPATH += . $$INC_DIR
	INCLUDEPATH += . $$INC_DIR
}

################################# Windows ##########################################

win32-g++|win32-clang-g++ {
	CONFIG(debug, debug|release) {
		CONFIG += console
	} else {
		# Tell linker to use ASLR protection
		QMAKE_LFLAGS += -Wl,-dynamicbase
		# Tell linker to use DEP protection
		QMAKE_LFLAGS += -Wl,-nxcompat
	}

	# Fix linking error (ld.exe: Error: export ordinal too large) due to too
	# many exported symbols.
	QMAKE_LFLAGS += -Wl,--exclude-libs,ALL

	OBJECTS_DIR = temp/obj

	dLib = ws2_32 gdi32 uuid ole32 iphlpapi crypt32 winmm
	LIBS *= $$linkDynamicLibs(dLib)

	RC_ICONS = ../../retroshare-service/src/logo.ico
}
