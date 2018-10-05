!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

TARGET = retroshare-service

QT += core
QT -= gui

!include("../../libretroshare/src/use_libretroshare.pri"):error("Including")

SOURCES += retroshare-service.cc

android-* {
    QT += androidextras

    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

    DISTFILES += android/AndroidManifest.xml \
        android/res/drawable/retroshare_128x128.png \
        android/res/drawable/retroshare_retroshare_48x48.png \
        android/gradle/wrapper/gradle-wrapper.jar \
        android/gradlew \
        android/res/values/libs.xml \
        android/build.gradle \
        android/gradle/wrapper/gradle-wrapper.properties \
        android/gradlew.bat
}


appimage {
    icon_files.path = "$${PREFIX}/share/icons/hicolor/scalable/"
    icon_files.files = ../data/retroshare-service.svg
    INSTALLS += icon_files

    desktop_files.path = "$${PREFIX}/share/applications"
    desktop_files.files = ../data/retroshare-service.desktop
    INSTALLS += desktop_files
}

unix {
    target.path = "$${RS_BIN_DIR}"
    INSTALLS += target
}
