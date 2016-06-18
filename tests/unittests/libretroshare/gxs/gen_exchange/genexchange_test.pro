#-------------------------------------------------
#
# Project created by QtCreator 2012-05-06T09:19:26
#
#-------------------------------------------------

 
#  
QT       += core network 

QT       -= gui

CONFIG   += gen_exchange_target


CONFIG += bitdht



TARGET = gen_exchange_test

CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

CONFIG += debug

debug {
#	DEFINES *= DEBUG
#	DEFINES *= OPENDHT_DEBUG DHT_DEBUG CONN_DEBUG DEBUG_UDP_SORTER P3DISC_DEBUG DEBUG_UDP_LAYER FT_DEBUG EXTADDRSEARCH_DEBUG
#	DEFINES *= CONTROL_DEBUG FT_DEBUG DEBUG_FTCHUNK P3TURTLE_DEBUG
#	DEFINES *= P3TURTLE_DEBUG 
#	DEFINES *= NET_DEBUG
#	DEFINES *= DISTRIB_DEBUG
#	DEFINES *= P3TURTLE_DEBUG FT_DEBUG DEBUG_FTCHUNK MPLEX_DEBUG
#	DEFINES *= STATUS_DEBUG SERV_DEBUG RSSERIAL_DEBUG #CONN_DEBUG 

        QMAKE_CXXFLAGS -= -O2 -fomit-frame-pointer
        QMAKE_CXXFLAGS *= -g -fno-omit-frame-pointer
}
################################# Linux ##########################################
# Put lib dir in QMAKE_LFLAGS so it appears before -L/usr/lib
linux-* {
	#CONFIG += version_detail_bash_script
	QMAKE_CXXFLAGS *= -D_FILE_OFFSET_BITS=64

	system(which gpgme-config >/dev/null 2>&1) {
		INCLUDEPATH += $$system(gpgme-config --cflags | sed -e "s/-I//g")
	} else {
		message(Could not find gpgme-config on your system, assuming gpgme.h is in /usr/include)
	}

	PRE_TARGETDEPS *= ../../../lib/libretroshare.a

	LIBS += ../../../lib/libretroshare.a
	LIBS += ../../../../../libbitdht/src/lib/libbitdht.a	
	LIBS += ../../../../../openpgpsdk/src/lib/libops.a	
	LIBS += -lssl -lgpgme -lupnp -lixml  -lgnome-keyring  -lbz2
	# We need a explicit path here, to force using the home version of sqlite3 that really encrypts the database.
	LIBS += /home/crispy/Development/retroshare/sqlcipher/sqlcipher/.libs/libsqlite3.a
	LIBS *= -rdynamic -frtti
	DEFINES *= HAVE_XSS # for idle time, libx screensaver extensions
	DEFINES *= HAS_GNOME_KEYRING
}

linux-g++ {
	OBJECTS_DIR = temp/linux-g++/obj
}

linux-g++-64 {
	OBJECTS_DIR = temp/linux-g++-64/obj
}

#################################### Windows #####################################

win32 {

            DEFINES *= WINDOWS_SYS \
                WIN32 \
                STATICLIB \
                MINGW
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

#    PRE_TARGETDEPS += ../../libretroshare/src/lib/libretroshare.a
    PRE_TARGETDEPS += ../../../../../libretroshare/libretroshare-build-desktop/lib/libretroshare.a

    LIBS += ../../../../../libretroshare/libretroshare-build-desktop/lib/libretroshare.a
    LIBS += C:\Development\Rs\v0.5-gxs-b1\openpgpsdk\openpgpsdk-build-desktop\lib\libops.a
    LIBS += C:\Development\Libraries\sqlite\sqlite-autoconf-3070900\lib\libsqlite3.a
    LIBS += -L"../../../../../lib"
    LIBS += -lssl -lcrypto -lgpgme -lpthreadGC2d -lminiupnpc -lz -lbz2
# added after bitdht
#    LIBS += -lws2_32
        LIBS += -luuid -lole32 -liphlpapi -lcrypt32 -lgdi32
        LIBS += -lole32 -lwinmm

        # export symbols for the plugins
        #LIBS += -Wl,--export-all-symbols,--out-implib,lib/libretroshare-gui.a

    GPG_ERROR_DIR = ../../../../libgpg-error-1.7
    GPGME_DIR  = ../../../../gpgme-1.1.8
    GPG_ERROR_DIR = ../../../../lib/libgpg-error-1.7
    GPGME_DIR  = ../../../../lib/gpgme-1.1.8
    SSL_DIR = ../../../../../OpenSSL
    OPENPGPSDK_DIR = ../../../../openpgpsdk/src
    INCLUDEPATH += . $${SSL_DIR}/include $${GPGME_DIR}/src $${GPG_ERROR_DIR}/src \
                $${OPENPGPSDK_DIR}

                SQLITE_DIR = ../../../../../../Libraries/sqlite/sqlite-autoconf-3070900
                INCLUDEPATH += . \
                    $${SQLITE_DIR}




}

bitdht {

        # Chris version.
        #LIBS += ../../libbitdht/libbitdht-build-desktop/lib/libbitdht.a
        #PRE_TARGETDEPS *= ../../libbitdht/libbitdht-build-desktop/lib/libbitdht.a
}

win32 {
# must be added after bitdht
    LIBS += -lws2_32
}

version_detail_bash_script {
	DEFINES += ADD_LIBRETROSHARE_VERSION_INFO
	QMAKE_EXTRA_TARGETS += write_version_detail
	PRE_TARGETDEPS = write_version_detail
	write_version_detail.commands = ./version_detail.sh
}

install_rs {
	INSTALLS += binary_rs
	binary_rs.path = $$(PREFIX)/usr/bin
	binary_rs.files = ./RetroShare
}


gen_exchange_target {

        SOURCES += \
            ../common/support.cc \
            genexchangetester.cpp \
            genexchangetestservice.cpp \
            rsdummyservices.cc \
            gxspublishgrouptest.cc \
            gxspublishmsgtest.cc \
            rsgenexchange_test.cc

        HEADERS += ../common/support.h \
            ../data_service/rsdataservice_test.h \
            gxspublishgrouptest.h \
            gxspublishmsgtest.h \
            rsdummyservices.h \
            ../common/data_support.h \
            ../common/support.h
            
}


INCLUDEPATH += ../../../
INCLUDEPATH += ../common
