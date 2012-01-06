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

#include "util/rsrandom.h"
#include "serialiser/rsmsgitems.h"
#include "serialiser/rstlvutil.h"
#include "util/utest.h"
#include "support.h"
#include "rsmsgitem_test.h"

INITTEST();

RsSerialType* init_item(RsChatMsgItem& cmi)
{
	cmi.chatFlags = rand()%34;
	cmi.sendTime = rand()%422224;
	randString(LARGE_STR, cmi.message);

	return new RsChatSerialiser();
}
RsSerialType* init_item(RsChatLobbyListRequestItem& cmi)
{
	return new RsChatSerialiser();
}
RsSerialType* init_item(RsChatLobbyListItem& cmi)
{
	int n = rand()%20 ;

	cmi.lobby_ids.resize(n) ;
	cmi.lobby_names.resize(n) ;
	cmi.lobby_counts.resize(n) ;

	for(int i=0;i<n;++i)
	{
		cmi.lobby_ids[i] = RSRandom::random_u64() ;
		randString(5+(rand()%10), cmi.lobby_names[i]);
		cmi.lobby_counts[i] = RSRandom::random_u32() ;
	}

	return new RsChatSerialiser();
}
RsSerialType* init_item(RsChatLobbyMsgItem& cmi)
{
	RsSerialType *serial = init_item( *dynamic_cast<RsChatMsgItem*>(&cmi)) ;

	cmi.msg_id = RSRandom::random_u64() ;
	cmi.lobby_id = RSRandom::random_u64() ;
	cmi.nick = "My nickname" ;
	cmi.subpacket_id = rand()%256 ;
	cmi.parent_msg_id = RSRandom::random_u64() ;

	return serial ;
}
RsSerialType *init_item(RsChatLobbyEventItem& cmi)
{
	cmi.event_type = rand()%256 ;
	randString(20, cmi.string1);

	return new RsChatSerialiser();
}

RsSerialType* init_item(RsChatLobbyInviteItem& cmi)
{
	cmi.lobby_id = RSRandom::random_u64() ;
	cmi.lobby_name = "Name of the lobby" ;

	return new RsChatSerialiser();
}

RsSerialType* init_item(RsPrivateChatMsgConfigItem& pcmi)
{
	randString(SHORT_STR, pcmi.configPeerId);
	pcmi.chatFlags = rand()%34;
	pcmi.configFlags = rand()%21;
	pcmi.sendTime = rand()%422224;
	randString(LARGE_STR, pcmi.message);
	pcmi.recvTime = rand()%344443;

	return new RsChatSerialiser();
}

RsSerialType* init_item(RsChatStatusItem& csi)
{

	randString(SHORT_STR, csi.status_string);
	csi.flags = rand()%232;

	return new RsChatSerialiser();

}

RsSerialType* init_item(RsChatAvatarItem& cai)
{
	std::string image_data;
	randString(LARGE_STR, image_data);
	cai.image_data = new unsigned char[image_data.size()];

	memcpy(cai.image_data, image_data.c_str(), image_data.size());
	cai.image_size = image_data.size();

	return new RsChatSerialiser();
}

RsSerialType* init_item(RsMsgItem& mi)
{
	init_item(mi.attachment);
	init_item(mi.msgbcc);
	init_item(mi.msgcc);
	init_item(mi.msgto);

	randString(LARGE_STR, mi.message);
	randString(SHORT_STR, mi.subject);

	mi.msgId = rand()%324232;
	mi.recvTime = rand()%44252;
	mi.sendTime = mi.recvTime;
	mi.msgFlags = mi.recvTime;

	return new RsMsgSerialiser(true);
}

RsSerialType* init_item(RsMsgTagType& mtt)
{
	mtt.rgb_color = rand()%5353;
	mtt.tagId = rand()%24242;
	randString(SHORT_STR, mtt.text);

	return new RsMsgSerialiser();
}


RsSerialType* init_item(RsMsgTags& mt)
{
	mt.msgId = rand()%3334;

	int i;
	for (i = 0; i < 10; i++) {
		mt.tagIds.push_back(rand()%21341);
	}

	return new RsMsgSerialiser();
}

RsSerialType* init_item(RsMsgSrcId& ms)
{
	ms.msgId = rand()%434;
	randString(SHORT_STR, ms.srcId);

	return new RsMsgSerialiser();
}

RsSerialType* init_item(RsMsgParentId& ms)
{
	ms.msgId = rand()%354;
	ms.msgParentId = rand()%476;

	return new RsMsgSerialiser();
}

bool operator ==(const RsChatLobbyListItem& cmiLeft,const  RsChatLobbyListItem& cmiRight)
{
	if(cmiLeft.lobby_ids.size() != cmiRight.lobby_ids.size()) return false;
	if(cmiLeft.lobby_names.size() != cmiRight.lobby_names.size()) return false;
	if(cmiLeft.lobby_counts.size() != cmiRight.lobby_counts.size()) return false;

	for(uint32_t i=0;i<cmiLeft.lobby_ids.size();++i)
	{
		if(cmiLeft.lobby_ids[i] != cmiRight.lobby_ids[i]) return false ;
		if(cmiLeft.lobby_names[i] != cmiRight.lobby_names[i]) return false ;
		if(cmiLeft.lobby_counts[i] != cmiRight.lobby_counts[i]) return false ;
	}
	return true ;
}
bool operator ==(const RsChatLobbyListRequestItem& cmiLeft,const  RsChatLobbyListRequestItem& cmiRight)
{
	return true ;
}
bool operator ==(const RsChatMsgItem& cmiLeft,const  RsChatMsgItem& cmiRight)
{

	if(cmiLeft.chatFlags != cmiRight.chatFlags) return false;
	if(cmiLeft.message != cmiRight.message) return false;
	if(cmiLeft.sendTime != cmiRight.sendTime) return false;

	return true;
}

bool operator ==(const RsPrivateChatMsgConfigItem& pcmiLeft,const  RsPrivateChatMsgConfigItem& pcmiRight)
{

	if(pcmiLeft.configPeerId != pcmiRight.configPeerId) return false;
	if(pcmiLeft.chatFlags != pcmiRight.chatFlags) return false;
	if(pcmiLeft.configFlags != pcmiRight.configFlags) return false;
	if(pcmiLeft.message != pcmiRight.message) return false;
	if(pcmiLeft.sendTime != pcmiRight.sendTime) return false;
	if(pcmiLeft.recvTime != pcmiRight.recvTime) return false;

	return true;
}

bool operator ==(const RsChatStatusItem& csiLeft, const RsChatStatusItem& csiRight)
{
	if(csiLeft.flags != csiRight.flags) return false;
	if(csiLeft.status_string != csiRight.status_string) return false;

	return true;
}
bool operator ==(const RsChatLobbyMsgItem& csiLeft, const RsChatLobbyMsgItem& csiRight)
{
	if(! ( (RsChatMsgItem&)csiLeft == (RsChatMsgItem&)csiRight))
		return false ;

	if(csiLeft.lobby_id != csiRight.lobby_id) return false ;
	if(csiLeft.msg_id != csiRight.msg_id) return false ;
	if(csiLeft.nick != csiRight.nick) return false ;

	return true;
}
bool operator ==(const RsChatLobbyEventItem& csiLeft, const RsChatLobbyEventItem& csiRight)
{
	if(csiLeft.lobby_id != csiRight.lobby_id) return false ;
	if(csiLeft.msg_id != csiRight.msg_id) return false ;
	if(csiLeft.nick != csiRight.nick) return false ;
	if(csiLeft.event_type != csiRight.event_type) return false ;
	if(csiLeft.string1 != csiRight.string1) return false ;

	return true;
}
bool operator ==(const RsChatLobbyInviteItem& csiLeft, const RsChatLobbyInviteItem& csiRight)
{
	if(csiLeft.lobby_id != csiRight.lobby_id) return false ;
	if(csiLeft.lobby_name != csiRight.lobby_name) return false ;

	return true;
}

bool operator ==(const RsChatAvatarItem& caiLeft, const RsChatAvatarItem& caiRight)
{
	unsigned char* image_dataLeft = (unsigned char*)caiLeft.image_data;
	unsigned char* image_dataRight = (unsigned char*)caiRight.image_data;

	// make image sizes are the same to prevent dereferencing garbage
	if(caiLeft.image_size == caiRight.image_size)
	{
		image_dataLeft = (unsigned char*)caiLeft.image_data;
		image_dataRight = (unsigned char*)caiRight.image_data;
	}
	else
	{
		return false;
	}

	for(uint32_t i = 0; i < caiLeft.image_size; i++)
		if(image_dataLeft[i] != image_dataRight[i]) return false;

	return true;

}

bool operator ==(const RsMsgItem& miLeft, const RsMsgItem& miRight)
{
	if(miLeft.message != miRight.message) return false;
	if(miLeft.msgFlags != miRight.msgFlags) return false;
	if(miLeft.recvTime != miRight.recvTime) return false;
	if(miLeft.sendTime != miRight.sendTime) return false;
	if(miLeft.subject != miRight.subject) return false;
	if(miLeft.msgId != miRight.msgId) return false;

	if(!(miLeft.attachment == miRight.attachment)) return false;
	if(!(miLeft.msgbcc == miRight.msgbcc)) return false;
	if(!(miLeft.msgcc == miRight.msgcc)) return false;
	if(!(miLeft.msgto == miRight.msgto)) return false;

	return true;
}

bool operator ==(const RsMsgTagType& mttLeft, const RsMsgTagType& mttRight)
{
	if(mttLeft.rgb_color != mttRight.rgb_color) return false;
	if(mttLeft.tagId != mttRight.tagId) return false;
	if(mttLeft.text != mttRight.text) return false;

	return true;
}

bool operator ==(const RsMsgTags& mtLeft, const RsMsgTags& mtRight)
{
	if(mtLeft.msgId != mtRight.msgId) return false;
	if(mtLeft.tagIds != mtRight.tagIds) return false;

	return true;
}

bool operator ==(const RsMsgSrcId& msLeft, const RsMsgSrcId& msRight)
{
	if(msLeft.msgId != msRight.msgId) return false;
	if(msLeft.srcId != msRight.srcId) return false;

	return true;
}

bool operator ==(const RsMsgParentId& msLeft, const RsMsgParentId& msRight)
{
	if(msLeft.msgId != msRight.msgId) return false;
	if(msLeft.msgParentId != msRight.msgParentId) return false;

	return true;
}

int main()
{
	test_RsItem<RsChatMsgItem >(); REPORT("Serialise/Deserialise RsChatMsgItem");
	test_RsItem<RsChatLobbyMsgItem >(); REPORT("Serialise/Deserialise RsChatLobbyMsgItem");
	test_RsItem<RsChatLobbyInviteItem >(); REPORT("Serialise/Deserialise RsChatLobbyInviteItem");
	test_RsItem<RsChatLobbyEventItem >(); REPORT("Serialise/Deserialise RsChatLobbyEventItem");
	test_RsItem<RsChatLobbyListRequestItem >(); REPORT("Serialise/Deserialise RsChatLobbyListRequestItem");
	test_RsItem<RsChatLobbyListItem >(); REPORT("Serialise/Deserialise RsChatLobbyListItem");
	test_RsItem<RsChatStatusItem >(); REPORT("Serialise/Deserialise RsChatStatusItem");
	test_RsItem<RsChatAvatarItem >(); REPORT("Serialise/Deserialise RsChatAvatarItem");
	test_RsItem<RsMsgItem >(); REPORT("Serialise/Deserialise RsMsgItem");
	test_RsItem<RsMsgTagType>(); REPORT("Serialise/Deserialise RsMsgTagType");
	test_RsItem<RsMsgTags>(); REPORT("Serialise/Deserialise RsMsgTags");
	test_RsItem<RsMsgSrcId>(); REPORT("Serialise/Deserialise RsMsgSrcId");
	test_RsItem<RsMsgParentId>(); REPORT("Serialise/Deserialise RsMsgParentId");

	std::cerr << std::endl;

	FINALREPORT("RsMsgItem Tests");

	return TESTRESULT();
}





