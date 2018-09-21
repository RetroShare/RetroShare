################################################################################
# uselibretroshare.pri                                                         #
# Copyright (C) 2018, Retroshare team <retroshare.team@gmailcom>               #
#                                                                              #
# This program is free software: you can redistribute it and/or modify         #
# it under the terms of the GNU Lesser General Public License as               #
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
DEPENDPATH *= $$system_path($$clean_path($${PWD}/../../libretroshare/src/))
INCLUDEPATH  *= $$system_path($$clean_path($${PWD}/../../libretroshare/src))

equals(TARGET, retroshare):equals(TEMPLATE, lib){
} else {
    staticlibretroshare:PRE_TARGETDEPS *= $$system_path($$clean_path($$OUT_PWD/../../libretroshare/src/lib/libretroshare.a))
    LIBS *= -L$$system_path($$clean_path($${OUT_PWD}/../../libretroshare/src/lib/)) -lretroshare
}

!include("../../openpgpsdk/src/use_openpgpsdk.pri"):error("Including")

bitdht {
    !include("../../libbitdht/src/use_libbitdht.pri"):error("Including")
}

# when rapidjson is mainstream on all distribs, we will not need the sources
# anymore in the meantime, they are part of the RS directory so that it is
# always possible to find them
RAPIDJSON_AVAILABLE = $$system(pkg-config --atleast-version 1.1 RapidJSON && echo yes)
isEmpty(RAPIDJSON_AVAILABLE) {
    message("using built-in rapidjson")
    INCLUDEPATH *= $$system_path($$clean_path($${PWD}/../../rapidjson-1.1.0))
} else {
    message("using systems rapidjson")
    DEFINES *= HAS_RAPIDJSON
}


sLibs =
mLibs = $$RS_SQL_LIB ssl crypto $$RS_THREAD_LIB $$RS_UPNP_LIB
dLibs =

rs_jsonapi {
    RS_SRC_PATH=$$system_path($$clean_path($${PWD}/../../))
    RS_BUILD_PATH=$$system_path($$clean_path($${OUT_PWD}/../../))
    RESTBED_SRC_PATH=$$system_path($$clean_path($${RS_SRC_PATH}/supportlibs/restbed))
    RESTBED_BUILD_PATH=$$system_path($$clean_path($${RS_BUILD_PATH}/supportlibs/restbed))

    INCLUDEPATH *= $$system_path($$clean_path($${RESTBED_BUILD_PATH}/include/))
    QMAKE_LIBDIR *= $$system_path($$clean_path($${RESTBED_BUILD_PATH}/library/))
    # Using sLibs would fail as librestbed.a is generated at compile-time
    LIBS *= -L$$system_path($$clean_path($${RESTBED_BUILD_PATH}/library/)) -lrestbed
}

linux-* {
    mLibs += dl
}

rs_deep_search {
    mLibs += xapian
    win32-g++:mLibs += rpcrt4
}

static {
    sLibs *= $$mLibs
} else {
    dLibs *= $$mLibs
}

LIBS += $$linkStaticLibs(sLibs)
PRE_TARGETDEPS += $$pretargetStaticLibs(sLibs)

LIBS += $$linkDynamicLibs(dLibs)
