#-------------------------------------------------
#
# Project created by QtCreator 2011-06-06T19:21:58
#
#-------------------------------------------------

TARGET = PeerNetQt
TEMPLATE = app

RCC_DIR = temp/qrc
UI_DIR  = temp/ui
MOC_DIR = temp/moc

CONFIG += bitdht librs
CONFIG += qt gui uic qrc resources uitools idle bitdht # framecatcher# blogs
QT     += network xml script

SOURCES += main.cpp\
        mainwindow.cpp \
	peernet.cc

HEADERS  += mainwindow.h \
	peernet.h

FORMS    += mainwindow.ui


librs {
	LIBRS_DIR = ../../libretroshare/src/
	INCLUDEPATH += $${LIBRS_DIR}
}


bitdht {
	BITDHT_DIR = ../../libbitdht/src
	INCLUDEPATH += . $${BITDHT_DIR}

	# The next line if for compliance with debian packages. Keep it!
	INCLUDEPATH += ../libbitdht
	DEFINES *= RS_USE_BITDHT

        LIBS += ../../libbitdht/src/lib/libbitdht.a
        PRE_TARGETDEPS *= ../../libbitdht/src/lib/libbitdht.a
}



macx {
    	# ENABLE THIS OPTION FOR Univeral Binary BUILD.
    	# CONFIG += ppc x86

        #CONFIG += version_detail_bash_script
        LIBS += ../../libretroshare/src/lib/libretroshare.a
        
	LIBS += -lssl -lcrypto -lz -lgpgme -lgpg-error -lassuan
        LIBS += ../../../miniupnpc-1.0/libminiupnpc.a
        LIBS += -framework CoreFoundation
        LIBS += -framework Security

        INCLUDEPATH += .
        #DEFINES* = MAC_IDLE # for idle feature
        CONFIG -= uitools

}
linux-g++ {
        #CONFIG += version_detail_bash_script
        LIBS += ../../libretroshare/src/lib/libretroshare.a
        
	LIBS += -lssl -lcrypto -lz -lgpgme -lgpg-error 

        INCLUDEPATH += .
        #DEFINES* = MAC_IDLE # for idle feature
        CONFIG -= uitools

}


############################## Common stuff ######################################

# On Linux systems that alredy have libssl and libcrypto it is advisable
# to rename the patched version of SSL to something like libsslxpgp.a and libcryptoxpg.a

# ###########################################

