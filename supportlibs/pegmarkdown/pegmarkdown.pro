TEMPLATE = lib
CONFIG += staticlib
CONFIG += create_prl
CONFIG -= qt
TARGET = pegmarkdown
DESTDIR = lib

QMAKE_CFLAGS *= -Wall -ansi  -D_GNU_SOURCE
QMAKE_CC = gcc

#CONFIG += debug
debug {
        QMAKE_CFLAGS -= -O2 
        QMAKE_CFLAGS *= -g 
}

################################# Linux ##########################################
linux-* {
	CONFIG += link_pkgconfig

	PKGCONFIG *= glib-2.0
}

linux-g++ {
	OBJECTS_DIR = temp/linux-g++/obj
}

linux-g++-64 {
	OBJECTS_DIR = temp/linux-g++-64/obj
}

################################# Windows ##########################################

win32 {
		OBJECTS_DIR = temp/obj
		MOC_DIR = temp/moc

		# Switch on extra warnings
		QMAKE_CFLAGS += -Wextra

		# Switch off optimization for release version
		QMAKE_CFLAGS_RELEASE -= -O2
		QMAKE_CFLAGS_RELEASE += -O0

		CONFIG += dummy_glib 

		# With GCC package 4.8, including io.h either directly or indirectly causes off64_t not to be defined when compiling with -ansi switch
		DEFINES *= off64_t=_off64_t
		DEFINES *= off_t=_off_t
}

################################# MacOSX ##########################################

mac {
		OBJECTS_DIR = temp/obj
		MOC_DIR = temp/moc

		CONFIG += dummy_glib 
}

################################# FreeBSD ##########################################

freebsd-* {
}

################################# OpenBSD ##########################################

openbsd-* {
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

