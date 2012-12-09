TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt
TARGET = pegmarkdown
QMAKE_CXXFLAGS *= -Wall
QMAKE_CFLAGS *= -Wall

profiling {
	QMAKE_CXXFLAGS -= -fomit-frame-pointer
	QMAKE_CXXFLAGS *= -pg -g -fno-omit-frame-pointer
}

release {
	# not much here yet.
}

#CONFIG += debug
debug {
        QMAKE_CXXFLAGS -= -O2 -fomit-frame-pointer
        QMAKE_CXXFLAGS *= -g -fno-omit-frame-pointer
}

################################# Linux ##########################################
linux-* {
	DESTDIR = lib
	QMAKE_CC = g++
}

linux-g++ {
	OBJECTS_DIR = temp/linux-g++/obj
}

linux-g++-64 {
	OBJECTS_DIR = temp/linux-g++-64/obj
}

################################# Windows ##########################################

win32 {
		QMAKE_CC = g++
		OBJECTS_DIR = temp/obj
		MOC_DIR = temp/moc
		DESTDIR = lib

		# Switch on extra warnings
		QMAKE_CFLAGS += -Wextra
		QMAKE_CXXFLAGS += -Wextra

		# Switch off optimization for release version
		QMAKE_CXXFLAGS_RELEASE -= -O2
		QMAKE_CXXFLAGS_RELEASE += -O0
		QMAKE_CFLAGS_RELEASE -= -O2
		QMAKE_CFLAGS_RELEASE += -O0

		CONFIG += dummy_glib 
}

################################# MacOSX ##########################################

mac {
		QMAKE_CC = g++
		OBJECTS_DIR = temp/obj
		MOC_DIR = temp/moc
		DESTDIR = lib

		CONFIG += dummy_glib 
}

################################# FreeBSD ##########################################

freebsd-* {
		DESTDIR = lib
}

################################### COMMON stuff ##################################
################################### COMMON stuff ##################################

#DEPENDPATH += . \
INCLUDEPATH += . \

HEADERS += \
	markdown_lib.h \
	markdown_peg.h \
	odf.h \
	parsing_functions.h \
	utility_functions.h \

SOURCES += \
	markdown_lib.c \
	markdown_parser.c \
	parsing_functions.c \
	markdown_output.c \
	odf.c \
	utility_functions.c \

dummy_glib {
		HEADERS += GLibFacade.h 
		SOURCES += GLibFacade.c 
}

