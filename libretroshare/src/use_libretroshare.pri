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
