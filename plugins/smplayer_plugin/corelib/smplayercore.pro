TEMPLATE = lib
LANGUAGE = c++
CONFIG += qt warn_on release staticlib

INCLUDEPATH = ..
DEPENDOATH = ..

DEFINES += MINILIB NO_USE_INI_FILES

HEADERS += ../config.h \
        ../constants.h \
        ../global.h \
        ../helper.h \
        ../subtracks.h \
        ../audiotracks.h \
        ../titletracks.h \
        ../mediadata.h \
        ../mediasettings.h \
        ../preferences.h \
        ../myprocess.h \
        ../mplayerversion.h \
        ../mplayerprocess.h \
        ../infoprovider.h \
        ../desktopinfo.h \
        ../mplayerwindow.h \
        ../core.h \
        smplayercorelib.h


SOURCES += ../global.cpp \
        ../helper.cpp \
        ../subtracks.cpp \
        ../audiotracks.cpp \
        ../titletracks.cpp \
        ../mediadata.cpp \
        ../mediasettings.cpp \
        ../preferences.cpp \
        ../myprocess.cpp \
        ../mplayerversion.cpp \
        ../mplayerprocess.cpp \
        ../infoprovider.cpp \
        ../desktopinfo.cpp \
        ../mplayerwindow.cpp \
        ../core.cpp \
        smplayercorelib.cpp


unix {
	UI_DIR = .ui
	MOC_DIR = .moc
	OBJECTS_DIR = .obj
}

win32 {
	HEADERS += ../screensaver.h
	SOURCES += ../screensaver.cpp
}
