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
#include "retroshare/rsiface.h"

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
        out << "ChannelMsgSummary: ChannelId: " << info.channelId << std::endl;

	return out;
}

std::ostream &operator<<(std::ostream &out, const ChannelMsgInfo &info)
{
        out << "ChannelMsgInfo: ChannelId: " << info.channelId << std::endl;

	return out;
}


RsChannels *rsChannels = NULL;


/* remember 2^16 = 64K max units in store period.
 * PUBPERIOD * 2^16 = max STORE PERIOD */
#define CHANNEL_STOREPERIOD (30*24*3600)    /*  30 * 24 * 3600 - secs in a 30 day month */
#define CHANNEL_PUBPERIOD   120            /* 2 minutes ... (max = 455 days) */
#define MAX_AUTO_DL 1E9 /* auto download of attachment limit; 1 GIG */

p3Channels::p3Channels(uint16_t type, CacheStrapper *cs, 
		CacheTransfer *cft, RsFiles *files, 
                std::string srcdir, std::string storedir, std::string chanDir)
	:p3GroupDistrib(type, cs, cft, srcdir, storedir, chanDir,
                        CONFIG_TYPE_CHANNELS, CHANNEL_STOREPERIOD, CHANNEL_PUBPERIOD),
	mRsFiles(files), 
	mChannelsDir(chanDir)
{ 

	/* create chanDir */
        if (!RsDirUtil::checkCreateDirectory(mChannelsDir)) {
                std::cerr << "p3Channels() Failed to create Channels Directory: " << mChannelsDir << std::endl;
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


bool p3Channels::getChannelInfo(const std::string &cId, ChannelInfo &ci)
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

	ci.pngChanImage = gi->grpIcon.pngImageData;

	if(ci.pngChanImage != NULL)
		ci.pngImageLen = gi->grpIcon.imageSize;
	else
		ci.pngImageLen = 0;

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


bool p3Channels::getChannelMsgList(const std::string &cId, std::list<ChannelMsgSummary> &msgs)
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

bool p3Channels::getChannelMessage(const std::string &cId, const std::string &mId, ChannelMsgInfo &info)
{
	std::list<std::string> msgIds;
	std::list<std::string>::iterator it;

	processCacheOptReq(cId);

	RsStackMutex stack(distribMtx); /***** STACK LOCKED MUTEX *****/

	RsDistribMsg *msg = locked_getGroupMsg(cId, mId);
	RsChannelMsg *cmsg = dynamic_cast<RsChannelMsg *>(msg);
	if (!cmsg)
		return false;


	info.channelId = msg->grpId;
	info.msgId = msg->msgId;
	info.ts = msg->timestamp;

	/* the rest must be gotten from the derived Msg */
		
	info.subject = cmsg->subject;
	info.msg  = cmsg->message;
        info.count = 0;
        info.size = 0;

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

	if((cmsg->thumbnail.binData.bin_data != NULL) && (cmsg->thumbnail.image_type == RSTLV_IMAGE_TYPE_PNG))
	{
		info.thumbnail.image_thumbnail =
				(unsigned char*) cmsg->thumbnail.binData.bin_data;

		info.thumbnail.im_thumbnail_size =
				cmsg->thumbnail.binData.bin_len;
	}

	return true;
}

bool p3Channels::channelRestoreKeys(const std::string &chId){

	return p3GroupDistrib::restoreGrpKeys(chId);
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

	// explicit member wise copy for grp image
	if((info.thumbnail.image_thumbnail != NULL) &&
			(info.thumbnail.im_thumbnail_size > 0)){

		cmsg->thumbnail.binData.bin_data =
				new unsigned char[info.thumbnail.im_thumbnail_size];

		memcpy(cmsg->thumbnail.binData.bin_data, info.thumbnail.image_thumbnail,
				info.thumbnail.im_thumbnail_size*sizeof(unsigned char));
		cmsg->thumbnail.binData.bin_len = info.thumbnail.im_thumbnail_size;
		cmsg->thumbnail.image_type = RSTLV_IMAGE_TYPE_PNG;

	}else{

		cmsg->thumbnail.binData.bin_data = NULL;
		cmsg->thumbnail.binData.bin_len = 0;
		cmsg->thumbnail.image_type = 0;
	}

	bool toSign = false; 
	// Channels are not personally signed yet... (certainly not by default!).
	// This functionality can easily be added once its available in the gui + API.
	// should check the GroupFlags for default.

	std::string msgId = publishMsg(cmsg, toSign);

	if (msgId.empty()) {
		return false;
	}

	return setMessageStatus(info.channelId, msgId, CHANNEL_MSG_STATUS_READ, CHANNEL_MSG_STATUS_MASK);
}

bool p3Channels::setMessageStatus(const std::string& cId,const std::string& mId,const uint32_t status, const uint32_t statusMask)
{
	bool changed = false;
	uint32_t newStatus = 0;

	{
		RsStackMutex stack(distribMtx); /***** STACK LOCKED MUTEX *****/

		std::list<RsChannelReadStatus *>::iterator lit = mReadStatus.begin();

		for(; lit != mReadStatus.end(); lit++)
		{

			if((*lit)->channelId == cId)
			{
					RsChannelReadStatus* rsi = *lit;
					uint32_t oldStatus = rsi->msgReadStatus[mId];
					rsi->msgReadStatus[mId] &= ~statusMask;
					rsi->msgReadStatus[mId] |= (status & statusMask);
			
					newStatus = rsi->msgReadStatus[mId];
					if (oldStatus != newStatus) {
						changed = true;
					}
					break;
			}

		}

		// if channel id does not exist create one
		if(lit == mReadStatus.end())
		{
			RsChannelReadStatus* rsi = new RsChannelReadStatus();
			rsi->channelId = cId;
			rsi->msgReadStatus[mId] = status & statusMask;
			mReadStatus.push_back(rsi);
			saveList.push_back(rsi);

			newStatus = rsi->msgReadStatus[mId];
			changed = true;
		}

		if (changed) {
			IndicateConfigChanged();
		}
	} /******* UNLOCKED ********/

	if (changed) {
		rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_CHANNELLIST_LOCKED, NOTIFY_TYPE_MOD);
		rsicontrol->getNotify().notifyChannelMsgReadSatusChanged(cId, mId, newStatus);
	}

	return true;
}

bool p3Channels::getMessageStatus(const std::string& cId, const std::string& mId, uint32_t& status)
{

	status = 0;

	RsStackMutex stack(distribMtx);

	std::list<RsChannelReadStatus *>::iterator lit = mReadStatus.begin();

	for(; lit != mReadStatus.end(); lit++)
	{

		if((*lit)->channelId == cId)
		{
				break;
		}

	}

	if(lit == mReadStatus.end())
	{
		return false;
	}

	std::map<std::string, uint32_t >::iterator mit = (*lit)->msgReadStatus.find(mId);

	if(mit != (*lit)->msgReadStatus.end())
	{
		status = mit->second;
		return true;
	}

	return false;
}

bool p3Channels::getMessageCount(const std::string &cId, unsigned int &newCount, unsigned int &unreadCount)
{
	newCount = 0;
	unreadCount = 0;

	std::list<std::string> grpIds;

	if (cId.empty()) {
		// count all messages of all subscribed channels
		getAllGroupList(grpIds);
	} else {
		// count all messages of one channels
		grpIds.push_back(cId);
	}

	std::list<std::string>::iterator git;
	for (git = grpIds.begin(); git != grpIds.end(); git++) {
		std::string cId = *git;
		uint32_t grpFlags;

		{
			// only flag is needed
			RsStackMutex stack(distribMtx); /***** STACK LOCKED MUTEX *****/
			GroupInfo *gi = locked_getGroupInfo(cId);
			if (gi == NULL) {
				return false;
			}
			grpFlags = gi->flags;
		} /******* UNLOCKED ********/

		if (grpFlags & (RS_DISTRIB_ADMIN | RS_DISTRIB_SUBSCRIBED)) {
			std::list<std::string> msgIds;
			if (getAllMsgList(cId, msgIds)) {
				std::list<std::string>::iterator mit;

				RsStackMutex stack(distribMtx); /***** STACK LOCKED MUTEX *****/

				std::list<RsChannelReadStatus *>::iterator lit;
				for(lit = mReadStatus.begin(); lit != mReadStatus.end(); lit++) {
					if ((*lit)->channelId == cId) {
						break;
					}
				}

				if (lit == mReadStatus.end()) {
					// no status available -> all messages are new
					newCount += msgIds.size();
					unreadCount += msgIds.size();
					continue;
				}

				for (mit = msgIds.begin(); mit != msgIds.end(); mit++) {
					std::map<std::string, uint32_t >::iterator rit = (*lit)->msgReadStatus.find(*mit);

					if (rit == (*lit)->msgReadStatus.end()) {
						// no status available -> message is new
						newCount++;
						unreadCount++;
						continue;
					}

					if (rit->second & CHANNEL_MSG_STATUS_READ) {
						// message is not new
						if (rit->second & CHANNEL_MSG_STATUS_UNREAD_BY_USER) {
							// message is unread
							unreadCount++;
						}
					} else {
						newCount++;
						unreadCount++;
					}
				}
			} /******* UNLOCKED ********/
		}
	}

	return true;
}

bool p3Channels::channelExtraFileHash(const std::string &path, const std::string & /*chId*/, FileInfo& fInfo){

	// get file name
	std::string fname, fnameBuff;
	std::string::const_reverse_iterator rit;

	for(rit = path.rbegin(); *rit != '/' ; rit++){
		fnameBuff.push_back(*rit);
	}

	// reverse string buff for correct file name
	fname.append(fnameBuff.rbegin(), fnameBuff.rend());

	TransferRequestFlags flags = RS_FILE_REQ_ANONYMOUS_ROUTING;

	// then hash file and get file info too

	if(!mRsFiles->ExtraFileHash(path, CHANNEL_STOREPERIOD, flags))
		return false;

	fInfo.path = path;
	fInfo.fname = fname;

	return true;
}


bool p3Channels::channelExtraFileRemove(const std::string &hash, const std::string &chId)
{
	TransferRequestFlags tflags = RS_FILE_REQ_ANONYMOUS_ROUTING | RS_FILE_REQ_EXTRA;
	FileSearchFlags sflags = RS_FILE_HINTS_NETWORK_WIDE | RS_FILE_HINTS_EXTRA;

	/* remove copy from channels directory */

	FileInfo fInfo;
	mRsFiles->FileDetails(hash, sflags, fInfo);
	std::string chPath = mChannelsDir + "/" + chId + "/" + fInfo.fname;

	if(remove(chPath.c_str()) == 0){
		std::cerr << "p3Channel::channelExtraFileRemove() Removed file :"
				  << chPath.c_str() << std::endl;

	}else{
		std::cerr << "p3Channel::channelExtraFileRemove() Failed to remove file :"
				  << chPath.c_str() << std::endl;
	}

	return mRsFiles->ExtraFileRemove(hash, tflags);
}


std::string p3Channels::createChannel(std::wstring channelName, std::wstring channelDesc, uint32_t channelFlags,
		unsigned char* pngImageData, uint32_t imageSize)
{

        std::string grpId = createGroup(channelName, channelDesc, channelFlags, pngImageData, imageSize);

        // create channel directory
        std::string channelDir = mChannelsDir + "/" + grpId;

        if(RsDirUtil::checkCreateDirectory(channelDir))
            std::cerr << "p3Channels::createChannel(): Failed to create channel directory "
                      << channelDir << std::endl;

        return grpId;
}

RsSerialType *p3Channels::createSerialiser()
{
        return new RsChannelSerialiser();
}

bool    p3Channels::locked_checkDistribMsg(RsDistribMsg */*msg*/)
{
	return true;
}




bool p3Channels::channelSubscribe(const std::string &cId, bool subscribe, bool autoDl)
{
#ifdef CHANNEL_DEBUG
        std::cerr << "p3Channels::channelSubscribe() " << cId << std::endl;
#endif

    {
        RsStackMutex stack(distribMtx);

        if(subscribe)
        	mChannelStatus[cId] |= autoDl ?
           		(RS_CHAN_STATUS_AUTO_DL & RS_CHAN_STATUS_MASK) : ~RS_CHAN_STATUS_MASK;
    }

    bool ok = subscribeToGroup(cId, subscribe);


    // if subscribing set channel status bit field on whether
    // or not to auto download
    {
        RsStackMutex stack(distribMtx);

        if(!ok || !subscribe){
        	mChannelStatus.erase(cId);
        	removeChannelReadStatusEntry(cId);
        }
        else{
        	addChannelReadStatusEntry(cId);
        }

        IndicateConfigChanged();
    }
	return ok;
}

void p3Channels::addChannelReadStatusEntry(const std::string& cId)
{
	std::list<RsChannelReadStatus*>::iterator lit = mReadStatus.begin();
	RsChannelReadStatus* rds = NULL;

	// check to ensure an entry does not exist
	for(; lit != mReadStatus.end(); lit++){

		if((*lit)->channelId == cId)
		{
			break;
		}
	}

	if(lit == mReadStatus.end()){
		rds = new RsChannelReadStatus();
		rds->channelId = cId;
		mReadStatus.push_back(rds);
		saveList.push_back(rds);
	}

	return;
}

//TODO: delete unsubscribed channels from entry
void p3Channels::removeChannelReadStatusEntry(const std::string& cId)
{
	std::list<RsChannelReadStatus*>::iterator lit = mReadStatus.begin();
	statMap::iterator mit;
	// check to ensure an entry does not exist
	for(; lit != mReadStatus.end(); lit++){

		if((*lit)->channelId == cId)
		{
			if((mit = (*lit)->msgReadStatus.find(cId)) !=
					(*lit)->msgReadStatus.end()){
				(*lit)->msgReadStatus.erase(mit);
				break;
			}
		}
	}

}
bool p3Channels::channelShareKeys(const std::string &chId, std::list<std::string>& peers){

#ifdef CHANNEL_DEBUG
	std::cerr << "p3Channels::channelShareKeys() " << chId << std::endl;
#endif

	return sharePubKey(chId, peers);

}

bool p3Channels::channelEditInfo(const std::string &chId, ChannelInfo& info){

#ifdef CHANNEL_DEBUG
    std::cerr << "p3Channels::channelUdateInfo() " << chId << std::endl;
#endif

    GroupInfo gi;

    RsStackMutex stack(distribMtx);

    gi.grpName = info.channelName;
    gi.grpDesc = info.channelDesc;


    if((info.pngChanImage != NULL) && (info.pngImageLen != 0)){
        gi.grpIcon.imageSize = info.pngImageLen;
        gi.grpIcon.pngImageData = info.pngChanImage;
    }
    else{
        gi.grpIcon.imageSize = 0;
        gi.grpIcon.pngImageData = NULL;
    }

    return locked_editGroup(chId, gi);

}


void p3Channels::getPubKeysAvailableGrpIds(std::list<std::string>& grpIds)
{

	getGrpListPubKeyAvailable(grpIds);
	return;

}

bool p3Channels::channelSetAutoDl(const std::string& chId, bool autoDl)
{

	RsStackMutex stack(distribMtx);

	statMap::iterator it = mChannelStatus.find(chId);
	bool changed = false;

	if(it != mChannelStatus.end()){

		if(autoDl)
			it->second |= RS_CHAN_STATUS_AUTO_DL;
		else
			it->second &= ~(RS_CHAN_STATUS_AUTO_DL & RS_CHAN_STATUS_MASK);

		changed = true;
	}
	else
	{
#ifdef CHANNEL_DEBUG
		std::cerr << "p3Channels::channelSetAutoDl(): " << "Channel does not exist"
				  << std::endl;
#endif
		return false;
	}

	// save configuration
	if(changed)
		IndicateConfigChanged();

	return true;
}


bool p3Channels::channelGetAutoDl(const std::string& chId, bool& autoDl)
{
	RsStackMutex stack(distribMtx);

	statMap::iterator it = mChannelStatus.find(chId);

	if(it != mChannelStatus.end())
	{
		autoDl = it->second & RS_CHAN_STATUS_AUTO_DL;
		return true;
	}

	return false;
}

/***************************************************************************************/
/****************** Event Feedback (Overloaded form p3distrib) *************************/
/***************************************************************************************/
/* only download in the first week of channel
 * older stuff can be manually downloaded.
 */

const uint32_t DOWNLOAD_PERIOD = 7 * 24 * 3600; 

/* This is called when we receive a msg, and also recalled
 * on a subscription to a channel..
 */

bool p3Channels::locked_eventDuplicateMsg(GroupInfo *grp, RsDistribMsg *msg, const std::string& id, bool /*historical*/)
{
	std::string grpId = msg->grpId;
	std::string msgId = msg->msgId;
	std::string nullId;


	RsChannelMsg *chanMsg = dynamic_cast<RsChannelMsg *>(msg);
	if (!chanMsg)
	{
		return true;
	}

	// return if msg has d download already

	statMap MsgMap;
	statMap::iterator mit1, mit2;
	std::list<RsChannelReadStatus*>::iterator lit = mReadStatus.begin();

	for(;lit != mReadStatus.end();lit++){

		if((*lit)->channelId == grpId)
			break;
	}

	mit2 = mChannelStatus.find(grpId);

	if(mit2 != mChannelStatus.end())
	{
		if(!(mit2->second & RS_CHAN_STATUS_AUTO_DL)){

			if(lit != mReadStatus.end()){

				if(( mit1=(*lit)->msgReadStatus.find(msgId)) != (*lit)->msgReadStatus.end())
					(*lit)->msgReadStatus[msgId] |= (CHANNEL_MSG_STATUS_MASK & CHANNEL_MSG_STATUS_DOWLOADED);

			}
			return false;
		}
	}

	if(lit != mReadStatus.end()){

		if(( mit1=(*lit)->msgReadStatus.find(msgId)) != (*lit)->msgReadStatus.end()){
			if(mit1->second & CHANNEL_MSG_STATUS_DOWLOADED)
				return false;
		}else{
			// create an entry for msg id
			(*lit)->msgReadStatus[msgId] = ~CHANNEL_MSG_STATUS_MASK;
		}
	}



	/* request the files 
	 * NB: This will result in duplicates.
	 * it is upto ftserver/ftcontroller/ftextralist to handle this!
	 * */

	bool download = (grp->flags & RS_DISTRIB_SUBSCRIBED);

 	if (id == mOwnId)
	{
		download = false;
#ifdef CHANNEL_DEBUG
               	std::cerr << "p3Channels::locked_eventDuplicateMsg() msg from self - not downloading";
               	std::cerr << std::endl;
#endif
	}

	/* check subscribed */
	if (!download)
	{
		return true;
	}

	/* check age */
	time_t age = time(NULL) - msg->timestamp;

	if (age > (time_t)DOWNLOAD_PERIOD )
	{
		return true;
	}

	// get channel info to determine if channel is private or not
	ChannelInfo cInfo;
	bool chanPrivate = false;

	// tho if empty don't bother
	if(!chanMsg->attachment.items.empty()){


                if(grp->flags & RS_DISTRIB_PRIVATE)
                	chanPrivate = true;
	}

	/* Iterate through files */
	std::list<RsTlvFileItem>::iterator fit;
	for(fit = chanMsg->attachment.items.begin(); fit != chanMsg->attachment.items.end(); fit++)
	{
		std::string fname = fit->name;
		std::string hash  = fit->hash;
		uint64_t size     = fit->filesize;
		std::string channelname = grpId;

		std::string localpath;
		TransferRequestFlags flags;

		// send to download directory if file is private
		// We also add explicit sources only if the channel is private. Otherwise we DL in network wide mode
		// using anonymous tunnels.
		//
		std::list<std::string> srcIds;

		if(chanPrivate)
		{
			localpath = mChannelsDir;
			flags = RS_FILE_REQ_BACKGROUND | RS_FILE_REQ_EXTRA;

			srcIds.push_back(id);
		}
		else
		{
			localpath = ""; // forces dl to default directory
			flags = RS_FILE_REQ_BACKGROUND | RS_FILE_REQ_ANONYMOUS_ROUTING;
		}

		/* download it ... and flag for ExtraList 
		 * don't do pre-search check as FileRequest does it better
		 *
		 * FileRequest will ignore request if file is already indexed.
		 */

#ifdef CHANNEL_DEBUG
                std::cerr << "p3Channels::locked_eventDuplicateMsg() " << " Downloading: " << fname;
                std::cerr << " to: " << localpath << " from: " << id << std::endl;
#endif

        if(size < MAX_AUTO_DL)
        	mRsFiles->FileRequest(fname, hash, size, localpath, flags, srcIds);
	}

	if(lit != mReadStatus.end()){

		(*lit)->msgReadStatus[msgId] |= (CHANNEL_MSG_STATUS_MASK &
					CHANNEL_MSG_STATUS_DOWLOADED);

	}

	IndicateConfigChanged();

	return true;
}

#include "pqi/pqinotify.h"

bool p3Channels::locked_eventNewMsg(GroupInfo *grp, RsDistribMsg *msg, const std::string& id, bool historical)
{
	std::string grpId = msg->grpId;
	std::string msgId = msg->msgId;
	std::string nullId;

	if (!historical)
	{
		getPqiNotify()->AddFeedItem(RS_FEED_ITEM_CHAN_MSG, grpId, msgId, nullId);
	}
	
	/* request the files 
	 * NB: This could result in duplicates.
	 * which must be handled by ft side.
	 *
	 * this is exactly what DuplicateMsg does.
	 * */
	return locked_eventDuplicateMsg(grp, msg, id, historical);
}

void p3Channels::locked_notifyGroupChanged(GroupInfo &grp, uint32_t flags, bool historical)
{
	std::string grpId = grp.grpId;
	std::string msgId;
	std::string nullId;


	switch(flags)
	{
		case GRP_NEW_UPDATE:
                        #ifdef CHANNEL_DEBUG
                        std::cerr << "p3Channels::locked_notifyGroupChanged() NEW UPDATE" << std::endl;
                        #endif
			if (!historical)
			{
				getPqiNotify()->AddFeedItem(RS_FEED_ITEM_CHAN_NEW, grpId, msgId, nullId);
			}
                        rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_CHANNELLIST_LOCKED, NOTIFY_TYPE_ADD);
			break;
		case GRP_UPDATE:
                        #ifdef CHANNEL_DEBUG
                        std::cerr << "p3Channels::locked_notifyGroupChanged() UPDATE" << std::endl;
                        #endif
			if (!historical)
			{
				getPqiNotify()->AddFeedItem(RS_FEED_ITEM_CHAN_UPDATE, grpId, msgId, nullId);
			}
                        rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_CHANNELLIST_LOCKED, NOTIFY_TYPE_MOD);
			break;
		case GRP_LOAD_KEY:
                        #ifdef CHANNEL_DEBUG
                        std::cerr << "p3Channels::locked_notifyGroupChanged() LOAD_KEY" << std::endl;
                        #endif
			break;
		case GRP_NEW_MSG:
                        #ifdef CHANNEL_DEBUG
                        std::cerr << "p3Channels::locked_notifyGroupChanged() NEW MSG" << std::endl;
                        #endif
                        rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_CHANNELLIST_LOCKED, NOTIFY_TYPE_ADD);
                        break;
		case GRP_SUBSCRIBED:
                        #ifdef CHANNEL_DEBUG
                        std::cerr << "p3Channels::locked_notifyGroupChanged() SUBSCRIBED" << std::endl;
                        #endif
	{
		std::string channeldir = mChannelsDir + "/" + grpId;
                #ifdef CHANNEL_DEBUG
                std::cerr << "p3Channels::locked_notifyGroupChanged() creating directory: " << channeldir << std::endl;
                #endif

		/* create chanDir */
                if (!RsDirUtil::checkCreateDirectory(channeldir)) {
                        std::cerr << "p3Channels::locked_notifyGroupChanged() Failed to create Channels Directory: " << channeldir << std::endl;
		}

		/* check if downloads need to be started? */
	}

                        rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_CHANNELLIST_LOCKED, NOTIFY_TYPE_ADD);
			break;
		case GRP_UNSUBSCRIBED:
                        #ifdef CHANNEL_DEBUG
                        std::cerr << "p3Channels::locked_notifyGroupChanged() UNSUBSCRIBED" << std::endl;
                         #endif
                        rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_CHANNELLIST_LOCKED, NOTIFY_TYPE_DEL);

		/* won't stop downloads... */

			break;

		default:
                        std::cerr << "p3Channels::locked_notifyGroupChanged() Unknown DEFAULT" << std::endl;
			break;
	}

	return p3GroupDistrib::locked_notifyGroupChanged(grp, flags, historical);
}


//TODO: if you want to config saving and loading for channel distrib service implement this method further
bool p3Channels::childLoadList(std::list<RsItem* >& configSaves)
{
	RsChannelReadStatus* drs = NULL;
	std::list<RsItem* >::iterator it;

	for(it = configSaves.begin(); it != configSaves.end(); it++)
	{
		if(NULL != (drs = dynamic_cast<RsChannelReadStatus* >(*it)))
		{
			processChanReadStatus(drs);
		}
		else
		{
			std::cerr << "p3Channels::childLoadList(): Configs items loaded were incorrect!"
					  << std::endl;

			if(*it != NULL)
				delete *it;
		}
	}

	return true;
}

void p3Channels::processChanReadStatus(RsChannelReadStatus* drs)
{


	mReadStatus.push_back(drs);
	std::string chId = drs->channelId;

	statMap::iterator sit = drs->msgReadStatus.find(chId);

	if(sit != drs->msgReadStatus.end()){
		mChannelStatus[chId] = sit->second;
		mChanReadStatus.insert(std::make_pair<std::string, RsChannelReadStatus*>
			(chId, drs));
	}

	// first pull out the channel id status

	mMsgReadStatus[drs->channelId] = drs->msgReadStatus;
	mReadStatus.push_back(drs);

	saveList.push_back(drs);


}

std::list<RsItem *> p3Channels::childSaveList()
{

	std::list<RsChannelReadStatus* >::iterator lit = mReadStatus.begin();
	statMap::iterator sit, mit;

	// update or add current channel id status
	for(; lit != mReadStatus.end(); lit++)
	{
		if((sit = mChannelStatus.find((*lit)->channelId)) != mChannelStatus.end())
		{
			mit = (*lit)->msgReadStatus.find((*lit)->channelId);
			if(mit != (*lit)->msgReadStatus.end())
			{
				mit->second = sit->second;
			}
			else
			{
				(*lit)->msgReadStatus[(*lit)->channelId] = sit->second;
			}
		}
	}

	return saveList;
}

/****************************************/
