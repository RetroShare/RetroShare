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

p3Channels::p3Channels(uint16_t type, CacheStrapper *cs, 
		CacheTransfer *cft, RsFiles *files, 
                std::string srcdir, std::string storedir, std::string chanDir)
	:p3GroupDistrib(type, cs, cft, srcdir, storedir, 
			CONFIG_TYPE_CHANNELS, CHANNEL_STOREPERIOD, CHANNEL_PUBPERIOD), 
	mRsFiles(files), 
	mChannelsDir(chanDir)
{ 
	//loadDummyData();
	
	/* create chanDir */
	if (!RsDirUtil::checkCreateDirectory(mChannelsDir))
	{
		std::cerr << "p3Channels() Failed to create Channels Directory: ";
		std::cerr << mChannelsDir;
		std::cerr << std::endl;
	}

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
	std::cerr << "p3Channels::channelSubscribe() ";
	std::cerr << cId;
	std::cerr << std::endl;

        if (subscribe)
	{
		std::string channeldir = mChannelsDir + "/" + cId;

		/* create chanDir */
		if (!RsDirUtil::checkCreateDirectory(channeldir))
		{
			std::cerr << "p3Channels::channelSubscribe()";
			std::cerr << " Failed to create Channels Directory: ";
			std::cerr << channeldir;
			std::cerr << std::endl;
		}
	}

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

	std::cerr << "p3Channels::locked_eventUpdateGroup() ";
	std::cerr << grpId;
	std::cerr << std::endl;

	if (isNew)
	{
		getPqiNotify()->AddFeedItem(RS_FEED_ITEM_CHAN_NEW, grpId, msgId, nullId);
	}
	else
	{
		getPqiNotify()->AddFeedItem(RS_FEED_ITEM_CHAN_UPDATE, grpId, msgId, nullId);
	}

        if (info->flags & RS_DISTRIB_SUBSCRIBED)
	{
		std::string channeldir = mChannelsDir + "/" + grpId;

		std::cerr << "p3Channels::locked_eventUpdateGroup() ";
		std::cerr << " creating directory: " << channeldir;
		std::cerr << std::endl;

		/* create chanDir */
		if (!RsDirUtil::checkCreateDirectory(channeldir))
		{
			std::cerr << "p3Channels::locked_eventUpdateGroup() ";
			std::cerr << "Failed to create Channels Directory: ";
			std::cerr << channeldir;
			std::cerr << std::endl;
		}
	}


	return true;
}

/* only download in the first week of channel
 * older stuff can be manually downloaded.
 */

const uint32_t DOWNLOAD_PERIOD = 7 * 24 * 3600; 

bool p3Channels::locked_eventDuplicateMsg(GroupInfo *grp, RsDistribMsg *msg, std::string id)
{
	std::string grpId = msg->grpId;
	std::string msgId = msg->msgId;
	std::string nullId;


	std::cerr << "p3Channels::locked_eventDuplicateMsg() ";
	std::cerr << " grpId: " << grpId << " msgId: " << msgId;
	std::cerr << " peerId: " << id;
	std::cerr << std::endl;


	RsChannelMsg *chanMsg = dynamic_cast<RsChannelMsg *>(msg);
	if (!chanMsg)
	{
		return true;
	}

	/* request the files 
	 * NB: This will result in duplicates.
	 * it is upto ftserver/ftcontroller/ftextralist
	 *
	 * download, then add to 
	 *
	 * */

        //bool download = (grp->flags & (RS_DISTRIB_ADMIN | 
	//		RS_DISTRIB_PUBLISH | RS_DISTRIB_SUBSCRIBED))
        bool download = (grp->flags & RS_DISTRIB_SUBSCRIBED);

	/* check subscribed */
	if (!download)
	{
		return true;
	}

	/* check age */
	time_t age = time(NULL) - msg->timestamp;

	if (age > DOWNLOAD_PERIOD)
	{
		return true;
	}

	/* Iterate through files */
	std::list<RsTlvFileItem>::iterator fit;
	for(fit = chanMsg->attachment.items.begin(); 
		fit != chanMsg->attachment.items.end(); fit++)
	{
		std::string fname = fit->name;
		std::string hash  = fit->hash;
		uint64_t size     = fit->filesize;
		std::string channelname = grpId;
		std::string localpath = mChannelsDir + "/" + channelname;
		uint32_t flags = RS_FILE_HINTS_EXTRA;
		std::list<std::string> srcIds;

		srcIds.push_back(id);

		/* download it ... and flag for ExtraList 
		 * don't do pre-search check as FileRequest does it better
		 */

		std::cerr << "p3Channels::locked_eventDuplicateMsg() ";
		std::cerr << " Downloading: " << fname;
		std::cerr << " to: " << localpath;
		std::cerr << " from: " << id;
		std::cerr << std::endl;

		mRsFiles->FileRequest(fname, hash, size, 
					localpath, flags, srcIds);
	}


	return true;
}


bool p3Channels::locked_eventNewMsg(GroupInfo *grp, RsDistribMsg *msg, std::string id)
{
	std::string grpId = msg->grpId;
	std::string msgId = msg->msgId;
	std::string nullId;

	std::cerr << "p3Channels::locked_eventNewMsg() ";
	std::cerr << " grpId: " << grpId;
	std::cerr << " msgId: " << msgId;
	std::cerr << " peerId: " << id;
	std::cerr << std::endl;

	getPqiNotify()->AddFeedItem(RS_FEED_ITEM_CHAN_MSG, grpId, msgId, nullId);

	/* request the files 
	 * NB: This could result in duplicates.
	 * which must be handled by ft side.
	 *
	 * this is exactly what DuplicateMsg does.
	 * */
	return locked_eventDuplicateMsg(grp, msg, id);
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

