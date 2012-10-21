TEMPLATE = lib

# CONFIG += staticlib release
# CONFIG += staticlib testnetwork
CONFIG += staticlib \
    bitdht
CONFIG -= qt
TARGET = retroshare
CONFIG += test_voip

# GXS Stuff.
CONFIG += newcache
#CONFIG += newservices

# Beware: All data of the stripped services are lost
DEFINES *= PQI_DISABLE_TUNNEL

# ENABLE_CACHE_OPT
profiling { 
    QMAKE_CXXFLAGS -= -fomit-frame-pointer
    QMAKE_CXXFLAGS *= -pg \
        -g \
        -fno-omit-frame-pointer
}
release:

# UDP and TUNNEL dont work anymore.
# DEFINES *= PQI_DISABLE_UDP
# treat warnings as error for better removing
# QMAKE_CFLAGS += -Werror
# QMAKE_CXXFLAGS += -Werror
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
    QMAKE_CXXFLAGS -= -fomit-frame-pointer
    QMAKE_CXXFLAGS -= -O2
    QMAKE_CXXFLAGS *= -g \
        -fno-omit-frame-pointer
}
CONFIG += debug
debug { 
    # DEFINES *= DEBUG
    # DEFINES *= OPENDHT_DEBUG DHT_DEBUG CONN_DEBUG DEBUG_UDP_SORTER P3DISC_DEBUG DEBUG_UDP_LAYER FT_DEBUG EXTADDRSEARCH_DEBUG
    # DEFINES *= CONTROL_DEBUG FT_DEBUG DEBUG_FTCHUNK P3TURTLE_DEBUG
    # DEFINES *= P3TURTLE_DEBUG
    # DEFINES *= NET_DEBUG
    # DEFINES *= DISTRIB_DEBUG
    # DEFINES *= P3TURTLE_DEBUG FT_DEBUG DEBUG_FTCHUNK MPLEX_DEBUG
    # DEFINES *= STATUS_DEBUG SERV_DEBUG RSSERIAL_DEBUG #CONN_DEBUG
    QMAKE_CXXFLAGS -= -O2 \
        -fomit-frame-pointer
    QMAKE_CXXFLAGS *= -g \
        -fno-omit-frame-pointer
}
bitdht { 
    HEADERS += dht/p3bitdht.h \
        dht/connectstatebox.h \
        dht/stunaddrassist.h
    SOURCES += dht/p3bitdht.cc \
        dht/p3bitdht_interface.cc \
        dht/p3bitdht_peers.cc \
        dht/p3bitdht_peernet.cc \
        dht/p3bitdht_relay.cc \
        dht/connectstatebox.cc
    HEADERS += tcponudp/udppeer.h \
        tcponudp/bio_tou.h \
        tcponudp/tcppacket.h \
        tcponudp/tcpstream.h \
        tcponudp/tou.h \
        tcponudp/udpstunner.h \
        tcponudp/udprelay.h
    SOURCES += tcponudp/udppeer.cc \
        tcponudp/tcppacket.cc \
        tcponudp/tcpstream.cc \
        tcponudp/tou.cc \
        tcponudp/bss_tou.c \
        tcponudp/udpstunner.cc \
        tcponudp/udprelay.cc
    
    # These two aren't actually used (and don't compile) ....
    # but could be useful later
    # tcponudp/udpstunner.h \
    # tcponudp/udpstunner.cc \
    BITDHT_DIR = ../../libbitdht/src
    INCLUDEPATH += . \
        $${BITDHT_DIR}
    
    # The next line if for compliance with debian packages. Keep it!
    INCLUDEPATH += ../libbitdht
    DEFINES *= RS_USE_BITDHT
}
test_bitdht { 
    # DISABLE TCP CONNECTIONS...
    DEFINES *= P3CONNMGR_NO_TCP_CONNECTIONS
    
    # NO AUTO CONNECTIONS??? FOR TESTING DHT STATUS.
    DEFINES *= P3CONNMGR_NO_AUTO_CONNECTION
}

# ENABLED UDP NOW.
use_blogs { 
    HEADERS += services/p3blogs.h
    SOURCES += services/p3blogs.cc
    DEFINES *= RS_USE_BLOGS
}
PUBLIC_HEADERS = retroshare/rsblogs.h \
    retroshare/rschannels.h \
    retroshare/rsdisc.h \
    retroshare/rsdistrib.h \
    retroshare/rsexpr.h \
    retroshare/rsfiles.h \
    retroshare/rsforums.h \
    retroshare/rshistory.h \
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
    retroshare/rstypes.h \
    retroshare/rsdht.h \
    retroshare/rsdsdv.h \
    retroshare/rsconfig.h \
    retroshare/rsphotoV2.h
HEADERS += plugins/pluginmanager.h \
    plugins/dlfcn_win32.h \
    serialiser/rspluginitems.h
HEADERS += $$PUBLIC_HEADERS

# public headers to be...
HEADERS += retroshare/rsgame.h \
    retroshare/rsphoto.h

# ################################ Linux ##########################################
        linux-* { 
            QMAKE_CC = g++
            OBJECTS_DIR = temp/obj
            MOC_DIR = temp/moc
            DESTDIR = lib
            
            # miniupnp implementation files
            #HEADERS += upnp/upnputil.h
            #SOURCES += upnp/upnputil.c

            # libupnp implementation files
            HEADERS += upnp/UPnPBase.h
            SOURCES += upnp/UPnPBase.cpp
            
            # zeroconf disabled at the end of libretroshare.pro (but need the code)
            #CONFIG += zeroconf
            #CONFIG += zcnatassist
            
            OPENPGPSDK_DIR = ../../openpgpsdk/src
            INCLUDEPATH += $${OPENPGPSDK_DIR}

				INCLUDEPATH *= /usr/lib/x86_64-linux-gnu/glib-2.0/include/
				INCLUDEPATH *= /usr/lib/i386-linux-gnu/glib-2.0/include/
				INCLUDEPATH *= /usr/include/glib-2.0/ /usr/lib/glib-2.0/include
				INCLUDEPATH *= /usr/local/include/glib-2.0

				DEFINES *= UBUNTU
        }
 
        # ################### Cross compilation for windows under Linux ####################
        win32-x-g++ { 
            OBJECTS_DIR = temp/win32xgcc/obj
            DESTDIR = lib.win32xgcc
            DEFINES *= WINDOWS_SYS \
                WIN32 \
                WIN_CROSS_UBUNTU
            QMAKE_CXXFLAGS *= -Wmissing-include-dirs
            QMAKE_CC = i586-mingw32msvc-g++
            QMAKE_LIB = i586-mingw32msvc-ar
            QMAKE_AR = i586-mingw32msvc-ar
            DEFINES *= STATICLIB \
                WIN32
            
            # miniupnp implementation files
            HEADERS += upnp/upnputil.h
            SOURCES += upnp/upnputil.c
            SSL_DIR = ../../../../openssl
            UPNPC_DIR = ../../../../miniupnpc-1.3
            GPG_ERROR_DIR = ../../../../libgpg-error-1.7
            GPGME_DIR = ../../../../gpgme-1.1.8
            INCLUDEPATH *= /usr/i586-mingw32msvc/include \
                ${HOME}/.wine/drive_c/pthreads/include/
        }
        
        # ################################ Windows ##########################################
        win32 { 
            QMAKE_CC = g++
            OBJECTS_DIR = temp/obj
            MOC_DIR = temp/moc
            DEFINES *= WINDOWS_SYS \
                WIN32 \
                STATICLIB \
                MINGW
            DEFINES *= MINIUPNPC_VERSION=13
            DESTDIR = lib
            
            # Switch on extra warnings
            QMAKE_CFLAGS += -Wextra
            QMAKE_CXXFLAGS += -Wextra
            
            # Switch off optimization for release version
            QMAKE_CXXFLAGS_RELEASE -= -O2
            QMAKE_CXXFLAGS_RELEASE += -O0
            QMAKE_CFLAGS_RELEASE -= -O2
            QMAKE_CFLAGS_RELEASE += -O0
            
            # Switch on optimization for debug version
            # QMAKE_CXXFLAGS_DEBUG += -O2
            # QMAKE_CFLAGS_DEBUG += -O2
            DEFINES += USE_CMD_ARGS
            
            # miniupnp implementation files
            HEADERS += upnp/upnputil.h
            SOURCES += upnp/upnputil.c
            UPNPC_DIR = ../../../lib/miniupnpc-1.3
            PTHREADS_DIR = ../../../lib/pthreads-w32-2-8-0-release
            ZLIB_DIR = ../../../lib/zlib-1.2.3
            SSL_DIR = ../../../OpenSSL
            OPENPGPSDK_DIR = ../../openpgpsdk/src
            INCLUDEPATH += . \
                $${SSL_DIR}/include \
                $${UPNPC_DIR} \
                $${PTHREADS_DIR} \
                $${ZLIB_DIR} \
                $${OPENPGPSDK_DIR}
            newcache { 
                SQLITE_DIR = ../../../../Libraries/sqlite/sqlite-autoconf-3070900
                INCLUDEPATH += . \
                    $${SQLITE_DIR}
            }
        }
        
        # ################################ MacOSX ##########################################
        mac { 
            QMAKE_CC = g++
            OBJECTS_DIR = temp/obj
            MOC_DIR = temp/moc
            
            # DEFINES = WINDOWS_SYS WIN32 STATICLIB MINGW
            # DEFINES *= MINIUPNPC_VERSION=13
            DESTDIR = lib
            
            # miniupnp implementation files
            HEADERS += upnp/upnputil.h
            SOURCES += upnp/upnputil.c
            
            # zeroconf disabled at the end of libretroshare.pro (but need the code)
            CONFIG += zeroconf
            CONFIG += zcnatassist
            
            # Beautiful Hack to fix 64bit file access.
            QMAKE_CXXFLAGS *= -Dfseeko64=fseeko \
                -Dftello64=ftello \
                -Dfopen64=fopen \
                -Dvstatfs64=vstatfs
            UPNPC_DIR = ../../../miniupnpc-1.0
            
            # GPG_ERROR_DIR = ../../../../libgpg-error-1.7
            # GPGME_DIR  = ../../../../gpgme-1.1.8
            OPENPGPSDK_DIR = ../../openpgpsdk/src
            INCLUDEPATH += . \
                $${UPNPC_DIR}
            INCLUDEPATH += $${OPENPGPSDK_DIR}
        }
        
        # ../openpgpsdk
        # INCLUDEPATH += . $${UPNPC_DIR} $${GPGME_DIR}/src $${GPG_ERROR_DIR}/src
        # ################################ FreeBSD ##########################################
        freebsd-* { 
            INCLUDEPATH *= /usr/local/include/gpgme
            INCLUDEPATH *= /usr/local/include/glib-2.0
            QMAKE_CXXFLAGS *= -Dfseeko64=fseeko \
                -Dftello64=ftello \
                -Dstat64=stat \
                -Dstatvfs64=statvfs \
                -Dfopen64=fopen
            
            # libupnp implementation files
            HEADERS += upnp/UPnPBase.h
            SOURCES += upnp/UPnPBase.cpp
            DESTDIR = lib
        }
        
        # ################################## COMMON stuff ##################################
        HEADERS += dbase/cachestrapper.h \
            dbase/fimonitor.h \
            dbase/findex.h \
            dbase/fistore.h
        
        # HEADERS +=	dht/p3bitdht.h \
        HEADERS += ft/ftchunkmap.h \
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
        HEADERS += pqi/authssl.h \
            pqi/authgpg.h \
            pgp/pgphandler.h \
            pgp/pgpkeyutil.h \
            pqi/cleanupxpgp.h \
            pqi/p3cfgmgr.h \
            pqi/p3peermgr.h \
            pqi/p3linkmgr.h \
            pqi/p3netmgr.h \
            pqi/p3dhtmgr.h \
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
            pqi/pqiqosstreamer.h \
            pqi/sslfns.h \
            pqi/pqinetstatebox.h
        HEADERS += rsserver/p3discovery.h \
            rsserver/p3face.h \
            rsserver/p3history.h \
            rsserver/p3msgs.h \
            rsserver/p3peers.h \
            rsserver/p3status.h \
            rsserver/p3serverconfig.h
        HEADERS += serialiser/rsbaseitems.h \
            serialiser/rsbaseserial.h \
            serialiser/rsblogitems.h \
            serialiser/rschannelitems.h \
            serialiser/rsconfigitems.h \
            serialiser/rsdiscitems.h \
            serialiser/rsdistribitems.h \
            serialiser/rsforumitems.h \
            serialiser/rsgameitems.h \
            serialiser/rshistoryitems.h \
            serialiser/rsmsgitems.h \
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
            serialiser/rstlvdsdv.h \
            serialiser/rsdsdvitems.h \
            serialiser/rstlvbanlist.h \
            serialiser/rsbanlistitems.h \
            serialiser/rsbwctrlitems.h \
            serialiser/rstunnelitems.h
        HEADERS += services/p3channels.h \
            services/p3chatservice.h \
            services/p3disc.h \
            services/p3forums.h \
            services/p3gamelauncher.h \
            services/p3gameservice.h \
            services/p3msgservice.h \
            services/p3service.h \
            services/p3statusservice.h \
            services/p3dsdv.h \
            services/p3banlist.h \
            services/p3bwctrl.h \
            services/p3tunnel.h
        HEADERS += distrib/p3distrib.h \
            distrib/p3distribsecurity.h
        
        # services/p3blogs.h \
        HEADERS += turtle/p3turtle.h \
            turtle/rsturtleitem.h \
            turtle/turtletypes.h
        HEADERS += upnp/upnphandler.h
        HEADERS += util/folderiterator.h \
            util/rsdebug.h \
            util/smallobject.h \
            util/rsdir.h \
            util/rsdiscspace.h \
            util/rsnet.h \
            util/extaddrfinder.h \
            util/dnsresolver.h \
            util/rsprint.h \
            util/rsstring.h \
            util/rsthreads.h \
            util/rsversion.h \
            util/rswin.h \
            util/rsrandom.h \
            util/radix64.h \
            util/pugiconfig.h
        SOURCES += dbase/cachestrapper.cc \
            dbase/fimonitor.cc \
            dbase/findex.cc \
            dbase/fistore.cc \
            dbase/rsexpr.cc
        SOURCES += ft/ftchunkmap.cc \
            ft/ftcontroller.cc \
            ft/ftdata.cc \
            ft/ftdatamultiplex.cc \
            ft/ftdbase.cc \
            ft/ftextralist.cc \
            ft/ftfilecreator.cc \
            ft/ftfileprovider.cc \
            ft/ftfilesearch.cc \
            ft/ftserver.cc \
            ft/fttransfermodule.cc
        SOURCES += pqi/authgpg.cc \
            pqi/authssl.cc \
            pgp/pgphandler.cc \
            pgp/pgpkeyutil.cc \
            pqi/cleanupxpgp.cc \
            pqi/p3cfgmgr.cc \
            pqi/p3peermgr.cc \
            pqi/p3linkmgr.cc \
            pqi/p3netmgr.cc \
            pqi/p3dhtmgr.cc \
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
            pqi/pqisecurity.cc \
            pqi/pqiservice.cc \
            pqi/pqissl.cc \
            pqi/pqissllistener.cc \
            pqi/pqisslpersongrp.cc \
            pqi/pqissltunnel.cc \
            pqi/pqissludp.cc \
            pqi/pqistore.cc \
            pqi/pqistreamer.cc \
            pqi/pqiqosstreamer.cc \
            pqi/sslfns.cc \
            pqi/pqinetstatebox.cc
        SOURCES += rsserver/p3discovery.cc \
            rsserver/p3face-config.cc \
            rsserver/p3face-msgs.cc \
            rsserver/p3face-server.cc \
            rsserver/p3history.cc \
            rsserver/p3msgs.cc \
            rsserver/p3peers.cc \
            rsserver/p3status.cc \
            rsserver/rsiface.cc \
            rsserver/rsinit.cc \
            rsserver/rsloginhandler.cc \
            rsserver/rstypes.cc \
            rsserver/p3serverconfig.cc
        SOURCES += plugins/pluginmanager.cc \
            plugins/dlfcn_win32.cc \
            serialiser/rspluginitems.cc
        SOURCES += serialiser/rsbaseitems.cc \
            serialiser/rsbaseserial.cc \
            serialiser/rsblogitems.cc \
            serialiser/rschannelitems.cc \
            serialiser/rsconfigitems.cc \
            serialiser/rsdiscitems.cc \
            serialiser/rsdistribitems.cc \
            serialiser/rsforumitems.cc \
            serialiser/rsgameitems.cc \
            serialiser/rshistoryitems.cc \
            serialiser/rsmsgitems.cc \
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
            serialiser/rstlvdsdv.cc \
            serialiser/rsdsdvitems.cc \
            serialiser/rstlvbanlist.cc \
            serialiser/rsbanlistitems.cc \
            serialiser/rsbwctrlitems.cc \
            serialiser/rstunnelitems.cc
        SOURCES += services/p3channels.cc \
            services/p3chatservice.cc \
            services/p3disc.cc \
            services/p3forums.cc \
            services/p3gamelauncher.cc \
            services/p3msgservice.cc \
            services/p3service.cc \
            services/p3statusservice.cc \
            services/p3dsdv.cc \
            services/p3banlist.cc \
            services/p3bwctrl.cc
        
        # removed because getPeer() doesn t exist			services/p3tunnel.cc
        SOURCES += distrib/p3distrib.cc \
            distrib/p3distribsecurity.cc
        SOURCES += turtle/p3turtle.cc \
            turtle/rsturtleitem.cc
        
        # turtle/turtlerouting.cc \
        # turtle/turtlesearch.cc \
        # turtle/turtletunnels.cc
        SOURCES += upnp/upnphandler.cc
        SOURCES += util/folderiterator.cc \
            util/rsdebug.cc \
            util/smallobject.cc \
            util/rsdir.cc \
            util/rsdiscspace.cc \
            util/rsnet.cc \
            util/extaddrfinder.cc \
            util/dnsresolver.cc \
            util/rsprint.cc \
            util/rsstring.cc \
            util/rsthreads.cc \
            util/rsversion.cc \
            util/rswin.cc \
            util/rsrandom.cc
        zeroconf { 
            HEADERS += zeroconf/p3zeroconf.h
            SOURCES += zeroconf/p3zeroconf.cc
        }
        
        # Disable Zeroconf (we still need the code for zcnatassist
        # DEFINES *= RS_ENABLE_ZEROCONF
        # This is seperated from the above for windows/linux platforms.
        # It is acceptable to build in zeroconf and have it not work,
        # but unacceptable to rely on Apple's libraries for Upnp when we have alternatives. '
        zcnatassist { 
            HEADERS += zeroconf/p3zcnatassist.h
            SOURCES += zeroconf/p3zcnatassist.cc
            DEFINES *= RS_ENABLE_ZCNATASSIST
        }
        
        # new gxs cache system
        newcache { 
            HEADERS += serialiser/rsnxsitems.h \
                gxs/rsgds.h \
                gxs/rsgxs.h \
                gxs/rsdataservice.h \
                gxs/rsgxsnetservice.h \
                gxs/rsgxsflags.h \
                gxs/rsgenexchange.h \
                gxs/rsnxsobserver.h \
                gxs/rsgxsdata.h \
                gxs/rstokenservice.h \
                gxs/rsgxsdataaccess.h \
                retroshare/rsgxsservice.h \
                serialiser/rsgxsitems.h \
                serialiser/rsphotov2items.h \
                util/retrodb.h \
                util/contentvalue.h \
                gxs/gxscoreserver.h \
                gxs/gxssecurity.h \
                gxs/gxssecurity.h \
                gxs/rsgxsifaceimpl.h \
                services/p3posted.h \
                retroshare/rsposted.h \


            SOURCES += serialiser/rsnxsitems.cc \
                gxs/rsdataservice.cc \
                gxs/rsgenexchange.cc \
                gxs/rsgxsnetservice.cc \
                gxs/rsgxsdata.cc \
                serialiser/rsgxsitems.cc \
                services/p3photoserviceV2.cc \
                gxs/rsgxsdataaccess.cc \
                serialiser/rsphotov2items.cc \
                util/retrodb.cc \
                util/contentvalue.cc \
                gxs/gxscoreserver.cc \
                gxs/gxssecurity.cc \
                gxs/gxssecurity.cc \
                gxs/rsgxsifaceimpl.cc \
                services/p3posted.cc \

            # Identity Service
            HEADERS += retroshare/rsidentity.h \
                gxs/rsgixs.h \
                services/p3idservice.h \
                serialiser/rsgxsiditems.h \

            SOURCES += services/p3idservice.cc \
            #    serialiser/rsgxsiditems.cc \

            # Wiki Service
            HEADERS += retroshare/rswiki.h \
                services/p3wiki.h \
                serialiser/rswikiitems.h \

            SOURCES += services/p3wiki.cc \
            #    serialiser/rswikiitems.cc \

        }

        newservices { 
            HEADERS += services/p3photoserviceV2.h \
                retroshare/rsphotoVEG.h \
                services/p3gxsserviceVEG.h \
                retroshare/rsidentityVEG.h \
                services/p3wikiserviceVEG.h \
                retroshare/rswikiVEG.h \
                retroshare/rswireVEG.h \
                services/p3wireVEG.h \
                services/p3idserviceVEG.h \
                retroshare/rsforumsVEG.h \
                services/p3forumsVEG.h \
                retroshare/rspostedVEG.h \
                services/p3postedVEG.h

		# Do I need this?
                #serialiser/rsphotoitemsVEG.h \

            SOURCES += services/p3gxsserviceVEG.cc \
                services/p3wikiserviceVEG.cc \
                services/p3wireVEG.cc \
                services/p3idserviceVEG.cc \
                services/p3forumsVEG.cc \
                services/p3postedVEG.cc

		# Do I need this?
                # serialiser/rsphotoitemsVEG.cc \
        }



