################################################################################
# uselibretroshare.pri                                                         #
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
DEPENDPATH *= $$system_path($$clean_path($${PWD}/../../libretroshare/src/))
INCLUDEPATH  *= $$system_path($$clean_path($${PWD}/../../libretroshare/src))
LIBS *= -L$$system_path($$clean_path($${OUT_PWD}/../../libretroshare/src/lib/)) -lretroshare

equals(TARGET, retroshare):equals(TEMPLATE, lib){
} else {
    PRE_TARGETDEPS *= $$system_path($$clean_path($$OUT_PWD/../../libretroshare/src/lib/libretroshare.a))
}

!include("../../openpgpsdk/src/use_openpgpsdk.pri"):error("Including")

bitdht {
    !include("../../libbitdht/src/use_libbitdht.pri"):error("Including")
}

sLibs =
mLibs = $$RS_SQL_LIB ssl crypto $$RS_THREAD_LIB $$RS_UPNP_LIB
dLibs =

linux-* {
    mLibs += dl
}

static {
    sLibs *= $$mLibs
} else {
    dLibs *= $$mLibs
}

LIBS += $$linkStaticLibs(sLibs)
PRE_TARGETDEPS += $$pretargetStaticLibs(sLibs)

LIBS += $$linkDynamicLibs(dLibs)
