# SPDX-FileCopyrightText: (C) 2004-2021 Retroshare Team <contact@retroshare.cc>
# SPDX-License-Identifier: CC0-1.0

!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

TEMPLATE = lib
CONFIG -= qt
libretroshare_shared {
	CONFIG += shared
} else {
	CONFIG += staticlib
}

TARGET = retroshare
TARGET_PRL = libretroshare
DESTDIR = lib

!include("use_libretroshare.pri"):error("Including")

QMAKE_CXXFLAGS += -fPIC

## Uncomment to enable Unfinished Services.
#CONFIG += wikipoos
#CONFIG += gxsthewire
#CONFIG += gxsphotoshare

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

CONFIG += file_lists

file_lists {
	HEADERS *= file_sharing/p3filelists.h \
			file_sharing/hash_cache.h \
			file_sharing/filelist_io.h \
			file_sharing/directory_storage.h \
			file_sharing/directory_updater.h \
			file_sharing/rsfilelistitems.h \
			file_sharing/dir_hierarchy.h \
			file_sharing/file_sharing_defaults.h

	SOURCES *= file_sharing/p3filelists.cc \
			file_sharing/hash_cache.cc \
			file_sharing/filelist_io.cc \
			file_sharing/directory_storage.cc \
			file_sharing/directory_updater.cc \
			file_sharing/dir_hierarchy.cc \
			file_sharing/file_tree.cc \
			file_sharing/rsfilelistitems.cc
}


dsdv {
DEFINES *= SERVICES_DSDV
HEADERS += unused/p3dsdv.h \
			  unused/rstlvdsdv.h \
			  unused/rsdsdvitems.h \
			  unused/rsdsdv.h 

SOURCES *= unused/rstlvdsdv.cc \
			  unused/rsdsdvitems.cc \
		  	  unused/p3dsdv.cc 
}
bitdht {

HEADERS +=	dht/p3bitdht.h \
		dht/connectstatebox.h

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
		tcponudp/udprelay.h \
		tcponudp/rsudpstack.h \
		pqi/pqissludp.h \

SOURCES +=	tcponudp/udppeer.cc \
		tcponudp/tcppacket.cc \
		tcponudp/tcpstream.cc \
		tcponudp/tou.cc \
                tcponudp/bss_tou.cc \
		tcponudp/udprelay.cc \
		pqi/pqissludp.cc \

	useDhtStunner {
		HEADERS +=	dht/stunaddrassist.h \
				tcponudp/udpstunner.h

		SOURCES +=	tcponudp/udpstunner.cc

		DEFINES += RS_USE_DHT_STUNNER
	}

	DEFINES *= RS_USE_BITDHT

	BITDHT_DIR = ../../libbitdht/src
	DEPENDPATH += . $${BITDHT_DIR}
	INCLUDEPATH += . $${BITDHT_DIR}
	PRE_TARGETDEPS *= $${BITDHT_DIR}/lib/libbitdht.a
	LIBS *= $${BITDHT_DIR}/lib/libbitdht.a
}




PUBLIC_HEADERS =	retroshare/rsdisc.h \
    retroshare/rsgossipdiscovery \
    retroshare/rsevents.h \
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
					retroshare/rsstatus.h \
					retroshare/rsturtle.h \
					retroshare/rsbanlist.h \
					retroshare/rstypes.h \
					retroshare/rsdht.h \
					retroshare/rsrtt.h \
					retroshare/rsconfig.h \
					retroshare/rsversion.h \
					retroshare/rsservicecontrol.h \
					retroshare/rsgxsdistsync.h 

rs_webui {
    PUBLIC_HEADERS += retroshare/rswebui.h
    SOURCES += jsonapi/p3webui.cc
    HEADERS += jsonapi/p3webui.h
}

HEADERS += plugins/pluginmanager.h \
		plugins/dlfcn_win32.h \
    rsitems/rspluginitems.h \
    util/i2pcommon.h \
    util/rsinitedptr.h

HEADERS += $$PUBLIC_HEADERS

################################# Linux ##########################################
linux-* {
    CONFIG += link_pkgconfig

	QMAKE_CXXFLAGS *= -D_FILE_OFFSET_BITS=64
	QMAKE_CC = $${QMAKE_CXX}

    no_sqlcipher {
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
                error("libsqlcipher is not installed and libsqlcipher.a not found. SQLCIPHER is necessary for encrypted database, to build with unencrypted database, run: qmake CONFIG+=no_sqlcipher")
			}
		} else {
			# Workaround for broken sqlcipher packages, e.g. Ubuntu 14.04
			# https://bugs.launchpad.net/ubuntu/+source/sqlcipher/+bug/1493928
			# PKGCONFIG *= sqlcipher
			LIBS *= -lsqlcipher
		}
	}

    contains(RS_UPNP_LIB, threadutil) { # ensure we don't break libpnp-1.8.x
        # Check if the systems libupnp-1.6.x has been Debian-patched
        !system(grep -E 'char[[:space:]]+PublisherUrl' /usr/include/upnp/upnp.h >/dev/null 2>&1) {
            # Patched libupnp or new unreleased version
            DEFINES *= PATCHED_LIBUPNP
        }
    }

    PKGCONFIG *= libssl
    equals(RS_UPNP_LIB, "upnp ixml threadutil"):PKGCONFIG *= libupnp
    PKGCONFIG *= libcrypto zlib
    no_sqlcipher:PKGCONFIG *= sqlite3
    LIBS *= -ldl

	DEFINES *= PLUGIN_DIR=\"\\\"$${PLUGIN_DIR}\\\"\"
        DEFINES *= RS_DATA_DIR=\"\\\"$${RS_DATA_DIR}\\\"\"
}

linux-g++ {
	OBJECTS_DIR = temp/linux-g++/obj
}

linux-g++-64 {
	OBJECTS_DIR = temp/linux-g++-64/obj
}

version_detail_bash_script {
	warning("Version detail script is deprecated.")
	warning("Remove references to version_detail_bash_script from all of your build scripts!")
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

        SSL_DIR=../../../../openssl
	UPNPC_DIR = ../../../../miniupnpc-1.3
	GPG_ERROR_DIR = ../../../../libgpg-error-1.7
	GPGME_DIR  = ../../../../gpgme-1.1.8

	INCLUDEPATH *= /usr/i586-mingw32msvc/include ${HOME}/.wine/drive_c/pthreads/include/
}
################################# Windows ##########################################

win32-g++|win32-clang-g++ {
	QMAKE_CC = $${QMAKE_CXX}
	OBJECTS_DIR = temp/obj
	MOC_DIR = temp/moc
    !libretroshare_shared:DEFINES *= STATICLIB

	# Switch off optimization for release version
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O0
	QMAKE_CFLAGS_RELEASE -= -O2
	QMAKE_CFLAGS_RELEASE += -O0

	# Switch on optimization for debug version
	#QMAKE_CXXFLAGS_DEBUG += -O2
	#QMAKE_CFLAGS_DEBUG += -O2

	DEFINES += USE_CMD_ARGS

    wLibs = ws2_32 gdi32 uuid iphlpapi crypt32 ole32 winmm
    LIBS += $$linkDynamicLibs(wLibs)
}

################################# MacOSX ##########################################

mac {
		QMAKE_CC = $${QMAKE_CXX}
		OBJECTS_DIR = temp/obj
		MOC_DIR = temp/moc

		# Beautiful Hack to fix 64bit file access.
		QMAKE_CXXFLAGS *= -Dfseeko64=fseeko -Dftello64=ftello -Dfopen64=fopen -Dvstatfs64=vstatfs

		for(lib, LIB_DIR):LIBS += -L"$$lib"
		for(bin, BIN_DIR):LIBS += -L"$$bin"

		DEPENDPATH += . $$INC_DIR
		INCLUDEPATH += . $$INC_DIR
		INCLUDEPATH += ../../../.

		# We need a explicit path here, to force using the home version of sqlite3 that really encrypts the database.
		LIBS += /usr/local/lib/libsqlcipher.a
		#LIBS += -lsqlite3

		DEFINES *= PLUGIN_DIR=\"\\\"$${PLUGIN_DIR}\\\"\"
                DEFINES *= RS_DATA_DIR=\"\\\"$${RS_DATA_DIR}\\\"\"
}

################################# FreeBSD ##########################################

freebsd-* {
	INCLUDEPATH *= /usr/local/include/gpgme
	INCLUDEPATH *= /usr/local/include/glib-2.0

	QMAKE_CXXFLAGS *= -Dfseeko64=fseeko -Dftello64=ftello -Dstat64=stat -Dstatvfs64=statvfs -Dfopen64=fopen
}

################################# OpenBSD ##########################################

openbsd-* {
	INCLUDEPATH *= /usr/local/include
	INCLUDEPATH += $$system(pkg-config --cflags glib-2.0 | sed -e "s/-I//g")

	QMAKE_CXXFLAGS *= -Dfseeko64=fseeko -Dftello64=ftello -Dstat64=stat -Dstatvfs64=statvfs -Dfopen64=fopen
}

################################# Haiku ##########################################

haiku-* {

	QMAKE_CXXFLAGS *= -Dfseeko64=fseeko -Dftello64=ftello -Dstat64=stat -Dstatvfs64=statvfs -Dfopen64=fopen
	OPENPGPSDK_DIR = ../../openpgpsdk/src
	INCLUDEPATH *= $${OPENPGPSDK_DIR} ../openpgpsdk
	DEFINES *= NO_SQLCIPHER
	CONFIG += release
	DESTDIR = lib
}

################################### COMMON stuff ##################################

# openpgpsdk
OPENPGPSDK_DIR = ../../openpgpsdk/src
DEPENDPATH *= $${OPENPGPSDK_DIR}
INCLUDEPATH *= $${OPENPGPSDK_DIR}
PRE_TARGETDEPS *= $${OPENPGPSDK_DIR}/lib/libops.a
LIBS *= $${OPENPGPSDK_DIR}/lib/libops.a -lbz2

################################### HEADERS & SOURCES #############################

HEADERS +=	ft/ftchunkmap.h \
			ft/ftcontroller.h \
			ft/ftdata.h \
			ft/ftdatamultiplex.h \
			ft/ftextralist.h \
			ft/ftfilecreator.h \
			ft/ftfileprovider.h \
			ft/ftfilesearch.h \
			ft/ftsearch.h \
			ft/ftserver.h \
			ft/fttransfermodule.h \
			ft/ftturtlefiletransferitem.h 

HEADERS += crypto/chacha20.h \
           crypto/rsaes.h \
           crypto/hashstream.h \
           crypto/rscrypto.h

HEADERS += file_sharing/directory_updater.h \
           file_sharing/directory_list.h \
           file_sharing/p3filelists.h

HEADERS += chat/distantchat.h \
			  chat/p3chatservice.h \
			  chat/distributedchat.h \
			  chat/rschatitems.h

HEADERS +=	pqi/authssl.h \
			pqi/authgpg.h \
			pgp/pgphandler.h \
			pgp/openpgpsdkhandler.h \
			pgp/pgpkeyutil.h \
			pqi/pqifdbin.h \
			pqi/rstcpsocket.h \
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
			pqi/pqiservicemonitor.h \
			pqi/pqissl.h \
			pqi/pqissllistener.h \
			pqi/pqisslpersongrp.h \
			pqi/pqisslproxy.h \
			pqi/pqistore.h \
			pqi/pqistreamer.h \
			pqi/pqithreadstreamer.h \
			pqi/pqiqosstreamer.h \
			pqi/sslfns.h \
			pqi/pqinetstatebox.h \
                        pqi/p3servicecontrol.h

HEADERS +=	rsserver/p3face.h \
			rsserver/p3history.h \
			rsserver/p3msgs.h \
			rsserver/p3peers.h \
			rsserver/p3status.h \
			rsserver/rsaccounts.h \
			rsserver/p3serverconfig.h

HEADERS +=  grouter/groutercache.h \
				retroshare/rsgrouter.h \
				grouter/grouteritems.h \
				grouter/p3grouter.h \
                                grouter/groutermatrix.h \
				grouter/groutertypes.h \
				grouter/grouterclientservice.h

HEADERS +=	rsitems/rsitem.h \
			rsitems/itempriorities.h \
			serialiser/rsbaseserial.h \
			rsitems/rsfiletransferitems.h \
			rsitems/rsconfigitems.h \
			rsitems/rshistoryitems.h \
			rsitems/rsmsgitems.h \
			serialiser/rsserial.h \
			rsitems/rsserviceids.h \
			rsitems/rsstatusitems.h \
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
			rsitems/rsbanlistitems.h \
			rsitems/rsbwctrlitems.h \
    gossipdiscovery/gossipdiscoveryitems.h \
			rsitems/rsheartbeatitems.h \
			rsitems/rsrttitems.h \
			rsitems/rsgxsrecognitems.h \
			rsitems/rsgxsupdateitems.h \
			rsitems/rsserviceinfoitems.h \

HEADERS +=  \
            services/rseventsservice.h \
            services/autoproxy/rsautoproxymonitor.h \
            services/p3msgservice.h \
			services/p3service.h \
			services/p3statusservice.h \
			services/p3banlist.h \
    services/p3bwctrl.h \
    gossipdiscovery/p3gossipdiscovery.h \
			services/p3heartbeat.h \
			services/p3rtt.h \
			services/p3serviceinfo.h  \

HEADERS +=	turtle/p3turtle.h \
			turtle/rsturtleitem.h \
			turtle/turtletypes.h \
			turtle/turtleclientservice.h

HEADERS +=	util/folderiterator.h \
    util/rsdebug.h \
    util/rsdebuglevel0.h \
    util/rsdebuglevel1.h \
    util/rsdebuglevel2.h \
    util/rsdebuglevel3.h \
    util/rsdebuglevel4.h \
			util/rskbdinput.h \
			util/rsmemory.h \
			util/smallobject.h \
			util/rsdir.h \
			util/rsfile.h \
			util/argstream.h \
			util/rsdiscspace.h \
			util/rsnet.h \
			util/extaddrfinder.h \
			util/dnsresolver.h \
                        util/radix32.h \
                        util/radix64.h \
                        util/rsbase64.h \
                        util/rsendian.h \
                        util/rsinitedptr.h \
			util/rsprint.h \
			util/rsstring.h \
			util/rsstd.h \
			util/rsthreads.h \
			util/rswin.h \
			util/rsrandom.h \
			util/rsmemcache.h \
			util/rstickevent.h \
			util/rsrecogn.h \
			util/rstime.h \
            util/stacktrace.h \
            util/rsdeprecate.h \
    util/cxx11retrocompat.h \
    util/cxx14retrocompat.h \
    util/cxx17retrocompat.h \
    util/cxx23retrocompat.h \
    util/rsurl.h

SOURCES +=	ft/ftchunkmap.cc \
			ft/ftcontroller.cc \
			ft/ftdatamultiplex.cc \
			ft/ftextralist.cc \
			ft/ftfilecreator.cc \
			ft/ftfileprovider.cc \
			ft/ftfilesearch.cc \
			ft/ftserver.cc \
			ft/fttransfermodule.cc \
            ft/ftturtlefiletransferitem.cc \
    util/i2pcommon.cpp

SOURCES += crypto/chacha20.cpp \
           crypto/hashstream.cc\
           crypto/rsaes.cc \
           crypto/rscrypto.cpp

SOURCES += chat/distantchat.cc \
			  chat/p3chatservice.cc \
			  chat/distributedchat.cc \
			  chat/rschatitems.cc

SOURCES +=	pqi/authgpg.cc \
			pqi/authssl.cc \
			pgp/pgphandler.cc \
			pgp/openpgpsdkhandler.cc \
			pgp/pgpkeyutil.cc \
			pgp/rscertificate.cc \
			pgp/pgpauxutils.cc \
			pqi/p3cfgmgr.cc \
			pqi/p3peermgr.cc \
			pqi/p3linkmgr.cc \
			pqi/pqifdbin.cc \
			pqi/rstcpsocket.cc \
			pqi/p3netmgr.cc \
			pqi/p3notify.cc \
			pqi/pqiqos.cc \
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
			pqi/pqisslproxy.cc \
			pqi/pqistore.cc \
			pqi/pqistreamer.cc \
			pqi/pqithreadstreamer.cc \
			pqi/pqiqosstreamer.cc \
			pqi/sslfns.cc \
			pqi/pqinetstatebox.cc \
                        pqi/p3servicecontrol.cc

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
			rsserver/p3serverconfig.cc

SOURCES +=  grouter/p3grouter.cc \
				grouter/grouteritems.cc \ 
				grouter/groutermatrix.cc 

SOURCES += plugins/pluginmanager.cc \
				plugins/dlfcn_win32.cc 

SOURCES +=	serialiser/rsbaseserial.cc \
			rsitems/rsfiletransferitems.cc \
			rsitems/rsconfigitems.cc \
			rsitems/rshistoryitems.cc \
			rsitems/rsmsgitems.cc \
			serialiser/rsserial.cc \
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
			rsitems/rsbanlistitems.cc \
			rsitems/rsbwctrlitems.cc \
    gossipdiscovery/gossipdiscoveryitems.cc \
			rsitems/rsrttitems.cc \
			rsitems/rsgxsrecognitems.cc \
			rsitems/rsgxsupdateitems.cc \
			rsitems/rsserviceinfoitems.cc \


SOURCES +=  services/autoproxy/rsautoproxymonitor.cc \
    services/rseventsservice.cc \
            services/p3msgservice.cc \
			services/p3service.cc \
			services/p3statusservice.cc \
			services/p3banlist.cc \
			services/p3bwctrl.cc \
    gossipdiscovery/p3gossipdiscovery.cc \
			services/p3heartbeat.cc \
			services/p3rtt.cc \
			services/p3serviceinfo.cc \

SOURCES +=	turtle/p3turtle.cc \
                                turtle/rsturtleitem.cc

SOURCES +=	util/folderiterator.cc \
			util/rsdebug.cc \
			util/rskbdinput.cc \
			util/rsexpr.cc \
			util/smallobject.cc \
			util/rsdir.cc \
			util/rsfile.cc \
			util/rsdiscspace.cc \
			util/rsnet.cc \
			util/rsnet_ss.cc \
			util/rsdnsutils.cc \
			util/extaddrfinder.cc \
			util/dnsresolver.cc \
			util/rsprint.cc \
			util/rsstring.cc \
			util/rsthreads.cc \
			util/rsrandom.cc \
			util/rstickevent.cc \
			util/rsrecogn.cc \
            util/rstime.cc \
            util/rsurl.cc \
            util/rsbase64.cc

equals(RS_UPNP_LIB, miniupnpc) {
        HEADERS += rs_upnp/upnputil.h rs_upnp/upnphandler_miniupnp.h
        SOURCES += rs_upnp/upnputil.cc rs_upnp/upnphandler_miniupnp.cc
}

contains(RS_UPNP_LIB, upnp) {
        HEADERS += rs_upnp/upnp18_retrocompat.h
        HEADERS += rs_upnp/UPnPBase.h   rs_upnp/upnphandler_libupnp.h
        SOURCES += rs_upnp/UPnPBase.cpp rs_upnp/upnphandler_libupnp.cc
}

# new gxs cache system
# this should be disabled for releases until further notice.

DEFINES *= SQLITE_HAS_CODEC
DEFINES *= GXS_ENABLE_SYNC_MSGS

HEADERS += rsitems/rsnxsitems.h \
	rsitems/rsgxsitems.h \
	retroshare/rstokenservice.h \
	retroshare/rsgxsservice.h \
	retroshare/rsgxsflags.h \
	retroshare/rsgxsifacetypes.h \
	retroshare/rsgxsiface.h \
	retroshare/rsgxscommon.h \
	retroshare/rsgxsifacehelper.h \
	util/retrodb.h \
	util/rsdbbind.h \
	util/contentvalue.h \
	gxs/rsgxsutil.h \
	gxs/rsgxsnotify.h \
	gxs/gxssecurity.h \
	gxs/rsgds.h \
	gxs/rsgxs.h \
	gxs/rsdataservice.h \
	gxs/rsgxsnetservice.h \
	gxs/rsgxsnettunnel.h \
	gxs/rsgenexchange.h \
	gxs/rsnxs.h \
	gxs/rsnxsobserver.h \
	gxs/rsgxsdata.h \
	gxs/rsgxsdataaccess.h \
	gxs/gxstokenqueue.h \
	gxs/rsgxsnetutils.h \
	gxs/rsgxsrequesttypes.h


SOURCES += rsitems/rsnxsitems.cc \
	rsitems/rsgxsitems.cc \
	util/retrodb.cc \
	util/contentvalue.cc \
	util/rsdbbind.cc \
	gxs/gxssecurity.cc \
	gxs/rsgxsdataaccess.cc \
	gxs/rsdataservice.cc \
	gxs/rsgenexchange.cc \
	gxs/rsgxsnetservice.cc \
	gxs/rsgxsnettunnel.cc \
	gxs/rsgxsdata.cc \
	gxs/gxstokenqueue.cc \
	gxs/rsgxsnetutils.cc \
	gxs/rsgxsutil.cc \
        gxs/rsgxsrequesttypes.cc \
        gxs/rsnxsobserver.cpp

# Tor
HEADERS += 	retroshare/rstor.h 

HEADERS += 	tor/AddOnionCommand.h \
           	tor/AuthenticateCommand.h \
           	tor/CryptoKey.h \
           	tor/GetConfCommand.h \
           	tor/HiddenService.h \
           	tor/PendingOperation.h  \
           	tor/ProtocolInfoCommand.h \
                tor/TorTypes.h \
                tor/SetConfCommand.h \
           	tor/StrUtil.h \
           	tor/bytearray.h \
           	tor/TorControl.h \
           	tor/TorControlCommand.h \
           	tor/TorControlSocket.h \
           	tor/TorManager.h \
                tor/TorProcess.h

SOURCES += 	tor/AddOnionCommand.cpp \
		tor/AuthenticateCommand.cpp \
		tor/GetConfCommand.cpp \
		tor/HiddenService.cpp \
		tor/ProtocolInfoCommand.cpp \
		tor/SetConfCommand.cpp \
		tor/TorControlCommand.cpp \
		tor/TorControl.cpp \
		tor/TorControlSocket.cpp \
		tor/TorManager.cpp \
		tor/TorProcess.cpp \
		tor/CryptoKey.cpp         \
		tor/PendingOperation.cpp  \
		tor/StrUtil.cpp        

# gxs tunnels
HEADERS += gxstunnel/p3gxstunnel.h \
			  gxstunnel/rsgxstunnelitems.h \
			  retroshare/rsgxstunnel.h

SOURCES += gxstunnel/p3gxstunnel.cc \
				gxstunnel/rsgxstunnelitems.cc 

# new serialization code
HEADERS += serialiser/rsserializable.h \
           serialiser/rsserializer.h \
           serialiser/rstypeserializer.h \
           util/rsjson.h

SOURCES += serialiser/rsserializable.cc \
           serialiser/rsserializer.cc \
           serialiser/rstypeserializer.cc \
           util/rsjson.cc

# Identity Service
HEADERS += retroshare/rsidentity.h \
    retroshare/rsreputations.h \
	gxs/rsgixs.h \
	services/p3idservice.h \
	rsitems/rsgxsiditems.h \
	services/p3gxsreputation.h \
	rsitems/rsgxsreputationitems.h \

SOURCES += services/p3idservice.cc \
	rsitems/rsgxsiditems.cc \
	services/p3gxsreputation.cc \
	rsitems/rsgxsreputationitems.cc \

# GxsCircles Service
HEADERS += services/p3gxscircles.h \
	rsitems/rsgxscircleitems.h \
	retroshare/rsgxscircles.h \

SOURCES += services/p3gxscircles.cc \
	rsitems/rsgxscircleitems.cc \

# GxsForums Service
HEADERS += retroshare/rsgxsforums.h \
	services/p3gxsforums.h \
	rsitems/rsgxsforumitems.h

SOURCES += services/p3gxsforums.cc \
	rsitems/rsgxsforumitems.cc \

# GxsChannels Service
HEADERS += retroshare/rsgxschannels.h \
	services/p3gxschannels.h \
	services/p3gxscommon.h \
	rsitems/rsgxscommentitems.h \
	rsitems/rsgxschannelitems.h \

SOURCES += services/p3gxschannels.cc \
	services/p3gxscommon.cc \
	rsitems/rsgxscommentitems.cc \
	rsitems/rsgxschannelitems.cc \

wikipoos {
	DEFINES *= RS_USE_WIKI

	# Wiki Service
	HEADERS += retroshare/rswiki.h \
		services/p3wiki.h \
		rsitems/rswikiitems.h

	SOURCES += services/p3wiki.cc \
		rsitems/rswikiitems.cc \
}

gxsthewire {
	DEFINES *= RS_USE_WIRE

	# Wire Service
	HEADERS += retroshare/rswire.h \
		services/p3wire.h \
		rsitems/rswireitems.h

	SOURCES += services/p3wire.cc \
		rsitems/rswireitems.cc \
}

# Posted Service
HEADERS += services/p3postbase.h \
	services/p3posted.h \
	retroshare/rsposted.h \
	rsitems/rsposteditems.h

SOURCES +=  services/p3postbase.cc \
	services/p3posted.cc \
	rsitems/rsposteditems.cc

gxsphotoshare {
	DEFINES *= RS_USE_PHOTO

	#Photo Service
	HEADERS += services/p3photoservice.h \
		retroshare/rsphoto.h \
		rsitems/rsphotoitems.h \

	SOURCES += services/p3photoservice.cc \
		rsitems/rsphotoitems.cc \
}

rs_gxs_trans {
    HEADERS += gxstrans/p3gxstransitems.h gxstrans/p3gxstrans.h
    SOURCES += gxstrans/p3gxstransitems.cc gxstrans/p3gxstrans.cc
}

rs_jsonapi {
    JSONAPI_GENERATOR_SRC=$$clean_path($${RS_SRC_PATH}/jsonapi-generator/src/)
    JSONAPI_GENERATOR_OUT=$$clean_path($${RS_BUILD_PATH}/jsonapi-generator/src/)
    isEmpty(JSONAPI_GENERATOR_EXE) {
        win32 {
            CONFIG(release, debug|release) {
                JSONAPI_GENERATOR_EXE=$$clean_path($${JSONAPI_GENERATOR_OUT}/release/jsonapi-generator.exe)
            }
        CONFIG(debug, debug|release) {
                JSONAPI_GENERATOR_EXE=$$clean_path($${JSONAPI_GENERATOR_OUT}/debug/jsonapi-generator.exe)
            }
        } else {
            JSONAPI_GENERATOR_EXE=$$clean_path($${JSONAPI_GENERATOR_OUT}/jsonapi-generator)
        }
    }

    DOXIGEN_INPUT_DIRECTORY=$$clean_path($${PWD})
    DOXIGEN_CONFIG_SRC=$$clean_path($${RS_SRC_PATH}/jsonapi-generator/src/jsonapi-generator-doxygen.conf)
    DOXIGEN_CONFIG_OUT=$$clean_path($${JSONAPI_GENERATOR_OUT}/jsonapi-generator-doxygen-final.conf)
    WRAPPERS_INCL_FILE=$$clean_path($${JSONAPI_GENERATOR_OUT}/jsonapi-includes.inl)
    WRAPPERS_REG_FILE=$$clean_path($${JSONAPI_GENERATOR_OUT}/jsonapi-wrappers.inl)

    no_rs_cross_compiling {
        DUMMYRESTBEDINPUT = FORCE
        CMAKE_GENERATOR_OVERRIDE=""
        win32-g++|win32-clang-g++ {
            isEmpty(QMAKE_SH) {
                CMAKE_GENERATOR_OVERRIDE="-G \"MinGW Makefiles\""
            } else {
                CMAKE_GENERATOR_OVERRIDE="-G \"MSYS Makefiles\""
            }
        }
        genrestbedlib.name = Generating librestbed.
        genrestbedlib.input = DUMMYRESTBEDINPUT
        genrestbedlib.output = $$clean_path($${RESTBED_BUILD_PATH}/librestbed.a)
        genrestbedlib.CONFIG += target_predeps combine
        genrestbedlib.variable_out = PRE_TARGETDEPS
        win32-g++:isEmpty(QMAKE_SH) {
            genrestbedlib.commands = \
                cd /D $$shell_path($${RS_SRC_PATH}) && git submodule update --init supportlibs/restbed || cd . $$escape_expand(\\n\\t) \
                cd /D $$shell_path($${RESTBED_SRC_PATH}) && git submodule update --init dependency/asio || cd . $$escape_expand(\\n\\t) \
                cd /D $$shell_path($${RESTBED_SRC_PATH}) && git submodule update --init dependency/catch || cd . $$escape_expand(\\n\\t )\
                cd /D $$shell_path($${RESTBED_SRC_PATH}) && git submodule update --init dependency/kashmir || cd . $$escape_expand(\\n\\t) \
                $(CHK_DIR_EXISTS) $$shell_path($$UDP_DISCOVERY_BUILD_PATH) $(MKDIR) $$shell_path($${UDP_DISCOVERY_BUILD_PATH}) $$escape_expand(\\n\\t)
        } else {
            genrestbedlib.commands = \
                cd $${RS_SRC_PATH} && ( \
                git submodule update --init supportlibs/restbed ; \
                cd $${RESTBED_SRC_PATH} ; \
                git submodule update --init dependency/asio ; \
                git submodule update --init dependency/catch ; \
                git submodule update --init dependency/kashmir ; \
                true ) && \
                mkdir -p $${RESTBED_BUILD_PATH} &&
        }
        genrestbedlib.commands += \
            cd $$shell_path($${RESTBED_BUILD_PATH}) && \
            cmake \
                -DCMAKE_CXX_COMPILER=$$QMAKE_CXX \
                \"-DCMAKE_CXX_FLAGS=$${QMAKE_CXXFLAGS}\" \
                $${CMAKE_GENERATOR_OVERRIDE} -DBUILD_SSL=OFF \
                -DCMAKE_INSTALL_PREFIX=. -B. \
                -H$$shell_path($${RESTBED_SRC_PATH}) && \
            $(MAKE)
        QMAKE_EXTRA_COMPILERS += genrestbedlib

        RESTBED_HEADER_FILE=$$clean_path($${RESTBED_BUILD_PATH}/include/restbed)
        genrestbedheader.name = Generating restbed header.
        genrestbedheader.input = genrestbedlib.output
        genrestbedheader.output = $${RESTBED_HEADER_FILE}
        genrestbedheader.CONFIG += target_predeps no_link
        genrestbedheader.variable_out = HEADERS
        genrestbedheader.commands = cd $$shell_path($${RESTBED_BUILD_PATH}) && $(MAKE) install
        QMAKE_EXTRA_COMPILERS += genrestbedheader
    }

    INCLUDEPATH *= $${JSONAPI_GENERATOR_OUT}
    DEPENDPATH *= $${JSONAPI_GENERATOR_OUT}
    APIHEADERS = $$files($${RS_SRC_PATH}/libretroshare/src/retroshare/*.h)
    #Make sure that the jsonapigenerator executable are ready
    APIHEADERS += $${JSONAPI_GENERATOR_EXE}

    genjsonapi.name = Generating jsonapi headers.
    genjsonapi.input = APIHEADERS
    genjsonapi.output = $${WRAPPERS_INCL_FILE} $${WRAPPERS_REG_FILE}
    genjsonapi.clean = $${WRAPPERS_INCL_FILE} $${WRAPPERS_REG_FILE}
    genjsonapi.CONFIG += target_predeps combine no_link
    genjsonapi.variable_out = HEADERS
    win32-g++:isEmpty(QMAKE_SH) {
        genjsonapi.commands = \
            $(CHK_DIR_EXISTS) $$shell_path($$JSONAPI_GENERATOR_OUT) $(MKDIR) $$shell_path($${JSONAPI_GENERATOR_OUT}) $$escape_expand(\\n\\t)
    } else {
        genjsonapi.commands = \
            mkdir -p $${JSONAPI_GENERATOR_OUT} && \
            cp $${DOXIGEN_CONFIG_SRC} $${DOXIGEN_CONFIG_OUT} && \
            echo OUTPUT_DIRECTORY=$${JSONAPI_GENERATOR_OUT} >> $${DOXIGEN_CONFIG_OUT} && \
            echo INPUT=$${DOXIGEN_INPUT_DIRECTORY} >> $${DOXIGEN_CONFIG_OUT} && \
            doxygen $${DOXIGEN_CONFIG_OUT} &&
    }
    genjsonapi.commands += \
        $${JSONAPI_GENERATOR_EXE} $${JSONAPI_GENERATOR_SRC} $${JSONAPI_GENERATOR_OUT}
    QMAKE_EXTRA_COMPILERS += genjsonapi

    # Force recalculation of libretroshare dependencies see https://stackoverflow.com/a/47884045
    QMAKE_EXTRA_TARGETS += libretroshare

    HEADERS += jsonapi/jsonapi.h jsonapi/jsonapiitems.h retroshare/rsjsonapi.h
    SOURCES += jsonapi/jsonapi.cpp
}

rs_deep_forums_index {
    HEADERS *= deep_search/commonutils.hpp
    SOURCES *= deep_search/commonutils.cpp

    HEADERS += deep_search/forumsindex.hpp
    SOURCES += deep_search/forumsindex.cpp
}

rs_deep_channels_index {
    HEADERS *= deep_search/commonutils.hpp
    SOURCES *= deep_search/commonutils.cpp

    HEADERS += deep_search/channelsindex.hpp
    SOURCES += deep_search/channelsindex.cpp
}

rs_deep_files_index {
    HEADERS *= deep_search/commonutils.hpp
    SOURCES *= deep_search/commonutils.cpp

    HEADERS += deep_search/filesindex.hpp
    SOURCES += deep_search/filesindex.cpp
}

rs_deep_files_index_ogg {
    HEADERS += deep_search/filesoggindexer.hpp
}

rs_deep_files_index_flac {
    HEADERS += deep_search/filesflacindexer.hpp
}

rs_deep_files_index_taglib {
    HEADERS += deep_search/filestaglibindexer.hpp
}

rs_broadcast_discovery {
    HEADERS += retroshare/rsbroadcastdiscovery.h \
        services/broadcastdiscoveryservice.h
    SOURCES += services/broadcastdiscoveryservice.cc

    no_rs_cross_compiling {
        DUMMYQMAKECOMPILERINPUT = FORCE
        CMAKE_GENERATOR_OVERRIDE=""
        win32-g++|win32-clang-g++ {
            isEmpty(QMAKE_SH) {
                CMAKE_GENERATOR_OVERRIDE="-G \"MinGW Makefiles\""
            } else {
                CMAKE_GENERATOR_OVERRIDE="-G \"MSYS Makefiles\""
            }
        }
        udpdiscoverycpplib.name = Generating libudp-discovery.a.
        udpdiscoverycpplib.input = DUMMYQMAKECOMPILERINPUT
        udpdiscoverycpplib.output = $$clean_path($${UDP_DISCOVERY_BUILD_PATH}/libudp-discovery.a)
        udpdiscoverycpplib.CONFIG += target_predeps combine
        udpdiscoverycpplib.variable_out = PRE_TARGETDEPS
        win32-g++:isEmpty(QMAKE_SH) {
            udpdiscoverycpplib.commands = \
                cd /D $$shell_path($${RS_SRC_PATH}) && git submodule update --init supportlibs/udp-discovery-cpp || cd . $$escape_expand(\\n\\t) \
                $(CHK_DIR_EXISTS) $$shell_path($$UDP_DISCOVERY_BUILD_PATH) $(MKDIR) $$shell_path($${UDP_DISCOVERY_BUILD_PATH}) $$escape_expand(\\n\\t)
        } else {
            udpdiscoverycpplib.commands = \
                cd $${RS_SRC_PATH} && ( \
                git submodule update --init supportlibs/udp-discovery-cpp || \
                true ) && \
                mkdir -p $${UDP_DISCOVERY_BUILD_PATH} &&
        }
        udpdiscoverycpplib.commands += \
            cd $$shell_path($${UDP_DISCOVERY_BUILD_PATH}) && \
            cmake -DCMAKE_C_COMPILER=$$fixQmakeCC($$QMAKE_CC) \
                -DCMAKE_CXX_COMPILER=$$QMAKE_CXX \
                \"-DCMAKE_CXX_FLAGS=$${QMAKE_CXXFLAGS}\" \
                $${CMAKE_GENERATOR_OVERRIDE} \
                -DBUILD_EXAMPLE=OFF -DBUILD_TOOL=OFF \
                -DCMAKE_INSTALL_PREFIX=. -B. \
                -H$$shell_path($${UDP_DISCOVERY_SRC_PATH}) && \
            $(MAKE)
        QMAKE_EXTRA_COMPILERS += udpdiscoverycpplib
    }
}

rs_sam3 {
    SOURCES += \
        services/autoproxy/p3i2psam3.cpp \
        pqi/pqissli2psam3.cpp \

    HEADERS += \
        services/autoproxy/p3i2psam3.h \
        pqi/pqissli2psam3.h \
}

rs_sam3_libsam3 {
    DUMMYQMAKECOMPILERINPUT = FORCE
    libsam3.name = Generating libsam3.
    libsam3.input = DUMMYQMAKECOMPILERINPUT
    libsam3.output = $$clean_path($${LIBSAM3_BUILD_PATH}/libsam3.a)
    libsam3.CONFIG += target_predeps combine
    libsam3.variable_out = PRE_TARGETDEPS
    win32-g++:isEmpty(QMAKE_SH) {
        LIBSAM3_MAKE_PARAMS = CC=gcc
        libsam3.commands = \
            cd /D $$shell_path($${RS_SRC_PATH}) && git submodule update --init supportlibs/libsam3 || cd . $$escape_expand(\\n\\t) \
            $(CHK_DIR_EXISTS) $$shell_path($$LIBSAM3_BUILD_PATH) $(MKDIR) $$shell_path($${LIBSAM3_BUILD_PATH}) $$escape_expand(\\n\\t) \
            $(COPY_DIR) $$shell_path($${LIBSAM3_SRC_PATH}) $$shell_path($${LIBSAM3_BUILD_PATH}) || cd . $$escape_expand(\\n\\t)
    } else {
        LIBSAM3_MAKE_PARAMS =
        libsam3.commands = \
            cd $${RS_SRC_PATH} && ( \
            git submodule update --init supportlibs/libsam3 || \
            true ) && \
            mkdir -p $${LIBSAM3_BUILD_PATH} && \
            (cp -r $${LIBSAM3_SRC_PATH}/* $${LIBSAM3_BUILD_PATH} || true) &&
    }
    libsam3.commands += \
        cd $$shell_path($${LIBSAM3_BUILD_PATH}) && \
        $(MAKE) build $${LIBSAM3_MAKE_PARAMS}
    QMAKE_EXTRA_COMPILERS += libsam3
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

################################# Android #####################################

android-* {
    lessThan(ANDROID_API_VERSION, 24) {

## TODO: This probably disable largefile support and maybe is not necessary with
## __ANDROID_API__ >= 24 hence should be made conditional or moved to a
## compatibility header
    DEFINES *= "fopen64=fopen"
    DEFINES *= "fseeko64=fseeko"
    DEFINES *= "ftello64=ftello"

## @See: rs_android/README-ifaddrs-android.adoc
    HEADERS += \
        rs_android/ifaddrs-android.h \
        rs_android/LocalArray.h \
        rs_android/ScopedFd.h
    }

## Static library are very susceptible to order in command line
    sLibs = bz2 $$RS_UPNP_LIB $$RS_SQL_LIB ssl crypto

    LIBS += $$linkStaticLibs(sLibs)
    PRE_TARGETDEPS += $$pretargetStaticLibs(sLibs)

    HEADERS += \
        rs_android/androidcoutcerrcatcher.hpp \
        rs_android/retroshareserviceandroid.hpp \
        rs_android/rsjni.hpp

    SOURCES += rs_android/rsjni.cpp \
        rs_android/retroshareserviceandroid.cpp \
        rs_android/errorconditionwrap.cpp
}

