!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

TARGET = retroshare-service

QT += core
QT -= gui

!include("../../libretroshare/src/use_libretroshare.pri"):error("Including")

SOURCES += retroshare-service.cc

android-* {
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

    DISTFILES += android/AndroidManifest.xml \
        android/res/drawable/retroshare_128x128.png \
        android/res/drawable/retroshare_retroshare_48x48.png
}

DISTFILES += \
    android/AndroidManifest.xml \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradlew \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew.bat
