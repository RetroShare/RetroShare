!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

TARGET = retroshare-android-notify-service

QT += core network qml
QT -= gui

CONFIG += c++11
CONFIG += dll

RESOURCES += qml.qrc

TEMPLATE = app

android-* {
    TEMPLATE = lib
    QT += androidextras
}

HEADERS += libresapilocalclient.h notificationsbridge.h
SOURCES += libresapilocalclient.cpp main.cpp
