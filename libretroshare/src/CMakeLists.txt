# RetroShare decentralized communication platform
#
# Copyright (C) 2021  Gioacchino Mazzurco <gio@eigenlab.org>
# Copyright (C) 2021  Asociación Civil Altermundi <info@altermundi.net>
#
# SPDX-License-Identifier: CC0-1.0

list(
	APPEND RS_PUBLIC_HEADERS
	retroshare/rsexpr.h
	retroshare/rsgxsdistsync.h
	retroshare/rsiface.h
	retroshare/rsrtt.h
	retroshare/rsbanlist.h
	retroshare/rsconfig.h
	retroshare/rsdisc.h
	retroshare/rsflags.h
	retroshare/rsgrouter.h
	retroshare/rsgxsflags.h
	retroshare/rsgxsservice.h
	retroshare/rsgxstrans.h
	retroshare/rsgxstunnel.h
	retroshare/rsids.h
	retroshare/rsnotify.h
	retroshare/rsphoto.h
	retroshare/rsplugin.h
	retroshare/rsreputations.h
	retroshare/rsservicecontrol.h
	retroshare/rstokenservice.h
	retroshare/rsturtle.h	
	retroshare/rsgossipdiscovery.h
	retroshare/rsgxscommon.h
	retroshare/rsposted.h
	retroshare/rsstatus.h
	retroshare/rsversion.h
	retroshare/rsgxsifacehelper.h
	retroshare/rshistory.h
	retroshare/rsidentity.h
	retroshare/rsmsgs.h
	retroshare/rsgxschannels.h
	retroshare/rsgxscircles.h
	retroshare/rsgxsiface.h
	retroshare/rsgxsifacetypes.h
	retroshare/rstypes.h
	retroshare/rsgxsforums.h
	retroshare/rsevents.h
	retroshare/rsfiles.h
	retroshare/rsinit.h
	retroshare/rspeers.h )

list(
	APPEND RS_SOURCES
	chat/distantchat.cc
	chat/p3chatservice.cc
	chat/rschatitems.cc
	chat/distributedchat.cc
	crypto/chacha20.cpp
	crypto/hashstream.cc
	crypto/rsaes.cc
	crypto/rscrypto.cpp )

if(RS_BITDHT)
	list(
		APPEND RS_PUBLIC_HEADERS
		retroshare/rsdht.h )

	list(
		APPEND RS_SOURCES
		dht/connectstatebox.cc
		dht/p3bitdht.cc
		dht/p3bitdht_interface.cc
		dht/p3bitdht_peernet.cc
		dht/p3bitdht_peers.cc
		dht/p3bitdht_relay.cc )
endif(RS_BITDHT)

list(
	APPEND RS_SOURCES
	file_sharing/filelist_io.cc
	file_sharing/rsfilelistitems.cc
	file_sharing/file_tree.cc
	file_sharing/directory_updater.cc
	file_sharing/p3filelists.cc
	file_sharing/hash_cache.cc
	file_sharing/dir_hierarchy.cc
	file_sharing/directory_storage.cc
	ft/ftchunkmap.cc
	ft/ftfilecreator.cc
	ft/ftfileprovider.cc
	ft/ftfilesearch.cc
	ft/ftturtlefiletransferitem.cc
	ft/fttransfermodule.cc
	ft/ftcontroller.cc
	ft/ftdatamultiplex.cc
	ft/ftextralist.cc
	ft/ftserver.cc )

list(
	APPEND RS_SOURCES
	grouter/groutermatrix.cc
	grouter/grouteritems.cc
	grouter/p3grouter.cc )

list(
	APPEND RS_SOURCES
	gxs/rsgxsdata.cc
	gxs/rsgxsrequesttypes.cc
	gxs/gxssecurity.cc
	gxs/gxstokenqueue.cc
	gxs/rsdataservice.cc
	gxs/rsgxsdataaccess.cc
	gxs/rsgxsnetutils.cc
	gxs/rsgxsnettunnel.cc
	gxs/rsgxsutil.cc
	gxs/rsnxsobserver.cpp
	gxs/rsgenexchange.cc
	gxs/rsgxsnetservice.cc )

list(
	APPEND RS_SOURCES
	gxstrans/p3gxstransitems.cc
	gxstrans/p3gxstrans.cc )

list(
	APPEND RS_SOURCES
	gxstunnel/rsgxstunnelitems.cc
	gxstunnel/p3gxstunnel.cc )

if(RS_JSON_API)
	list(
		APPEND RS_PUBLIC_HEADERS
		retroshare/rsjsonapi.h )

	list(
		APPEND RS_SOURCES
		jsonapi/jsonapi.cpp )
endif(RS_JSON_API)

list(
	APPEND RS_SOURCES
	pgp/pgpkeyutil.cc
	pgp/pgpauxutils.cc
	pgp/pgphandler.cc
	pgp/rscertificate.cc )

#./plugins/dlfcn_win32.cc
#./plugins/dlfcn_win32.h
#./plugins/pluginmanager.h
#./plugins/rscacheservice.h
#./plugins/rspqiservice.h
#./plugins/pluginmanager.cc

list(
	APPEND RS_SOURCES
	pqi/pqibin.cc
	pqi/pqiipset.cc
	pqi/pqiloopback.cc
	pqi/pqimonitor.cc
	pqi/pqipersongrp.cc
	pqi/pqiqos.cc
	pqi/pqiqosstreamer.cc
	pqi/pqisslproxy.cc
	pqi/pqistore.cc
	pqi/authgpg.cc
	pqi/p3cfgmgr.cc
	pqi/p3notify.cc
	pqi/p3servicecontrol.cc
	pqi/pqinetstatebox.cc
	pqi/pqiperson.cc
	pqi/pqiservice.cc
	pqi/pqissllistener.cc
	pqi/pqissludp.cc
	pqi/pqithreadstreamer.cc
	pqi/sslfns.cc
	pqi/authssl.cc
	pqi/p3historymgr.cc
	pqi/p3linkmgr.cc
	pqi/pqihandler.cc
	pqi/pqistreamer.cc
	pqi/p3netmgr.cc
	pqi/p3peermgr.cc
	pqi/pqinetwork.cc
	pqi/pqissl.cc
	pqi/pqisslpersongrp.cc )

#./pqi/pqissli2psam3.cpp
#./pqi/pqissli2psam3.h

list(
	APPEND RS_SOURCES
	rsitems/rsbanlistitems.cc
	rsitems/rsbwctrlitems.cc
	rsitems/rsconfigitems.cc
	rsitems/rsfiletransferitems.cc
	rsitems/rsgxscommentitems.cc
	rsitems/rsgxsforumitems.cc
	rsitems/rsgxsiditems.cc
	rsitems/rsgxsrecognitems.cc
	rsitems/rsgxsreputationitems.cc
	rsitems/rsgxsupdateitems.cc
	rsitems/rshistoryitems.cc
	rsitems/rsrttitems.cc
	rsitems/rsserviceinfoitems.cc )

#retroshare/rswiki.h
#./rsitems/rswikiitems.cc
#./rsitems/rswikiitems.h
#./rsitems/rswireitems.h

list(
	APPEND RS_SOURCES
	rsitems/rsgxschannelitems.cc
	rsitems/rsgxscircleitems.cc
	rsitems/rsgxsitems.cc
	rsitems/rsmsgitems.cc )

#./rsitems/rsphotoitems.cc
#./rsitems/rsphotoitems.h
#./rsitems/rsposteditems.cc
#./rsitems/rsposteditems.h
#./rsitems/rswireitems.cc
#retroshare/rswire.h

list(
	APPEND RS_SOURCES
	rsitems/rsnxsitems.cc )

list(
	APPEND RS_SOURCES
	rsserver/p3status.cc
	rsserver/p3face-config.cc
	rsserver/p3face-info.cc
	rsserver/p3history.cc
	rsserver/p3serverconfig.cc
	rsserver/rsloginhandler.cc
	rsserver/p3face-server.cc
	rsserver/p3msgs.cc
	rsserver/p3peers.cc
	rsserver/rsaccounts.cc
	rsserver/rsinit.cc )

list(
	APPEND RS_SOURCES
	serialiser/rsbaseserial.cc
	serialiser/rsserializable.cc
	serialiser/rstlvaddrs.cc
	serialiser/rstlvbanlist.cc
	serialiser/rstlvbase.cc
	serialiser/rstlvbinary.cc
	serialiser/rstlvfileitem.cc
	serialiser/rstlvgenericmap.inl
	serialiser/rstlvgenericparam.cc
	serialiser/rstlvidset.cc
	serialiser/rstlvimage.cc
	serialiser/rstlvitem.cc
	serialiser/rstlvkeys.cc
	serialiser/rstlvkeyvalue.cc
	serialiser/rstlvstring.cc
	serialiser/rsserializer.cc
	serialiser/rstypeserializer.cc
	serialiser/rsserial.cc )

# ./services/autoproxy
#./services/autoproxy/p3i2psam3.cpp
#./services/autoproxy/p3i2psam3.h

list(
	APPEND RS_SOURCES
	services/autoproxy/rsautoproxymonitor.cc
	services/p3bwctrl.cc
	services/p3heartbeat.cc
	services/p3service.cc
	services/p3serviceinfo.cc
	services/p3statusservice.cc
	services/p3banlist.cc
	services/p3rtt.cc
	services/rseventsservice.cc
	services/p3gxscircles.cc
	services/p3gxscommon.cc
	services/p3gxsreputation.cc
	services/p3msgservice.cc
	services/p3idservice.cc
	services/p3gxschannels.cc
	services/p3gxsforums.cc )

#./services/p3wiki.cc
#./services/p3wiki.h
#./services/p3wire.cc
#./services/p3wire.h

#./services/p3photoservice.cc
#./services/p3photoservice.h
#./services/p3postbase.cc
#./services/p3postbase.h
#./services/p3posted.cc
#./services/p3posted.h

if(RS_BRODCAST_DISCOVERY)
	list(
		APPEND RS_PUBLIC_HEADERS
		retroshare/rsbroadcastdiscovery.h )

	list(
		APPEND RS_SOURCES
		services/broadcastdiscoveryservice.cc )
endif(RS_BRODCAST_DISCOVERY)

list(
	APPEND RS_SOURCES
	tcponudp/tcppacket.cc
	tcponudp/tcpstream.cc
	tcponudp/tou.cc
	tcponudp/udppeer.cc
	tcponudp/bss_tou.cc
	tcponudp/udprelay.cc )

if(RS_BITDHT_STUNNER)
	list(
		APPEND RS_SOURCES
		tcponudp/udpstunner.cc )
endif(RS_BITDHT_STUNNER)

list(
	APPEND RS_SOURCES
	turtle/rsturtleitem.cc
	turtle/p3turtle.cc )

list(
	APPEND RS_SOURCES
#	util/contentvalue.cc
#	util/exampletst.c
#	util/rsdbbind.cc
#	util/rsdiscspace.cc
	util/rsexpr.cc
	util/rsprint.cc
#	util/rsrecogn.cc
#	util/rssharedptr.h
#	util/rstickevent.cc
	util/rstime.cc
	util/smallobject.cc
#	util/retrodb.cc
	util/rsbase64.cc
	util/rsjson.cc
#	util/rskbdinput.cc
	util/rsrandom.cc
	util/rsstring.cc
	util/rsurl.cc
	util/folderiterator.cc
	util/rsdir.cc
	util/rsfile.cc
	util/dnsresolver.cc
	util/extaddrfinder.cc
	util/rsdebug.cc
	util/rsdnsutils.cc
	util/rsnet.cc
	util/rsnet_ss.cc
	util/rsthreads.cc )

#	util/i2pcommon.cpp
# util/i2pcommon.h

if(RS_FORUM_DEEP_INDEX)
	list(
		APPEND RS_SOURCES
		deep_search/commonutils.cpp
		deep_search/forumsindex.cpp )
endif(RS_FORUM_DEEP_INDEX)


#./deep_search/filesflacindexer.hpp
#./deep_search/filesoggindexer.hpp
#./deep_search/filestaglibindexer.hpp
#./deep_search/filesindex.cpp
#./deep_search/filesindex.hpp
#./deep_search/channelsindex.cpp
#./deep_search/channelsindex.hpp

list(
	APPEND RS_SOURCES
	gossipdiscovery/gossipdiscoveryitems.cc
	gossipdiscovery/p3gossipdiscovery.cc )

if(RS_MINIUPNPC)
list(
	APPEND RS_SOURCES
	rs_upnp/upnphandler_miniupnp.cc )
endif(RS_MINIUPNPC)

#./rs_upnp/UPnPBase.cpp
#./rs_upnp/upnphandler_libupnp.cc
#./rs_upnp/upnptest.cc
#./rs_upnp/upnputil.cc

#if(CMAKE_SYSTEM_NAME STREQUAL "Android")
if(RS_ANDROID)
	list(
		APPEND RS_SOURCES
		rs_android/errorconditionwrap.cpp
		rs_android/retroshareserviceandroid.cpp
		rs_android/rsjni.cpp )
endif()
