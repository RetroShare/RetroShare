
/*
 * "$Id: compress.cc,v 1.1 2007-02-19 20:08:30 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2012-2012 by Cyril Soler
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */


#include <stdint.h>

#include <openssl/rand.h>
#include "util/rscompress.h"
#include "util/rsdir.h"
#include "util/utest.h"
#include "util/argstream.h"

#include <iostream>
#include <list>
#include <string>
#include <stdio.h>

void printHelp(int argc,char *argv[])
{
	std::cerr << argv[0] << ": tests AES encryption/decryption functions." << std::endl;
	std::cerr << "Usage: " << argv[0] << std::endl ;
}

bool compareStrings(const std::string& s1,const std::string& s2)
{
	uint32_t L = std::min(s1.length(),s2.length()) ;

	for(int i=0;i<L;++i)
		if(s1[i] != s2[i])
		{
			std::cerr << "Strings differ at position " << i << std::endl;
			return false ;
		}
	return s1==s2;
}
void printHex(unsigned char *data,uint32_t length)
{
	static const char outh[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' } ;
	static const char outl[16] = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' } ;

	for(uint32_t j = 0; j < length; j++)
	{
		std::cerr << outh[ (data[j]>>4) ] ;
		std::cerr << outh[ data[j] & 0xf ] ;
	}
}

INITTEST() ;

int main(int argc,char *argv[])
{
	std::string inputfile ;
	argstream as(argc,argv) ;

	as >> help() ;

	as.defaultErrorHandling() ;

	std::cerr << "Testing RsCompress" << std::endl;

	std::string source_string = "This is a very secret string, but ultimately it will always be decyphered \
										  ar cqs libretroshare.a temp/linux-g++-64/obj/p3bitdht.o \
										  temp/linux-g++-64/obj/p3bitdht_interface.o \
										  temp/linux-g++-64/obj/p3bitdht_peers.o \
										  temp/linux-g++-64/obj/p3bitdht_peernet.o \
										  temp/linux-g++-64/obj/p3bitdht_relay.o \
										  temp/linux-g++-64/obj/connectstatebox.o temp/linux-g++-64/obj/udppeer.o \
										  temp/linux-g++-64/obj/tcppacket.o temp/linux-g++-64/obj/tcpstream.o \
										  temp/linux-g++-64/obj/tou.o temp/linux-g++-64/obj/bss_tou.o \
										  temp/linux-g++-64/obj/udpstunner.o temp/linux-g++-64/obj/udprelay.o \
										  temp/linux-g++-64/obj/cachestrapper.o temp/linux-g++-64/obj/fimonitor.o \
										  temp/linux-g++-64/obj/findex.o temp/linux-g++-64/obj/fistore.o \
										  temp/linux-g++-64/obj/rsexpr.o temp/linux-g++-64/obj/ftchunkmap.o \
										  temp/linux-g++-64/obj/ftcontroller.o \
										  temp/linux-g++-64/obj/ftdatamultiplex.o temp/linux-g++-64/obj/ftdbase.o \
										  temp/linux-g++-64/obj/ftextralist.o temp/linux-g++-64/obj/ftfilecreator.o \
										  temp/linux-g++-64/obj/ftfileprovider.o \
										  temp/linux-g++-64/obj/ftfilesearch.o temp/linux-g++-64/obj/ftserver.o \
										  temp/linux-g++-64/obj/fttransfermodule.o \
										  temp/linux-g++-64/obj/ftturtlefiletransferitem.o \
										  temp/linux-g++-64/obj/authgpg.o temp/linux-g++-64/obj/authssl.o \
										  temp/linux-g++-64/obj/pgphandler.o temp/linux-g++-64/obj/pgpkeyutil.o \
										  temp/linux-g++-64/obj/rscertificate.o temp/linux-g++-64/obj/p3cfgmgr.o \
										  temp/linux-g++-64/obj/p3peermgr.o temp/linux-g++-64/obj/p3linkmgr.o \
										  temp/linux-g++-64/obj/p3netmgr.o temp/linux-g++-64/obj/p3notify.o \
										  temp/linux-g++-64/obj/pqiqos.o temp/linux-g++-64/obj/pqiarchive.o \
										  temp/linux-g++-64/obj/pqibin.o temp/linux-g++-64/obj/pqihandler.o \
										  temp/linux-g++-64/obj/p3historymgr.o temp/linux-g++-64/obj/pqiipset.o \
										  temp/linux-g++-64/obj/pqiloopback.o temp/linux-g++-64/obj/pqimonitor.o \
										  temp/linux-g++-64/obj/pqinetwork.o temp/linux-g++-64/obj/pqiperson.o \
										  temp/linux-g++-64/obj/pqipersongrp.o temp/linux-g++-64/obj/pqisecurity.o \
										  temp/linux-g++-64/obj/pqiservice.o temp/linux-g++-64/obj/pqissl.o \
										  temp/linux-g++-64/obj/pqissllistener.o \
										  temp/linux-g++-64/obj/pqisslpersongrp.o temp/linux-g++-64/obj/pqissludp.o \
										  temp/linux-g++-64/obj/pqisslproxy.o temp/linux-g++-64/obj/pqistore.o \
										  temp/linux-g++-64/obj/pqistreamer.o \
										  temp/linux-g++-64/obj/pqithreadstreamer.o \
										  temp/linux-g++-64/obj/pqiqosstreamer.o temp/linux-g++-64/obj/sslfns.o \
										  temp/linux-g++-64/obj/pqinetstatebox.o \
										  temp/linux-g++-64/obj/p3face-config.o temp/linux-g++-64/obj/p3face-msgs.o \
										  temp/linux-g++-64/obj/p3face-server.o temp/linux-g++-64/obj/p3history.o \
										  temp/linux-g++-64/obj/p3msgs.o temp/linux-g++-64/obj/p3peers.o \
										  temp/linux-g++-64/obj/p3status.o temp/linux-g++-64/obj/rsinit.o \
										  temp/linux-g++-64/obj/rsloginhandler.o temp/linux-g++-64/obj/rstypes.o \
										  temp/linux-g++-64/obj/p3serverconfig.o \
										  temp/linux-g++-64/obj/pluginmanager.o temp/linux-g++-64/obj/dlfcn_win32.o \
										  temp/linux-g++-64/obj/rspluginitems.o \
										  temp/linux-g++-64/obj/rsbaseserial.o \
										  temp/linux-g++-64/obj/rsfiletransferitems.o \
										  temp/linux-g++-64/obj/rsserviceserialiser.o \
										  temp/linux-g++-64/obj/rsconfigitems.o \
										  temp/linux-g++-64/obj/rshistoryitems.o temp/linux-g++-64/obj/rsmsgitems.o \
										  temp/linux-g++-64/obj/rsserial.o temp/linux-g++-64/obj/rsstatusitems.o \
										  temp/linux-g++-64/obj/rstlvaddrs.o temp/linux-g++-64/obj/rstlvbase.o \
										  temp/linux-g++-64/obj/rstlvfileitem.o temp/linux-g++-64/obj/rstlvimage.o \
										  temp/linux-g++-64/obj/rstlvkeys.o temp/linux-g++-64/obj/rstlvkvwide.o \
										  temp/linux-g++-64/obj/rstlvtypes.o temp/linux-g++-64/obj/rstlvutil.o \
										  temp/linux-g++-64/obj/rstlvdsdv.o temp/linux-g++-64/obj/rsdsdvitems.o \
										  temp/linux-g++-64/obj/rstlvbanlist.o \
										  temp/linux-g++-64/obj/rsbanlistitems.o \
										  temp/linux-g++-64/obj/rsbwctrlitems.o \
										  temp/linux-g++-64/obj/rsdiscovery2items.o \
										  temp/linux-g++-64/obj/rsheartbeatitems.o \
										  temp/linux-g++-64/obj/rsrttitems.o temp/linux-g++-64/obj/p3chatservice.o \
										  temp/linux-g++-64/obj/p3msgservice.o temp/linux-g++-64/obj/p3service.o \
										  temp/linux-g++-64/obj/p3statusservice.o temp/linux-g++-64/obj/p3dsdv.o \
										  temp/linux-g++-64/obj/p3banlist.o temp/linux-g++-64/obj/p3bwctrl.o \
										  temp/linux-g++-64/obj/p3discovery2.o temp/linux-g++-64/obj/p3heartbeat.o \
										  temp/linux-g++-64/obj/p3rtt.o temp/linux-g++-64/obj/p3turtle.o \
										  temp/linux-g++-64/obj/rsturtleitem.o \
										  temp/linux-g++-64/obj/folderiterator.o temp/linux-g++-64/obj/rsdebug.o \
										  temp/linux-g++-64/obj/rscompress.o temp/linux-g++-64/obj/smallobject.o \
										  temp/linux-g++-64/obj/rsdir.o temp/linux-g++-64/obj/rsdiscspace.o \
										  temp/linux-g++-64/obj/rsnet.o temp/linux-g++-64/obj/rsnet_ss.o \
										  temp/linux-g++-64/obj/extaddrfinder.o temp/linux-g++-64/obj/dnsresolver.o \
										  temp/linux-g++-64/obj/rsprint.o temp/linux-g++-64/obj/rsstring.o \
										  temp/linux-g++-64/obj/rsthreads.o temp/linux-g++-64/obj/rsversion.o \
										  temp/linux-g++-64/obj/rswin.o temp/linux-g++-64/obj/rsaes.o \
										  temp/linux-g++-64/obj/rsrandom.o temp/linux-g++-64/obj/rstickevent.o \
										  temp/linux-g++-64/obj/UPnPBase.o \
										  temp/linux-g++-64/obj/upnphandler_linux.o \
										  temp/linux-g++-64/obj/rsnxsitems.o temp/linux-g++-64/obj/rsdataservice.o \
										  temp/linux-g++-64/obj/rsgenexchange.o \
										  temp/linux-g++-64/obj/rsgxsnetservice.o temp/linux-g++-64/obj/rsgxsdata.o \
										  temp/linux-g++-64/obj/rsgxsitems.o \
										  temp/linux-g++-64/obj/rsgxsdataaccess.o temp/linux-g++-64/obj/retrodb.o \
										  temp/linux-g++-64/obj/contentvalue.o temp/linux-g++-64/obj/rsdbbind.o \
										  temp/linux-g++-64/obj/gxssecurity.o temp/linux-g++-64/obj/gxstokenqueue.o \
										  temp/linux-g++-64/obj/rsgxsnetutils.o temp/linux-g++-64/obj/rsgxsutil.o \
										  temp/linux-g++-64/obj/p3idservice.o temp/linux-g++-64/obj/rsgxsiditems.o \
										  temp/linux-g++-64/obj/p3gxscircles.o \
										  temp/linux-g++-64/obj/rsgxscircleitems.o \
										  temp/linux-g++-64/obj/p3gxsforums.o \
										  temp/linux-g++-64/obj/rsgxsforumitems.o \
										  temp/linux-g++-64/obj/p3gxschannels.o temp/linux-g++-64/obj/p3gxscommon.o \
										  temp/linux-g++-64/obj/rsgxscommentitems.o \
										  temp/linux-g++-64/obj/rsgxschannelitems.o temp/linux-g++-64/obj/p3wiki.o \
										  temp/linux-g++-64/obj/rswikiitems.o temp/linux-g++-64/obj/p3wire.o \
										  temp/linux-g++-64/obj/rswireitems.o temp/linux-g++-64/obj/p3posted.o \
										  temp/linux-g++-64/obj/rsposteditems.o \
										  temp/linux-g++-64/obj/p3photoservice.o \
										  temp/linux-g++-64/obj/rsphotoitems.o"  ;

	for(int i=0;i<5;++i)
		source_string = source_string+source_string ;

	source_string = source_string.substr(0,550000) ;

	for(int i=0;i<source_string.length()/10;++i)
		source_string[lrand48()%(source_string.length())] = lrand48()&0xff ;

	std::cerr << "Input string: length=" << source_string.length() << std::endl;
	std::cerr << "Input string: hash  =" << RsDirUtil::sha1sum((uint8_t*)source_string.c_str(),source_string.length()).toStdString() << std::endl;

	uint8_t *output_data ;
	uint32_t output_length ;

	CHECK(RsCompress::compress_memory_chunk((uint8_t*)source_string.c_str(),source_string.length(),output_data,output_length)) ;

	std::cerr << "Compressed data: " << std::endl;
	std::cerr << "   Length = " << output_length << std::endl;
	std::cerr << "   hash   = " << RsDirUtil::sha1sum(output_data,output_length).toStdString() << std::endl;

	std::cerr << "Uncompressing..." << std::endl;

	uint8_t *decomp_output_data=NULL ;
	uint32_t decomp_output_length=0 ;

	CHECK(RsCompress::uncompress_memory_chunk(output_data,output_length,decomp_output_data,decomp_output_length)) ;

	std::cerr << "Decompressed data: size=" << decomp_output_length << std::endl;
	std::cerr << "Decompressed data: hash=" << RsDirUtil::sha1sum(decomp_output_data,decomp_output_length).toStdString() << std::endl;

	std::string decompress_string((char *)decomp_output_data,decomp_output_length) ;
	CHECK(compareStrings(decompress_string, source_string)) ;

	free(decomp_output_data) ;
	free(output_data) ;

	FINALREPORT("RSCompress") ;
	return TESTRESULT() ;
}

