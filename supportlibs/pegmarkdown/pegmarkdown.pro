TEMPLATE = lib
CONFIG += staticlib
CONFIG += create_prl
CONFIG -= qt
TARGET = pegmarkdown
DESTDIR = lib

################################# Windows ##########################################

win32 {
		OBJECTS_DIR = temp/obj
		MOC_DIR = temp/moc

		# Switch on extra warnings
		QMAKE_CFLAGS += -Wextra

		CONFIG += dummy_glib 

		DEFINES *= _USE_32BIT_TIME_T

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

