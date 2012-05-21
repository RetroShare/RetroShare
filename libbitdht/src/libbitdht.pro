TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt
TARGET = bitdht
QMAKE_CXXFLAGS *= -Wall -DBE_DEBUG

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

# treat warnings as error for better removing
#QMAKE_CFLAGS += -Werror
#QMAKE_CXXFLAGS += -Werror

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

#################### Cross compilation for windows under Linux ####################

win32-x-g++ {	
	OBJECTS_DIR = temp/win32xgcc/obj
	DESTDIR = lib.win32xgcc
	# These have been replaced by _WIN32 && __MINGW32__
	# DEFINES *= WINDOWS_SYS WIN32 WIN_CROSS_UBUNTU
	QMAKE_CXXFLAGS *= -Wmissing-include-dirs
	QMAKE_CC = i586-mingw32msvc-g++
	QMAKE_LIB = i586-mingw32msvc-ar
	QMAKE_AR = i586-mingw32msvc-ar
	DEFINES *= STATICLIB WIN32

	INCLUDEPATH *= /usr/i586-mingw32msvc/include ${HOME}/.wine/drive_c/pthreads/include/
}
################################# Windows ##########################################

win32 {
		QMAKE_CC = g++
		OBJECTS_DIR = temp/obj
		MOC_DIR = temp/moc
		DEFINES *= STATICLIB 
		# These have been replaced by _WIN32 && __MINGW32__
		#DEFINES *= WINDOWS_SYS WIN32 STATICLIB MINGW
		DESTDIR = lib

		# Switch on extra warnings
		QMAKE_CFLAGS += -Wextra
		QMAKE_CXXFLAGS += -Wextra

		# Switch off optimization for release version
		QMAKE_CXXFLAGS_RELEASE -= -O2
		QMAKE_CXXFLAGS_RELEASE += -O0
		QMAKE_CFLAGS_RELEASE -= -O2
		QMAKE_CFLAGS_RELEASE += -O0

		# Switch on optimization for debug version
		#QMAKE_CXXFLAGS_DEBUG += -O2
		#QMAKE_CFLAGS_DEBUG += -O2
}

################################# MacOSX ##########################################

mac {
		QMAKE_CC = g++
		OBJECTS_DIR = temp/obj
		MOC_DIR = temp/moc
		DESTDIR = lib
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
	bitdht/bdiface.h	\
	bitdht/bencode.h	\
	bitdht/bdobj.h		\
	bitdht/bdmsgs.h		\
	bitdht/bdpeer.h		\
	bitdht/bdquery.h    	\
	bitdht/bdhash.h		\
	bitdht/bdstore.h	\
	bitdht/bdnode.h		\
	bitdht/bdmanager.h	\
	bitdht/bdstddht.h	\
	bitdht/bdhistory.h	\
	util/bdnet.h	\
	util/bdthreads.h	\
	util/bdrandom.h		\
	util/bdstring.h		\
	udp/udplayer.h   	\
	udp/udpstack.h		\
	udp/udpbitdht.h   	\
	bitdht/bdconnection.h	\
	bitdht/bdfilter.h	\
	bitdht/bdaccount.h	\
	bitdht/bdquerymgr.h	\
	util/bdbloom.h		\
	bitdht/bdfriendlist.h	\

SOURCES += \
	bitdht/bencode.c	\
	bitdht/bdobj.cc    	\
	bitdht/bdmsgs.cc	\
	bitdht/bdpeer.cc	\
	bitdht/bdquery.cc	\
	bitdht/bdhash.cc	\
	bitdht/bdstore.cc	\
	bitdht/bdnode.cc	\
	bitdht/bdmanager.cc	\
	bitdht/bdstddht.cc	\
	bitdht/bdhistory.cc	\
	util/bdnet.cc 	 	\
	util/bdthreads.cc  	\
	util/bdrandom.cc  	\
	util/bdstring.cc	\
	udp/udplayer.cc		\
	udp/udpstack.cc		\
	udp/udpbitdht.cc  	\
	bitdht/bdconnection.cc	\
	bitdht/bdfilter.cc	\
	bitdht/bdaccount.cc	\
	bitdht/bdquerymgr.cc	\
	util/bdbloom.cc		\
	bitdht/bdfriendlist.cc	\


