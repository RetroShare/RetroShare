!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

TARGET = retroshare-android-service

QT += core network
QT -= gui

CONFIG += c++11
android-g++:CONFIG += dll

android-g++:TEMPLATE = lib
!android-g++:TEMPLATE = app

DEPENDPATH *= ../../libresapi/src
INCLUDEPATH *= ../../libresapi/src
PRE_TARGETDEPS *= ../../libresapi/src/lib/libresapi.a
LIBS *= ../../libresapi/src/lib/libresapi.a

DEPENDPATH *= ../../libretroshare/src
INCLUDEPATH *= ../../libretroshare/src
PRE_TARGETDEPS *= ../../libretroshare/src/lib/libretroshare.a
LIBS *= ../../libretroshare/src/lib/libretroshare.a

win32 {
        OBJECTS_DIR = temp/obj

        LIBS_DIR = $$PWD/../../libs/lib
        LIBS += $$OUT_PWD/../../libretroshare/src/lib/libretroshare.a
        LIBS += $$OUT_PWD/../../openpgpsdk/src/lib/libops.a

        for(lib, LIB_DIR):LIBS += -L"$$lib"
        for(bin, BIN_DIR):LIBS += -L"$$bin"


        LIBS += -lssl -lcrypto -lpthread -lminiupnpc -lz -lws2_32
        LIBS += -luuid -lole32 -liphlpapi -lcrypt32 -lgdi32
        LIBS += -lwinmm

        DEFINES *= WINDOWS_SYS WIN32_LEAN_AND_MEAN _USE_32BIT_TIME_T

        DEPENDPATH += . $$INC_DIR
        INCLUDEPATH += . $$INC_DIR

        greaterThan(QT_MAJOR_VERSION, 4) {
                # Qt 5
                RC_INCLUDEPATH += $$_PRO_FILE_PWD_/../../libretroshare/src
        } else {
                # Qt 4
                QMAKE_RC += --include-dir=$$_PRO_FILE_PWD_/../../libretroshare/src
        }
}

SOURCES += service.cpp
