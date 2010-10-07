/*
 * libretroshare/src/services: rsforums.cc
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2007-2008 by Robert Fernie.
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

#include "services/p3forums.h"
#include "pqi/authssl.h"
#include "util/rsdir.h"
#include "retroshare/rsiface.h"

uint32_t convertToInternalFlags(uint32_t extFlags);
uint32_t convertToExternalFlags(uint32_t intFlags);

std::ostream &operator<<(std::ostream &out, const ForumInfo &info)
{
	std::string name(info.forumName.begin(), info.forumName.end());
	std::string desc(info.forumDesc.begin(), info.forumDesc.end());

	out << "ForumInfo:";
	out << std::endl;
	out << "ForumId: " << info.forumId << std::endl;
	out << "ForumName: " << name << std::endl;
	out << "ForumDesc: " << desc << std::endl;
	out << "ForumFlags: " << info.forumFlags << std::endl;
	out << "Pop: " << info.pop << std::endl;
	out << "LastPost: " << info.lastPost << std::endl;

	return out;
}

std::ostream &operator<<(std::ostream &out, const ThreadInfoSummary &info)
{
	out << "ThreadInfoSummary:";
	out << std::endl;
	//out << "ForumId: " << forumId << std::endl;
	//out << "ThreadId: " << threadId << std::endl;

	return out;
}

std::ostream &operator<<(std::ostream &out, const ForumMsgInfo &info)
{
	out << "ForumMsgInfo:";
	out << std::endl;
	//out << "ForumId: " << forumId << std::endl;
	//out << "ThreadId: " << threadId << std::endl;

	return out;
}


RsForums *rsForums = NULL;


/* Forums will be initially stored for 1 year 
 * remember 2^16 = 64K max units in store period.
 * PUBPERIOD * 2^16 = max STORE PERIOD */
#define FORUM_STOREPERIOD (365*24*3600)    /* 365 * 24 * 3600 - secs in a year */
#define FORUM_PUBPERIOD   600 		   /* 10 minutes ... (max = 455 days) */

p3Forums::p3Forums(uint16_t type, CacheStrapper *cs, CacheTransfer *cft,
                        std::string srcdir, std::string storedir, std::string forumDir)
	:p3GroupDistrib(type, cs, cft, srcdir, storedir, forumDir,
                CONFIG_TYPE_FORUMS, FORUM_STOREPERIOD, FORUM_PUBPERIOD),
	mForumsDir(forumDir)
{ 

	/* create chanDir */
        if (!RsDirUtil::checkCreateDirectory(mForumsDir)) {
                std::cerr << "p3Channels() Failed to create forums Directory: " << mForumsDir << std::endl;
	}

	return; 
}

p3Forums::~p3Forums() 
{ 
	return; 
}

/****************************************/

bool p3Forums::forumsChanged(std::list<std::string> &forumIds)
{
	return groupsChanged(forumIds);
}

bool p3Forums::getForumInfo(std::string fId, ForumInfo &fi)
{
	RsStackMutex stack(distribMtx); /***** STACK LOCKED MUTEX *****/

	/* extract details */
	GroupInfo *gi = locked_getGroupInfo(fId);

	if (!gi)
		return false;

	fi.forumId = gi->grpId;
	fi.forumName = gi->grpName;
	fi.forumDesc = gi->grpDesc;
	fi.forumFlags = gi->grpFlags;

	fi.subscribeFlags = gi->flags;

	fi.pop = gi->sources.size();
	fi.lastPost = gi->lastPost;

	return true;
}

/*!
 * allows peers to change information for the forum:
 * can only change name and descriptions
 *
 */
bool p3Forums::setForumInfo(std::string fId, ForumInfo &fi)
{
	GroupInfo gi;

	RsStackMutex stack(distribMtx);

	gi.grpName = fi.forumName;
	gi.grpDesc = fi.forumDesc;

	return locked_editGroup(fId, gi);
 }

bool p3Forums::getForumList(std::list<ForumInfo> &forumList)
{
	std::list<std::string> grpIds;
	std::list<std::string>::iterator it;

	getAllGroupList(grpIds);

	for(it = grpIds.begin(); it != grpIds.end(); it++)
	{
		ForumInfo fi;
		if (getForumInfo(*it, fi))
		{
			forumList.push_back(fi);
		}
	}
	return true;
}

bool p3Forums::getForumThreadList(std::string fId, std::list<ThreadInfoSummary> &msgs)
{
	std::list<std::string> msgIds;
	std::list<std::string>::iterator it;

	getParentMsgList(fId, "", msgIds);

	RsStackMutex stack(distribMtx); /***** STACK LOCKED MUTEX *****/
	for(it = msgIds.begin(); it != msgIds.end(); it++)
	{
		/* get details */
		RsDistribMsg *msg = locked_getGroupMsg(fId, *it);
		RsForumMsg *fmsg = dynamic_cast<RsForumMsg *>(msg);
		if (!fmsg)
			continue;

		ThreadInfoSummary tis;

		tis.forumId = msg->grpId;
		tis.msgId = msg->msgId;
		tis.parentId = ""; // always NULL (see request)
		tis.threadId = msg->msgId; // these are the thread heads!

		tis.ts = msg->timestamp;
		tis.childTS = msg->childTS;

		/* the rest must be gotten from the derived Msg */
		
		tis.title = fmsg->title;
		tis.msg  = fmsg->msg;

		msgs.push_back(tis);
	}
	return true;
}

bool p3Forums::getForumThreadMsgList(std::string fId, std::string pId, std::list<ThreadInfoSummary> &msgs)
{
	std::list<std::string> msgIds;
	std::list<std::string>::iterator it;

	getParentMsgList(fId, pId, msgIds);

	RsStackMutex stack(distribMtx); /***** STACK LOCKED MUTEX *****/
	for(it = msgIds.begin(); it != msgIds.end(); it++)
	{
		/* get details */
		RsDistribMsg *msg = locked_getGroupMsg(fId, *it);
		RsForumMsg *fmsg = dynamic_cast<RsForumMsg *>(msg);
		if (!fmsg)
			continue;

		ThreadInfoSummary tis;

		tis.forumId = msg->grpId;
		tis.msgId = msg->msgId;
		tis.parentId = msg->parentId;
		tis.threadId = msg->threadId;

		tis.ts = msg->timestamp;
		tis.childTS = msg->childTS;

		/* the rest must be gotten from the derived Msg */
		
		tis.title = fmsg->title;
		tis.msg  = fmsg->msg;

		if (fmsg->personalSignature.keyId.empty() == false) {
			tis.msgflags |= RS_DISTRIB_AUTHEN_REQ;
		}

		msgs.push_back(tis);
	}
	return true;
}

bool p3Forums::getForumMessage(std::string fId, std::string mId, ForumMsgInfo &info)
{
	RsStackMutex stack(distribMtx); /***** STACK LOCKED MUTEX *****/

	RsDistribMsg *msg = locked_getGroupMsg(fId, mId);
	RsForumMsg *fmsg = dynamic_cast<RsForumMsg *>(msg);
	if (!fmsg)
		return false;


	info.forumId = msg->grpId;
	info.msgId = msg->msgId;
	info.parentId = msg->parentId;
	info.threadId = msg->threadId;

	info.ts = msg->timestamp;
	info.childTS = msg->childTS;

	/* the rest must be gotten from the derived Msg */
		
	info.title = fmsg->title;
	info.msg  = fmsg->msg;
	// should only use actual signature ....
	//info.srcId = fmsg->srcId;
	info.srcId = fmsg->personalSignature.keyId;

	if (fmsg->personalSignature.keyId.empty() == false) {
		info.msgflags |= RS_DISTRIB_AUTHEN_REQ;
	}

	return true;
}

bool p3Forums::ForumMessageSend(ForumMsgInfo &info)
{
	bool signIt = (info.msgflags == RS_DISTRIB_AUTHEN_REQ);

	std::string mId = createForumMsg(info.forumId, info.parentId,
		info.title, info.msg, signIt);

	if (mId.empty()) {
		return false;
	}

	return setMessageStatus(info.forumId, mId, FORUM_MSG_STATUS_READ, FORUM_MSG_STATUS_MASK);
}

bool p3Forums::setMessageStatus(const std::string& fId,const std::string& mId,const uint32_t status, const uint32_t statusMask)
{
	{
		RsStackMutex stack(distribMtx); /***** STACK LOCKED MUTEX *****/

		std::list<RsForumReadStatus *>::iterator lit = mReadStatus.begin();

		for(; lit != mReadStatus.end(); lit++)
		{

			if((*lit)->forumId == fId)
			{
					RsForumReadStatus* rsi = *lit;
					rsi->msgReadStatus[mId] &= ~statusMask;
					rsi->msgReadStatus[mId] |= (status & statusMask);
					break;
			}

		}

		// if forum id does not exist create one
		if(lit == mReadStatus.end())
		{
			RsForumReadStatus* rsi = new RsForumReadStatus();
			rsi->forumId = fId;
			rsi->msgReadStatus[mId] = status & statusMask;
			mReadStatus.push_back(rsi);
			mSaveList.push_back(rsi);
		}
		
		IndicateConfigChanged();
	} /******* UNLOCKED ********/

	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_FORUMLIST_LOCKED, NOTIFY_TYPE_MOD);

	return true;
}

bool p3Forums::getMessageStatus(const std::string& fId, const std::string& mId, uint32_t& status)
{

	status = 0;

	RsStackMutex stack(distribMtx);

	std::list<RsForumReadStatus *>::iterator lit = mReadStatus.begin();

	for(; lit != mReadStatus.end(); lit++)
	{

		if((*lit)->forumId == fId)
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


std::string p3Forums::createForum(std::wstring forumName, std::wstring forumDesc, uint32_t forumFlags)
{

        std::string id = createGroup(forumName, forumDesc, 
				convertToInternalFlags(forumFlags), NULL, 0);

	return id;
}

std::string p3Forums::createForumMsg(std::string fId, std::string pId, 
				std::wstring title, std::wstring msg, bool signIt)
{

	RsForumMsg *fmsg = new RsForumMsg();
	fmsg->grpId = fId;
	fmsg->parentId = pId;

      {
	RsStackMutex stack(distribMtx); /***** STACK LOCKED MUTEX *****/

	RsDistribMsg *msg = locked_getGroupMsg(fId, pId);
	if (!msg)
	{
		fmsg->parentId = "";
		fmsg->threadId = "";
	}
	else
	{
		if (msg->parentId == "")
		{
			fmsg->threadId = fmsg->parentId;
		}
		else
		{
			fmsg->threadId = msg->threadId;
		}
	}
      }

	fmsg->title = title;
	fmsg->msg   = msg;
	if (signIt)
	{
                fmsg->srcId = AuthSSL::getAuthSSL()->OwnId();
	}
	fmsg->timestamp = time(NULL);

	std::string msgId = publishMsg(fmsg, signIt);

	if (msgId.empty()) {
		delete(fmsg);
	}

	return msgId;
}

RsSerialType *p3Forums::createSerialiser()
{
        return new RsForumSerialiser();
}

bool    p3Forums::locked_checkDistribMsg(RsDistribMsg *msg)
{
	return true;
}


RsDistribGrp *p3Forums::locked_createPublicDistribGrp(GroupInfo &info)
{
	RsDistribGrp *grp = NULL; //new RsForumGrp();

	return grp;
}

RsDistribGrp *p3Forums::locked_createPrivateDistribGrp(GroupInfo &info)
{
	RsDistribGrp *grp = NULL; //new RsForumGrp();

	return grp;
}


uint32_t convertToInternalFlags(uint32_t extFlags)
{
	return extFlags;
}

uint32_t convertToExternalFlags(uint32_t intFlags)
{
	return intFlags;
}

bool p3Forums::forumSubscribe(std::string fId, bool subscribe)
{
	return subscribeToGroup(fId, subscribe);
}

bool p3Forums::getMessageCount(const std::string fId, unsigned int &newCount, unsigned int &unreadCount)
{
	newCount = 0;
	unreadCount = 0;

	std::list<std::string> grpIds;

	if (fId.empty()) {
		// count all messages of all subscribed forums
		getAllGroupList(grpIds);
	} else {
		// count all messages of one forum
		grpIds.push_back(fId);
	}

	std::list<std::string>::iterator git;
	for (git = grpIds.begin(); git != grpIds.end(); git++) {
		std::string fId = *git;
		uint32_t grpFlags;

		{
			// only flag is needed
			RsStackMutex stack(distribMtx); /***** STACK LOCKED MUTEX *****/
			GroupInfo *gi = locked_getGroupInfo(fId);
			if (gi == NULL) {
				return false;
			}
			grpFlags = gi->flags;
		} /******* UNLOCKED ********/

		if (grpFlags & (RS_DISTRIB_ADMIN | RS_DISTRIB_SUBSCRIBED)) {
			std::list<std::string> msgIds;
			if (getAllMsgList(fId, msgIds)) {
				std::list<std::string>::iterator mit;

				RsStackMutex stack(distribMtx); /***** STACK LOCKED MUTEX *****/

				std::list<RsForumReadStatus *>::iterator lit;
				for(lit = mReadStatus.begin(); lit != mReadStatus.end(); lit++) {
					if ((*lit)->forumId == fId) {
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

					if (rit->second & FORUM_MSG_STATUS_READ) {
						// message is not new
						if (rit->second & FORUM_MSG_STATUS_UNREAD_BY_USER) {
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

/***************************************************************************************/
/****************** Event Feedback (Overloaded form p3distrib) *************************/
/***************************************************************************************/

#include "pqi/pqinotify.h"

void p3Forums::locked_notifyGroupChanged(GroupInfo  &grp, uint32_t flags)
{
	std::string grpId = grp.grpId;
	std::string msgId;
	std::string nullId;

        switch(flags)
        {
                case GRP_NEW_UPDATE:
                        getPqiNotify()->AddFeedItem(RS_FEED_ITEM_FORUM_NEW, grpId, msgId, nullId);
                        rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_FORUMLIST_LOCKED, NOTIFY_TYPE_ADD);
                        break;
                case GRP_UPDATE:
                        getPqiNotify()->AddFeedItem(RS_FEED_ITEM_FORUM_UPDATE, grpId, msgId, nullId);
                        rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_FORUMLIST_LOCKED, NOTIFY_TYPE_MOD);
                        break;
                case GRP_LOAD_KEY:
                        break;
                case GRP_NEW_MSG:
                        rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_FORUMLIST_LOCKED, NOTIFY_TYPE_ADD);
                        break;
                case GRP_SUBSCRIBED:
                        rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_FORUMLIST_LOCKED, NOTIFY_TYPE_ADD);
                        break;
                case GRP_UNSUBSCRIBED:
                        rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_FORUMLIST_LOCKED, NOTIFY_TYPE_DEL);
                        break;
        }
	return p3GroupDistrib::locked_notifyGroupChanged(grp, flags);
}

bool p3Forums::locked_eventDuplicateMsg(GroupInfo *grp, RsDistribMsg *msg, std::string id)
{
	return true;
}

bool p3Forums::locked_eventNewMsg(GroupInfo *grp, RsDistribMsg *msg, std::string id)
{
	std::string grpId = msg->grpId;
	std::string msgId = msg->msgId;
	std::string nullId;

	getPqiNotify()->AddFeedItem(RS_FEED_ITEM_FORUM_MSG, grpId, msgId, nullId);
	return true;
}



/****************************************/

void    p3Forums::loadDummyData()
{
	ForumInfo fi;
	std::string forumId;
	std::string msgId;
	time_t now = time(NULL);

	fi.forumId = "FID1234";
	fi.forumName = L"Forum 1";
	fi.forumDesc = L"Forum 1";
	fi.forumFlags = RS_DISTRIB_ADMIN;
	fi.pop = 2;
	fi.lastPost = now - 123;

	forumId = createForum(fi.forumName, fi.forumDesc, fi.forumFlags);

	fi.forumId = "FID2345";
	fi.forumName = L"Forum 2";
	fi.forumDesc = L"Forum 2";
	fi.forumFlags = RS_DISTRIB_SUBSCRIBED;
	fi.pop = 3;
	fi.lastPost = now - 1234;

	forumId = createForum(fi.forumName, fi.forumDesc, fi.forumFlags);
	msgId = createForumMsg(forumId, "", L"WELCOME TO Forum1", L"Hello!", true);
	msgId = createForumMsg(forumId, msgId, L"Love this forum", L"Hello2!", true);

	return; 

	/* ignore this */

	fi.forumId = "FID3456";
	fi.forumName = L"Forum 3";
	fi.forumDesc = L"Forum 3";
	fi.forumFlags = 0;
	fi.pop = 3;
	fi.lastPost = now - 1234;

	forumId = createForum(fi.forumName, fi.forumDesc, fi.forumFlags);

	fi.forumId = "FID4567";
	fi.forumName = L"Forum 4";
	fi.forumDesc = L"Forum 4";
	fi.forumFlags = 0;
	fi.pop = 5;
	fi.lastPost = now - 1234;

	forumId = createForum(fi.forumName, fi.forumDesc, fi.forumFlags);

	fi.forumId = "FID5678";
	fi.forumName = L"Forum 5";
	fi.forumDesc = L"Forum 5";
	fi.forumFlags = 0;
	fi.pop = 1;
	fi.lastPost = now - 1234;

	forumId = createForum(fi.forumName, fi.forumDesc, fi.forumFlags);

	fi.forumId = "FID6789";
	fi.forumName = L"Forum 6";
	fi.forumDesc = L"Forum 6";
	fi.forumFlags = 0;
	fi.pop = 2;
	fi.lastPost = now - 1234;

	forumId = createForum(fi.forumName, fi.forumDesc, fi.forumFlags);

	fi.forumId = "FID7890";
	fi.forumName = L"Forum 7";
	fi.forumDesc = L"Forum 7";
	fi.forumFlags = 0;
	fi.pop = 4;
	fi.lastPost = now - 1234;

	forumId = createForum(fi.forumName, fi.forumDesc, fi.forumFlags);

	fi.forumId = "FID8901";
	fi.forumName = L"Forum 8";
	fi.forumDesc = L"Forum 8";
	fi.forumFlags = 0;
	fi.pop = 3;
	fi.lastPost = now - 1234;

	forumId = createForum(fi.forumName, fi.forumDesc, fi.forumFlags);

	fi.forumId = "FID9012";
	fi.forumName = L"Forum 9";
	fi.forumDesc = L"Forum 9";
	fi.forumFlags = 0;
	fi.pop = 2;
	fi.lastPost = now - 1234;

	forumId = createForum(fi.forumName, fi.forumDesc, fi.forumFlags);

	fi.forumId = "FID9123";
	fi.forumName = L"Forum 10";
	fi.forumDesc = L"Forum 10";
	fi.forumFlags = 0;
	fi.pop = 1;
	fi.lastPost = now - 1234;

	forumId = createForum(fi.forumName, fi.forumDesc, fi.forumFlags);
}

std::list<RsItem* > p3Forums::childSaveList()
{
	return mSaveList;
}

bool p3Forums::childLoadList(std::list<RsItem* >& configSaves)
{
	RsForumReadStatus* drs = NULL;
	std::list<RsItem* >::iterator it;

	for(it = configSaves.begin(); it != configSaves.end(); it++)
	{
		if(NULL != (drs = dynamic_cast<RsForumReadStatus* >(*it)))
		{
			mReadStatus.push_back(drs);
			mSaveList.push_back(drs);
		}
		else
		{
			std::cerr << "p3Forums::childLoadList(): Configs items loaded were incorrect!"
					  << std::endl;
			return false;
		}
	}

	return true;
}
