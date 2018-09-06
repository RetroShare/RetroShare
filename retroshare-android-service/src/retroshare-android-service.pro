!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

TARGET = retroshare-android-service

QT += core network
QT -= gui

CONFIG += c++11
android-*:CONFIG += dll

android-*:TEMPLATE = lib
!android-*:TEMPLATE = app

libresapilocalserver {
    !include("../../libresapi/src/use_libresapi.pri"):error("Including")
}

!include("../../libretroshare/src/use_libretroshare.pri"):error("Including")

SOURCES += service.cpp
