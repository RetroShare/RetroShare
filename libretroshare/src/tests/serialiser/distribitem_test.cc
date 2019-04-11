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
#include "util/rstime.h"
#include "serialiser/rsdistribitems.h"
#include "serialiser/rschannelitems.h"
#include "serialiser/rsforumitems.h"
#include "serialiser/rsblogitems.h"
#include "retroshare/rsdistrib.h"
#include "serialiser/rstlvutil.h"
#include "util/utest.h"
#include "support.h"

#include "distribitem_test.h"

INITTEST()

/** base rs distrib items **/




RsSerialType* init_item(RsDistribGrp& grp)
{

	rstime_t now = time(NULL);

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
	bMsg.timestamp = rand()%223;
	randString(SHORT_STR, bMsg.grpId);
	randString(LARGE_STR, bMsg.message);
	randString(SHORT_STR, bMsg.subject);
	randString(SHORT_STR, bMsg.parentId);
	randString(SHORT_STR, bMsg.threadId);
	RsTlvImage image;
	int nImages = rand()%5;

	for(int i=0; i < nImages; i++)
	{
		init_item(image);
		bMsg.graphic_set.push_back(image);
	}

	return new RsBlogSerialiser();
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



RsSerialType* init_item(RsForumReadStatus& fRdStatus)
{
	randString(SHORT_STR, fRdStatus.forumId);
	fRdStatus.save_type = rand()%42;

	std::map<std::string, uint32_t>::iterator mit = fRdStatus.msgReadStatus.begin();

	std::string id;
	uint32_t status = 0;

	int numMaps = rand()%12;

	for(int i = 0; i < numMaps; i++)
	{
		randString(SHORT_STR, id);
		status = rand()%23;

		fRdStatus.msgReadStatus.insert(std::pair<std::string, uint32_t>(id, status));
	}

	return new RsForumSerialiser();
}

bool operator==(const RsForumReadStatus& frs1, const RsForumReadStatus& frs2)
{
	if(frs1.forumId != frs2.forumId) return false;
	if(frs1.save_type != frs2.save_type) return false;

	if(frs1.msgReadStatus.size() != frs2.msgReadStatus.size()) return false;

	std::map<std::string, uint32_t>::const_iterator mit
		= frs1.msgReadStatus.begin();



	for(;mit != frs1.msgReadStatus.end(); mit++)
	{
		if(mit->second != frs2.msgReadStatus.find(mit->first)->second) return false;
	}

	return true;

}



RsSerialType* init_item(RsChannelReadStatus& fRdStatus)
{
	randString(SHORT_STR, fRdStatus.channelId);
	fRdStatus.save_type = rand()%42;

	std::map<std::string, uint32_t>::iterator mit = fRdStatus.msgReadStatus.begin();

	std::string id;
	uint32_t status = 0;

	int numMaps = rand()%12;

	for(int i = 0; i < numMaps; i++)
	{
		randString(SHORT_STR, id);
		status = rand()%23;

		fRdStatus.msgReadStatus.insert(std::pair<std::string, uint32_t>(id, status));
	}

	return new RsChannelSerialiser();
}

bool operator==(const RsChannelReadStatus& frs1, const RsChannelReadStatus& frs2)
{
	if(frs1.channelId != frs2.channelId) return false;
	if(frs1.save_type != frs2.save_type) return false;

	if(frs1.msgReadStatus.size() != frs2.msgReadStatus.size()) return false;

	std::map<std::string, uint32_t>::const_iterator mit
		= frs1.msgReadStatus.begin();



	for(;mit != frs1.msgReadStatus.end(); mit++)
	{
		if(mit->second != frs2.msgReadStatus.find(mit->first)->second) return false;
	}

	return true;

}


bool operator==(const RsBlogMsg& bMsg1,const RsBlogMsg& bMsg2)
{

	if(bMsg1.timestamp != bMsg2.timestamp) return false;
	if(bMsg1.grpId != bMsg2.grpId) return false;
	if(bMsg1.message != bMsg2.message) return false;
	if(bMsg1.subject != bMsg2.subject) return false;
	if(bMsg1.threadId != bMsg2.threadId) return false;
	if(bMsg1.parentId != bMsg2.parentId) return false;

	std::list<RsTlvImage >::const_iterator it1 = bMsg1.graphic_set.begin(),
			it2 = bMsg2.graphic_set.begin();

	if(bMsg1.graphic_set.size() != bMsg2.graphic_set.size()) return false;

	for(; it1 != bMsg1.graphic_set.end() ; it1++, it2++)
	{
		if(!(*it1 == *it2)) return false;
	}

	return true;
}

int main(){

	std::cerr << "RsDistribItem Tests" << std::endl;

	test_RsItem<RsDistribGrp>(); REPORT("Serialise/Deserialise RsDistribGrp");
	test_RsItem<RsDistribGrpKey>(); REPORT("Serialise/Deserialise RsDistribGrpKey");
	test_RsItem<RsDistribSignedMsg>(); REPORT("Serialise/Deserialise RsDistribSignedMsg");
	test_RsItem<RsChannelMsg>(); REPORT("Serialise/Deserialise RsChannelMsg");
	test_RsItem<RsChannelReadStatus>(); REPORT("Serialise/Deserialise RsChannelReadStatus");
	test_RsItem<RsForumMsg>(); REPORT("Serialise/Deserialise RsForumMsg");
	test_RsItem<RsForumReadStatus>(); REPORT("Serialise/Deserialise RsForumReadStatus");
	test_RsItem<RsBlogMsg>(); REPORT("Serialise/Deserialise RsBlogMsg");


	FINALREPORT("RsDistribItem Tests");

	return TESTRESULT();
}

