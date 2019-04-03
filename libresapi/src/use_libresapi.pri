################################################################################
# uselibresapi.pri                                                             #
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
DEPENDPATH *= $$system_path($$clean_path($$PWD/../../libresapi/src))
INCLUDEPATH  *= $$system_path($$clean_path($${PWD}/../../libresapi/src))
LIBS *= -L$$system_path($$clean_path($${OUT_PWD}/../../libresapi/src/lib/)) -lresapi

!equals(TARGET, resapi):PRE_TARGETDEPS *= $$system_path($$clean_path($${OUT_PWD}/../../libresapi/src/lib/libresapi.a))

!include("../../libretroshare/src/use_libretroshare.pri"):error("Including")

sLibs =
mLibs =
dLibs =

libresapilocalserver {
    CONFIG *= qt
    QT *= network
}

libresapi_settings {
    CONFIG *= qt
    QT *= core
}

libresapihttpserver {
    mLibs *= microhttpd
}

static {
    sLibs *= $$mLibs
} else {
    dLibs *= $$mLibs
}

LIBS += $$linkStaticLibs(sLibs)
PRE_TARGETDEPS += $$pretargetStaticLibs(sLibs)

LIBS += $$linkDynamicLibs(dLibs)

