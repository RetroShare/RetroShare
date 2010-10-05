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
#define TEST_CHANNEL_STOREPERIOD (24*3600)   /* one day */
#define CHANNEL_PUBPERIOD   60             /* 1 minutes ... (max = 455 days) */
#define MAX_AUTO_DL 1E9 /* auto download of attachment limit; 1 GIG */

p3Channels::p3Channels(uint16_t type, CacheStrapper *cs, 
		CacheTransfer *cft, RsFiles *files, 
                std::string srcdir, std::string storedir, std::string chanDir)
	:p3GroupDistrib(type, cs, cft, srcdir, storedir, chanDir,
                        CONFIG_TYPE_CHANNELS, CHANNEL_STOREPERIOD, CHANNEL_PUBPERIOD),
	mRsFiles(files), 
	mChannelsDir(chanDir)
{ 
	//loadDummyData();
	
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

bool p3Channels::channelRestoreKeys(std::string chId){

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

	return true;
}


bool p3Channels::channelExtraFileHash(std::string path, std::string chId, FileInfo& fInfo){

	// get file name
	std::string fname, fnameBuff;
	std::string::reverse_iterator rit;

	for(rit = path.rbegin(); *rit != '/' ; rit++){
		fnameBuff.push_back(*rit);
	}

	// reverse string buff for correct file name
	fname.append(fnameBuff.rbegin(), fnameBuff.rend());
	bool fileTooLarge = false;

	// first copy file into channel directory
	if(!cpyMsgFileToChFldr(path, fname, chId, fileTooLarge)){

		if(!fileTooLarge)
			return false;

	}

	uint32_t flags = RS_FILE_HINTS_NETWORK_WIDE | RS_FILE_HINTS_EXTRA;

	// then hash file, but hash file at original location if its too large
	// and get file info too
	if(fileTooLarge){

		if(!mRsFiles->ExtraFileHash(path, CHANNEL_STOREPERIOD, flags))
			return false;

		fInfo.path = path;

	}else{

		std::string localpath = mChannelsDir + "/" + chId + "/" + fname;
		if(!mRsFiles->ExtraFileHash(localpath, CHANNEL_STOREPERIOD, flags))
			return false;

		fInfo.path = localpath;
	}

	fInfo.fname = fname;

	return true;
}

bool p3Channels::cpyMsgFileToChFldr(std::string path, std::string fname, std::string chId, bool& fileTooLarge){

	FILE *outFile = NULL, *inFile;
#ifdef WINDOWS_SYS
	std::wstring wpath;
	librs::util::ConvertUtf8ToUtf16(path, wpath);
	inFile = _wfopen(wpath.c_str(), L"rb");
#else
	inFile = fopen(path.c_str(), "rb");
#endif

	long buffSize = 0;
	char* buffer = NULL;

	if(inFile){

		// obtain file size:
		fseek (inFile , 0 , SEEK_END);
		buffSize = ftell (inFile);
		rewind (inFile);

		// don't copy if file over 100mb
		if(buffSize > (MAX_AUTO_DL / 10) ){
			fileTooLarge = true;
			fclose(inFile);
			return false;

		}

		// allocate memory to contain the whole file:
		buffer = (char*) malloc (sizeof(char)*buffSize);

		if(!buffer){
			fclose(inFile);
			return false;
		}

		fread (buffer,1,buffSize,inFile);
		fclose(inFile);

		std::string localpath = mChannelsDir + "/" + chId + "/" + fname;
#ifdef WINDOWS_SYS
		std::wstring wlocalpath;
		librs::util::ConvertUtf8ToUtf16(localpath, wlocalpath);
		outFile = _wfopen(wlocalpath.c_str(), L"wb");
#else
		outFile = fopen(localpath.c_str(), "wb");
#endif
	}

	if(outFile){

		fwrite(buffer, 1, buffSize, outFile);
		fclose(outFile);
	}else{
		std::cerr << "p3Channels::cpyMsgFiletoFldr(): Failed to copy Channel Msg file to its channel folder"
				  << std::endl;

		if((buffSize > 0) && (buffer != NULL))
			free(buffer);

		return false;
	}

	if((buffSize > 0) && (buffer != NULL))
		free(buffer);

	return true;

}

bool p3Channels::channelExtraFileRemove(std::string hash, std::string chId){

	uint32_t flags = RS_FILE_HINTS_NETWORK_WIDE | RS_FILE_HINTS_EXTRA;

	/* remove copy from channels directory */

	FileInfo fInfo;
	mRsFiles->FileDetails(hash, flags, fInfo);
	std::string chPath = mChannelsDir + "/" + chId + "/" + fInfo.fname;

	if(remove(chPath.c_str()) == 0){
		std::cerr << "p3Channel::channelExtraFileRemove() Removed file :"
				  << chPath.c_str() << std::endl;

	}else{
		std::cerr << "p3Channel::channelExtraFileRemove() Failed to remove file :"
				  << chPath.c_str() << std::endl;
	}

	return mRsFiles->ExtraFileRemove(hash, flags);

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
#ifdef CHANNEL_DEBUG
        std::cerr << "p3Channels::channelSubscribe() " << cId << std::endl;
#endif

	return subscribeToGroup(cId, subscribe);
}

bool p3Channels::channelShareKeys(std::string chId, std::list<std::string>& peers){

#ifdef CHANNEL_DEBUG
	std::cerr << "p3Channels::channelShareKeys() " << chId << std::endl;
#endif

	return sharePubKey(chId, peers);

}

bool p3Channels::channelEditInfo(std::string chId, ChannelInfo& info){

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

bool p3Channels::locked_eventDuplicateMsg(GroupInfo *grp, RsDistribMsg *msg, std::string id)
{
	std::string grpId = msg->grpId;
	std::string msgId = msg->msgId;
	std::string nullId;


//	std::cerr << "p3Channels::locked_eventDuplicateMsg() ";
//	std::cerr << " grpId: " << grpId << " msgId: " << msgId;
//	std::cerr << " peerId: " << id;
//	std::cerr << std::endl;


	RsChannelMsg *chanMsg = dynamic_cast<RsChannelMsg *>(msg);
	if (!chanMsg)
	{
		return true;
	}

	/* request the files 
	 * NB: This will result in duplicates.
	 * it is upto ftserver/ftcontroller/ftextralist
	 * */

	bool download = (grp->flags & RS_DISTRIB_SUBSCRIBED);

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
		uint32_t flags = RS_FILE_HINTS_EXTRA | 
				RS_FILE_HINTS_BACKGROUND | 
				RS_FILE_HINTS_NETWORK_WIDE;

		std::list<std::string> srcIds;

		srcIds.push_back(id);

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
        	mRsFiles->FileRequest(fname, hash, size,
        			localpath, flags, srcIds);
	}


	return true;
}

#include "pqi/pqinotify.h"

bool p3Channels::locked_eventNewMsg(GroupInfo *grp, RsDistribMsg *msg, std::string id)
{
	std::string grpId = msg->grpId;
	std::string msgId = msg->msgId;
	std::string nullId;

//	std::cerr << "p3Channels::locked_eventNewMsg() ";
//	std::cerr << " grpId: " << grpId;
//	std::cerr << " msgId: " << msgId;
//	std::cerr << " peerId: " << id;
//	std::cerr << std::endl;

	getPqiNotify()->AddFeedItem(RS_FEED_ITEM_CHAN_MSG, grpId, msgId, nullId);

	/* request the files 
	 * NB: This could result in duplicates.
	 * which must be handled by ft side.
	 *
	 * this is exactly what DuplicateMsg does.
	 * */
	return locked_eventDuplicateMsg(grp, msg, id);
}


void p3Channels::locked_notifyGroupChanged(GroupInfo &grp, uint32_t flags)
{
	std::string grpId = grp.grpId;
	std::string msgId;
	std::string nullId;

//	std::cerr << "p3Channels::locked_notifyGroupChanged() ";
//	std::cerr << grpId;
//	std::cerr << " flags:" << flags;
//	std::cerr << std::endl;

	switch(flags)
	{
		case GRP_NEW_UPDATE:
                        #ifdef CHANNEL_DEBUG
                        std::cerr << "p3Channels::locked_notifyGroupChanged() NEW UPDATE" << std::endl;
                        #endif
			getPqiNotify()->AddFeedItem(RS_FEED_ITEM_CHAN_NEW, grpId, msgId, nullId);
			break;
		case GRP_UPDATE:
                        #ifdef CHANNEL_DEBUG
                        std::cerr << "p3Channels::locked_notifyGroupChanged() UPDATE" << std::endl;
                        #endif
			getPqiNotify()->AddFeedItem(RS_FEED_ITEM_CHAN_UPDATE, grpId, msgId, nullId);
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

			break;
		case GRP_UNSUBSCRIBED:
                        #ifdef CHANNEL_DEBUG
                        std::cerr << "p3Channels::locked_notifyGroupChanged() UNSUBSCRIBED" << std::endl;
                         #endif

		/* won't stop downloads... */

			break;

		default:
                        std::cerr << "p3Channels::locked_notifyGroupChanged() Unknown DEFAULT" << std::endl;
			break;
	}

	return p3GroupDistrib::locked_notifyGroupChanged(grp, flags);
}

void p3Channels::cleanUpOldFiles(){

	time_t now = time(NULL);
	std::list<ChannelInfo> chList;
	std::list<ChannelInfo>::iterator ch_it;

	// first get channel list
	if(!getChannelList(chList))
		return;

	std::list<ChannelMsgSummary> msgList;
	std::list<ChannelMsgSummary>::iterator msg_it;

	// then msg for each channel
	for(ch_it = chList.begin(); ch_it != chList.end(); ch_it++){

		if(!getChannelMsgList(ch_it->channelId, msgList))
			continue;

		std::string channelname = ch_it->channelId;
		std::string localpath = mChannelsDir + "/" + channelname;

		for(msg_it = msgList.begin(); msg_it != msgList.end(); msg_it++){

			ChannelMsgInfo chMsgInfo;
			if(!getChannelMessage(ch_it->channelId, msg_it->msgId, chMsgInfo))
				continue;

			// if msg not old, leave it alone
			if( chMsgInfo.ts > (now - CHANNEL_STOREPERIOD))
				continue;

			std::list<FileInfo>::iterator file_it;
			// get the files
			for(file_it = chMsgInfo.files.begin(); file_it != chMsgInfo.files.end(); file_it++){

				std::string msgFile = localpath + "/" + file_it->fname;

				if(mRsFiles){
					if(mRsFiles->ExtraFileRemove(file_it->hash, ~(RS_FILE_HINTS_DOWNLOAD | RS_FILE_HINTS_UPLOAD)))
							std::cerr << "p3Channels::clearOldFIles() failed to remove files from extras" << std::endl;

					if(remove(msgFile.c_str()) == 0){
						std::cerr << "p3Channels::clearOldFiles() file removed: "
								  << msgFile << std::endl;
					}else{
						std::cerr << "p3Channels::clearOldFiles() failed to remove file: "
								  << msgFile << std::endl;
					}

				}else{
					std::cerr << "p3Channels::cleanUpOldFiles() : Pointer passed to (this) Invalid" << std::endl;
				}


			}

		}

	}

	// clean up local caches
//.	clear_local_caches(now); //how about remote caches, remember its hardwired

	return;
}

//TODO: if you want to config saving and loading for channel distrib service implement this method further
bool p3Channels::childLoadList(std::list<RsItem* >& configSaves)
{
	return true;
}

std::list<RsItem *> p3Channels::childSaveList()
{
	return saveList;
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

