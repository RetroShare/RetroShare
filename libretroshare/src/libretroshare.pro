!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

TEMPLATE = lib
CONFIG += staticlib bitdht
CONFIG += create_prl
CONFIG -= qt
TARGET = retroshare
TARGET_PRL = libretroshare
DESTDIR = lib

#CONFIG += dsdv

dsdv {
DEFINES *= SERVICES_DSDV
HEADERS += services/p3dsdv.h \
			  serialiser/rstlvdsdv.h \
			  serialiser/rsdsdvitems.h \
			  retroshare/rsdsdv.h 

SOURCES *= serialiser/rstlvdsdv.cc \
			  serialiser/rsdsdvitems.cc \
		  	  services/p3dsdv.cc 
}
bitdht {

HEADERS +=	dht/p3bitdht.h \
		dht/connectstatebox.h \
		dht/stunaddrassist.h

SOURCES +=	dht/p3bitdht.cc  \
		dht/p3bitdht_interface.cc \
		dht/p3bitdht_peers.cc \
		dht/p3bitdht_peernet.cc \
		dht/p3bitdht_relay.cc \
		dht/connectstatebox.cc

HEADERS +=	tcponudp/udppeer.h \
		tcponudp/bio_tou.h \
		tcponudp/tcppacket.h \
		tcponudp/tcpstream.h \
		tcponudp/tou.h \
		tcponudp/udpstunner.h \
		tcponudp/udprelay.h \

SOURCES +=	tcponudp/udppeer.cc \
		tcponudp/tcppacket.cc \
		tcponudp/tcpstream.cc \
		tcponudp/tou.cc \
		tcponudp/bss_tou.c \
		tcponudp/udpstunner.cc \
		tcponudp/udprelay.cc \

	DEFINES *= RS_USE_BITDHT

	BITDHT_DIR = ../../libbitdht/src
	DEPENDPATH += . $${BITDHT_DIR}
	INCLUDEPATH += . $${BITDHT_DIR}
	PRE_TARGETDEPS *= $${BITDHT_DIR}/lib/libbitdht.a
	LIBS *= $${BITDHT_DIR}/lib/libbitdht.a
}




PUBLIC_HEADERS =	retroshare/rsdisc.h \
					retroshare/rsexpr.h \
					retroshare/rsfiles.h \
					retroshare/rshistory.h \
					retroshare/rsids.h \
					retroshare/rsiface.h \
					retroshare/rsinit.h \
					retroshare/rsplugin.h \
					retroshare/rsloginhandler.h \
					retroshare/rsmsgs.h \
					retroshare/rsnotify.h \
					retroshare/rspeers.h \
					retroshare/rsrank.h \
					retroshare/rsstatus.h \
					retroshare/rsturtle.h \
					retroshare/rsbanlist.h \
					retroshare/rstypes.h \
					retroshare/rsdht.h \
					retroshare/rsrtt.h \
					retroshare/rsconfig.h \
					retroshare/rsversion.h \
					retroshare/rsservicecontrol.h \


HEADERS += plugins/pluginmanager.h \
		plugins/dlfcn_win32.h \
		serialiser/rspluginitems.h \
    util/rsinitedptr.h

HEADERS += $$PUBLIC_HEADERS


################################# Linux ##########################################
linux-* {
	CONFIG += link_pkgconfig

	contains(CONFIG, NO_SQLCIPHER) {
		DEFINES *= NO_SQLCIPHER
		PKGCONFIG *= sqlite3
	} else {
		SQLCIPHER_OK = $$system(pkg-config --exists sqlcipher && echo yes)
		isEmpty(SQLCIPHER_OK) {
			# We need a explicit path here, to force using the home version of sqlite3 that really encrypts the database.
			exists(../../../lib/sqlcipher/.libs/libsqlcipher.a) {
				LIBS += ../../../lib/sqlcipher/.libs/libsqlcipher.a
				DEPENDPATH += ../../../lib/
				INCLUDEPATH += ../../../lib/
			} else {
				error("libsqlcipher is not installed and libsqlcipher.a not found. SQLCIPHER is necessary for encrypted database, to build with unencrypted database, run: qmake CONFIG+=NO_SQLCIPHER")
			}
		} else {
			# Workaround for broken sqlcipher packages, e.g. Ubuntu 14.04
			# https://bugs.launchpad.net/ubuntu/+source/sqlcipher/+bug/1493928
			# PKGCONFIG *= sqlcipher
			LIBS *= -lsqlcipher
		}
	}

	#CONFIG += version_detail_bash_script

	# linux/bsd can use either - libupnp is more complete and packaged.
	#CONFIG += upnp_miniupnpc 
	CONFIG += upnp_libupnp

	# Check if the systems libupnp has been Debian-patched
	system(grep -E 'char[[:space:]]+PublisherUrl' /usr/include/upnp/upnp.h >/dev/null 2>&1) {
		# Normal libupnp
	} else {
		# Patched libupnp or new unreleased version
		DEFINES *= PATCHED_LIBUPNP
	}

	DEFINES *= UBUNTU
	PKGCONFIG *= gnome-keyring-1
	PKGCONFIG *= libssl libupnp
	PKGCONFIG *= libcrypto zlib
	LIBS *= -lpthread -ldl
}

unix {
	DEFINES *= PLUGIN_DIR=\"\\\"$${PLUGIN_DIR}\\\"\"
	DEFINES *= DATA_DIR=\"\\\"$${DATA_DIR}\\\"\"

	## where to put the librarys interface
	#include_rsiface.path = "$${INC_DIR}"
	#include_rsiface.files = $$PUBLIC_HEADERS
	#INSTALLS += include_rsiface

	## where to put the shared library itself
	#target.path = "$$LIB_DIR"
	#INSTALLS *= target
}

version_detail_bash_script {
    linux-* {
        QMAKE_EXTRA_TARGETS += write_version_detail
        PRE_TARGETDEPS = write_version_detail
        write_version_detail.commands = ./version_detail.sh
    }
    win32 {
        QMAKE_EXTRA_TARGETS += write_version_detail
        PRE_TARGETDEPS = write_version_detail
        write_version_detail.commands = $$PWD/version_detail.bat
    }
}

#################### Cross compilation for windows under Linux ####################

win32-x-g++ {	
	OBJECTS_DIR = temp/win32xgcc/obj
	DEFINES *= WINDOWS_SYS WIN32 WIN_CROSS_UBUNTU
	QMAKE_CXXFLAGS *= -Wmissing-include-dirs
	QMAKE_CC = i586-mingw32msvc-g++
	QMAKE_LIB = i586-mingw32msvc-ar
	QMAKE_AR = i586-mingw32msvc-ar
	DEFINES *= STATICLIB WIN32

	CONFIG += upnp_miniupnpc

        SSL_DIR=../../../../openssl
	UPNPC_DIR = ../../../../miniupnpc-1.3
	GPG_ERROR_DIR = ../../../../libgpg-error-1.7
	GPGME_DIR  = ../../../../gpgme-1.1.8

	INCLUDEPATH *= /usr/i586-mingw32msvc/include ${HOME}/.wine/drive_c/pthreads/include/
}
################################# Windows ##########################################

win32 {
	OBJECTS_DIR = temp/obj
	MOC_DIR = temp/moc
	DEFINES *= WINDOWS_SYS WIN32 STATICLIB MINGW WIN32_LEAN_AND_MEAN _USE_32BIT_TIME_T
	# This defines the platform to be WinXP or later and is needed for getaddrinfo (_WIN32_WINNT_WINXP)
	DEFINES *= WINVER=0x0501

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

	DEFINES += USE_CMD_ARGS

	CONFIG += upnp_miniupnpc

	LIBS += -lsqlcipher

	DEPENDPATH += . $$INC_DIR
	INCLUDEPATH += . $$INC_DIR
}

################################# MacOSX ##########################################

mac {
		OBJECTS_DIR = temp/obj
		MOC_DIR = temp/moc
		#DEFINES = WINDOWS_SYS WIN32 STATICLIB MINGW
                DEFINES *= MINIUPNPC_VERSION=13

		CONFIG += upnp_miniupnpc
		CONFIG += c+11

		# zeroconf disabled at the end of libretroshare.pro (but need the code)
		#CONFIG += zeroconf
		#CONFIG += zcnatassist

		# Beautiful Hack to fix 64bit file access.
                QMAKE_CXXFLAGS *= -Dfseeko64=fseeko -Dftello64=ftello -Dfopen64=fopen -Dvstatfs64=vstatfs

		#GPG_ERROR_DIR = ../../../../libgpg-error-1.7
		#GPGME_DIR  = ../../../../gpgme-1.1.8

		for(lib, LIB_DIR):LIBS += -L"$$lib"
		for(bin, BIN_DIR):LIBS += -L"$$bin"

		DEPENDPATH += . $$INC_DIR
		INCLUDEPATH += . $$INC_DIR

		# We need a explicit path here, to force using the home version of sqlite3 that really encrypts the database.
		LIBS += /usr/local/lib/libsqlcipher.a
		#LIBS += -lsqlite3
}

################################# FreeBSD ##########################################

freebsd-* {
	INCLUDEPATH *= /usr/local/include/gpgme
	INCLUDEPATH *= /usr/local/include/glib-2.0

	QMAKE_CXXFLAGS *= -Dfseeko64=fseeko -Dftello64=ftello -Dstat64=stat -Dstatvfs64=statvfs -Dfopen64=fopen

	# linux/bsd can use either - libupnp is more complete and packaged.
	#CONFIG += upnp_miniupnpc 
	CONFIG += upnp_libupnp
}

################################# OpenBSD ##########################################

openbsd-* {
	INCLUDEPATH *= /usr/local/include
	INCLUDEPATH += $$system(pkg-config --cflags glib-2.0 | sed -e "s/-I//g")

	QMAKE_CXXFLAGS *= -Dfseeko64=fseeko -Dftello64=ftello -Dstat64=stat -Dstatvfs64=statvfs -Dfopen64=fopen

	CONFIG += upnp_libupnp
}

################################# Haiku ##########################################

haiku-* {

	QMAKE_CXXFLAGS *= -Dfseeko64=fseeko -Dftello64=ftello -Dstat64=stat -Dstatvfs64=statvfs -Dfopen64=fopen
	OPENPGPSDK_DIR = ../../openpgpsdk/src
	INCLUDEPATH *= $${OPENPGPSDK_DIR} ../openpgpsdk
	DEFINES *= NO_SQLCIPHER
	CONFIG += release
	CONFIG += upnp_libupnp
	DESTDIR = lib
}

################################### COMMON stuff ##################################

# openpgpsdk
OPENPGPSDK_DIR = ../../openpgpsdk/src
DEPENDPATH *= $${OPENPGPSDK_DIR}
INCLUDEPATH *= $${OPENPGPSDK_DIR}
PRE_TARGETDEPS *= $${OPENPGPSDK_DIR}/lib/libops.a
LIBS *= $${OPENPGPSDK_DIR}/lib/libops.a -lbz2

HEADERS +=	dbase/cachestrapper.h \
			dbase/fimonitor.h \
			dbase/findex.h \
			dbase/fistore.h

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
			ft/fttransfermodule.h \
			ft/ftturtlefiletransferitem.h 

HEADERS += chat/distantchat.h \
			  chat/p3chatservice.h \
			  chat/distributedchat.h \
			  chat/rschatitems.h

HEADERS +=	pqi/authssl.h \
			pqi/authgpg.h \
			pgp/pgphandler.h \
			pgp/pgpkeyutil.h \
			pgp/rsaes.h \
			pgp/rscertificate.h \
			pgp/pgpauxutils.h \
			pqi/p3cfgmgr.h \
			pqi/p3peermgr.h \
			pqi/p3linkmgr.h \
			pqi/p3netmgr.h \
			pqi/p3notify.h \
			pqi/p3upnpmgr.h \
			pqi/pqiqos.h \
			pqi/pqi.h \
			pqi/pqi_base.h \
			pqi/pqiarchive.h \
			pqi/pqiassist.h \
			pqi/pqibin.h \
			pqi/pqihandler.h \
			pqi/pqihash.h \
			pqi/p3historymgr.h \
			pqi/pqiindic.h \
			pqi/pqiipset.h \
			pqi/pqilistener.h \
			pqi/pqiloopback.h \
			pqi/pqimonitor.h \
			pqi/pqinetwork.h \
			pqi/pqiperson.h \
			pqi/pqipersongrp.h \
			pqi/pqiservice.h \
			pqi/pqissl.h \
			pqi/pqissllistener.h \
			pqi/pqisslpersongrp.h \
			pqi/pqissludp.h \
			pqi/pqisslproxy.h \
			pqi/pqistore.h \
			pqi/pqistreamer.h \
			pqi/pqithreadstreamer.h \
			pqi/pqiqosstreamer.h \
			pqi/sslfns.h \
			pqi/pqinetstatebox.h \
			pqi/p3servicecontrol.h \

#			pqi/p3dhtmgr.h \

HEADERS +=	rsserver/p3face.h \
			rsserver/p3history.h \
			rsserver/p3msgs.h \
			rsserver/p3peers.h \
			rsserver/p3status.h \
			rsserver/rsaccounts.h \
			rsserver/p3serverconfig.h

HEADERS +=  grouter/groutercache.h \
				grouter/rsgrouter.h \
				grouter/grouteritems.h \
				grouter/p3grouter.h \
				grouter/rsgroutermatrix.h \
				grouter/groutertypes.h \
				grouter/rsgrouterclient.h 

HEADERS +=	serialiser/itempriorities.h \
			serialiser/rsbaseserial.h \
			serialiser/rsfiletransferitems.h \
			serialiser/rsserviceserialiser.h \
			serialiser/rsconfigitems.h \
			serialiser/rshistoryitems.h \
			serialiser/rsmsgitems.h \
			serialiser/rsserial.h \
			serialiser/rsserviceids.h \
			serialiser/rsserviceitems.h \
			serialiser/rsstatusitems.h \
			serialiser/rstlvaddrs.h \
			serialiser/rstlvbase.h \
			serialiser/rstlvitem.h \
			serialiser/rstlvidset.h \
			serialiser/rstlvfileitem.h \
			serialiser/rstlvimage.h \
			serialiser/rstlvstring.h \
			serialiser/rstlvbinary.h \
			serialiser/rstlvkeys.h \
			serialiser/rstlvkeyvalue.h \
			serialiser/rstlvgenericparam.h \
			serialiser/rstlvgenericmap.h \
			serialiser/rstlvgenericmap.inl \
			serialiser/rstlvlist.h \
			serialiser/rstlvmaps.h \
			serialiser/rstlvbanlist.h \
			serialiser/rsbanlistitems.h \
			serialiser/rsbwctrlitems.h \
			serialiser/rsdiscovery2items.h \
			serialiser/rsheartbeatitems.h \
			serialiser/rsrttitems.h \
			serialiser/rsgxsrecognitems.h \
			serialiser/rsgxsupdateitems.h \
			serialiser/rsserviceinfoitems.h \

HEADERS +=	services/p3msgservice.h \
			services/p3service.h \
			services/p3statusservice.h \
			services/p3banlist.h \
			services/p3bwctrl.h \
			services/p3discovery2.h \
			services/p3heartbeat.h \
			services/p3rtt.h \
			services/p3serviceinfo.cc \

HEADERS +=	turtle/p3turtle.h \
			turtle/rsturtleitem.h \
			turtle/turtletypes.h \
			turtle/turtleclientservice.h

HEADERS +=	util/folderiterator.h \
			util/rsdebug.h \
			util/rsmemory.h \
			util/rscompress.h \
			util/smallobject.h \
			util/rsdir.h \
			util/rsdiscspace.h \
			util/rsnet.h \
			util/extaddrfinder.h \
			util/dnsresolver.h \
			util/rsprint.h \
			util/rsstring.h \
			util/rsstd.h \
			util/rsthreads.h \
			util/rsversioninfo.h \
			util/rswin.h \
			util/rsrandom.h \
			util/radix64.h \
			util/pugiconfig.h \  
			util/rsmemcache.h \
			util/rstickevent.h \
			util/rsrecogn.h \
			util/rsscopetimer.h \
			util/stacktrace.h

SOURCES +=	dbase/cachestrapper.cc \
			dbase/fimonitor.cc \
			dbase/findex.cc \
			dbase/fistore.cc \
			dbase/rsexpr.cc


SOURCES +=	ft/ftchunkmap.cc \
			ft/ftcontroller.cc \
			ft/ftdatamultiplex.cc \
			ft/ftdbase.cc \
			ft/ftextralist.cc \
			ft/ftfilecreator.cc \
			ft/ftfileprovider.cc \
			ft/ftfilesearch.cc \
			ft/ftserver.cc \
			ft/fttransfermodule.cc \
			ft/ftturtlefiletransferitem.cc 

SOURCES += chat/distantchat.cc \
			  chat/p3chatservice.cc \
			  chat/distributedchat.cc \
			  chat/rschatitems.cc

SOURCES +=	pqi/authgpg.cc \
			pqi/authssl.cc \
			pgp/pgphandler.cc \
			pgp/pgpkeyutil.cc \
			pgp/rscertificate.cc \
			pgp/pgpauxutils.cc \
			pqi/p3cfgmgr.cc \
			pqi/p3peermgr.cc \
			pqi/p3linkmgr.cc \
			pqi/p3netmgr.cc \
			pqi/p3notify.cc \
			pqi/pqiqos.cc \
			pqi/pqiarchive.cc \
			pqi/pqibin.cc \
			pqi/pqihandler.cc \
			pqi/p3historymgr.cc \
			pqi/pqiipset.cc \
			pqi/pqiloopback.cc \
			pqi/pqimonitor.cc \
			pqi/pqinetwork.cc \
			pqi/pqiperson.cc \
			pqi/pqipersongrp.cc \
			pqi/pqiservice.cc \
			pqi/pqissl.cc \
			pqi/pqissllistener.cc \
			pqi/pqisslpersongrp.cc \
			pqi/pqissludp.cc \
			pqi/pqisslproxy.cc \
			pqi/pqistore.cc \
			pqi/pqistreamer.cc \
			pqi/pqithreadstreamer.cc \
			pqi/pqiqosstreamer.cc \
			pqi/sslfns.cc \
			pqi/pqinetstatebox.cc \
			pqi/p3servicecontrol.cc \

#			pqi/p3dhtmgr.cc \

SOURCES += 		rsserver/p3face-config.cc \
			rsserver/p3face-server.cc \
			rsserver/p3face-info.cc \
			rsserver/p3history.cc \
			rsserver/p3msgs.cc \
			rsserver/p3peers.cc \
			rsserver/p3status.cc \
			rsserver/rsinit.cc \
			rsserver/rsaccounts.cc \
			rsserver/rsloginhandler.cc \
			rsserver/rstypes.cc \
			rsserver/p3serverconfig.cc

SOURCES +=  grouter/p3grouter.cc \
				grouter/grouteritems.cc \ 
				grouter/groutermatrix.cc 

SOURCES += plugins/pluginmanager.cc \
				plugins/dlfcn_win32.cc \
				serialiser/rspluginitems.cc

SOURCES +=	serialiser/rsbaseserial.cc \
			serialiser/rsfiletransferitems.cc \
			serialiser/rsserviceserialiser.cc \
			serialiser/rsconfigitems.cc \
			serialiser/rshistoryitems.cc \
			serialiser/rsmsgitems.cc \
			serialiser/rsserial.cc \
			serialiser/rsstatusitems.cc \
			serialiser/rstlvaddrs.cc \
			serialiser/rstlvbase.cc \
			serialiser/rstlvitem.cc \
			serialiser/rstlvidset.cc \
			serialiser/rstlvfileitem.cc \
			serialiser/rstlvimage.cc \
			serialiser/rstlvstring.cc \
			serialiser/rstlvbinary.cc \
			serialiser/rstlvkeys.cc \
			serialiser/rstlvkeyvalue.cc \
			serialiser/rstlvgenericparam.cc \
			serialiser/rstlvbanlist.cc \
			serialiser/rsbanlistitems.cc \
			serialiser/rsbwctrlitems.cc \
			serialiser/rsdiscovery2items.cc \
			serialiser/rsheartbeatitems.cc \
			serialiser/rsrttitems.cc \
			serialiser/rsgxsrecognitems.cc \
			serialiser/rsgxsupdateitems.cc \
			serialiser/rsserviceinfoitems.cc \

SOURCES +=	services/p3msgservice.cc \
			services/p3service.cc \
			services/p3statusservice.cc \
			services/p3banlist.cc \
			services/p3bwctrl.cc \
			services/p3discovery2.cc \
			services/p3heartbeat.cc \
			services/p3rtt.cc \
			services/p3serviceinfo.cc \

SOURCES +=	turtle/p3turtle.cc \
				turtle/rsturtleitem.cc 
#				turtle/turtlerouting.cc \
#				turtle/turtlesearch.cc \
#				turtle/turtletunnels.cc


SOURCES +=	util/folderiterator.cc \
			util/rsdebug.cc \
			util/rscompress.cc \
			util/smallobject.cc \
			util/rsdir.cc \
			util/rsmemory.cc \
			util/rsdiscspace.cc \
			util/rsnet.cc \
			util/rsnet_ss.cc \
			util/extaddrfinder.cc \
			util/dnsresolver.cc \
			util/rsprint.cc \
			util/rsstring.cc \
			util/rsthreads.cc \
			util/rsversioninfo.cc \
			util/rswin.cc \
			util/rsaes.cc \
			util/rsrandom.cc \
			util/rstickevent.cc \
			util/rsrecogn.cc \
			util/rsscopetimer.cc


upnp_miniupnpc {
	HEADERS += upnp/upnputil.h upnp/upnphandler_miniupnp.h
	SOURCES += upnp/upnputil.c upnp/upnphandler_miniupnp.cc
}

upnp_libupnp {
	HEADERS += upnp/UPnPBase.h  upnp/upnphandler_linux.h
	SOURCES += upnp/UPnPBase.cpp upnp/upnphandler_linux.cc
	DEFINES *= RS_USE_LIBUPNP
}



zeroconf {

HEADERS +=	zeroconf/p3zeroconf.h \

SOURCES +=	zeroconf/p3zeroconf.cc  \

# Disable Zeroconf (we still need the code for zcnatassist
#	DEFINES *= RS_ENABLE_ZEROCONF

}

# This is seperated from the above for windows/linux platforms.
# It is acceptable to build in zeroconf and have it not work, 
# but unacceptable to rely on Apple's libraries for Upnp when we have alternatives. '

zcnatassist {

HEADERS +=	zeroconf/p3zcnatassist.h \

SOURCES +=	zeroconf/p3zcnatassist.cc \

	DEFINES *= RS_ENABLE_ZCNATASSIST

}

# new gxs cache system
# this should be disabled for releases until further notice.

DEFINES *= SQLITE_HAS_CODEC
DEFINES *= GXS_ENABLE_SYNC_MSGS

HEADERS += serialiser/rsnxsitems.h \
	gxs/rsgds.h \
	gxs/rsgxs.h \
	gxs/rsdataservice.h \
	gxs/rsgxsnetservice.h \
	retroshare/rsgxsflags.h \
	retroshare/rsgxsifacetypes.h \
	gxs/rsgenexchange.h \
	gxs/rsnxsobserver.h \
	gxs/rsgxsdata.h \
	retroshare/rstokenservice.h \
	gxs/rsgxsdataaccess.h \
	retroshare/rsgxsservice.h \
	serialiser/rsgxsitems.h \
	util/retrodb.h \
	util/rsdbbind.h \
	gxs/rsgxsutil.h \
	util/contentvalue.h \
	gxs/gxssecurity.h \
	gxs/rsgxsifacehelper.h \
	gxs/gxstokenqueue.h \
	gxs/rsgxsnetutils.h \
	gxs/rsgxsiface.h \
	gxs/rsgxsrequesttypes.h


SOURCES += serialiser/rsnxsitems.cc \
	gxs/rsdataservice.cc \
	gxs/rsgenexchange.cc \
	gxs/rsgxsnetservice.cc \
	gxs/rsgxsdata.cc \
	serialiser/rsgxsitems.cc \
	gxs/rsgxsdataaccess.cc \
	util/retrodb.cc \
	util/contentvalue.cc \
	util/rsdbbind.cc \
	gxs/gxssecurity.cc \
	gxs/gxstokenqueue.cc \
	gxs/rsgxsnetutils.cc \
	gxs/rsgxsutil.cc \
	gxs/rsgxsrequesttypes.cc

# gxs tunnels
HEADERS += gxstunnel/p3gxstunnel.h \
			  gxstunnel/rsgxstunnelitems.h \
			  retroshare/rsgxstunnel.h

SOURCES += gxstunnel/p3gxstunnel.cc \
				gxstunnel/rsgxstunnelitems.cc 

# Identity Service
HEADERS += retroshare/rsidentity.h \
	gxs/rsgixs.h \
	services/p3idservice.h \
	serialiser/rsgxsiditems.h \
	services/p3gxsreputation.h \
	serialiser/rsgxsreputationitems.h \

SOURCES += services/p3idservice.cc \
	serialiser/rsgxsiditems.cc \
	services/p3gxsreputation.cc \
	serialiser/rsgxsreputationitems.cc \

# GxsCircles Service
HEADERS += services/p3gxscircles.h \
	serialiser/rsgxscircleitems.h \
	retroshare/rsgxscircles.h \

SOURCES += services/p3gxscircles.cc \
	serialiser/rsgxscircleitems.cc \

# GxsForums Service
HEADERS += retroshare/rsgxsforums.h \
	services/p3gxsforums.h \
	serialiser/rsgxsforumitems.h

SOURCES += services/p3gxsforums.cc \
	serialiser/rsgxsforumitems.cc \

# GxsChannels Service
HEADERS += retroshare/rsgxschannels.h \
	services/p3gxschannels.h \
	services/p3gxscommon.h \
	serialiser/rsgxscommentitems.h \
	serialiser/rsgxschannelitems.h \

SOURCES += services/p3gxschannels.cc \
	services/p3gxscommon.cc \
	serialiser/rsgxscommentitems.cc \
	serialiser/rsgxschannelitems.cc \

wikipoos {
	# Wiki Service
	HEADERS += retroshare/rswiki.h \
		services/p3wiki.h \
		serialiser/rswikiitems.h

	SOURCES += services/p3wiki.cc \
		serialiser/rswikiitems.cc \
}

gxsthewire {
	# Wire Service
	HEADERS += retroshare/rswire.h \
		services/p3wire.h \
		serialiser/rswireitems.h

	SOURCES += services/p3wire.cc \
		serialiser/rswireitems.cc \
}

# Posted Service
HEADERS += services/p3postbase.h \
	services/p3posted.h \
	retroshare/rsposted.h \
	serialiser/rsposteditems.h

SOURCES +=  services/p3postbase.cc \
	services/p3posted.cc \
	serialiser/rsposteditems.cc

gxsphotoshare {
	#Photo Service
	HEADERS += services/p3photoservice.h \
		retroshare/rsphoto.h \
		serialiser/rsphotoitems.h \

	SOURCES += services/p3photoservice.cc \
		serialiser/rsphotoitems.cc \
}






###########################################################################################################
# OLD CONFIG OPTIONS.
# Not used much - but might be useful one day.
#

testnetwork {
	# used in rsserver/rsinit.cc Enabled Port Restrictions, and makes Proxy Port next to Dht Port.
	DEFINES *= LOCALNET_TESTING  

	# used in tcponudp/udprelay.cc Debugging Info for Relays.
	DEFINES *= DEBUG_UDP_RELAY

	# used in tcponudp/udpstunner.[h | cc] enables local stun (careful - modifies class variables).
	DEFINES *= UDPSTUN_ALLOW_LOCALNET

	# used in pqi/p3linkmgr.cc prints out extra debug.
	DEFINES *= LINKMGR_DEBUG_LINKTYPE

	# used in dht/connectstatebox to reduce connection times and display debug.
	# DEFINES *= TESTING_PERIODS
	# DEFINES *= DEBUG_CONNECTBOX
}


test_bitdht {
	# DISABLE TCP CONNECTIONS...
	DEFINES *= P3CONNMGR_NO_TCP_CONNECTIONS 

	# NO AUTO CONNECTIONS??? FOR TESTING DHT STATUS.
	DEFINES *= P3CONNMGR_NO_AUTO_CONNECTION 

	# ENABLED UDP NOW.
}

