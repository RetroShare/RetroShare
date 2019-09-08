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

macx {
	# ENABLE THIS OPTION FOR Univeral Binary BUILD.
	#CONFIG += ppc x86
	#QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.4
	LIBS += -lz 
        #LIBS += -lssl -lcrypto -lz -lgpgme -lgpg-error -lassuan

	for(lib, LIB_DIR):exists($$lib/libminiupnpc.a){ LIBS += $$lib/libminiupnpc.a}

	LIBS += -framework CoreFoundation
	LIBS += -framework Security
	LIBS += -framework Carbon

	for(lib, LIB_DIR):LIBS += -L"$$lib"
	for(bin, BIN_DIR):LIBS += -L"$$bin"

	DEPENDPATH += . $$INC_DIR
	INCLUDEPATH += . $$INC_DIR
}

win32-g++ {
	CONFIG(debug, debug|release) {
		# show console output
		CONFIG += console
	} else {
		CONFIG -= console
	}

	# Switch on extra warnings
	QMAKE_CFLAGS += -Wextra
	QMAKE_CXXFLAGS += -Wextra

	CONFIG(debug, debug|release) {
	} else {
		# Tell linker to use ASLR protection
		QMAKE_LFLAGS += -Wl,-dynamicbase
		# Tell linker to use DEP protection
		QMAKE_LFLAGS += -Wl,-nxcompat
	}

    # Fix linking error (ld.exe: Error: export ordinal too large) due to too
    # many exported symbols.
    QMAKE_LFLAGS+=-Wl,--exclude-libs,ALL

	# Switch off optimization for release version
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O0
	QMAKE_CFLAGS_RELEASE -= -O2
	QMAKE_CFLAGS_RELEASE += -O0

	# Switch on optimization for debug version
	#QMAKE_CXXFLAGS_DEBUG += -O2
	#QMAKE_CFLAGS_DEBUG += -O2

	OBJECTS_DIR = temp/obj

    dLib = ws2_32 gdi32 uuid ole32 iphlpapi crypt32 winmm
    LIBS *= $$linkDynamicLibs(dLib)

	# export symbols for the plugins
	LIBS += -Wl,--export-all-symbols,--out-implib,lib/libretroshare-service.a

	# create lib directory
	isEmpty(QMAKE_SH) {
		QMAKE_PRE_LINK = $(CHK_DIR_EXISTS) lib $(MKDIR) lib
	} else {
		QMAKE_PRE_LINK = $(CHK_DIR_EXISTS) lib || $(MKDIR) lib
	}
}


