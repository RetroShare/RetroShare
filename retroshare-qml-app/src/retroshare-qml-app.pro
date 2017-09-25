!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

QT += core network qml quick svg

CONFIG += c++11

HEADERS += libresapilocalclient.h \
    rsqmlappengine.h \
    androidimagepicker.h \
    platforminteracions.h
SOURCES += main-app.cpp \
    libresapilocalclient.cpp \
    rsqmlappengine.cpp

RESOURCES += qml.qrc


# Platform interaction specific code

android-g++ {
    QT += androidextras
    HEADERS += NativeCalls.h androidplatforminteracions.h
    SOURCES += NativeCalls.cpp androidplatforminteracions.cpp
} else {
    HEADERS += defaultplatforminteracions.h
    SOURCES += defaultplatforminteracions.cpp
}

# Additional import path used to resolve QML modules in Qt Creator's code model
#QML_IMPORT_PATH =
#QML2_IMPORT_PATH =


# Default rules for deployment.
include(deployment.pri)

DISTFILES += \
    android/AndroidManifest.xml \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradlew \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew.bat \
    icons/retroshare.png

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

DEPENDPATH *= ../../libretroshare/src
INCLUDEPATH *= ../../libretroshare/src
PRE_TARGETDEPS *= ../../libretroshare/src/lib/libretroshare.a
LIBS *= ../../libretroshare/src/lib/libretroshare.a
