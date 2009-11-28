CONFIG += debug

HEADERS = ../filechooser.h videopreviewconfigdialog.h videopreview.h 
SOURCES = ../filechooser.cpp videopreviewconfigdialog.cpp videopreview.cpp main.cpp

FORMS = ../filechooser.ui videopreviewconfigdialog.ui

INCLUDEPATH += ..
DEPENDPATH += ..
DEFINES += NO_SMPLAYER_SUPPORT

unix {
	UI_DIR = .ui
	MOC_DIR = .moc
	OBJECTS_DIR = .obj
}

