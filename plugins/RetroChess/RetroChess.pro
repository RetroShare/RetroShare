!include("../Common/retroshare_plugin.pri"): error("Could not include file ../Common/retroshare_plugin.pri")

greaterThan(QT_MAJOR_VERSION, 4) {
	# Qt 5
	QT += widgets
}

exists($$[QMAKE_MKSPECS]/features/mobility.prf) {
  CONFIG += mobility
} else {
  QT += multimedia
}
CONFIG += qt uic qrc resources
MOBILITY = multimedia

DEPENDPATH += ../../retroshare-gui/src/temp/ui ../../libretroshare/src
INCLUDEPATH += ../../retroshare-gui/src/temp/ui ../../libretroshare/src
INCLUDEPATH += ../../retroshare-gui/src/retroshare-gui

INCLUDEPATH += ../../rapidjson-1.1.0

#################################### Windows #####################################

linux-* {
	#INCLUDEPATH += /usr/include
	#LIBS += $$system(pkg-config --libs opencv)
}

win32 {
	LIBS_DIR = $$PWD/../../../libs
	#LIBS += -L"$$LIBS_DIR/lib/opencv"

	#OPENCV_VERSION = 249
	#LIBS += -lopencv_core$$OPENCV_VERSION -lopencv_highgui$$OPENCV_VERSION -lopencv_imgproc$$OPENCV_VERSION -llibjpeg -llibtiff -llibpng -llibjasper -lIlmImf -lole32 -loleaut32 -luuid -lavicap32 -lavifil32 -lvfw32 -lz
}

QMAKE_CXXFLAGS *= -Wall

SOURCES = RetroChessPlugin.cpp               \
          services/p3RetroChess.cc           \
          services/rsRetroChessItems.cc \
    gui/NEMainpage.cpp \
    gui/RetroChessNotify.cpp \
    gui/chess.cpp \
    gui/tile.cpp \
    gui/validation.cpp \
    gui/RetroChessChatWidgetHolder.cpp

HEADERS = RetroChessPlugin.h                 \
          services/p3RetroChess.h            \
          services/rsRetroChessItems.h       \
          interface/rsRetroChess.h \
    gui/NEMainpage.h \
    gui/RetroChessNotify.h \
    gui/tile.h \
    gui/validation.h \
    gui/chess.h \
    gui/RetroChessChatWidgetHolder.h

#FORMS   = gui/AudioInputConfig.ui

TARGET = RetroChess

RESOURCES = gui/RetroChess_images.qrc


#LIBS += -lspeex -lspeexdsp

FORMS += \
    gui/NEMainpage.ui
