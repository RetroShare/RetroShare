#-------------------------------------------------
#
# Project created by QtCreator 2012-05-06T09:19:26
#
#-------------------------------------------------

QT       += core network 

QT       -= gui

TARGET = rs_test
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

	PRE_TARGETDEPS *= /home/crispy/workspace/v0.5-gxs-b1/libretroshare/src/lib/libretroshare.a

	LIBS += /home/crispy/workspace/v0.5-gxs-b1/libretroshare/src/lib/libretroshare.a
	LIBS += /home/crispy/workspace/v0.5-gxs-b1/libbitdht/src/lib/libbitdht.a	
	LIBS += /home/crispy/workspace/v0.5-gxs-b1/openpgpsdk/src/lib/libops.a	
	LIBS += -lssl -lgpgme -lupnp -lixml  -lgnome-keyring -lsqlite3 -lbz2
	LIBS *= -rdynamic -frtti
	DEFINES *= HAVE_XSS # for idle time, libx screensaver extensions
	DEFINES *= UBUNTU
}

linux-g++ {
	OBJECTS_DIR = temp/linux-g++/obj
}

linux-g++-64 {
	OBJECTS_DIR = temp/linux-g++-64/obj
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


SOURCES += \
    /home/crispy/workspace/v0.5-gxs-b1/libretroshare/src/tests/gxs/data_support.cc \
    /home/crispy/workspace/v0.5-gxs-b1/libretroshare/src/tests/gxs/nxstesthub.cc \
    /home/crispy/workspace/v0.5-gxs-b1/libretroshare/src/tests/gxs/nxstestscenario.cc \
    /home/crispy/workspace/v0.5-gxs-b1/libretroshare/src/tests/gxs/rsgxsnetservice_test.cc \
    /home/crispy/workspace/v0.5-gxs-b1/libretroshare/src/tests/gxs/support.cc
    

HEADERS += \
    /home/crispy/workspace/v0.5-gxs-b1/libretroshare/src/tests/gxs/data_support.h \
    /home/crispy/workspace/v0.5-gxs-b1/libretroshare/src/tests/gxs/nxstesthub.h \
    /home/crispy/workspace/v0.5-gxs-b1/libretroshare/src/tests/gxs/nxstestscenario.h \
    /home/crispy/workspace/v0.5-gxs-b1/libretroshare/src/tests/gxs/support.h 

INCLUDEPATH += /home/crispy/workspace/v0.5-gxs-b1/libretroshare/src

