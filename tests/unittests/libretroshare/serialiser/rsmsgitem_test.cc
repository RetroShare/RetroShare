/*******************************************************************************
 * unittests/libretroshare/serialiser/msgitem_test.cc                          *
 *                                                                             *
 * Copyright 2010 by Christopher Evi-Parker <retroshare.project@gmail.com>     *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 ******************************************************************************/

#include <iostream>

#include "util/rsrandom.h"
#include "rsitems/rsmsgitems.h"
#include "chat/rschatitems.h"

#include "support.h"
#include "rstlvutil.h"


void init_item(RsChatMsgItem& cmi)
{
	cmi.chatFlags = rand()%34;
	cmi.sendTime = rand()%422224;
	randString(LARGE_STR, cmi.message);
}
void init_item(RsChatLobbyListRequestItem& )
{
}
void init_item(RsChatLobbyListItem& cmi)
{
	int n = rand()%20 ;

	for(int i=0;i<n;++i)
	{
        struct VisibleChatLobbyInfo info;
        info.id = RSRandom::random_u64() ;
        randString(5+(rand()%10), info.name);
        randString(20+(rand()%15), info.topic);
        info.count = RSRandom::random_u32() ;
        info.flags = ChatLobbyFlags(RSRandom::random_u32()%3) ;
        cmi.lobbies.push_back(info);
	}
}
void init_item(RsChatLobbyMsgItem& cmi)
{
	init_item( *dynamic_cast<RsChatMsgItem*>(&cmi)) ;

	cmi.msg_id = RSRandom::random_u64() ;
	cmi.lobby_id = RSRandom::random_u64() ;
	cmi.nick = "My nickname" ;
	cmi.parent_msg_id = RSRandom::random_u64() ;
}
void init_item(RsChatLobbyEventItem& cmi)
{
	cmi.lobby_id = RSRandom::random_u64() ;
	cmi.msg_id = RSRandom::random_u64() ;
	randString(20, cmi.nick);
	cmi.event_type = RSRandom::random_u32()%256 ;
	randString(20, cmi.string1);
}

void init_item(RsChatLobbyInviteItem& cmi)
{
	cmi.lobby_id = RSRandom::random_u64() ;
	cmi.lobby_name = "Name of the lobby" ;
	cmi.lobby_topic = "Topic of the lobby" ;
}

void init_item(RsPrivateChatMsgConfigItem& pcmi)
{
    pcmi.configPeerId = RsPeerId::random();
	pcmi.chatFlags = rand()%34;
	pcmi.configFlags = rand()%21;
	pcmi.sendTime = rand()%422224;
	randString(LARGE_STR, pcmi.message);
	pcmi.recvTime = rand()%344443;
}

void init_item(RsChatStatusItem& csi)
{

	randString(SHORT_STR, csi.status_string);
	csi.flags = rand()%232;

}

void init_item(RsChatAvatarItem& cai)
{
	std::string image_data;
	randString(LARGE_STR, image_data);
	cai.image_data = (unsigned char*)malloc(image_data.size());

	memcpy(cai.image_data, image_data.c_str(), image_data.size());
	cai.image_size = image_data.size();
}

void init_item(RsMsgItem& mi)
{
	init_item(mi.attachment);
	init_item(mi.rspeerid_msgbcc);
	init_item(mi.rspeerid_msgcc);
	init_item(mi.rspeerid_msgto);

	randString(LARGE_STR, mi.message);
	randString(SHORT_STR, mi.subject);

	mi.msgId = rand()%324232;
	mi.recvTime = rand()%44252;
	mi.sendTime = mi.recvTime;
	mi.msgFlags = mi.recvTime;
}

void init_item(RsMsgTagType& mtt)
{
	mtt.rgb_color = rand()%5353;
	mtt.tagId = rand()%24242;
	randString(SHORT_STR, mtt.text);
}


void init_item(RsMsgTags& mt)
{
	mt.msgId = rand()%3334;

	int i;
	for (i = 0; i < 10; i++) {
		mt.tagIds.push_back(rand()%21341);
	}
}

void init_item(RsMsgSrcId& ms)
{
	ms.msgId = rand()%434;
    ms.srcId = RsPeerId::random();
}

void init_item(RsMsgParentId& ms)
{
	ms.msgId = rand()%354;
	ms.msgParentId = rand()%476;
}

bool operator ==(const struct VisibleChatLobbyInfo& l, const struct VisibleChatLobbyInfo& r)
{
    return l.id == r.id
            && l.name == r.name
            && l.topic == r.topic
            && l.count == r.count
            && l.flags == r.flags;
}

bool operator ==(const RsChatLobbyListItem& cmiLeft,const  RsChatLobbyListItem& cmiRight)
{
    return cmiLeft.lobbies == cmiRight.lobbies;
}
bool operator ==(const RsChatLobbyListRequestItem& ,const  RsChatLobbyListRequestItem& )
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
	if(csiLeft.lobby_topic != csiRight.lobby_topic) return false ;

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
	//if(miLeft.msgId != miRight.msgId) return false;

	if(!(miLeft.attachment == miRight.attachment)) return false;
	if(!(miLeft.rspeerid_msgbcc == miRight.rspeerid_msgbcc)) return false;
	if(!(miLeft.rspeerid_msgcc == miRight.rspeerid_msgcc)) return false;
	if(!(miLeft.rspeerid_msgto == miRight.rspeerid_msgto)) return false;

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

TEST(libretroshare_serialiser, RsMsgItem)
{
	test_RsItem<RsChatMsgItem              ,RsChatSerialiser>();
	test_RsItem<RsChatLobbyMsgItem         ,RsChatSerialiser>();
	test_RsItem<RsChatLobbyInviteItem      ,RsChatSerialiser>();
	test_RsItem<RsChatLobbyEventItem       ,RsChatSerialiser>();
	test_RsItem<RsChatLobbyListRequestItem ,RsChatSerialiser>();
	test_RsItem<RsChatLobbyListItem        ,RsChatSerialiser>();
	test_RsItem<RsChatStatusItem           ,RsChatSerialiser>();
	test_RsItem<RsChatAvatarItem           ,RsChatSerialiser>();
	test_RsItem<RsMsgItem                  ,RsMsgSerialiser>();
	test_RsItem<RsMsgTagType               ,RsMsgSerialiser>();
	test_RsItem<RsMsgTags                  ,RsMsgSerialiser>();
	test_RsItem<RsMsgSrcId                 ,RsMsgSerialiser>();
	test_RsItem<RsMsgParentId              ,RsMsgSerialiser>();
}





