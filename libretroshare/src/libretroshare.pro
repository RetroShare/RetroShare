################################################################################
# libretroshare.pro                                                            #
# Copyright (C) 2018, Retroshare team <retroshare.team@gmailcom>               #
#                                                                              #
# This program is free software: you can redistribute it and/or modify         #
# it under the terms of the GNU Lesser General Public License as               #
# published by the Free Software Foundation, either version 3 of the           #
# License, or (at your option) any later version.                              #
#                                                                              #
# This program is distributed in the hope that it will be useful,              #
# but WITHOUT ANY WARRANTY; without even the implied warranty of               #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                #
# GNU Lesser General Public License for more details.                          #
#                                                                              #
# You should have received a copy of the GNU Lesser General Public License     #
# along with this program.  If not, see <https://www.gnu.org/licenses/>.       #
################################################################################
!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt
TARGET = retroshare
TARGET_PRL = libretroshare
DESTDIR = lib

!include("use_libretroshare.pri"):error("Including")

# the dht stunner is used to obtain RS external ip addr. when it is natted
# this system is unreliable and rs supports a newer and better one (asking connected peers)
# CONFIG += useDhtStunner

# treat warnings as error for better removing
#QMAKE_CFLAGS += -Werror
#QMAKE_CXXFLAGS += -Werror

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
			file_sharing/file_tree.h \
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
		tcponudp/bss_tou.c \
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
					retroshare/rsgxsdistsync.h 

HEADERS += plugins/pluginmanager.h \
		plugins/dlfcn_win32.h \
		rsitems/rspluginitems.h \
    util/rsinitedptr.h

HEADERS += $$PUBLIC_HEADERS


################################# Linux ##########################################
linux-* {
    CONFIG += link_pkgconfig

	QMAKE_CXXFLAGS *= -Wall -D_FILE_OFFSET_BITS=64
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

	# Check if the systems libupnp has been Debian-patched
	system(grep -E 'char[[:space:]]+PublisherUrl' /usr/include/upnp/upnp.h >/dev/null 2>&1) {
		# Normal libupnp
	} else {
		# Patched libupnp or new unreleased version
		DEFINES *= PATCHED_LIBUPNP
	}

    PKGCONFIG *= libssl
    equals(RS_UPNP_LIB, "upnp ixml threadutil"):PKGCONFIG *= libupnp
    PKGCONFIG *= libcrypto zlib
    no_sqlcipher:PKGCONFIG *= sqlite3
    LIBS *= -ldl

	DEFINES *= PLUGIN_DIR=\"\\\"$${PLUGIN_DIR}\\\"\"
	DEFINES *= DATA_DIR=\"\\\"$${DATA_DIR}\\\"\"
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

win32-g++ {
	QMAKE_CC = $${QMAKE_CXX}
	OBJECTS_DIR = temp/obj
	MOC_DIR = temp/moc
    DEFINES *= STATICLIB

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
		DEFINES *= DATA_DIR=\"\\\"$${DATA_DIR}\\\"\"
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

HEADERS += directory_updater.h \
				directory_list.h \
				p3filelists.h

HEADERS += chat/distantchat.h \
			  chat/p3chatservice.h \
			  chat/distributedchat.h \
			  chat/rschatitems.h

HEADERS +=	pqi/authssl.h \
			pqi/authgpg.h \
			pgp/pgphandler.h \
			pgp/pgpkeyutil.h \
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
                        pqi/pqissli2pbob.h \
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
				retroshare/rsgrouter.h \
				grouter/grouteritems.h \
				grouter/p3grouter.h \
				grouter/rsgroutermatrix.h \
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
			serialiser/rsserviceitems.h \
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
			rsitems/rsdiscovery2items.h \
			rsitems/rsheartbeatitems.h \
			rsitems/rsrttitems.h \
			rsitems/rsgxsrecognitems.h \
			rsitems/rsgxsupdateitems.h \
			rsitems/rsserviceinfoitems.h \

HEADERS +=  services/autoproxy/p3i2pbob.h \
            services/rseventsservice.h \
            services/autoproxy/rsautoproxymonitor.h \
            services/p3msgservice.h \
			services/p3service.h \
			services/p3statusservice.h \
			services/p3banlist.h \
			services/p3bwctrl.h \
			services/p3discovery2.h \
			services/p3heartbeat.h \
			services/p3rtt.h \
			services/p3serviceinfo.h  \

HEADERS +=	turtle/p3turtle.h \
			turtle/rsturtleitem.h \
			turtle/turtletypes.h \
			turtle/turtleclientservice.h

HEADERS +=	util/folderiterator.h \
			util/rsdebug.h \
			util/rsmemory.h \
			util/smallobject.h \
			util/rsdir.h \
			util/argstream.h \
			util/rsdiscspace.h \
			util/rsnet.h \
			util/extaddrfinder.h \
			util/dnsresolver.h \
                        util/radix32.h \
                        util/radix64.h \
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
            ft/ftturtlefiletransferitem.cc

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
			pgp/pgpkeyutil.cc \
			pgp/rscertificate.cc \
			pgp/pgpauxutils.cc \
			pqi/p3cfgmgr.cc \
			pqi/p3peermgr.cc \
			pqi/p3linkmgr.cc \
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
                        pqi/pqissli2pbob.cpp \
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
			rsitems/rsdiscovery2items.cc \
			rsitems/rsrttitems.cc \
			rsitems/rsgxsrecognitems.cc \
			rsitems/rsgxsupdateitems.cc \
			rsitems/rsserviceinfoitems.cc \


SOURCES +=  services/autoproxy/rsautoproxymonitor.cc \
    services/rseventsservice.cc \
            services/autoproxy/p3i2pbob.cc \
            services/p3msgservice.cc \
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
			util/rsexpr.cc \
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
			util/rsrandom.cc \
			util/rstickevent.cc \
			util/rsrecogn.cc \
            util/rstime.cc \
            util/rsurl.cc

equals(RS_UPNP_LIB, miniupnpc) {
	HEADERS += upnp/upnputil.h upnp/upnphandler_miniupnp.h
	SOURCES += upnp/upnputil.c upnp/upnphandler_miniupnp.cc
} else {
	HEADERS += upnp/UPnPBase.h  upnp/upnphandler_linux.h
	SOURCES += upnp/UPnPBase.cpp upnp/upnphandler_linux.cc
	DEFINES *= RS_USE_LIBUPNP
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
	gxs/rsgxsrequesttypes.cc

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
	# Wiki Service
	HEADERS += retroshare/rswiki.h \
		services/p3wiki.h \
		rsitems/rswikiitems.h

	SOURCES += services/p3wiki.cc \
		rsitems/rswikiitems.cc \
}

gxsthewire {
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
        win32-g++:CMAKE_GENERATOR_OVERRIDE="-G \"MSYS Makefiles\""
        genrestbedlib.name = Generating librestbed.
        genrestbedlib.input = DUMMYRESTBEDINPUT
        genrestbedlib.output = $$clean_path($${RESTBED_BUILD_PATH}/librestbed.a)
        genrestbedlib.CONFIG += target_predeps combine
        genrestbedlib.variable_out = PRE_TARGETDEPS
        genrestbedlib.commands = \
            cd $${RS_SRC_PATH} && ( \
            git submodule update --init --recommend-shallow supportlibs/restbed ; \
            cd $${RESTBED_SRC_PATH} ; \
            git submodule update --init --recommend-shallow dependency/asio ; \
            git submodule update --init --recommend-shallow dependency/catch ; \
            git submodule update --init --recommend-shallow dependency/kashmir ; \
            true ) && \
            mkdir -p $${RESTBED_BUILD_PATH} && cd $${RESTBED_BUILD_PATH} && \
            cmake -DCMAKE_C_COMPILER=$$fixQmakeCC($$QMAKE_CC) \
                -DCMAKE_CXX_COMPILER=$$QMAKE_CXX \
                $${CMAKE_GENERATOR_OVERRIDE} -DBUILD_SSL=OFF \
                -DCMAKE_INSTALL_PREFIX=. -B. \
                -H$$shell_path($${RESTBED_SRC_PATH}) && \
            $(MAKE)
        QMAKE_EXTRA_COMPILERS += genrestbedlib

        RESTBED_HEADER_FILE=$$clean_path($${RESTBED_BUILD_PATH}/include/restbed)
        genrestbedheader.name = Generating restbed header.
        genrestbedheader.input = genrestbedlib.output
        genrestbedheader.output = $${RESTBED_HEADER_FILE}
        genrestbedheader.CONFIG += target_predeps combine no_link
        genrestbedheader.variable_out = HEADERS
        genrestbedheader.commands = cd $${RESTBED_BUILD_PATH} && $(MAKE) install
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
    genjsonapi.commands = \
        mkdir -p $${JSONAPI_GENERATOR_OUT} && \
        cp $${DOXIGEN_CONFIG_SRC} $${DOXIGEN_CONFIG_OUT} && \
        echo OUTPUT_DIRECTORY=$${JSONAPI_GENERATOR_OUT} >> $${DOXIGEN_CONFIG_OUT} && \
        echo INPUT=$${DOXIGEN_INPUT_DIRECTORY} >> $${DOXIGEN_CONFIG_OUT} && \
        doxygen $${DOXIGEN_CONFIG_OUT} && \
        $${JSONAPI_GENERATOR_EXE} $${JSONAPI_GENERATOR_SRC} $${JSONAPI_GENERATOR_OUT};
    QMAKE_EXTRA_COMPILERS += genjsonapi

    # Force recalculation of libretroshare dependencies see https://stackoverflow.com/a/47884045
    QMAKE_EXTRA_TARGETS += libretroshare

    HEADERS += jsonapi/jsonapi.h jsonapi/jsonapiitems.h
    SOURCES += jsonapi/jsonapi.cpp
}

rs_deep_search {
    HEADERS += deep_search/deep_search.h
}

rs_broadcast_discovery {
    HEADERS += retroshare/rsbroadcastdiscovery.h \
        services/broadcastdiscoveryservice.h
    SOURCES += services/broadcastdiscoveryservice.cc

    no_rs_cross_compiling {
        DUMMYQMAKECOMPILERINPUT = FORCE
        CMAKE_GENERATOR_OVERRIDE=""
        win32-g++:CMAKE_GENERATOR_OVERRIDE="-G \"MSYS Makefiles\""
        udpdiscoverycpplib.name = Generating libudp-discovery.a.
        udpdiscoverycpplib.input = DUMMYQMAKECOMPILERINPUT
        udpdiscoverycpplib.output = $$clean_path($${UDP_DISCOVERY_BUILD_PATH}/libudp-discovery.a)
        udpdiscoverycpplib.CONFIG += target_predeps combine
        udpdiscoverycpplib.variable_out = PRE_TARGETDEPS
        udpdiscoverycpplib.commands = \
            cd $${RS_SRC_PATH} && ( \
            git submodule update --init --recommend-shallow supportlibs/udp-discovery-cpp || \
            true ) && \
            mkdir -p $${UDP_DISCOVERY_BUILD_PATH} && \
            cd $${UDP_DISCOVERY_BUILD_PATH} && \
            cmake -DCMAKE_C_COMPILER=$$fixQmakeCC($$QMAKE_CC) \
                -DCMAKE_CXX_COMPILER=$$QMAKE_CXX \
                $${CMAKE_GENERATOR_OVERRIDE} \
                -DBUILD_EXAMPLE=OFF -DBUILD_TOOL=OFF \
                -DCMAKE_INSTALL_PREFIX=. -B. \
                -H$$shell_path($${UDP_DISCOVERY_SRC_PATH}) && \
            $(MAKE)
        QMAKE_EXTRA_COMPILERS += udpdiscoverycpplib
    }
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
## TODO: This probably disable largefile support and maybe is not necessary with
## __ANDROID_API__ >= 24 hence should be made conditional or moved to a
## compatibility header
    DEFINES *= "fopen64=fopen"
    DEFINES *= "fseeko64=fseeko"
    DEFINES *= "ftello64=ftello"

## Static library are very susceptible to order in command line
    sLibs = bz2 $$RS_UPNP_LIB $$RS_SQL_LIB ssl crypto

    LIBS += $$linkStaticLibs(sLibs)
    PRE_TARGETDEPS += $$pretargetStaticLibs(sLibs)

    HEADERS += util/androiddebug.h
}

