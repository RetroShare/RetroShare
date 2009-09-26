TEMPLATE = lib
CONFIG += static 
TARGET = retroshare
CONFIG += release

DEFINES *= MINIUPNPC_VERSION=13
DEFINES -= PQI_USE_XPGP
DEFINES += RS_USE_PGPSSL


profiling {
	QMAKE_CXXFLAGS -= -fomit-frame-pointer
	QMAKE_CXXFLAGS *= -pg -g -fno-omit-frame-pointer
}

DEFINES -= PQI_USE_XPGP
DEFINES *= RS_USE_PGPSSL

################################# Linux ##########################################

debug {
#	DEFINES *= DEBUG
#	DEFINES *= OPENDHT_DEBUG DHT_DEBUG CONN_DEBUG DEBUG_UDP_SORTER P3DISC_DEBUG DEBUG_UDP_LAYER FT_DEBUG EXTADDRSEARCH_DEBUG
#	DEFINES *= CHAT_DEBUG CONTROL_DEBUG FT_DEBUG
	DEFINES *= P3TURTLE_DEBUG 
	QMAKE_CXXFLAGS *= -g
}

linux-g++ {
	OBJECTS_DIR = temp/linux-g++/obj
	DESTDIR = lib.linux-g++
	QMAKE_CXXFLAGS *= -Wall 
	QMAKE_CC = g++
	
	SSL_DIR = /usr/include/openssl
	UPNPC_DIR = ../../../../miniupnpc-1.3
	GPG_ERROR_DIR = ../../../../libgpg-error-1.7
	GPGME_DIR  = ../../../../gpgme-1.1.8
	
	CONFIG += version_detail_bash_script
}
linux-g++-64 {
	OBJECTS_DIR = temp/linux-g++-64/obj
	DESTDIR = lib.linux-g++-64
	QMAKE_CXXFLAGS *= -Wall 
	QMAKE_CC = g++
	SSL_DIR = /usr/include/openssl
	CONFIG += version_detail_bash_script
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
		DEFINES = WINDOWS_SYS WIN32 STATICLIB MINGW
		DESTDIR = lib

		UPNPC_DIR = ../../../../miniupnpc-1.3
		GPG_ERROR_DIR = ../../../../libgpg-error-1.7
		GPGME_DIR  = ../../../../gpgme-1.1.8

		PTHREADS_DIR = ../../../../pthreads-w32-2-8-0-release
		ZLIB_DIR = ../../../../zlib-1.2.3
		SSL_DIR = ../../../../OpenSSL

		INCLUDEPATH += . $${SSL_DIR}/include $${UPNPC_DIR} $${PTHREADS_DIR} $${ZLIB_DIR}
}
################################### COMMON stuff ##################################

INCLUDEPATH += . $${SSL_DIR} $${UPNPC_DIR} $${GPGME_DIR}/src $${GPG_ERROR_DIR}/src

#DEPENDPATH += . \
#              util \
#              tcponudp \
#              serialiser \
#              pqi \
#              dbase \
#              services \
#              dht \
#              upnp \
#              ft \
#              rsserver 

#INCLUDEPATH += . \
#               util \
#               tcponudp \
#               serialiser \
#               pqi \
#               dbase \
#               services \
#               dht \
#               upnp \
#               ft \
#               rsserver 

# Input
HEADERS += dbase/cachestrapper.h \
           dbase/fimonitor.h \
           dbase/findex.h \
           dbase/fistore.h \
           dht/b64.h \
           dht/dhtclient.h \
           dht/opendht.h \
           dht/opendhtmgr.h \
           dht/opendhtstr.h \
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
           ft/fttransfermodule.h \
	         ft/ftdwlqueue.h \
           pqi/authssl.h \
           pqi/authgpg.h \
           pqi/cleanupxpgp.h \
           pqi/p3authmgr.h \
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
           pqi/pqilistener.h \
           pqi/pqiloopback.h \
           pqi/pqimonitor.h \
           pqi/pqinetwork.h \
           pqi/pqinotify.h \
           pqi/pqiperson.h \
           pqi/pqipersongrp.h \
           pqi/pqisecurity.h \
           pqi/pqiservice.h \
           pqi/pqistore.h \
           pqi/pqissl.h \
           pqi/pqissllistener.h \
           pqi/pqisslpersongrp.h \
           pqi/pqissludp.h \
           pqi/pqistreamer.h \
           pqi/sslcert.h \
           pqi/xpgpcert.h \
           rsiface/rschannels.h \
           rsiface/rsdisc.h \
           rsiface/rsdistrib.h \
           rsiface/rsexpr.h \
           rsiface/rsfiles.h \
           rsiface/rsforums.h \
           rsiface/rsgame.h \
           rsiface/rsiface.h \
           rsiface/rsmsgs.h \
           rsiface/rsnotify.h \
           rsiface/rspeers.h \
           rsiface/rsphoto.h \
           rsiface/rsQblog.h \
           rsiface/rsrank.h \
           rsiface/rsstatus.h \
           rsiface/rstypes.h \
           rsserver/p3Blog.h \
           rsserver/p3discovery.h \
           rsserver/p3face.h \
           rsserver/p3files.h \
           rsserver/p3msgs.h \
           rsserver/p3peers.h \
           rsserver/p3photo.h \
           rsserver/p3rank.h \
           serialiser/rsbaseitems.h \
           serialiser/rsbaseserial.h \
           serialiser/rschannelitems.h \
           serialiser/rsconfigitems.h \
           serialiser/rsdiscitems.h \
           serialiser/rsdistribitems.h \
           serialiser/rsforumitems.h \
           serialiser/rsgameitems.h \
           serialiser/rsmsgitems.h \
           serialiser/rsphotoitems.h \
           serialiser/rsqblogitems.h \
           serialiser/rsrankitems.h \
           serialiser/rsserial.h \
           serialiser/rsserviceids.h \
           serialiser/rsserviceitems.h \
           serialiser/rsstatusitems.h \
           serialiser/rstlvbase.h \
           serialiser/rstlvkeys.h \
           serialiser/rstlvkvwide.h \
           serialiser/rstlvtypes.h \
           serialiser/rstlvutil.h \
           services/p3channels.h \
           services/p3chatservice.h \
           services/p3disc.h \
           services/p3distrib.h \
           services/p3forums.h \
           services/p3gamelauncher.h \
           services/p3gameservice.h \
           services/p3msgservice.h \
           services/p3photoservice.h \
           services/p3portservice.h \
           services/p3Qblog.h \
           services/p3ranking.h \
           services/p3service.h \
           services/p3status.h \
			  turtle/p3turtle.h \
			  turtle/turtletypes.h \
			  turtle/rsturtleitem.h \
			  tcponudp/extaddrfinder.h \
           tcponudp/bio_tou.h \
           tcponudp/tcppacket.h \
           tcponudp/tcpstream.h \
           tcponudp/tou.h \
           tcponudp/tou_errno.h \
           tcponudp/tou_net.h \
           tcponudp/udplayer.h \
           tcponudp/udpsorter.h \
           upnp/upnphandler.h \
           upnp/upnputil.h \
           util/rsdebug.h \
           util/rsdir.h \
           util/rsnet.h \
           util/rsprint.h \
           util/rsthreads.h \
           util/rswin.h \
           util/rsversion.h 

SOURCES = \
				dht/dht_check_peers.cc \
				dht/dht_bootstrap.cc \
				rsserver/p3face-msgs.cc \
				rsserver/rsiface.cc \
				rsserver/rstypes.cc \
				rsserver/rsinit.cc \
				rsserver/p3face-config.cc \
				rsserver/p3face-server.cc \
				rsserver/p3Blog.cc \
				rsserver/p3discovery.cc \
				rsserver/p3msgs.cc \
				rsserver/p3photo.cc \
				rsserver/p3rank.cc \
				rsserver/p3peers.cc \
				ft/ftcontroller.cc \
				ft/ftserver.cc \
				ft/ftdbase.cc \
				ft/fttransfermodule.cc \
				ft/ftdatamultiplex.cc \
				ft/ftfilesearch.cc \
				ft/ftextralist.cc \
				ft/ftfilecreator.cc \
				ft/ftdata.cc \
				ft/ftfileprovider.cc \
				ft/ftdwlqueue.cc \
				upnp/upnputil.c \
				dht/opendhtmgr.cc \
				upnp/upnphandler.cc \
				dht/opendht.cc \
				dht/opendhtstr.cc \
				dht/b64.c \
				services/p3portservice.cc \
				services/p3channels.cc \
				services/p3forums.cc \
				services/p3Qblog.cc \
				services/p3status.cc \
				services/p3distrib.cc \
				services/p3photoservice.cc \
				services/p3disc.cc \
				services/p3ranking.cc \
				services/p3gamelauncher.cc \
				services/p3msgservice.cc \
				services/p3chatservice.cc \
				services/p3service.cc \
				turtle/p3turtle.cc \
				turtle/rsturtleitem.cc \
				dbase/rsexpr.cc \
				dbase/cachestrapper.cc \
				dbase/fistore.cc \
				dbase/fimonitor.cc \
				dbase/findex.cc \
				pqi/authssl.cc \
				pqi/authgpg.cc \
				pqi/cleanupxpgp.cc \
				pqi/p3notify.cc \
				pqi/pqipersongrp.cc \
				pqi/pqihandler.cc \
				pqi/pqiservice.cc \
				pqi/pqiperson.cc \
				pqi/pqissludp.cc \
				pqi/pqisslpersongrp.cc \
				pqi/pqissllistener.cc \
				pqi/pqissl.cc \
				pqi/pqistore.cc \
				pqi/p3authmgr.cc \
				pqi/p3cfgmgr.cc \
				pqi/p3connmgr.cc \
				pqi/p3dhtmgr.cc \
				pqi/pqiarchive.cc \
				pqi/pqibin.cc \
				pqi/pqimonitor.cc \
				pqi/pqistreamer.cc \
				pqi/pqiloopback.cc \
				pqi/pqinetwork.cc \
				pqi/pqisecurity.cc \
				serialiser/rsqblogitems.cc \
				serialiser/rsstatusitems.cc \
				serialiser/rschannelitems.cc \
				serialiser/rsforumitems.cc \
				serialiser/rsdistribitems.cc \
				serialiser/rsgameitems.cc \
				serialiser/rsphotoitems.cc \
				serialiser/rsrankitems.cc \
				serialiser/rsconfigitems.cc \
				serialiser/rsdiscitems.cc \
				serialiser/rsmsgitems.cc \
				serialiser/rsbaseitems.cc \
				serialiser/rstlvkvwide.cc \
				serialiser/rstlvimage.cc \
				serialiser/rstlvutil.cc \
				serialiser/rstlvfileitem.cc \
				serialiser/rstlvkeys.cc \
				serialiser/rsbaseserial.cc \
				serialiser/rstlvbase.cc \
				serialiser/rstlvtypes.cc \
				serialiser/rsserial.cc \
				tcponudp/extaddrfinder.cc \
				tcponudp/bss_tou.c \
				tcponudp/tcpstream.cc \
				tcponudp/tou.cc \
				tcponudp/tcppacket.cc \
				tcponudp/udpsorter.cc \
				tcponudp/tou_net.cc \
				tcponudp/udplayer.cc \
				util/rsdebug.cc \
				util/rsdir.cc \
				util/rsnet.cc \
				util/rsprint.cc \
				util/rsthreads.cc \
				util/rsversion.cc 


