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
RS_SRC_PATH=$$clean_path($${PWD}/../../)
RS_BUILD_PATH=$$clean_path($${OUT_PWD}/../../)

DEPENDPATH *= $$clean_path($${RS_SRC_PATH}/libretroshare/src/)
INCLUDEPATH  *= $$clean_path($${RS_SRC_PATH}/libretroshare/src)
LIBS *= -L$$clean_path($${RS_BUILD_PATH}/libretroshare/src/lib/) -lretroshare

equals(TARGET, retroshare):equals(TEMPLATE, lib){
} else {
    PRE_TARGETDEPS *= $$clean_path($${RS_BUILD_PATH}/libretroshare/src/lib/libretroshare.a)
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
    message("using rapidjson from submodule")
    INCLUDEPATH *= $$clean_path($${PWD}/../../supportlibs/rapidjson/include)
} else {
    message("using system rapidjson")
}


sLibs =
mLibs = $$RS_SQL_LIB ssl crypto $$RS_THREAD_LIB $$RS_UPNP_LIB
dLibs =

rs_jsonapi {
    no_rs_cross_compiling {
        RESTBED_SRC_PATH=$$clean_path($${RS_SRC_PATH}/supportlibs/restbed)
        RESTBED_BUILD_PATH=$$clean_path($${RS_BUILD_PATH}/supportlibs/restbed)
        INCLUDEPATH *= $$clean_path($${RESTBED_BUILD_PATH}/include/)
        DEPENDPATH *= $$clean_path($${RESTBED_BUILD_PATH}/include/)
        QMAKE_LIBDIR *= $$clean_path($${RESTBED_BUILD_PATH}/)
        # Using sLibs would fail as librestbed.a is generated at compile-time
        LIBS *= -L$$clean_path($${RESTBED_BUILD_PATH}/) -lrestbed
    } else:sLibs *= restbed

    win32-g++:dLibs *= wsock32
}

linux-* {
    mLibs += dl
}

rs_deep_channels_index | rs_deep_files_index {
    mLibs += xapian
    win32-g++:mLibs += rpcrt4
}

rs_deep_files_index_ogg {
    mLibs += vorbisfile
}

rs_deep_files_index_flac {
    mLibs += FLAC++
}

rs_deep_files_index_taglib {
    mLibs += tag
}

rs_broadcast_discovery {
    no_rs_cross_compiling {
        UDP_DISCOVERY_SRC_PATH=$$clean_path($${RS_SRC_PATH}/supportlibs/udp-discovery-cpp/)
        UDP_DISCOVERY_BUILD_PATH=$$clean_path($${RS_BUILD_PATH}/supportlibs/udp-discovery-cpp/)
        INCLUDEPATH *= $$clean_path($${UDP_DISCOVERY_SRC_PATH})
        DEPENDPATH *= $$clean_path($${UDP_DISCOVERY_BUILD_PATH})
        QMAKE_LIBDIR *= $$clean_path($${UDP_DISCOVERY_BUILD_PATH})
        # Using sLibs would fail as libudp-discovery.a is generated at compile-time
        LIBS *= -L$$clean_path($${UDP_DISCOVERY_BUILD_PATH}) -ludp-discovery
    } else:sLibs *= udp-discovery

    win32-g++:dLibs *= wsock32
}

static {
    sLibs *= $$mLibs
} else {
    dLibs *= $$mLibs
}

LIBS += $$linkStaticLibs(sLibs)
PRE_TARGETDEPS += $$pretargetStaticLibs(sLibs)

LIBS += $$linkDynamicLibs(dLibs)

android-* {
## ifaddrs is missing on Android to add them don't use the one from
## https://github.com/morristech/android-ifaddrs
## because it crash, use QNetworkInterface from Qt instead
    CONFIG *= qt
    QT *= network
}
