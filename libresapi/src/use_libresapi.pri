DEPENDPATH *= $$system_path($$clean_path($$PWD/../../libresapi/src))
INCLUDEPATH  *= $$system_path($$clean_path($${PWD}/../../libresapi/src))
LIBS *= -L$$system_path($$clean_path($${OUT_PWD}/../../libresapi/src/lib/)) -lresapi

!equals(TARGET, resapi):PRE_TARGETDEPS *= $$system_path($$clean_path($${OUT_PWD}/../../libresapi/src/lib/libresapi.a))

!include("../../libretroshare/src/use_libretroshare.pri"):error("Including")

sLibs =
mLibs =
dLibs =

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

