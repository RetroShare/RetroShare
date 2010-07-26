/*
 * libretroshare/src/serialiser: distribitem_test.cc
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

#include "serialiser/rsdistribitems.h"
#include "serialiser/rschannelitems.h"
#include "serialiser/rsforumitems.h"
#include "serialiser/rsblogitems.h"
#include "rsiface/rsdistrib.h"
#include "serialiser/rstlvutil.h"
#include "util/utest.h"
#include "support.h"

#include "distribitem_test.h"

INITTEST()

#define SHORT_STR 100
#define LARGE_STR 1000

/** base rs distrib items **/



void init_item(RsTlvSecurityKey& sk)
{
	int randnum = rand()%313131;

	sk.endTS = randnum;
	sk.keyFlags = randnum;
	sk.startTS = randnum;
	randString(SHORT_STR, sk.keyId);

	std::string randomStr;
	randString(LARGE_STR, randomStr);

	sk.keyData.setBinData(randomStr.c_str(), randomStr.size());

	return;
}

void init_item(RsTlvKeySignature& ks)
{
	randString(SHORT_STR, ks.keyId);

	std::string signData;
	randString(LARGE_STR, signData);

	ks.signData.setBinData(signData.c_str(), signData.size());

	return;
}

void init_item(RsTlvImage& im)
{
	std::string imageData;
	randString(LARGE_STR, imageData);
	im.binData.setBinData(imageData.c_str(), imageData.size());
	im.image_type = RSTLV_IMAGE_TYPE_PNG;

	return;
}

bool operator==(const RsTlvBinaryData& bd1, const RsTlvBinaryData& bd2)
{
	if(bd1.tlvtype != bd2.tlvtype) return false;
	if(bd1.bin_len != bd2.bin_len) return false;

	unsigned char *bin1 = (unsigned char*)(bd1.bin_data),
			*bin2 = (unsigned char*)(bd2.bin_data);

	for(uint32_t i=0; i < bd1.bin_len; bin1++, bin2++, i++)
	{
		if(*bin1 != *bin2)
			return false;
	}

	return true;
}

RsSerialType* init_item(RsDistribGrp& grp)
{

	time_t now = time(NULL);

	/* create Keys */

	/* Create Group Description */

	randString(SHORT_STR, grp.grpName);
	randString(LARGE_STR, grp.grpDesc);

	grp.timestamp = now;
	grp.grpFlags = (RS_DISTRIB_PRIVACY_MASK | RS_DISTRIB_AUTHEN_MASK);
	grp.grpControlFlags = 0;

	init_item(grp.grpPixmap);

	/* set keys */

	RsTlvSecurityKey publish_key;

	init_item(grp.adminKey);
	init_item(publish_key);
	init_item(grp.adminSignature);

	grp.publishKeys.keys[publish_key.keyId] = publish_key;
	grp.publishKeys.groupId = grp.adminKey.keyId;
	grp.grpId =  grp.adminKey.keyId;

	init_item(grp.grpControlList);

	return new RsDistribSerialiser();

}

bool operator==(const RsTlvSecurityKey& sk1, const RsTlvSecurityKey& sk2)
{

	if(sk1.startTS != sk2.startTS) return false;
	if(sk1.endTS != sk2.endTS) return false;
	if(sk1.keyFlags != sk2.keyFlags) return false;
	if(sk1.keyId != sk2.keyId) return false;
	if(!(sk1.keyData == sk1.keyData)) return false;

	return true;
}

bool operator==(const RsTlvKeySignature& ks1, const RsTlvKeySignature& ks2)
{

	if(ks1.keyId != ks2.keyId) return false;
	if(!(ks1.signData == ks2.signData)) return false;

	return true;
}

bool operator==(const RsTlvPeerIdSet& pids1, const RsTlvPeerIdSet& pids2)
{
	std::list<std::string>::const_iterator it1 = pids1.ids.begin(),
			it2 = pids2.ids.begin();


	for(; ((it1 != pids1.ids.end()) && (it2 != pids2.ids.end())); it1++, it2++)
	{
		if(*it1 != *it2) return false;
	}

	return true;

}

bool operator==(const RsDistribGrp& g1, const RsDistribGrp& g2)
{

	if(g1.grpCategory != g2.grpCategory) return false;
	if(g1.grpControlFlags != g2.grpControlFlags) return false;
	if(!(g1.grpControlList == g2.grpControlList)) return false;
	if(g1.grpDesc != g2.grpDesc) return false;
	if(g1.grpFlags != g2.grpFlags) return false;
	if(g1.grpId != g2.grpId) return false;
	if(g1.grpName != g2.grpName) return false;
	if(g1.timestamp != g2.timestamp) return false;

	// admin key

	if(!(g1.adminKey == g2.adminKey)) return false;
	if(!(g1.adminSignature == g2.adminSignature)) return false;
	if(g1.grpPixmap.image_type != g2.grpPixmap.image_type) return false;
	if(!(g1.grpPixmap.binData == g2.grpPixmap.binData)) return false;

	return true;
}

RsSerialType* init_item(RsDistribGrpKey& gk)
{

	init_item(gk.key);
	gk.grpId = gk.key.keyId;

	return new RsDistribSerialiser();

}


bool operator==(const RsDistribGrpKey& gk1, const RsDistribGrpKey& gk2)
{
	if(gk1.grpId != gk1.grpId) return false;
	if(!(gk1.key == gk2.key)) return false;

	return true;
}

RsSerialType* init_item(RsDistribSignedMsg& sMsg)
{

	RsChannelMsg chMsg;
	RsSerialType* ser = init_item(chMsg);
	uint32_t chMsgSize = ser->size(&chMsg);
	char* chMsgData = new char[chMsgSize];
	ser->serialise(&chMsg, chMsgData, &chMsgSize);
	delete ser;

	sMsg.packet.setBinData(chMsgData, chMsgSize);
	sMsg.flags = rand()%300;
	randString(SHORT_STR, sMsg.grpId);

	return new RsDistribSerialiser();
}

bool operator==(const RsDistribSignedMsg& sMsg1,const  RsDistribSignedMsg& sMsg2)
{

	if(!(sMsg1.packet == sMsg2.packet)) return false;
	if(sMsg1.grpId != sMsg2.grpId) return false;
	if(sMsg1.flags != sMsg2.flags) return false;

	return true;
}

/** channels, forums and blogs **/

void init_item(RsTlvHashSet& hs)
{
	std::string hash;

	for(int i=0; i < 10; i++)
	{
		randString(SHORT_STR, hash);
		hs.ids.push_back(hash);
	}

	hs.mType = TLV_TYPE_HASHSET;
	return;
}

void init_item(RsTlvPeerIdSet& ps)
{
	std::string peer;

	for(int i=0; i < 10; i++)
	{
		randString(SHORT_STR, peer);
		ps.ids.push_back(peer);
	}

	ps.mType = TLV_TYPE_PEERSET;
	return;
}

bool operator==(const RsTlvHashSet& hs1,const RsTlvHashSet& hs2)
{
	if(hs1.mType != hs2.mType) return false;

	std::list<std::string>::const_iterator it1 = hs1.ids.begin(),
			it2 = hs2.ids.begin();

	for(; ((it1 != hs1.ids.end()) && (it2 != hs2.ids.end())); it1++, it2++)
	{
		if(*it1 != *it2) return false;
	}

	return true;
}

void init_item(RsTlvFileItem& fi)
{
	fi.age = rand()%200;
	fi.filesize = rand()%34030313;
	randString(SHORT_STR, fi.hash);
	randString(SHORT_STR, fi.name);
	randString(SHORT_STR, fi.path);
	fi.piecesize = rand()%232;
	fi.pop = rand()%2354;
	init_item(fi.hashset);

	return;
}

void init_item(RsTlvFileSet& fSet){

	randString(LARGE_STR, fSet.comment);
	randString(SHORT_STR, fSet.title);
	RsTlvFileItem fi1, fi2;
	init_item(fi1);
	init_item(fi2);
	fSet.items.push_back(fi1);
	fSet.items.push_back(fi2);

	return;
}

bool operator==(const RsTlvFileSet& fs1,const  RsTlvFileSet& fs2)
{
	if(fs1.comment != fs2.comment) return false;
	if(fs1.title != fs2.title) return false;

	std::list<RsTlvFileItem>::const_iterator it1 = fs1.items.begin(),
			it2 = fs2.items.begin();

	for(;  ((it1 != fs1.items.end()) && (it2 != fs2.items.end())); it1++, it2++)
		if(!(*it1 == *it2)) return false;

	return true;
}

bool operator==(const RsTlvFileItem& fi1,const RsTlvFileItem& fi2)
{
	if(fi1.age != fi2.age) return false;
	if(fi1.filesize != fi2.filesize) return false;
	if(fi1.hash != fi2.hash) return false;
	if(!(fi1.hashset == fi2.hashset)) return false;
	if(fi1.name != fi2.name) return false;
	if(fi1.path != fi2.path) return false;
	if(fi1.piecesize != fi2.piecesize) return false;
	if(fi1.pop != fi2.pop) return false;

	return true;
}

RsSerialType* init_item(RsChannelMsg& chMsg)
{
	randString(SHORT_STR, chMsg.grpId);
	randString(LARGE_STR, chMsg.message);
	randString(SHORT_STR, chMsg.subject);
	randString(SHORT_STR, chMsg.threadId);
	chMsg.timestamp = rand()%31452;
	init_item(chMsg.thumbnail);
	init_item(chMsg.attachment);

	return new RsChannelSerialiser();
}


bool operator==(const RsChannelMsg& chMsg1,const  RsChannelMsg& chMsg2)
{

	if(chMsg1.grpId != chMsg2.grpId) return false;
	if(chMsg1.message != chMsg2.message) return false;
	if(!(chMsg1.attachment == chMsg2.attachment)) return false;
	if(chMsg1.subject != chMsg2.subject) return false;
	if(chMsg2.threadId != chMsg2.threadId) return false;
	if(chMsg1.timestamp != chMsg2.timestamp) return false;
	if(!(chMsg1.thumbnail.binData == chMsg2.thumbnail.binData)) return false;
	if(chMsg1.thumbnail.image_type != chMsg2.thumbnail.image_type) return false;

	return true;
}

RsSerialType* init_item(RsForumMsg& fMsg)
{

	fMsg.timestamp = rand()%242;
	randString(SHORT_STR, fMsg.grpId);
	randString(LARGE_STR, fMsg.msg);
	randString(SHORT_STR, fMsg.parentId);
	randString(SHORT_STR, fMsg.srcId);
	randString(SHORT_STR, fMsg.threadId);
	randString(SHORT_STR, fMsg.title);

	return new RsForumSerialiser();
}



bool operator==(const RsForumMsg& fMsg1, const RsForumMsg& fMsg2)
{
	if(fMsg1.grpId != fMsg2.grpId) return false;
	if(fMsg1.msg != fMsg2.msg) return false;
	if(fMsg1.parentId != fMsg2.parentId) return false;
	if(fMsg1.srcId != fMsg2.srcId) return false;
	if(fMsg1.threadId != fMsg2.threadId) return false;
	if(fMsg1.timestamp != fMsg2.timestamp) return false;
	if(fMsg1.title != fMsg2.title) return false;

	return true;
}

RsSerialType* init_item(RsBlogMsg& bMsg)
{

	return new RsBlogSerialiser();
}

bool operator==(RsBlogMsg& bMsg1, RsBlogMsg& bMsg2)
{


	return true;
}

/**
 * @param T item you want to serialise
 */
template<class T> int test_RsDistribItem()
{

	/* make a serialisable RsTurtleItem */

	RsSerialiser srl;

	/* initialise */
	T rsfi ;
	RsSerialType *rsfis = init_item(rsfi) ;

	/* attempt to serialise it before we add it to the serialiser */

	CHECK(0 == srl.size(&rsfi));

	static const uint32_t MAX_BUFSIZE = 16000 ;

	char *buffer = new char[MAX_BUFSIZE];
	uint32_t sersize = MAX_BUFSIZE;

	CHECK(false == srl.serialise(&rsfi, (void *) buffer, &sersize));

	/* now add to serialiser */

	srl.addSerialType(rsfis);

	uint32_t size = srl.size(&rsfi);
	bool done = srl.serialise(&rsfi, (void *) buffer, &sersize);

	std::cerr << "test_Item() size: " << size << std::endl;
	std::cerr << "test_Item() done: " << done << std::endl;
	std::cerr << "test_Item() sersize: " << sersize << std::endl;

	std::cerr << "test_Item() serialised:" << std::endl;
//	displayRawPacket(std::cerr, (void *) buffer, sersize);

	CHECK(done == true);

	uint32_t sersize2 = sersize;
	RsItem *output = srl.deserialise((void *) buffer, &sersize2);

	CHECK(output != NULL);
	CHECK(sersize2 == sersize);

	T *outfi = dynamic_cast<T *>(output);

	CHECK(outfi != NULL);

	if (outfi)
		CHECK(*outfi == rsfi) ;

	sersize2 = MAX_BUFSIZE;
	bool done2 = srl.serialise(outfi, (void *) &(buffer[16*8]), &sersize2);

	CHECK(done2) ;
	CHECK(sersize2 == sersize);

//	displayRawPacket(std::cerr, (void *) buffer, 16 * 8 + sersize2);

	delete[] buffer ;

	return 1;
}


int main(){

	std::cerr << "RsDistribItem Tests" << std::endl;

	test_RsDistribItem<RsDistribGrp>(); REPORT("Serialise/Deserialise RsDistribGrp");
	test_RsDistribItem<RsDistribGrpKey>(); REPORT("Serialise/Deserialise RsDistribGrpKey");
	test_RsDistribItem<RsDistribSignedMsg>(); REPORT("Serialise/Deserialise RsDistribSignedMsg");
	test_RsDistribItem<RsChannelMsg>(); REPORT("Serialise/Deserialise RsChannelMsg");
	test_RsDistribItem<RsForumMsg>(); REPORT("Serialise/Deserialise RsForumMsg");
	//test_RsDistribItem<RsForumReadStatus>(); REPORT("Serialise/Deserialise RsForumReadStatus");
	//test_RsDistribItem<RsBlogMsg>(); REPORT("Serialise/Deserialise RsBlogMsg");


	FINALREPORT("RsDistribItem Tests");

	return TESTRESULT();
}


