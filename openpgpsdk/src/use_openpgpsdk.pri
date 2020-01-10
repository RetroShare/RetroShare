# SPDX-FileCopyrightText: (C) 2004-2019 Retroshare Team <contact@retroshare.cc>
# SPDX-License-Identifier: CC0-1.0

DEPENDPATH *= $$system_path($$clean_path($${PWD}/../../openpgpsdk/src))
INCLUDEPATH  *= $$system_path($$clean_path($${PWD}/../../openpgpsdk/src))
LIBS *= -L$$system_path($$clean_path($${OUT_PWD}/../../openpgpsdk/src/lib/)) -lops

!equals(TARGET, ops):PRE_TARGETDEPS *= $$system_path($$clean_path($${OUT_PWD}/../../openpgpsdk/src/lib/libops.a))

sLibs =
mLibs = ssl crypto z bz2
dLibs =

static {
    sLibs *= $$mLibs
} else {
    dLibs *= $$mLibs
}

LIBS += $$linkStaticLibs(sLibs)
PRE_TARGETDEPS += $$pretargetStaticLibs(sLibs)

LIBS += $$linkDynamicLibs(dLibs)
