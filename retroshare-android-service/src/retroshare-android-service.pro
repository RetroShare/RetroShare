!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

TARGET = retroshare-android-service

QT += core network
QT -= gui

CONFIG += c++11
CONFIG += dll

android-g++:TEMPLATE = lib
!android-g++:TEMPLATE = app

SOURCES += service.cpp

DEPENDPATH *= ../../libresapi/src
INCLUDEPATH *= ../../libresapi/src
PRE_TARGETDEPS *= ../../libresapi/src/lib/libresapi.a
LIBS *= ../../libresapi/src/lib/libresapi.a

DEPENDPATH *= ../../libretroshare/src
INCLUDEPATH *= ../../libretroshare/src
PRE_TARGETDEPS *= ../../libretroshare/src/lib/libretroshare.a
LIBS *= ../../libretroshare/src/lib/libretroshare.a
