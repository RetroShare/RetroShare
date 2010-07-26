#ifndef RS_P3_FORUMS_INTERFACE_H
#define RS_P3_FORUMS_INTERFACE_H

/*
 * libretroshare/src/services: p3forums.h
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

#include "rsiface/rsforums.h"
#include "services/p3distrib.h"
#include "serialiser/rsforumitems.h"

#if 0
class RsForumGrp: public RsDistribGrp
{
	public:

	RsForumGrp();

	/* orig data (from RsDistribMsg)
	 * std::string grpId
	 */

	std::wstring name;
	std::wstring desc;
};

class RsForumMsg: public RsDistribMsg
{
	public:

	RsForumMsg();

	/* orig data (from RsDistribMsg)
	 * std::string grpId
	 * std::string msgId
	 * std::string threadId
	 * std::string parentId
	 * time_t timestamp
	 */

	/* new data */
	std::wstring title;
	std::wstring msg;
	std::string srcId;
};

#endif

const uint32_t FORUM_MSG_STATUS_READ = 1;

class p3Forums: public p3GroupDistrib, public RsForums 
{
	public:

	p3Forums(uint16_t type, CacheStrapper *cs, CacheTransfer *cft,
                std::string srcdir, std::string storedir, std::string forumdir);
virtual ~p3Forums();

void	loadDummyData();

/****************************************/
/********* rsForums Interface ***********/

virtual bool forumsChanged(std::list<std::string> &forumIds);

virtual std::string createForum(std::wstring forumName, std::wstring forumDesc, uint32_t forumFlags);

virtual bool getForumInfo(std::string fId, ForumInfo &fi);
virtual bool getForumList(std::list<ForumInfo> &forumList);
virtual bool getForumThreadList(std::string fId, std::list<ThreadInfoSummary> &msgs);
virtual bool getForumThreadMsgList(std::string fId, std::string tId, std::list<ThreadInfoSummary> &msgs);
virtual bool getForumMessage(std::string fId, std::string mId, ForumMsgInfo &msg);
virtual void setReadStatus(const std::string& forumId,const std::string& msgId,const uint32_t status);
virtual	bool ForumMessageSend(ForumMsgInfo &info);
virtual bool setMessageStatus(const std::string& fId, const std::string& mId,const uint32_t status);

virtual bool forumSubscribe(std::string fId, bool subscribe);

/***************************************************************************************/
/****************** Event Feedback (Overloaded form p3distrib) *************************/
/***************************************************************************************/

virtual void locked_notifyGroupChanged(GroupInfo &grp, uint32_t flags);
virtual bool locked_eventDuplicateMsg(GroupInfo *, RsDistribMsg *, std::string);
virtual bool locked_eventNewMsg(GroupInfo *, RsDistribMsg *, std::string);



/****************************************/
/********* Overloaded Functions *********/

//virtual RsSerialiser *setupSerialiser();
//virtual pqistreamer *createStreamer(BinInterface *bio, std::string src, uint32_t bioflags);
virtual RsSerialType *createSerialiser();

virtual bool    locked_checkDistribMsg(RsDistribMsg *msg);
virtual RsDistribGrp *locked_createPublicDistribGrp(GroupInfo &info);
virtual RsDistribGrp *locked_createPrivateDistribGrp(GroupInfo &info);
virtual bool childLoadList(std::list<RsItem *>& );
virtual std::list<RsItem *> childSaveList();


/****************************************/

std::string createForumMsg(std::string fId, std::string pId,
                     std::wstring title, std::wstring msg, bool signIt);

	private:



bool 	mForumsChanged;
std::string mForumsDir;
std::list<RsItem *> mSaveList; // store save data

std::list<RsForumReadStatus *> mReadStatus;


};


#endif
