# RetroShare service qmake build script
#
# Copyright (C) 2021-2021, retroshare team <retroshare.project@gmail.com>
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

TARGET = retroshare-friendserver

!include("../../libretroshare/src/use_libretroshare.pri"):error("Including")

SOURCES += retroshare-friendserver.cc \
           friendserver.cc \
           network.cc 

HEADERS += friendserver.h \
           network.h      \
           fsitem.h	   

################################# Linux ##########################################

unix {
    target.path = "$${RS_BIN_DIR}"
    INSTALLS += target
}

################################# Windows ##########################################

win32-g++|win32-clang-g++ {
    dLib = ws2_32 iphlpapi crypt32
    LIBS *= $$linkDynamicLibs(dLib)
    CONFIG += console
}

################################### COMMON stuff ##################################

