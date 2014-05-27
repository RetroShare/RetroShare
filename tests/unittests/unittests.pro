QT     += network xml script
CONFIG += bitdht

CONFIG += gxs debug

LIBS += -lgtest

gxs {
	DEFINES += RS_ENABLE_GXS
}

TEMPLATE = app
TARGET = unittests

OPENPGPSDK_DIR = ../../openpgpsdk/src
INCLUDEPATH *= $${OPENPGPSDK_DIR} ../openpgpsdk

################################# Linux ##########################################
# Put lib dir in QMAKE_LFLAGS so it appears before -L/usr/lib
linux-* {
	#CONFIG += version_detail_bash_script
	QMAKE_CXXFLAGS *= -D_FILE_OFFSET_BITS=64

	PRE_TARGETDEPS *= ../../libretroshare/src/lib/libretroshare.a
	PRE_TARGETDEPS *= ../../openpgpsdk/src/lib/libops.a

	LIBS += ../../libretroshare/src/lib/libretroshare.a
	LIBS += ../librssimulator/lib/librssimulator.a
	LIBS += ../../openpgpsdk/src/lib/libops.a -lbz2
	LIBS += -lssl -lupnp -lixml -lXss -lgnome-keyring
	LIBS *= -lcrypto -ldl -lX11 -lz

gxs {
		LIBS += ../../supportlibs/pegmarkdown/lib/libpegmarkdown.a

		# We need a explicit path here, to force using the home version of sqlite3 that really encrypts the database.
		LIBS += ../../../lib/sqlcipher/.libs/libsqlcipher.a
	}

	LIBS *= -lglib-2.0
	LIBS *= -rdynamic
	DEFINES *= HAVE_XSS # for idle time, libx screensaver extensions
	DEFINES *= UBUNTU
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

		LIBS += ../../libretroshare/src/lib.win32xgcc/libretroshare.a
		LIBS += ../../../../lib/win32-x-g++-v0.5/libssl.a
		LIBS += ../../../../lib/win32-x-g++-v0.5/libcrypto.a
		LIBS += ../../../../lib/win32-x-g++-v0.5/libgpgme.dll.a
		LIBS += ../../../../lib/win32-x-g++-v0.5/libminiupnpc.a
		LIBS += ../../../../lib/win32-x-g++-v0.5/libz.a
		LIBS += -L${HOME}/.wine/drive_c/pthreads/lib -lpthreadGCE2
		LIBS += -lQtUiTools
		LIBS += -lws2_32 -luuid -lole32 -liphlpapi -lcrypt32 -gdi32
		LIBS += -lole32 -lwinmm

		DEFINES *= WINDOWS_SYS WIN32 WIN32_CROSS_UBUNTU

		INCLUDEPATH += ../../../../gpgme-1.1.8/src/
		INCLUDEPATH += ../../../../libgpg-error-1.7/src/

		RC_FILE = gui/images/retroshare_win.rc
}

#################################### Windows #####################################

win32 {
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

	OBJECTS_DIR = temp/obj
	#LIBS += -L"D/Qt/2009.03/qt/plugins/imageformats"
	#QTPLUGIN += qjpeg

	PRE_TARGETDEPS *= ../../libretroshare/src/lib/libretroshare.a
	PRE_TARGETDEPS *= ../librssimulator/lib/librssimulator.a
	PRE_TARGETDEPS *= ../../openpgpsdk/src/lib/libops.a

	LIBS += ../../libretroshare/src/lib/libretroshare.a
	LIBS += ../../openpgpsdk/src/lib/libops.a -lbz2
	LIBS += -L"$$PWD/../../../lib"

	gxs {
		LIBS += ../../supportlibs/pegmarkdown/lib/libpegmarkdown.a
		LIBS += -lsqlcipher
	}

	LIBS += -lssl -lcrypto -lpthread -lminiupnpc -lz
# added after bitdht
#	LIBS += -lws2_32
	LIBS += -luuid -lole32 -liphlpapi -lcrypt32-cygwin -lgdi32
	LIBS += -lole32 -lwinmm
	RC_FILE = gui/images/retroshare_win.rc

	# export symbols for the plugins
	LIBS += -Wl,--export-all-symbols,--out-implib,lib/libretroshare-gui.a

	# create lib directory
	QMAKE_PRE_LINK = $(CHK_DIR_EXISTS) lib $(MKDIR) lib

	DEFINES *= WINDOWS_SYS WIN32_LEAN_AND_MEAN _USE_32BIT_TIME_T

	INCLUDEPATH += .
}

##################################### MacOS ######################################

macx {
    # ENABLE THIS OPTION FOR Univeral Binary BUILD.
    	CONFIG += ppc x86
	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.4

	CONFIG += version_detail_bash_script
	LIBS += ../../libretroshare/src/lib/libretroshare.a
	LIBS += ../librssimulator/lib/librssimulator.a
	LIBS += ../../openpgpsdk/src/lib/libops.a -lbz2
        LIBS += -lssl -lcrypto -lz 
        #LIBS += -lssl -lcrypto -lz -lgpgme -lgpg-error -lassuan
	LIBS += ../../../miniupnpc-1.0/libminiupnpc.a
	LIBS += -framework CoreFoundation
	LIBS += -framework Security

        gxs {
                LIBS += ../../supportlibs/pegmarkdown/lib/libpegmarkdown.a

		LIBS += ../../../lib/libsqlcipher.a
                #LIBS += -lsqlite3

        }


    	INCLUDEPATH += .
	#DEFINES* = MAC_IDLE # for idle feature
	CONFIG -= uitools


}

##################################### FreeBSD ######################################

freebsd-* {
	INCLUDEPATH *= /usr/local/include/gpgme
	LIBS *= ../../libretroshare/src/lib/libretroshare.a
	LIBS *= ../librssimulator/lib/librssimulator.a
	LIBS *= -lssl
	LIBS *= -lgpgme
	LIBS *= -lupnp
	LIBS *= -lgnome-keyring
	PRE_TARGETDEPS *= ../../libretroshare/src/lib/libretroshare.a

        gxs {
                LIBS += ../../supportlibs/pegmarkdown/lib/libpegmarkdown.a
                LIBS += -lsqlite3
        }

}

##################################### OpenBSD ######################################

openbsd-* {
	INCLUDEPATH *= /usr/local/include

	PRE_TARGETDEPS *= ../../libretroshare/src/lib/libretroshare.a
	PRE_TARGETDEPS *= ../../openpgpsdk/src/lib/libops.a

	LIBS *= ../../libretroshare/src/lib/libretroshare.a
	LIBS *= ../librssimulator/lib/librssimulator.a
	LIBS *= ../../openpgpsdk/src/lib/libops.a -lbz2
	LIBS *= -lssl -lcrypto
	LIBS *= -lgpgme
	LIBS *= -lupnp
	LIBS *= -lgnome-keyring
	PRE_TARGETDEPS *= ../../libretroshare/src/lib/libretroshare.a

        gxs {
                LIBS += ../../supportlibs/pegmarkdown/lib/libpegmarkdown.a
                LIBS += -lsqlite3
        }

	LIBS *= -rdynamic
}



############################## Common stuff ######################################

# On Linux systems that alredy have libssl and libcrypto it is advisable
# to rename the patched version of SSL to something like libsslxpgp.a and libcryptoxpg.a

# ###########################################

bitdht {
	LIBS += ../../libbitdht/src/lib/libbitdht.a
	PRE_TARGETDEPS *= ../../libbitdht/src/lib/libbitdht.a
}

win32 {
# must be added after bitdht
    LIBS += -lws2_32
}

DEPENDPATH += . \

INCLUDEPATH += ../../libretroshare/src/
INCLUDEPATH += ../librssimulator/

SOURCES +=  unittests.cc \

################################ Serialiser ################################
HEADERS +=  libretroshare/serialiser/support.h \
	libretroshare/serialiser/rstlvutil.h \

SOURCES +=  libretroshare/serialiser/rsturtleitem_test.cc \
		libretroshare/serialiser/rsbaseitem_test.cc \
		libretroshare/serialiser/rsgxsupdateitem_test.cc \
		libretroshare/serialiser/rsmsgitem_test.cc \
		libretroshare/serialiser/rsstatusitem_test.cc \
		libretroshare/serialiser/rsnxsitems_test.cc \
		libretroshare/serialiser/rsgxsiditem_test.cc \
		libretroshare/serialiser/rsphotoitem_test.cc \
		libretroshare/serialiser/tlvbase_test2.cc \
		libretroshare/serialiser/tlvrandom_test.cc \
		libretroshare/serialiser/tlvbase_test.cc \
		libretroshare/serialiser/tlvstack_test.cc \
		libretroshare/serialiser/tlvitems_test.cc \
		libretroshare/serialiser/tlvtypes_test.cc \
		libretroshare/serialiser/tlvkey_test.cc \
		libretroshare/serialiser/support.cc \
		libretroshare/serialiser/rstlvutil.cc \

# Still to convert these.
#		libretroshare/serialiser/rsconfigitem_test.cc \
#		libretroshare/serialiser/rsgrouteritem_test.cc \


################################## GXS #####################################

HEADERS += libretroshare/gxs/common/data_support.h \

SOURCES += libretroshare/gxs/common/data_support.cc \

HEADERS +=  libretroshare/gxs/nxs_test/nxsdummyservices.h \
	libretroshare/gxs/nxs_test/nxsgrptestscenario.h \
	libretroshare/gxs/nxs_test/nxsmsgtestscenario.h \
	libretroshare/gxs/nxs_test/nxsgrpsync_test.h \
	libretroshare/gxs/nxs_test/nxsmsgsync_test.h \
	libretroshare/gxs/nxs_test/nxstesthub.h \
	libretroshare/gxs/nxs_test/nxstestscenario.h \
	libretroshare/gxs/nxs_test/nxsgrpsyncdelayed.h

SOURCES +=  libretroshare/gxs/nxs_test/nxsdummyservices.cc \
	libretroshare/gxs/nxs_test/nxsgrptestscenario.cc \
	libretroshare/gxs/nxs_test/nxsmsgtestscenario.cc \
	libretroshare/gxs/nxs_test/nxstesthub.cc \
	libretroshare/gxs/nxs_test/rsgxsnetservice_test.cc \
	libretroshare/gxs/nxs_test/nxsmsgsync_test.cc \
	libretroshare/gxs/nxs_test/nxsgrpsync_test.cc \ 
	libretroshare/gxs/nxs_test/nxsgrpsyncdelayed.cc
	
HEADERS += libretroshare/gxs/gen_exchange/genexchangetester.h \
	libretroshare/gxs/gen_exchange/gxspublishmsgtest.h \
	libretroshare/gxs/gen_exchange/genexchangetestservice.h \
	libretroshare/gxs/gen_exchange/gxspublishgrouptest.h \
	libretroshare/gxs/gen_exchange/rsdummyservices.h \

#	libretroshare/gxs/gen_exchange/gxsmsgrelatedtest.h \

SOURCES += libretroshare/gxs/gen_exchange/gxspublishgrouptest.cc \
	libretroshare/gxs/gen_exchange/gxspublishmsgtest.cc \
	libretroshare/gxs/gen_exchange/rsdummyservices.cc \
	libretroshare/gxs/gen_exchange/rsgenexchange_test.cc \
	libretroshare/gxs/gen_exchange/genexchangetester.cc \
	libretroshare/gxs/gen_exchange/genexchangetestservice.cc \

SOURCES += libretroshare/gxs/security/gxssecurity_test.cc

#	libretroshare/gxs/gen_exchange/gxsmsgrelatedtest.cc \

HEADERS += libretroshare/gxs/data_service/rsdataservice_test.h \

SOURCES += libretroshare/gxs/data_service/rsdataservice_test.cc \
	libretroshare/gxs/data_service/rsgxsdata_test.cc \


################################ dbase #####################################


SOURCES += libretroshare/dbase/fisavetest.cc \
	libretroshare/dbase/fitest2.cc \
	libretroshare/dbase/searchtest.cc \

#	libretroshare/dbase/ficachetest.cc \
#	libretroshare/dbase/fimontest.cc \


############################### services ###################################

SOURCES += libretroshare/services/status/status_test.cc \

############################### gxs ########################################

HEADERS += libretroshare/services/gxs/rsgxstestitems.h \
	libretroshare/services/gxs/gxstestservice.h \
	libretroshare/services/gxs/GxsIsolatedServiceTester.h \
	libretroshare/services/gxs/GxsPeerNode.h \
	libretroshare/services/gxs/GxsPairServiceTester.h \
	libretroshare/services/gxs/FakePgpAuxUtils.h \

#	libretroshare/services/gxs/RsGxsNetServiceTester.h \

SOURCES += libretroshare/services/gxs/rsgxstestitems.cc \
	libretroshare/services/gxs/gxstestservice.cc \
	libretroshare/services/gxs/GxsIsolatedServiceTester.cc \
	libretroshare/services/gxs/GxsPeerNode.cc \
	libretroshare/services/gxs/GxsPairServiceTester.cc \
	libretroshare/services/gxs/FakePgpAuxUtils.cc \
	libretroshare/services/gxs/nxsbasic_test.cc \
	libretroshare/services/gxs/nxspair_tests.cc \
	libretroshare/services/gxs/gxscircle_tests.cc \

#	libretroshare/services/gxs/gxscircle_mintest.cc \


#	libretroshare/services/gxs/RsGxsNetServiceTester.cc \
