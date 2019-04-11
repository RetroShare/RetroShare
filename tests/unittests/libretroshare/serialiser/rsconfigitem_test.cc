/*
 * libretroshare/src/tests/serialiser: rsconfigitemstest.cc
 *
 * RetroShare Serialiser tests.
 *
 * Copyright 2011 by Christopher Evi-Parker
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

#include "support.h"
#include "serialiser/rsconfigitems.h"

RsSerialType* init_item(RsPeerNetItem& rpn)
{
	randString(SHORT_STR, rpn.dyndns);
	rpn.nodePeerId.random();
	rpn.pgpId.random();
	randString(SHORT_STR, rpn.location);

	rpn.lastContact = rand()%42424;
	rpn.netMode = rand()%313190;
	rpn.visState = rand()%63635;


	inet_aton("10.0.0.111", &(rpn.currentlocaladdr.sin_addr));
	rpn.currentlocaladdr.sin_port = htons(1111);

	inet_aton("123.1.2.123", &(rpn.currentremoteaddr.sin_addr));
	rpn.currentremoteaddr.sin_port = htons(1234);

	RsTlvIpAddressInfo ipa1, ipa2;

	ipa1.addr = rpn.currentlocaladdr;
	ipa1.seenTime = rand()%40149013;
	ipa1.source = rand()%13423;

	ipa2.addr = rpn.currentremoteaddr;
	ipa2.seenTime = rand()%40139013;
	ipa2.source = rand()%1343443;

	rpn.extAddrList.mList.push_back(ipa1);
	rpn.extAddrList.mList.push_back(ipa2);

	rpn.localAddrList.mList.push_back(ipa1);
	rpn.localAddrList.mList.push_back(ipa2);


	return new RsPeerConfigSerialiser();
}

RsSerialType* init_item(RsPeerGroupItem& rpgi){

	rpgi.flag = rand()%134344;
	randString(SHORT_STR, rpgi.id);
	randString(SHORT_STR, rpgi.name);
	std::string p1, p2, p3;
	randString(SHORT_STR, p1);
	randString(SHORT_STR, p2);
	randString(SHORT_STR, p3);

	rpgi.peerIds.push_back(p1);
	rpgi.peerIds.push_back(p2);
	rpgi.peerIds.push_back(p3);

	return new RsPeerConfigSerialiser();
}

RsSerialType* init_item(CompressedChunkMap& map)
{
	map._map.clear() ;
	for(uint32_t i=0;i<15;++i)
		map._map.push_back(rand()) ;

	return new RsTurtleSerialiser();
}

RsSerialType* init_item(RsPeerStunItem& rpsi)
{
	std::string p1, p2, p3;
	randString(SHORT_STR, p1);
	randString(SHORT_STR, p2);
	randString(SHORT_STR, p3);

	rpsi.stunList.ids.push_back(p1);
	rpsi.stunList.ids.push_back(p2);
	rpsi.stunList.ids.push_back(p3);

	rpsi.stunList.mType = TLV_TYPE_PEERSET;

	return new RsPeerConfigSerialiser();
}
RsSerialType* init_item(RsCacheConfig& rcc)
{

	rcc.cachesubid = rand()%2342;
	rcc.cachetypeid = rand()%323;
	rcc.recvd = rand()%2252243;
	rcc.size = rand()%02203;
	randString(SHORT_STR, rcc.hash);
	randString(SHORT_STR, rcc.name);
	randString(SHORT_STR, rcc.path);
	randString(SHORT_STR, rcc.pid);

	return new RsCacheConfigSerialiser();
}

RsSerialType* init_item(RsFileTransfer& rft)
{

	std::string p1, p2, p3;
	randString(SHORT_STR, p1);
	randString(SHORT_STR, p2);
	randString(SHORT_STR, p3);

	rft.allPeerIds.ids.push_back(p1);
	rft.allPeerIds.ids.push_back(p2);
	rft.allPeerIds.ids.push_back(p3);
	rft.allPeerIds.mType = TLV_TYPE_PEERSET;

//
//	rft.compressed_chunk_map._map.clear();
//	const int mapSize = 15;
//	rft.compressed_chunk_map._map.resize(mapSize);
//	for(int i=0; i<mapSize; i++)
//		rft.compressed_chunk_map._map[i] = rand();
	init_item(rft.compressed_chunk_map);

	randString(SHORT_STR, rft.cPeerId);

	rft.chunk_strategy = rand()%4242;
	rft.crate = rand()%4242;
	rft.flags =  rand()%42422;
	rft.in = rand()%2323;
	rft.lrate = rand()%25234;
	rft.ltransfer = rand()%42232;
	rft.state = rand()%42122;
	rft.transferred = rand()%42242;
	rft.trate = rand()%1212;

	init_item(rft.file);

	return new RsFileConfigSerialiser();
}



bool operator==(const RsPeerNetItem& left, const RsPeerNetItem& right)
{
	if(left.dyndns != right.dyndns) return false;
	if(left.pgpId != right.pgpId) return false;
	if(left.location != right.location) return false;
	if(left.nodePeerId != right.nodePeerId) return false;
	if(left.lastContact != right.lastContact) return false;
	if(left.netMode != right.netMode) return false;
	if(left.visState != right.visState) return false;

	if(left.currentlocaladdr != right.currentlocaladdr) return false;
	if(left.currentremoteaddr != right.currentremoteaddr) return false;
	if(!(left.extAddrList.mList == right.extAddrList.mList)) return false;
	if(!(left.localAddrList.mList == right.localAddrList.mList)) return false;

	return true;
}

bool operator==(const std::list<RsTlvIpAddressInfo>& left,
		const std::list<RsTlvIpAddressInfo>& right)
{
	std::list<RsTlvIpAddressInfo>::const_iterator cit1 = left.begin(),
			cit2 = right.begin();

	for(; cit1 != left.end() ; cit1++, cit2++){

		if(*cit1 != *cit2)
			return false;
	}

	return true;
}

bool operator!=(const sockaddr_in& left, const sockaddr_in& right)
{
	if(left.sin_addr.s_addr != right.sin_addr.s_addr) return true;
	if(left.sin_port != right.sin_port) return true;

	return false;
}

bool operator!=(const RsTlvIpAddressInfo& left, const RsTlvIpAddressInfo& right)
{

	if(left.addr != right.addr) return true;
	if(left.seenTime != right.seenTime) return true;
	if(left.source != right.source) return true;

	return false;
}

bool operator==(const RsPeerGroupItem& left, const RsPeerGroupItem& right)
{
	if(left.flag != right.flag) return false;
	if(left.id != right.id) return false;
	if(left.name != right.name) return false;
	if(left.peerIds != right.peerIds) return false;

	return true;
}

bool operator!=(const std::list<std::string>& left,
		const std::list<std::string>& right)
{
	std::list<std::string>::const_iterator cit1 = left.begin(),
			cit2 = right.begin();

	for(; cit1 != left.end(); cit1++, cit2++){

		if(*cit1 != *cit2)
			return true;
	}

	return false;
}

bool operator==(const RsPeerStunItem& left, const RsPeerStunItem& right)
{
	if(!(left.stunList == right.stunList)) return false;

	return true;
}

//bool operator==(const RsTlvPeerIdSet& left, const RsTlvPeerIdSet& right)
//{
//	if(left.mType != right.mType) return false;
//
//	std::list<std::string>::iterator cit1 = left.ids.begin(),
//			cit2 = right.ids.begin();
//
//	for(; cit1 != left.ids.end(); cit1++, cit2++){
//
//		if(*cit1 != *cit2)
//			return false;
//	}
//
//	return true;
//}

bool operator==(const RsCacheConfig& left, const RsCacheConfig& right)
{

	if(left.cachesubid != right.cachesubid) return false;
	if(left.cachetypeid != right.cachetypeid) return false;
	if(left.hash != right.hash) return false;
	if(left.path != right.path) return false;
	if(left.pid != right.pid) return false;
	if(left.recvd != right.recvd) return false;
	if(left.size != right.size) return false;

	return true;
}

bool operator==(const RsFileTransfer& left, const RsFileTransfer& right)
{

	if(!(left.allPeerIds == right.allPeerIds)) return false;
	if(left.cPeerId != right.cPeerId) return false;
	if(left.chunk_strategy != right.chunk_strategy) return false;
	if(left.compressed_chunk_map._map != right.compressed_chunk_map._map) return false;
	if(left.crate != right.crate) return false;
	if(!(left.file == right.file)) return false;
	if(left.flags != right.flags) return false;
	if(left.in != right.in) return false;
	if(left.lrate != right.lrate) return false;
	if(left.ltransfer != right.ltransfer) return false;
	if(left.state != right.state) return false;
	if(left.transferred != right.transferred) return false;
	if(left.trate != right.trate) return false;

	return true;
}



TEST(libretroshare_serialiser, RsConfigItem)
{
	test_RsItem<RsPeerNetItem>();
	test_RsItem<RsPeerGroupItem>();
	test_RsItem<RsPeerStunItem>();
	test_RsItem<RsCacheConfig>();
	test_RsItem<RsFileTransfer>();
}
