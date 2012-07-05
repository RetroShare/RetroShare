#-------------------------------------------------
#
# Project created by QtCreator 2012-05-06T09:19:26
#
#-------------------------------------------------

QT       += core network

QT       -= gui

TARGET = rs_test
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

win32 {
        # Switch on extra warnings
        QMAKE_CFLAGS += -Wextra
        QMAKE_CXXFLAGS += -Wextra

        # Switch off optimization for release version
        QMAKE_CXXFLAGS_RELEASE -= -O2
        QMAKE_CXXFLAGS_RELEASE += -O0
        QMAKE_CFLAGS_RELEASE -= -O2
        QMAKE_CFLAGS_RELEASE += -O0

        # Switch on optimization for debug version
        #QMAKE_CXXFLAGS_DEBUG += -O2
        #QMAKE_CFLAGS_DEBUG += -O2
    DEFINES *= WINDOWS_SYS
    PRE_TARGETDEPS += C:\Development\Rs\v0.5-new_cache_system\libretroshare\libretroshare-build-desktop\lib\libretroshare.a
    LIBS += C:\Development\Rs\v0.5-new_cache_system\libretroshare\libretroshare-build-desktop\lib\libretroshare.a
    LIBS += -L"../lib"
    LIBS += -lssl -lcrypto -lgpgme -lpthreadGC2d -lminiupnpc -lz
# added after bitdht
#    LIBS += -lws2_32
        LIBS += -luuid -lole32 -liphlpapi -lcrypt32-cygwin -lgdi32
        LIBS += -lole32 -lwinmm

        # export symbols for the plugins
        #LIBS += -Wl,--export-all-symbols,--out-implib,lib/libretroshare-gui.a

    GPG_ERROR_DIR = ../../../../libgpg-error-1.7
    GPGME_DIR  = ../../../../gpgme-1.1.8
    GPG_ERROR_DIR = ../../../../lib/libgpg-error-1.7
    GPGME_DIR  = ../../../../lib/gpgme-1.1.8
    INCLUDEPATH += . $${GPGME_DIR}/src $${GPG_ERROR_DIR}/src ../../Libraries/sqlite/sqlite-autoconf-3070900
    LIBS += C:\Development\Libraries\sqlite\sqlite-autoconf-3070900\.libs\libsqlite3.a
}

win32 {
# must be added after bitdht
    LIBS += -lws2_32
}

SOURCES += \
    support.cc \
    #rsnxsitems_test.cc
    rsdataservice_test.cc \
    data_support.cc
    #rsnxsservice_test.cc \
    #nxstesthub.cc
    #rsgxsdata_test.cc

HEADERS += support.h \
    #rsnxsitems_test.h
    rsdataservice_test.h \
    data_support.h
    #rsnxsservice_test.h \
    #nxstesthub.h

INCLUDEPATH += C:\Development\Rs\v0.5-new_cache_system\libretroshare\src
