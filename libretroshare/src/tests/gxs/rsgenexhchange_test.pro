
TEMPLATE = app
TARGET = rsgenexcahnge_test


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

#    PRE_TARGETDEPS += ../../libretroshare/src/lib/libretroshare.a
    PRE_TARGETDEPS += C:\Development\Rs\v0.5-gxs-b1/libretroshare/libretroshare-build-desktop/lib/libretroshare.a

    LIBS += C:\Development\Rs\v0.5-gxs-b1/libretroshare/libretroshare-build-desktop/lib/libretroshare.a
    LIBS += C:\Development\Rs\v0.5-gxs-b1\openpgpsdk\openpgpsdk-build-desktop\lib\libops.a
    LIBS += C:\Development\Libraries\sqlite\sqlite-autoconf-3070900\lib\libsqlite3.a
    LIBS += -L"../../../lib"
    LIBS += -lssl -lcrypto -lgpgme -lpthreadGC2d -lminiupnpc -lz -lbz2
# added after bitdht
#    LIBS += -lws2_32
        LIBS += -luuid -lole32 -liphlpapi -lcrypt32-cygwin -lgdi32
        LIBS += -lole32 -lwinmm
        RC_FILE = gui/images/retroshare_win.rc

        # export symbols for the plugins
        #LIBS += -Wl,--export-all-symbols,--out-implib,lib/libretroshare-gui.a

    GPG_ERROR_DIR = ../../../../libgpg-error-1.7
    GPGME_DIR  = ../../../../gpgme-1.1.8
    GPG_ERROR_DIR = ../../../../lib/libgpg-error-1.7
    GPGME_DIR  = ../../../../lib/gpgme-1.1.8
    INCLUDEPATH += . $${GPGME_DIR}/src $${GPG_ERROR_DIR}/src



}

INCLUDEPATH +=  C:\Development\Rs\v0.5-gxs-b1\libretroshare\src

HEADERS += \
    genexchangetestservice.h \
    genexchangetester.h \
    rsdummyservices.h \
    support.h

SOURCES += \
    genexchangetestservice.cpp \
    genexchangetester.cpp \
    rsgenexchange_test.cc \
    support.cc \
    rsdummyservices.cc
