!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt
TARGET = bitdht
DESTDIR = lib

################################# Linux ##########################################

unix {
	data_files.path = "$${DATA_DIR}"
	data_files.files = bitdht/bdboot.txt
	INSTALLS += data_files
}


#################### Cross compilation for windows under Linux ####################

win32-x-g++ {	
	OBJECTS_DIR = temp/win32xgcc/obj
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
		DEFINES *= STATICLIB WIN32_LEAN_AND_MEAN _USE_32BIT_TIME_T
		# These have been replaced by _WIN32 && __MINGW32__
		#DEFINES *= WINDOWS_SYS WIN32 STATICLIB MINGW
}

################################### COMMON stuff ##################################

DEPENDPATH += .
INCLUDEPATH += .

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
	util/bdfile.h		\
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
	util/bdfile.cc		\
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
