
/*
 * libretroshare/src/serialiser: rstunnelitems_test.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Cyril Soler
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

/******************************************************************
 */

#include <iostream>
#include <sstream>
#include <serialiser/rstunnelitems.h>
#include <serialiser/rstlvutil.h>
#include "util/utest.h"
#include "support.h"



RsTunnelSerialiser* init_item(RsTunnelDataItem& item)
{
	uint32_t S = lrand48()%20000 ;
	item.encoded_data = malloc(S) ;
	item.encoded_data_len = S ;
	for(uint32_t i=0;i<S;++i)
		((unsigned char *)item.encoded_data)[i] = lrand48()%256 ;
	item.sourcePeerId = "67641e38df0e75432033d222eae93fff" ;
	item.relayPeerId = "6013bfc2cea7ab823af7a79fb3ca0df1" ;
	item.destPeerId = "1d5768db7cd4720d0eb75cc1917da332" ;

	return new RsTunnelSerialiser();
}
bool operator==(const RsTunnelDataItem& it1,const RsTunnelDataItem& it2)
{
	if(it1.encoded_data_len != it2.encoded_data_len) return false ;
	if(it1.sourcePeerId != it2.sourcePeerId) return false ;
	if(it1.relayPeerId != it2.relayPeerId) return false ;
	if(it1.destPeerId != it2.destPeerId) return false ;

	for(uint32_t i=0;i<it1.encoded_data_len;++i)
		if( ((unsigned char *)it1.encoded_data)[i] != ((unsigned char *)it2.encoded_data)[i])
			return false ;
	return true ;
}

RsTunnelSerialiser* init_item(RsTunnelHandshakeItem& item)
{
	item.sourcePeerId = "67641e38df0e75432033d222eae93fff" ;
	item.relayPeerId = "6013bfc2cea7ab823af7a79fb3ca0df1" ;
	item.destPeerId = "1d5768db7cd4720d0eb75cc1917da332" ;

	item.sslCertPEM = "" ;
	uint32_t s=lrand48()%20 ;
	for(uint32_t i=0;i<s;++i)
		item.sslCertPEM += "6013bfc2cea7ab823af7a79fb3ca0df1" ;
	item.connection_accepted = lrand48() ;

	return new RsTunnelSerialiser();
}
bool operator==(const RsTunnelHandshakeItem& it1,const RsTunnelHandshakeItem& it2)
{
	if(it1.sourcePeerId != it2.sourcePeerId) return false ;
	if(it1.relayPeerId != it2.relayPeerId) return false ;
	if(it1.destPeerId != it2.destPeerId) return false ;
	if(it1.sslCertPEM != it2.sslCertPEM) return false ;
	if(it1.connection_accepted != it2.connection_accepted) return false ;

	return true ;
}

INITTEST();

int main()
{
//	srand48(1) ;	// always use the same random numbers
	std::cerr << "RsTurtleItem Tests" << std::endl;

	for(uint32_t i=0;i<20;++i)
	{
		test_RsItem<RsTunnelDataItem     >(); REPORT("Serialise/Deserialise RsTunnelDataItem");          
		test_RsItem<RsTunnelHandshakeItem>(); REPORT("Serialise/Deserialise RsTunnelHandshakeItem");
	}
	
	FINALREPORT("RstunnelItem Tests");

	return TESTRESULT();
}


