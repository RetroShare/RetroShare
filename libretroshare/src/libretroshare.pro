TEMPLATE = lib
#CONFIG += staticlib release
#CONFIG += staticlib testnetwork
CONFIG += staticlib testnetwork bitdht
CONFIG -= qt
TARGET = retroshare

# Beware: All data of the stripped services are lost
#CONFIG += minimal

minimal {
	CONFIG -= use_blogs bitdht

	DEFINES += MINIMAL_LIBRS
}

profiling {
	QMAKE_CXXFLAGS -= -fomit-frame-pointer
	QMAKE_CXXFLAGS *= -pg -g -fno-omit-frame-pointer
}

release {
	# UDP and TUNNEL dont work anymore.
	#DEFINES *= PQI_DISABLE_UDP
	DEFINES *= PQI_DISABLE_TUNNEL
}



testnetwork {
	#DEFINES *= PQI_DISABLE_UDP
	DEFINES *= PQI_DISABLE_TUNNEL

	# DEFINES *= AUTHSSL_DEBUG GPG_DEBUG 
	# DEFINES *= CONN_DEBUG 
	# DEFINES *= P3DISC_DEBUG 

 	# DEFINES *= PGRP_DEBUG
 	# DEFINES *= PERSON_DEBUG
	# DEFINES *= DEBUG_PQISSL

	#DEFINES *= DEBUG_UDP_SORTER DEBUG_UDP_LAYER EXTADDRSEARCH_DEBUG

        QMAKE_CXXFLAGS -= -fomit-frame-pointer
        QMAKE_CXXFLAGS -= -O2 
        QMAKE_CXXFLAGS *= -g -fno-omit-frame-pointer
}


#CONFIG += debug
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

bitdht {

HEADERS +=	dht/p3bitdht.h 
SOURCES +=	dht/p3bitdht.cc 


HEADERS +=	tcponudp/udppeer.h \
		tcponudp/bio_tou.h \
		tcponudp/tcppacket.h \
		tcponudp/tcpstream.h \
		tcponudp/tou.h \

SOURCES +=	tcponudp/udppeer.cc \
		tcponudp/tcppacket.cc \
		tcponudp/tcpstream.cc \
		tcponudp/tou.cc \
		tcponudp/bss_tou.c \

# These two aren't actually used (and don't compile) .... 
# but could be useful later
#
#		tcponudp/udpstunner.h \
#		tcponudp/udpstunner.cc \
#


        BITDHT_DIR = ../../libbitdht/src
	INCLUDEPATH += . $${BITDHT_DIR}
	# The next line if for compliance with debian packages. Keep it!
	INCLUDEPATH += ../libbitdht
	DEFINES *= RS_USE_BITDHT
}


test_bitdht {
	# DISABLE TCP CONNECTIONS...
	DEFINES *= P3CONNMGR_NO_TCP_CONNECTIONS 

	# NO AUTO CONNECTIONS??? FOR TESTING DHT STATUS.
	DEFINES *= P3CONNMGR_NO_AUTO_CONNECTION 

	# ENABLED UDP NOW.
}





use_blogs {

	HEADERS +=	services/p3blogs.h
	SOURCES +=	services/p3blogs.cc 

	DEFINES *= RS_USE_BLOGS
}

PUBLIC_HEADERS =	retroshare/rsblogs.h \
					retroshare/rschannels.h \
					retroshare/rsdisc.h \
					retroshare/rsdistrib.h \
					retroshare/rsexpr.h \
					retroshare/rsfiles.h \
					retroshare/rsforums.h \
					retroshare/rsiface.h \
					retroshare/rsinit.h \
					retroshare/rsmsgs.h \
					retroshare/rsnotify.h \
					retroshare/rspeers.h \
					retroshare/rsrank.h \
					retroshare/rsstatus.h \
					retroshare/rsturtle.h \
					retroshare/rstypes.h

HEADERS += $$PUBLIC_HEADERS

DEFINES *= UBUNTU
INCLUDEPATH += /usr/include/glib-2.0/ /usr/lib/glib-2.0/include
LIBS *= -lgnome-keyring

# public headers to be...
HEADERS +=		retroshare/rsgame.h \
					retroshare/rsphoto.h

################################# Linux ##########################################
linux-* {
	isEmpty(PREFIX)  { PREFIX = /usr }
	isEmpty(INC_DIR) { INC_DIR = $${PREFIX}/include/retroshare/ }
	isEmpty(LIB_DIR) { LIB_DIR = $${PREFIX}/lib/ }

	DESTDIR = lib
	QMAKE_CXXFLAGS *= -Wall -D_FILE_OFFSET_BITS=64
	QMAKE_CC = g++

	SSL_DIR = /usr/include/openssl
	UPNP_DIR = /usr/include/upnp
	INCLUDEPATH += . $${SSL_DIR} $${UPNP_DIR}

	#gpg files
	system(which gpg-error-config >/dev/null 2>&1) {
		INCLUDEPATH += $$system(gpg-error-config --cflags | sed -e "s/-I//g")
	} else {
		message(Could not find gpg-error-config on your system, assuming gpg-error.h is in /usr/include)
	}
	system(which gpgme-config >/dev/null 2>&1) {
		INCLUDEPATH += $$system(gpgme-config --cflags | sed -e "s/-I//g")
	} else {
		message(Could not find gpgme-config on your system, assuming gpgme.h is in /usr/include)
	}

	#libupnp implementation files
	HEADERS += upnp/UPnPBase.h
	SOURCES += upnp/UPnPBase.cpp

	# where to put the shared library itself
	target.path = $$LIB_DIR
	INSTALLS *= target

	# where to put the library's interface
	include_rsiface.path = $${INC_DIR}
	include_rsiface.files = $$PUBLIC_HEADERS
	INSTALLS += include_rsiface

	#CONFIG += version_detail_bash_script
}

linux-g++ {
	OBJECTS_DIR = temp/linux-g++/obj
}

linux-g++-64 {
	OBJECTS_DIR = temp/linux-g++-64/obj
}

version_detail_bash_script {
    QMAKE_EXTRA_TARGETS += write_version_detail
    PRE_TARGETDEPS = write_version_detail
    write_version_detail.commands = ./version_detail.sh
}

#################### Cross compilation for windows under Linux ####################

win32-x-g++ {	
	OBJECTS_DIR = temp/win32xgcc/obj
	DESTDIR = lib.win32xgcc
	DEFINES *= WINDOWS_SYS WIN32 WIN_CROSS_UBUNTU
	QMAKE_CXXFLAGS *= -Wmissing-include-dirs
	QMAKE_CC = i586-mingw32msvc-g++
	QMAKE_LIB = i586-mingw32msvc-ar
	QMAKE_AR = i586-mingw32msvc-ar
	DEFINES *= STATICLIB WIN32

        #miniupnp implementation files
        HEADERS += upnp/upnputil.h
        SOURCES += upnp/upnputil.c

        SSL_DIR=../../../../openssl
	UPNPC_DIR = ../../../../miniupnpc-1.3
	GPG_ERROR_DIR = ../../../../libgpg-error-1.7
	GPGME_DIR  = ../../../../gpgme-1.1.8

	INCLUDEPATH *= /usr/i586-mingw32msvc/include ${HOME}/.wine/drive_c/pthreads/include/
}
################################# Windows ##########################################


win32 {
                QMAKE_CC = g++
                OBJECTS_DIR = temp/obj
                MOC_DIR = temp/moc
                DEFINES *= WINDOWS_SYS WIN32 STATICLIB MINGW
                DEFINES *= MINIUPNPC_VERSION=13
                DESTDIR = lib

                DEFINES -= DEBUG_PQISSL
                DEFINES += USE_CMD_ARGS

                #miniupnp implementation files
                HEADERS += upnp/upnputil.h
                SOURCES += upnp/upnputil.c


                UPNPC_DIR = ../../../../miniupnpc-1.3
                GPG_ERROR_DIR = ../../../../libgpg-error-1.7
                GPGME_DIR  = ../../../../gpgme-1.1.8

                PTHREADS_DIR = ../../../../pthreads-w32-2-8-0-release
                ZLIB_DIR = ../../../../zlib-1.2.3
                SSL_DIR = ../../../../OpenSSL


                INCLUDEPATH += . $${SSL_DIR}/include $${UPNPC_DIR} $${PTHREADS_DIR} $${ZLIB_DIR} $${GPGME_DIR}/src $${GPG_ERROR_DIR}/src
}


################################# MacOSX ##########################################

mac {
		QMAKE_CC = g++
		OBJECTS_DIR = temp/obj
		MOC_DIR = temp/moc
		#DEFINES = WINDOWS_SYS WIN32 STATICLIB MINGW
                #DEFINES *= MINIUPNPC_VERSION=13
		DESTDIR = lib

                #miniupnp implementation files
                HEADERS += upnp/upnputil.h
                SOURCES += upnp/upnputil.c

		# Beautiful Hack to fix 64bit file access.
                QMAKE_CXXFLAGS *= -Dfseeko64=fseeko -Dftello64=ftello -Dfopen64=fopen -Dvstatfs64=vstatfs

                UPNPC_DIR = ../../../miniupnpc-1.0
		GPG_ERROR_DIR = ../../../../libgpg-error-1.7
		GPGME_DIR  = ../../../../gpgme-1.1.8

		INCLUDEPATH += . $${UPNPC_DIR} 
		#INCLUDEPATH += . $${UPNPC_DIR} $${GPGME_DIR}/src $${GPG_ERROR_DIR}/src
}

################################### COMMON stuff ##################################

HEADERS +=	dbase/cachestrapper.h \
			dbase/fimonitor.h \
			dbase/findex.h \
			dbase/fistore.h

#HEADERS +=	dht/p3bitdht.h \

HEADERS +=	ft/ftchunkmap.h \
			ft/ftcontroller.h \
			ft/ftdata.h \
			ft/ftdatamultiplex.h \
			ft/ftdbase.h \
			ft/ftextralist.h \
			ft/ftfilecreator.h \
			ft/ftfileprovider.h \
			ft/ftfilesearch.h \
			ft/ftsearch.h \
			ft/ftserver.h \
			ft/fttransfermodule.h

HEADERS +=	pqi/authssl.h \
			pqi/authgpg.h \
			pqi/cleanupxpgp.h \
			pqi/p3cfgmgr.h \
			pqi/p3connmgr.h \
			pqi/p3dhtmgr.h \
			pqi/p3notify.h \
			pqi/p3upnpmgr.h \
			pqi/pqi.h \
			pqi/pqi_base.h \
			pqi/pqiarchive.h \
			pqi/pqiassist.h \
			pqi/pqibin.h \
			pqi/pqihandler.h \
			pqi/pqihash.h \
			pqi/pqiindic.h \
			pqi/pqiipset.h \
			pqi/pqilistener.h \
			pqi/pqiloopback.h \
			pqi/pqimonitor.h \
			pqi/pqinetwork.h \
			pqi/pqinotify.h \
			pqi/pqiperson.h \
			pqi/pqipersongrp.h \
			pqi/pqisecurity.h \
			pqi/pqiservice.h \
			pqi/pqissl.h \
			pqi/pqissllistener.h \
			pqi/pqisslpersongrp.h \
			pqi/pqissltunnel.h \
			pqi/pqissludp.h \
			pqi/pqistore.h \
			pqi/pqistreamer.h \
			pqi/sslfns.h

HEADERS +=	rsserver/p3discovery.h \
			rsserver/p3face.h \
			rsserver/p3msgs.h \
			rsserver/p3peers.h \
			rsserver/p3photo.h \
			rsserver/p3rank.h \
			rsserver/p3status.h

HEADERS +=	serialiser/rsbaseitems.h \
			serialiser/rsbaseserial.h \
			serialiser/rsblogitems.h \
			serialiser/rschannelitems.h \
			serialiser/rsconfigitems.h \
			serialiser/rsdiscitems.h \
			serialiser/rsdistribitems.h \
			serialiser/rsforumitems.h \
			serialiser/rsgameitems.h \
			serialiser/rsmsgitems.h \
			serialiser/rsphotoitems.h \
			serialiser/rsrankitems.h \
			serialiser/rsserial.h \
			serialiser/rsserviceids.h \
			serialiser/rsserviceitems.h \
			serialiser/rsstatusitems.h \
			serialiser/rstlvaddrs.h \
			serialiser/rstlvbase.h \
			serialiser/rstlvkeys.h \
			serialiser/rstlvkvwide.h \
			serialiser/rstlvtypes.h \
			serialiser/rstlvutil.h \
			serialiser/rstunnelitems.h

HEADERS +=	services/p3channels.h \
			services/p3chatservice.h \
			services/p3disc.h \
			services/p3distrib.h \
			services/p3forums.h \
			services/p3gamelauncher.h \
			services/p3gameservice.h \
			services/p3msgservice.h \
			services/p3photoservice.h \
			services/p3portservice.h \
			services/p3ranking.h \
			services/p3service.h \
			services/p3statusservice.h \
			services/p3tunnel.h
#	services/p3blogs.h \

HEADERS +=	turtle/p3turtle.h \
			turtle/rsturtleitem.h \
			turtle/turtletypes.h

HEADERS +=	upnp/upnphandler.h

HEADERS +=	util/folderiterator.h \
			util/rsdebug.h \
			util/rsdir.h \
			util/rsdiscspace.h \
			util/rsnet.h \
			util/extaddrfinder.h \
			util/dnsresolver.h \
			util/rsprint.h \
			util/rsthreads.h \
			util/rsversion.h \
			util/rswin.h \
			util/rsrandom.h \

SOURCES +=	dbase/cachestrapper.cc \
			dbase/fimonitor.cc \
			dbase/findex.cc \
			dbase/fistore.cc \
			dbase/rsexpr.cc


SOURCES +=	ft/ftchunkmap.cc \
			ft/ftcontroller.cc \
			ft/ftdata.cc \
			ft/ftdatamultiplex.cc \
			ft/ftdbase.cc \
			ft/ftextralist.cc \
			ft/ftfilecreator.cc \
			ft/ftfileprovider.cc \
			ft/ftfilesearch.cc \
			ft/ftserver.cc \
			ft/fttransfermodule.cc \

SOURCES +=	pqi/authgpg.cc \
			pqi/authssl.cc \
			pqi/cleanupxpgp.cc \
			pqi/p3cfgmgr.cc \
			pqi/p3connmgr.cc \
			pqi/p3dhtmgr.cc \
			pqi/p3notify.cc \
			pqi/pqiarchive.cc \
			pqi/pqibin.cc \
			pqi/pqihandler.cc \
			pqi/pqiipset.cc \
			pqi/pqiloopback.cc \
			pqi/pqimonitor.cc \
			pqi/pqinetwork.cc \
			pqi/pqiperson.cc \
			pqi/pqipersongrp.cc \
			pqi/pqisecurity.cc \
			pqi/pqiservice.cc \
			pqi/pqissl.cc \
			pqi/pqissllistener.cc \
			pqi/pqisslpersongrp.cc \
			pqi/pqissltunnel.cc \
			pqi/pqissludp.cc \
			pqi/pqistore.cc \
			pqi/pqistreamer.cc \
			pqi/sslfns.cc

SOURCES +=	rsserver/p3discovery.cc \
			rsserver/p3face-config.cc \
			rsserver/p3face-msgs.cc \
			rsserver/p3face-server.cc \
			rsserver/p3msgs.cc \
			rsserver/p3peers.cc \
			rsserver/p3photo.cc \
			rsserver/p3rank.cc \
			rsserver/p3status.cc \
			rsserver/rsiface.cc \
			rsserver/rsinit.cc \
			rsserver/rstypes.cc

SOURCES +=	serialiser/rsbaseitems.cc \
			serialiser/rsbaseserial.cc \
			serialiser/rsblogitems.cc \
			serialiser/rschannelitems.cc \
			serialiser/rsconfigitems.cc \
			serialiser/rsdiscitems.cc \
			serialiser/rsdistribitems.cc \
			serialiser/rsforumitems.cc \
			serialiser/rsgameitems.cc \
			serialiser/rsmsgitems.cc \
			serialiser/rsphotoitems.cc \
			serialiser/rsrankitems.cc \
			serialiser/rsserial.cc \
			serialiser/rsstatusitems.cc \
			serialiser/rstlvaddrs.cc \
			serialiser/rstlvbase.cc \
			serialiser/rstlvfileitem.cc \
			serialiser/rstlvimage.cc \
			serialiser/rstlvkeys.cc \
			serialiser/rstlvkvwide.cc \
			serialiser/rstlvtypes.cc \
			serialiser/rstlvutil.cc \
			serialiser/rstunnelitems.cc

SOURCES +=	services/p3channels.cc \
			services/p3chatservice.cc \
			services/p3disc.cc \
			services/p3distrib.cc \
			services/p3forums.cc \
			services/p3gamelauncher.cc \
			services/p3msgservice.cc \
			services/p3photoservice.cc \
			services/p3portservice.cc \
			services/p3ranking.cc \
			services/p3service.cc \
			services/p3statusservice.cc
# removed because getPeer() doesn t exist			services/p3tunnel.cc


SOURCES +=	turtle/p3turtle.cc \
				turtle/rsturtleitem.cc 
#				turtle/turtlerouting.cc \
#				turtle/turtlesearch.cc \
#				turtle/turtletunnels.cc

SOURCES +=	upnp/upnphandler.cc

SOURCES +=	util/folderiterator.cc \
			util/rsdebug.cc \
			util/rsdir.cc \
			util/rsdiscspace.cc \
			util/rsnet.cc \
			util/extaddrfinder.cc \
			util/dnsresolver.cc \
			util/rsprint.cc \
			util/rsthreads.cc \
			util/rsversion.cc \
			util/rswin.cc \
			util/rsrandom.cc

minimal {
	SOURCES -= rsserver/p3msgs.cc \
			rsserver/p3rank.cc \
			rsserver/p3status.cc \
			rsserver/p3photo.cc

	SOURCES -= serialiser/rsforumitems.cc \
			serialiser/rsstatusitems.cc \
			serialiser/rsrankitems.cc \
			serialiser/rschannelitems.cc \
			serialiser/rsgameitems.cc \
			serialiser/rsphotoitems.cc

	SOURCES -= services/p3forums.cc \
			services/p3msgservice.cc \
			services/p3statusservice.cc \
			services/p3ranking.cc \
			services/p3channels.cc \
			services/p3gamelauncher.cc \
			services/p3photoservice.cc
}
