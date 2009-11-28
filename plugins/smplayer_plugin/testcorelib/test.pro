TEMPLATE = app
LANGUAGE = C++

CONFIG  += qt warn_on release

DEFINES += MINILIB NO_USE_INI_FILES
INCLUDEPATH += ../corelib ..
DEPENDPATH += ..

HEADERS = myslider.h timeslider.h test.h
SOURCES = myslider.cpp timeslider.cpp test.cpp

#SOURCES = test2.cpp

LIBS += -L../corelib -L../corelib/release -lsmplayercore

unix {
	UI_DIR = .ui
	MOC_DIR = .moc
	OBJECTS_DIR = .obj
}

win32 {
	CONFIG += console
}
