/*
 * libretroshare/src/tests/serialiser: msgitem_test.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2010 by Christopher Evi-Parker.
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

#include <iostream>
#include <math.h>

#include "util/rsrandom.h"
#include "grouter/grouteritems.h"
#include "serialiser/rstlvutil.h"
#include "util/utest.h"
#include "support.h"
#include "rsmsgitem_test.h"

INITTEST();

RsSerialType* init_item(RsGRouterGenericDataItem& cmi)
{
	cmi.data_size = lrand48()%1000 + 1000 ;
	cmi.data_bytes = (uint8_t*)malloc(cmi.data_size) ;
	RSRandom::random_bytes(cmi.data_bytes,cmi.data_size) ;
	cmi.routing_id = RSRandom::random_u32() ;

	Sha1CheckSum cs ;
	for(int i=0;i<5;++i) cs.fourbytes[i] = RSRandom::random_u32() ;
	cmi.destination_key = cs ;

	return new RsGRouterSerialiser();
}
RsSerialType* init_item(RsGRouterACKItem& cmi)
{
	cmi.mid = RSRandom::random_u32() ;
	cmi.state = RSRandom::random_u32() ;

	return new RsGRouterSerialiser();
}
RsSerialType* init_item(RsGRouterPublishKeyItem& cmi)
{
	cmi.diffusion_id = RSRandom::random_u32() ;
	cmi.service_id = RSRandom::random_u32() ;
	cmi.randomized_distance = RSRandom::random_f32() ;

	Sha1CheckSum cs ;
	for(int i=0;i<5;++i) cs.fourbytes[i] = RSRandom::random_u32() ;
	cmi.published_key = cs ;
	cmi.description_string = "test key" ;

	return new RsGRouterSerialiser();
}
RsSerialType* init_item(RsGRouterRoutingInfoItem& cmi)
{
	cmi.origin = SSLIdType::random() ;
	cmi.received_time = RSRandom::random_u64() ;
	cmi.status_flags = RSRandom::random_u32() ;

	cmi.data_item = new RsGRouterGenericDataItem ;	

	uint32_t n = 10+(RSRandom::random_u32()%30) ;

	for(uint32_t i=0;i<n;++i)
	{
		FriendTrialRecord ftr ;
		ftr.friend_id = SSLIdType::random() ;
		ftr.time_stamp = RSRandom::random_u64() ;

		cmi.tried_friends.push_back(ftr) ;
	}

	return init_item(*cmi.data_item) ;
}
RsSerialType* init_item(RsGRouterMatrixFriendListItem& cmi)
{
	uint32_t n = 10+(RSRandom::random_u32()%30) ;

	cmi.reverse_friend_indices.clear() ;
	for(uint32_t i=0;i<n;++i)
		cmi.reverse_friend_indices.push_back(SSLIdType::random()) ;

	return new RsGRouterSerialiser();
}
RsSerialType* init_item(RsGRouterMatrixCluesItem& cmi)
{
	cmi.destination_key = Sha1CheckSum::random() ;

	uint32_t n = 10+(RSRandom::random_u32()%30) ;

	for(uint32_t i=0;i<n;++i)
	{
		RoutingMatrixHitEntry rmhe ;
		rmhe.friend_id = RSRandom::random_u32() ;
		rmhe.weight = RSRandom::random_f32() ;
		rmhe.time_stamp = RSRandom::random_u64() ;

		cmi.clues.push_back(rmhe) ;
	}

	return new RsGRouterSerialiser();
}
bool operator ==(const RsGRouterGenericDataItem& cmiLeft,const  RsGRouterGenericDataItem& cmiRight)
{
	if(cmiLeft.routing_id != cmiRight.routing_id) return false;
	if(cmiLeft.data_size  != cmiRight.data_size) return false;
	if(!(cmiLeft.destination_key == cmiRight.destination_key)) return false;
	if(memcmp(cmiLeft.data_bytes,cmiRight.data_bytes,cmiLeft.data_size)) return false;

	return true ;
}
bool operator ==(const RsGRouterPublishKeyItem& cmiLeft,const  RsGRouterPublishKeyItem& cmiRight)
{
	if(cmiLeft.diffusion_id != cmiRight.diffusion_id) return false;
	if(!(cmiLeft.published_key == cmiRight.published_key)) return false;
	if(cmiLeft.service_id != cmiRight.service_id) return false;
	if(fabs(cmiLeft.randomized_distance -  cmiRight.randomized_distance) > 0.001) return false;
	if(cmiLeft.description_string != cmiRight.description_string) return false;

	return true ;
}
bool operator ==(const RsGRouterACKItem& cmiLeft,const RsGRouterACKItem& cmiRight)
{
	if(cmiLeft.mid != cmiRight.mid) return false;
	if(cmiLeft.state != cmiRight.state) return false;

	return true;
}
bool operator ==(const RsGRouterMatrixCluesItem& cmiLeft,const RsGRouterMatrixCluesItem& cmiRight)
{
	if(!(cmiLeft.destination_key == cmiRight.destination_key)) return false;
	if(cmiLeft.clues.size() != cmiRight.clues.size()) return false;

	std::list<RoutingMatrixHitEntry>::const_iterator itl = cmiLeft.clues.begin() ;
	std::list<RoutingMatrixHitEntry>::const_iterator itr = cmiRight.clues.begin() ;

	while(itl != cmiLeft.clues.end())
	{
		if( (*itl).friend_id != (*itr).friend_id) return false ;
		if( (*itl).time_stamp != (*itr).time_stamp) return false ;

		++itl ;
		++itr ;
	}

	return true;
}
bool operator ==(const RsGRouterMatrixFriendListItem& cmiLeft,const RsGRouterMatrixFriendListItem& cmiRight)
{
	if(cmiLeft.reverse_friend_indices.size() != cmiRight.reverse_friend_indices.size()) return false;

	for(uint32_t i=0;i<cmiLeft.reverse_friend_indices.size();++i)
		if(cmiLeft.reverse_friend_indices[i] != cmiRight.reverse_friend_indices[i])
			return false ;

	return true;
}
bool operator ==(const RsGRouterRoutingInfoItem& cmiLeft,const RsGRouterRoutingInfoItem& cmiRight)
{
	if(cmiLeft.status_flags != cmiRight.status_flags) return false ;
	if(cmiLeft.origin != cmiRight.origin) return false ;
	if(cmiLeft.received_time != cmiRight.received_time) return false ;
	if(cmiLeft.tried_friends.size() != cmiRight.tried_friends.size()) return false ;

	std::list<FriendTrialRecord>::const_iterator itl(cmiLeft.tried_friends.begin()) ;
	std::list<FriendTrialRecord>::const_iterator itr(cmiRight.tried_friends.begin()) ;

	while(itl != cmiLeft.tried_friends.end())
	{
		if( (*itl).friend_id != (*itr).friend_id) return false ;
		if( (*itl).time_stamp != (*itr).time_stamp) return false ;

		++itl ;
		++itr ;
	}

	if(!(*cmiLeft.data_item == *cmiRight.data_item)) return false ;

	return true;
}
int main()
{
	test_RsItem<RsGRouterGenericDataItem >(); REPORT("Serialise/Deserialise RsGRouterGenericDataItem");
	test_RsItem<RsGRouterACKItem >(); REPORT("Serialise/Deserialise RsGRouterACKItem");
	test_RsItem<RsGRouterPublishKeyItem >(); REPORT("Serialise/Deserialise RsGRouterPublishKeyItem");
	test_RsItem<RsGRouterRoutingInfoItem >(); REPORT("Serialise/Deserialise RsGRouterRoutingInfoItem");
	test_RsItem<RsGRouterMatrixFriendListItem >(); REPORT("Serialise/Deserialise RsGRouterMatrixFriendListItem");
	test_RsItem<RsGRouterMatrixCluesItem >(); REPORT("Serialise/Deserialise RsGRouterMatrixCluesItem");

	std::cerr << std::endl;

	FINALREPORT("RsGRouter Tests");

	return TESTRESULT();
}





