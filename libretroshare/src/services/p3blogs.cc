/*
 * libretroshare/src/services: p3Blogs.cc
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

#include "services/p3blogs.h"
#include "util/rsdir.h"

std::ostream &operator<<(std::ostream &out, const BlogInfo &info)
{
	std::string name(info.blogName.begin(), info.blogName.end());
	std::string desc(info.blogDesc.begin(), info.blogDesc.end());

	out << "BlogInfo:";
	out << std::endl;
	out << "BlogId: " << info.blogId << std::endl;
	out << "BlogName: " << name << std::endl;
	out << "BlogDesc: " << desc << std::endl;
	out << "BlogFlags: " << info.blogFlags << std::endl;
	out << "Pop: " << info.pop << std::endl;
	out << "LastPost: " << info.lastPost << std::endl;

	return out;
}

std::ostream &operator<<(std::ostream &out, const BlogMsgSummary &info)
{
	out << "BlogMsgSummary:";
	out << std::endl;
	out << "BlogId: " << info.blogId << std::endl;

	return out;
}

std::ostream &operator<<(std::ostream &out, const BlogMsgInfo &info)
{
	out << "BlogMsgInfo:";
	out << std::endl;
	out << "BlogId: " << info.blogId << std::endl;

	return out;
}


RsBlogs *rsBlogs = NULL;


/* Blogs will be initially stored for 1 year
 * remember 2^16 = 64K max units in store period.
 * PUBPERIOD * 2^16 = max STORE PERIOD */
#define BLOG_STOREPERIOD (90*24*3600)    /*  30 * 24 * 3600 - secs in a year */
#define BLOG_PUBPERIOD   600              /* 10 minutes ... (max = 455 days) */

p3Blogs::p3Blogs(uint16_t type, CacheStrapper *cs, 
		CacheTransfer *cft, RsFiles *files, 
                std::string srcdir, std::string storedir, std::string blogDir)
	:p3GroupDistrib(type, cs, cft, srcdir, storedir, blogDir,
                        CONFIG_TYPE_QBLOG, BLOG_STOREPERIOD, BLOG_PUBPERIOD),
	mRsFiles(files), 
	mBlogsDir(blogDir)
{ 
	//loadDummyData();
	
	/* create chanDir */
	if (!RsDirUtil::checkCreateDirectory(mBlogsDir))
	{
		std::cerr << "p3Blogs() Failed to create Channels Directory: ";
		std::cerr << mBlogsDir;
		std::cerr << std::endl;
	}

	return; 
}

p3Blogs::~p3Blogs() 
{ 
	return; 
}

/****************************************/

bool p3Blogs::blogsChanged(std::list<std::string> &blogIds)
{
	return groupsChanged(blogIds);
}


bool p3Blogs::getBlogInfo(std::string cId, BlogInfo &ci)
{
	RsStackMutex stack(distribMtx); /***** STACK LOCKED MUTEX *****/

	/* extract details */
	GroupInfo *gi = locked_getGroupInfo(cId);

	if (!gi)
		return false;

	ci.blogId = gi->grpId;
	ci.blogName = gi->grpName;
	ci.blogDesc = gi->grpDesc;

	ci.blogFlags = gi->flags;

	ci.pop = gi->sources.size();
	ci.lastPost = gi->lastPost;

	ci.pngChanImage = gi->grpIcon.pngImageData;

	if(ci.pngChanImage != NULL)
		ci.pngImageLen = gi->grpIcon.imageSize;
	else
		ci.pngImageLen = 0;

	return true;
}


bool p3Blogs::getBlogList(std::list<BlogInfo> &blogList)
{
	std::list<std::string> grpIds;
	std::list<std::string>::iterator it;

	getAllGroupList(grpIds);

	for(it = grpIds.begin(); it != grpIds.end(); it++)
	{
		BlogInfo ci;
		if (getBlogInfo(*it, ci))
		{
			blogList.push_back(ci);
		}
	}
	return true;
}


bool p3Blogs::getBlogMsgList(std::string cId, std::list<BlogMsgSummary> &msgs)
{
	std::list<std::string> msgIds;
	std::list<std::string>::iterator it;

	getAllMsgList(cId, msgIds);

	RsStackMutex stack(distribMtx); /***** STACK LOCKED MUTEX *****/
	for(it = msgIds.begin(); it != msgIds.end(); it++)
	{
		/* get details */
		RsDistribMsg *msg = locked_getGroupMsg(cId, *it);
		RsBlogMsg *cmsg = dynamic_cast<RsBlogMsg *>(msg);
		if (!cmsg)
			continue;

		BlogMsgSummary tis;

		tis.blogId = msg->grpId;
		tis.msgId = msg->msgId;
		tis.ts = msg->timestamp;

		/* the rest must be gotten from the derived Msg */
		
		tis.subject = cmsg->subject;
		tis.msg  = cmsg->message;
		tis.count = cmsg->attachment.items.size();
		tis.msgIdReply = cmsg->mIdReply;

		msgs.push_back(tis);
	}
	return true;
}

bool p3Blogs::getBlogMessage(std::string fId, std::string mId, BlogMsgInfo &info)
{
	std::list<std::string> msgIds;
	std::list<std::string>::iterator it;

	RsStackMutex stack(distribMtx); /***** STACK LOCKED MUTEX *****/

	RsDistribMsg *msg = locked_getGroupMsg(fId, mId);
	RsBlogMsg *cmsg = dynamic_cast<RsBlogMsg *>(msg);
	if (!cmsg)
		return false;


	info.blogId = msg->grpId;
	info.msgId = msg->msgId;
	info.ts = msg->timestamp;

	/* the rest must be gotten from the derived Msg */
		
	info.subject = cmsg->subject;
	info.msg  = cmsg->message;
	info.msgIdReply = cmsg->mIdReply;

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

bool p3Blogs::BlogMessageSend(BlogMsgInfo &info)
{

	RsBlogMsg *cmsg = new RsBlogMsg();
	cmsg->grpId = info.blogId;
	cmsg->mIdReply = info.msgIdReply;

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

bool p3Blogs::BlogMessageReply(BlogMsgInfo& reply){

	// ensure it has a value
	if(isReply(reply)){
		std::cerr << "p3Blogs::BlogMessageReply()" << " This is not a reply " << std::endl;
		return false;
	}


	// also check that msgId exists for group

	return BlogMessageSend(reply);
}

bool p3Blogs::isReply(BlogMsgInfo& info){

	// if replies list is empty then this is not a reply to a blog
	return !info.msgIdReply.empty();
}

std::string p3Blogs::createBlog(std::wstring blogName, std::wstring blogDesc, uint32_t blogFlags,
		unsigned char* pngImageData, uint32_t imageSize)
{

	std::string id = createGroup(blogName, blogDesc, blogFlags, pngImageData, imageSize);

	return id;
}

RsSerialType *p3Blogs::createSerialiser()
{
        return new RsBlogSerialiser();
}

bool    p3Blogs::locked_checkDistribMsg(RsDistribMsg *msg)
{
	return true;
}


RsDistribGrp *p3Blogs::locked_createPublicDistribGrp(GroupInfo &info)
{
	RsDistribGrp *grp = NULL; //new RsChannelGrp();

	return grp;
}

RsDistribGrp *p3Blogs::locked_createPrivateDistribGrp(GroupInfo &info)
{
	RsDistribGrp *grp = NULL; //new RsChannelGrp();

	return grp;
}


bool p3Blogs::blogSubscribe(std::string cId, bool subscribe)
{
	std::cerr << "p3Blogs::channelSubscribe() ";
	std::cerr << cId;
	std::cerr << std::endl;

	return subscribeToGroup(cId, subscribe);
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

bool p3Blogs::locked_eventDuplicateMsg(GroupInfo *grp, RsDistribMsg *msg, std::string id)
{
	std::string grpId = msg->grpId;
	std::string msgId = msg->msgId;
	std::string nullId;


	std::cerr << "p3Blogs::locked_eventDuplicateMsg() ";
	std::cerr << " grpId: " << grpId << " msgId: " << msgId;
	std::cerr << " peerId: " << id;
	std::cerr << std::endl;


	RsBlogMsg *chanMsg = dynamic_cast<RsBlogMsg *>(msg);
	if (!chanMsg)
	{
		return true;
	}

	/* request the files 
	 * NB: This will result in duplicates.
	 * it is upto ftserver/ftcontroller/ftextralist
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
		std::string blogname = grpId;
		std::string localpath = mBlogsDir + "/" + blogname;
		uint32_t flags = RS_FILE_HINTS_EXTRA;
		std::list<std::string> srcIds;

		srcIds.push_back(id);

		/* download it ... and flag for ExtraList 
		 * don't do pre-search check as FileRequest does it better
		 *
		 * FileRequest will ignore request if file is already indexed.
		 */

		std::cerr << "p3Blogs::locked_eventDuplicateMsg() ";
		std::cerr << " Downloading: " << fname;
		std::cerr << " to: " << localpath;
		std::cerr << " from: " << id;
		std::cerr << std::endl;

		mRsFiles->FileRequest(fname, hash, size, 
					localpath, flags, srcIds);
	}


	return true;
}

#include "pqi/pqinotify.h"

bool p3Blogs::locked_eventNewMsg(GroupInfo *grp, RsDistribMsg *msg, std::string id)
{
	std::string grpId = msg->grpId;
	std::string msgId = msg->msgId;
	std::string nullId;

	std::cerr << "p3Blogs::locked_eventNewMsg() ";
	std::cerr << " grpId: " << grpId;
	std::cerr << " msgId: " << msgId;
	std::cerr << " peerId: " << id;
	std::cerr << std::endl;

	getPqiNotify()->AddFeedItem(RS_FEED_ITEM_BLOG_MSG, grpId, msgId, nullId);

	/* request the files 
	 * NB: This could result in duplicates.
	 * which must be handled by ft side.
	 *
	 * this is exactly what DuplicateMsg does.
	 * */
	return locked_eventDuplicateMsg(grp, msg, id);
}




void p3Blogs::locked_notifyGroupChanged(GroupInfo &grp, uint32_t flags)
{
	std::string grpId = grp.grpId;
	std::string msgId;
	std::string nullId;

	std::cerr << "p3Blogs::locked_notifyGroupChanged() ";
	std::cerr << grpId;
	std::cerr << " flags:" << flags;
	std::cerr << std::endl;

	switch(flags)
	{
		case GRP_NEW_UPDATE:
			std::cerr << "p3Blogs::locked_notifyGroupChanged() NEW UPDATE";
			std::cerr << std::endl;
			getPqiNotify()->AddFeedItem(RS_FEED_ITEM_BLOG_NEW, grpId, msgId, nullId);
			break;
		case GRP_UPDATE:
			std::cerr << "p3Blogs::locked_notifyGroupChanged() UPDATE";
			std::cerr << std::endl;
			getPqiNotify()->AddFeedItem(RS_FEED_ITEM_BLOG_UPDATE, grpId, msgId, nullId);
			break;
		case GRP_LOAD_KEY:
			std::cerr << "p3Blogs::locked_notifyGroupChanged() LOAD_KEY";
			std::cerr << std::endl;
			break;
		case GRP_NEW_MSG:
			std::cerr << "p3Blogs::locked_notifyGroupChanged() NEW MSG";
			std::cerr << std::endl;
			break;
		case GRP_SUBSCRIBED:
			std::cerr << "p3Blogs::locked_notifyGroupChanged() SUBSCRIBED";
			std::cerr << std::endl;
	{
		std::string blogdir = mBlogsDir + "/" + grpId;

		std::cerr << "p3Blogs::locked_notifyGroupChanged() ";
		std::cerr << " creating directory: " << blogdir;
		std::cerr << std::endl;

		/* create chanDir */
		if (!RsDirUtil::checkCreateDirectory(blogdir))
		{
			std::cerr << "p3Blogs::locked_notifyGroupChanged() ";
			std::cerr << "Failed to create Blogs Directory: ";
			std::cerr << blogdir;
			std::cerr << std::endl;
		}

		/* check if downloads need to be started? */
	}

			break;
		case GRP_UNSUBSCRIBED:
			std::cerr << "p3Blogs::locked_notifyGroupChanged() UNSUBSCRIBED";
			std::cerr << std::endl;

		/* won't stop downloads... */

			break;

		default:
			std::cerr << "p3Blogs::locked_notifyGroupChanged() Unknown DEFAULT";
			std::cerr << std::endl;
			break;
	}

	return p3GroupDistrib::locked_notifyGroupChanged(grp, flags);
}



/****************************************/



