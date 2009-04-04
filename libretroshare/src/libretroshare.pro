TEMPLATE = lib
CONFIG *= staticlib
TARGET = retroshare

DEFINES *= OPENDHT_DEBUG DHT_DEBUG CONN_DEBUG DEBUG_UDP_SORTER P3DISC_DEBUG DEBUG_UDP_LAYER
################################# Linux ##########################################

linux-g++ {
	OBJECTS_DIR = temp/linux-g++/obj
	DESTDIR = lib.linux-g++
	QMAKE_CXXFLAGS *= -fomit-frame-pointer -Wall 
	QMAKE_CC = g++
}
linux-g++-64 {
	OBJECTS_DIR = temp/linux-g++-64/obj
	DESTDIR = lib.linux-g++-64
	QMAKE_CXXFLAGS *= -fomit-frame-pointer -Wall 
	QMAKE_CC = g++
}
#################### Cross compilation for windows under Linux ####################

win32-x-g++ {	
	OBJECTS_DIR = temp/win32xgcc/obj
	DESTDIR = lib.win32xgcc
	DEFINES *= WINDOWS_SYS WIN32
	QMAKE_CXXFLAGS *= -Wmissing-include-dirs
	QMAKE_CC = i586-mingw32msvc-g++
	QMAKE_LIB = i586-mingw32msvc-ar
	DEFINES *= STATICLIB WIN32

	INCLUDEPATH *= /usr/i586-mingw32msvc/include ${HOME}/.wine/drive_c/pthreads/include/
}
################################# Windows ##########################################

win32 {
	QMAKE_CC = g++
  OBJECTS_DIR = temp/obj
	MOC_DIR = temp/moc
  DEFINES = WINDOWS_SYS WIN32 STATICLIB
	DESTDIR = lib
	  
	SSL_DIR = ../../../../openssl-0.9.7g-xpgp-0.1c/include
	UPNPC_DIR = ../../../../miniupnpc-1.0
	PTHREADS_DIR = ../../../../pthreads-w32-2-8-0-release
  ZLIB_DIR = ../../../../zlib-1.2.3
        
  INCLUDEPATH += . $${SSL_DIR} $${UPNPC_DIR} $${PTHREADS_DIR} $${ZLIB_DIR}
}
################################### COMMON stuff ##################################

DEFINES *=  PQI_USE_XPGP MINIUPNPC_VERSION=10

SSL_DIR=../../../../openssl-0.9.7g-xpgp-0.1c
UPNPC_DIR=../../../../miniupnpc-1.0

INCLUDEPATH += . $${SSL_DIR}/include $${UPNPC_DIR}

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
           pqi/authssl.h \
           pqi/authxpgp.h \
           pqi/cleanupxpgp.h \
           pqi/gpgauthmgr.h \
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
           rsserver/pqistrings.h \
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
			  services/p3turtle.h \
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
           util/rswin.h 

SOURCES = \
				dht/dht_check_peers.cc \
				dht/dht_bootstrap.cc \
				pqi/xpgp_id.cc \
				rsserver/p3face-msgs.cc \
				rsserver/rsiface.cc \
				rsserver/rstypes.cc \
				rsserver/p3face-startup.cc \
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
				services/p3turtle.cc \
				dbase/rsexpr.cc \
				dbase/cachestrapper.cc \
				dbase/fistore.cc \
				dbase/fimonitor.cc \
				dbase/findex.cc \
				pqi/p3notify.cc \
				pqi/pqipersongrp.cc \
				pqi/pqihandler.cc \
				pqi/pqiservice.cc \
				pqi/pqiperson.cc \
				pqi/pqissludp.cc \
				pqi/authxpgp.cc \
				pqi/cleanupxpgp.cc \
				pqi/pqisslpersongrp.cc \
				pqi/pqissllistener.cc \
				pqi/pqissl.cc \
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
				util/rsthreads.cc 

