TEMPLATE = app
TARGET = retroshare-nogui
CONFIG += bitdht
#CONFIG += introserver

################################# Linux ##########################################
linux-* {
	#CONFIG += version_detail_bash_script
	QMAKE_CXXFLAGS *= -D_FILE_OFFSET_BITS=64

	system(which gpgme-config >/dev/null 2>&1) {
		INCLUDEPATH += $$system(gpgme-config --cflags | sed -e "s/-I//g")
	} else {
		message(Could not find gpgme-config on your system, assuming gpgme.h is in /usr/include)
	}

	LIBS += ../../libretroshare/src/lib/libretroshare.a
	LIBS += -lssl -lgpgme -lupnp -lixml -lgnome-keyring
}

linux-g++ {
	OBJECTS_DIR = temp/linux-g++/obj
}

linux-g++-64 {
	OBJECTS_DIR = temp/linux-g++-64/obj
}

#################### Cross compilation for windows under Linux ###################

win32-x-g++ {
	OBJECTS_DIR = temp/win32-x-g++/obj

	LIBS += ../../../../lib/win32-x-g++/libretroshare.a 
	LIBS += ../../../../lib/win32-x-g++/libssl.a 
	LIBS += ../../../../lib/win32-x-g++/libcrypto.a 
	LIBS += ../../../../lib/win32-x-g++/libminiupnpc.a 
	LIBS += ../../../../lib/win32-x-g++/libz.a 
	LIBS += -L${HOME}/.wine/drive_c/pthreads/lib -lpthreadGCE2
	LIBS += -lws2_32 -luuid -lole32 -liphlpapi -lcrypt32 -gdi32
	LIBS += -lole32 -lwinmm

	RC_FILE = gui/images/retroshare_win.rc

	DEFINES *= WIN32
}

#################################### Windows #####################################

win32 {
    CONFIG += console
    OBJECTS_DIR = temp/obj
    RCC_DIR = temp/qrc
	UI_DIR  = temp/ui
	MOC_DIR = temp/moc

    LIBS += ../../libretroshare/src/lib/libretroshare.a
    LIBS += -L"../../../../lib" -lssl -lcrypto -lpthreadGC2d -lminiupnpc -lz
    LIBS += -lssl -lcrypto -lgpgme -lpthreadGC2d -lminiupnpc -lz
# added after bitdht
#    LIBS += -lws2_32
    LIBS += -luuid -lole32 -liphlpapi -lcrypt32-cygwin -lgdi32
    LIBS += -lole32 -lwinmm
    
    RC_FILE = resources/retroshare_win.rc
    
    DEFINES *= WINDOWS_SYS
}

##################################### MacOS ######################################

macx {
    # ENABLE THIS OPTION FOR Univeral Binary BUILD.
    # CONFIG += ppc x86 

    LIBS += -Wl,-search_paths_first
}

##################################### FreeBSD ######################################

freebsd-* {
	INCLUDEPATH *= /usr/local/include/gpgme
	LIBS *= ../../libretroshare/src/lib/libretroshare.a
	LIBS *= -lssl
	LIBS *= -lgpgme
	LIBS *= -lupnp
	LIBS *= -lgnome-keyring
	PRE_TARGETDEPS *= ../../libretroshare/src/lib/libretroshare.a
}

############################## Common stuff ######################################

# bitdht config
bitdht {
	LIBS += ../../libbitdht/src/lib/libbitdht.a
}

win32 {
# must be added after bitdht
    LIBS += -lws2_32
}

DEPENDPATH += ../../libretroshare/src
            
INCLUDEPATH += . ../../libretroshare/src

# Input
HEADERS +=  notifytxt.h 
SOURCES +=  notifytxt.cc \
            retroshare.cc 

introserver {
	HEADERS += introserver.h
	SOURCES += introserver.cc
	DEFINES *= RS_INTRO_SERVER
}





