/*
 * libretroshare/src/services: p3channels.cc
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2008 by Robert Fernie.
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

#include "services/p3channels.h"
#include "util/rsdir.h"

std::ostream &operator<<(std::ostream &out, const ChannelInfo &info)
{
	std::string name(info.channelName.begin(), info.channelName.end());
	std::string desc(info.channelDesc.begin(), info.channelDesc.end());

	out << "ChannelInfo:";
	out << std::endl;
	out << "ChannelId: " << info.channelId << std::endl;
	out << "ChannelName: " << name << std::endl;
	out << "ChannelDesc: " << desc << std::endl;
	out << "ChannelFlags: " << info.channelFlags << std::endl;
	out << "Pop: " << info.pop << std::endl;
	out << "LastPost: " << info.lastPost << std::endl;

	return out;
}

std::ostream &operator<<(std::ostream &out, const ChannelMsgSummary &info)
{
	out << "ChannelMsgSummary:";
	out << std::endl;
	out << "ChannelId: " << info.channelId << std::endl;

	return out;
}

std::ostream &operator<<(std::ostream &out, const ChannelMsgInfo &info)
{
	out << "ChannelMsgInfo:";
	out << std::endl;
	out << "ChannelId: " << info.channelId << std::endl;

	return out;
}


RsChannels *rsChannels = NULL;


#define CHANNEL_STOREPERIOD 10000
#define CHANNEL_PUBPERIOD   600

p3Channels::p3Channels(uint16_t type, CacheStrapper *cs, CacheTransfer *cft,
	                std::string srcdir, std::string storedir, std::string chanDir)
	:p3GroupDistrib(type, cs, cft, srcdir, storedir, 
			CONFIG_TYPE_CHANNELS, CHANNEL_STOREPERIOD, CHANNEL_PUBPERIOD), 
	mChannelsDir(chanDir)
{ 
	//loadDummyData();
	return; 
}

p3Channels::~p3Channels() 
{ 
	return; 
}

/****************************************/

bool p3Channels::channelsChanged(std::list<std::string> &chanIds)
{
	return groupsChanged(chanIds);
}


bool p3Channels::getChannelInfo(std::string cId, ChannelInfo &ci)
{
	RsStackMutex stack(distribMtx); /***** STACK LOCKED MUTEX *****/

	/* extract details */
	GroupInfo *gi = locked_getGroupInfo(cId);

	if (!gi)
		return false;

	ci.channelId = gi->grpId;
	ci.channelName = gi->grpName;
	ci.channelDesc = gi->grpDesc;

	ci.channelFlags = gi->flags;

	ci.pop = gi->sources.size();
	ci.lastPost = gi->lastPost;

	return true;
}


bool p3Channels::getChannelList(std::list<ChannelInfo> &channelList)
{
	std::list<std::string> grpIds;
	std::list<std::string>::iterator it;

	getAllGroupList(grpIds);

	for(it = grpIds.begin(); it != grpIds.end(); it++)
	{
		ChannelInfo ci;
		if (getChannelInfo(*it, ci))
		{
			channelList.push_back(ci);
		}
	}
	return true;
}


bool p3Channels::getChannelMsgList(std::string cId, std::list<ChannelMsgSummary> &msgs)
{
	std::list<std::string> msgIds;
	std::list<std::string>::iterator it;

	getAllMsgList(cId, msgIds);

	RsStackMutex stack(distribMtx); /***** STACK LOCKED MUTEX *****/
	for(it = msgIds.begin(); it != msgIds.end(); it++)
	{
		/* get details */
		RsDistribMsg *msg = locked_getGroupMsg(cId, *it);
		RsChannelMsg *cmsg = dynamic_cast<RsChannelMsg *>(msg);
		if (!cmsg)
			continue;

		ChannelMsgSummary tis;

		tis.channelId = msg->grpId;
		tis.msgId = msg->msgId;
		tis.ts = msg->timestamp;

		/* the rest must be gotten from the derived Msg */
		
		tis.subject = cmsg->subject;
		tis.msg  = cmsg->message;
		tis.count = cmsg->attachment.items.size();

		msgs.push_back(tis);
	}
	return true;
}

bool p3Channels::getChannelMessage(std::string fId, std::string mId, ChannelMsgInfo &info)
{
	std::list<std::string> msgIds;
	std::list<std::string>::iterator it;

	RsStackMutex stack(distribMtx); /***** STACK LOCKED MUTEX *****/

	RsDistribMsg *msg = locked_getGroupMsg(fId, mId);
	RsChannelMsg *cmsg = dynamic_cast<RsChannelMsg *>(msg);
	if (!cmsg)
		return false;


	info.channelId = msg->grpId;
	info.msgId = msg->msgId;
	info.ts = msg->timestamp;

	/* the rest must be gotten from the derived Msg */
		
	info.subject = cmsg->subject;
	info.msg  = cmsg->message;


	std::list<RsTlvFileItem>::iterator fit;
	for(fit = cmsg->attachment.items.begin(); 
			fit != cmsg->attachment.items.end(); fit++)
	{
		FileInfo fi;
	        fi.fname = RsDirUtil::getTopDir(fit->name);
		fi.size  = fit->filesize;
		fi.hash  = fit->hash;
		fi.path  = fit->path;

		info.files.push_back(fi);
		info.count++;
		info.size += fi.size;
	}

	return true;
}

bool p3Channels::ChannelMessageSend(ChannelMsgInfo &info)
{

	RsChannelMsg *cmsg = new RsChannelMsg();
	cmsg->grpId = info.channelId;

	cmsg->subject   = info.subject;
	cmsg->message   = info.msg;
	cmsg->timestamp = time(NULL);

	std::list<FileInfo>::iterator it;
	for(it = info.files.begin(); it != info.files.end(); it++)
	{
		RsTlvFileItem mfi;
		mfi.hash = it -> hash;
		mfi.name = it -> fname;
		mfi.filesize = it -> size;
		cmsg -> attachment.items.push_back(mfi);
	}


	std::string msgId = publishMsg(cmsg, true);

	return true;
}


std::string p3Channels::createChannel(std::wstring channelName, std::wstring channelDesc, uint32_t channelFlags)
{
        std::string id = createGroup(channelName, channelDesc, channelFlags);

	return id;
}

RsSerialType *p3Channels::createSerialiser()
{
        return new RsChannelSerialiser();
}

bool    p3Channels::locked_checkDistribMsg(RsDistribMsg *msg)
{
	return true;
}


RsDistribGrp *p3Channels::locked_createPublicDistribGrp(GroupInfo &info)
{
	RsDistribGrp *grp = NULL; //new RsChannelGrp();

	return grp;
}

RsDistribGrp *p3Channels::locked_createPrivateDistribGrp(GroupInfo &info)
{
	RsDistribGrp *grp = NULL; //new RsChannelGrp();

	return grp;
}


bool p3Channels::channelSubscribe(std::string cId, bool subscribe)
{
	return subscribeToGroup(cId, subscribe);
}


/***************************************************************************************/
/****************** Event Feedback (Overloaded form p3distrib) *************************/
/***************************************************************************************/

#include "pqi/pqinotify.h"

bool p3Channels::locked_eventUpdateGroup(GroupInfo  *info, bool isNew)
{
	std::string grpId = info->grpId;
	std::string msgId;
	std::string nullId;

	if (isNew)
	{
		getPqiNotify()->AddFeedItem(RS_FEED_ITEM_CHAN_NEW, grpId, msgId, nullId);
	}
	else
	{
		getPqiNotify()->AddFeedItem(RS_FEED_ITEM_CHAN_UPDATE, grpId, msgId, nullId);
	}

	return true;
}

bool p3Channels::locked_eventNewMsg(RsDistribMsg *msg)
{
	std::string grpId = msg->grpId;
	std::string msgId = msg->msgId;
	std::string nullId;

	getPqiNotify()->AddFeedItem(RS_FEED_ITEM_CHAN_MSG, grpId, msgId, nullId);
	return true;
}



/****************************************/

#if 0

void    p3Channels::loadDummyData()
{
	ChannelInfo fi;
	std::string channelId;
	std::string msgId;
	time_t now = time(NULL);

	fi.channelId = "FID1234";
	fi.channelName = L"Channel 1";
	fi.channelDesc = L"Channel 1";
	fi.channelFlags = RS_DISTRIB_ADMIN;
	fi.pop = 2;
	fi.lastPost = now - 123;

	channelId = createChannel(fi.channelName, fi.channelDesc, fi.channelFlags);

	fi.channelId = "FID2345";
	fi.channelName = L"Channel 2";
	fi.channelDesc = L"Channel 2";
	fi.channelFlags = RS_DISTRIB_SUBSCRIBED;
	fi.pop = 3;
	fi.lastPost = now - 1234;

	channelId = createChannel(fi.channelName, fi.channelDesc, fi.channelFlags);
	msgId = createChannelMsg(channelId, "", L"WELCOME TO Channel1", L"Hello!");
	msgId = createChannelMsg(channelId, msgId, L"Love this channel", L"Hello2!");

	return; 

	/* ignore this */

	fi.channelId = "FID3456";
	fi.channelName = L"Channel 3";
	fi.channelDesc = L"Channel 3";
	fi.channelFlags = 0;
	fi.pop = 3;
	fi.lastPost = now - 1234;

	channelId = createChannel(fi.channelName, fi.channelDesc, fi.channelFlags);

	fi.channelId = "FID4567";
	fi.channelName = L"Channel 4";
	fi.channelDesc = L"Channel 4";
	fi.channelFlags = 0;
	fi.pop = 5;
	fi.lastPost = now - 1234;

	channelId = createChannel(fi.channelName, fi.channelDesc, fi.channelFlags);

	fi.channelId = "FID5678";
	fi.channelName = L"Channel 5";
	fi.channelDesc = L"Channel 5";
	fi.channelFlags = 0;
	fi.pop = 1;
	fi.lastPost = now - 1234;

	channelId = createChannel(fi.channelName, fi.channelDesc, fi.channelFlags);

	fi.channelId = "FID6789";
	fi.channelName = L"Channel 6";
	fi.channelDesc = L"Channel 6";
	fi.channelFlags = 0;
	fi.pop = 2;
	fi.lastPost = now - 1234;

	channelId = createChannel(fi.channelName, fi.channelDesc, fi.channelFlags);

	fi.channelId = "FID7890";
	fi.channelName = L"Channel 7";
	fi.channelDesc = L"Channel 7";
	fi.channelFlags = 0;
	fi.pop = 4;
	fi.lastPost = now - 1234;

	channelId = createChannel(fi.channelName, fi.channelDesc, fi.channelFlags);

	fi.channelId = "FID8901";
	fi.channelName = L"Channel 8";
	fi.channelDesc = L"Channel 8";
	fi.channelFlags = 0;
	fi.pop = 3;
	fi.lastPost = now - 1234;

	channelId = createChannel(fi.channelName, fi.channelDesc, fi.channelFlags);

	fi.channelId = "FID9012";
	fi.channelName = L"Channel 9";
	fi.channelDesc = L"Channel 9";
	fi.channelFlags = 0;
	fi.pop = 2;
	fi.lastPost = now - 1234;

	channelId = createChannel(fi.channelName, fi.channelDesc, fi.channelFlags);

	fi.channelId = "FID9123";
	fi.channelName = L"Channel 10";
	fi.channelDesc = L"Channel 10";
	fi.channelFlags = 0;
	fi.pop = 1;
	fi.lastPost = now - 1234;

	channelId = createChannel(fi.channelName, fi.channelDesc, fi.channelFlags);

	mChannelsChanged = true;
}

#endif

