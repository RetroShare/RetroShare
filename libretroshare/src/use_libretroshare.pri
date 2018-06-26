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

# when rapidjson is mainstream on all distribs, we will not need the sources
# anymore in the meantime, they are part of the RS directory so that it is
# always possible to find them
INCLUDEPATH *= $$system_path($$clean_path($${PWD}/../../rapidjson-1.1.0))

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

static {
    sLibs *= $$mLibs
} else {
    dLibs *= $$mLibs
}

LIBS += $$linkStaticLibs(sLibs)
PRE_TARGETDEPS += $$pretargetStaticLibs(sLibs)

LIBS += $$linkDynamicLibs(dLibs)
