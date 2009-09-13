TEMPLATE = app
TARGET = retroshare-nogui

################################# Linux ##########################################

linux-g++ {
	OBJECTS_DIR = temp/linux-g++/obj

	LIBS += ../../libretroshare/src/lib.linux-g++/libretroshare.a 
	LIBS += ../../../../build/lib.linux-g++/libminiupnpc.a 
	LIBS += ../../../../build/lib.linux-g++/libssl_xpgp.a 
	LIBS += ../../../../build/lib.linux-g++/libcrypto_xpgp.a
    LIBS += -lz -lpthread
}
linux-g++-64 {
	OBJECTS_DIR = temp/linux-g++-64/obj

	LIBS += ../../libretroshare/src/lib.linux-g++-64/libretroshare.a 
	LIBS += ../../../../build/lib.linux-g++-64/libminiupnpc.a 
	LIBS += ../../../../build/lib.linux-g++-64/libssl_xpgp.a 
	LIBS += ../../../../build/lib.linux-g++-64/libcrypto_xpgp.a
    LIBS += -lz
}

#################### Cross compilation for windows under Linux ###################

win32-x-g++ {
	OBJECTS_DIR = temp/win32-x-g++/obj

	LIBS += ../../libretroshare/src/lib.win32xgcc/libretroshare.a 
	LIBS += ../../../../lib/win32-x-g++/libssl_xpgp.a 
	LIBS += ../../../../lib/win32-x-g++/libcrypto_xpgp.a 
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
    OBJECTS_DIR = temp/obj
    RCC_DIR = temp/qrc
	UI_DIR  = temp/ui
	MOC_DIR = temp/moc

    LIBS += -L"../../../../lib" -lretroshare -lssl -lcrypto -lpthreadGC2d -lminiupnpc -lz
    LIBS += -lws2_32 -luuid -lole32 -liphlpapi -lcrypt32-cygwin -gdi32
    LIBS += -lole32 -lwinmm
}

##################################### MacOS ######################################

macx {
    # ENABLE THIS OPTION FOR Univeral Binary BUILD.
    # CONFIG += ppc x86 

    LIBS += -Wl,-search_paths_first
}

############################## Common stuff ######################################

DEPENDPATH += ../../libretroshare/src
            
LIBS *= -lpthread
INCLUDEPATH += . ../../libretroshare/src

# Input
HEADERS +=  notifytxt.h 
SOURCES +=  notifytxt.cc \
            retroshare.cc 






