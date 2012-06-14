#ifndef RETROSHARE_FORUMV2_GUI_INTERFACE_H
#define RETROSHARE_FORUMV2_GUI_INTERFACE_H

/*
 * libretroshare/src/retroshare: rsforumv2.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#include <inttypes.h>
#include <string>
#include <list>

#include <retroshare/rsidentity.h>

/* The Main Interface Class - for information about your Peers */
class RsForumsV2;
extern RsForumsV2 *rsForumsV2;

class RsForumV2Group
{
	public:

	// All the MetaData is Stored here:
	RsGroupMetaData mMeta;

	// THESE ARE IN THE META DATA.
	//std::string mGroupId;
	//std::string mName;

	std::string mDescription;

	// THESE ARE CURRENTLY UNUSED.
	//std::string mCategory;
	//std::string mHashTags;
};

class RsForumV2Msg
{
	public:

	// All the MetaData is Stored here:
	RsMsgMetaData mMeta;

	// THESE ARE IN THE META DATA.
	//std::string mGroupId;
	//std::string mMsgId;
	//std::string mOrigMsgId;
	//std::string mThreadId;
	//std::string mParentId;
	//std::string mName;      (aka. Title)

	std::string mMsg; // all the text is stored here.

	// THESE ARE CURRENTLY UNUSED.
	//std::string mHashTags;
};

class RsForumsV2: public RsTokenService
{
	public:

	RsForumsV2()  { return; }
virtual ~RsForumsV2() { return; }

	/* changed? */
virtual bool updated() = 0;

        /* Data Requests */
//virtual bool requestGroupInfo(     uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds) = 0;
//virtual bool requestMsgInfo(       uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds) = 0;
//virtual bool requestMsgRelatedInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &msgIds) = 0;

        /* Generic Lists */
//virtual bool getGroupList(         const uint32_t &token, std::list<std::string> &groupIds) = 0;
//virtual bool getMsgList(           const uint32_t &token, std::list<std::string> &msgIds) = 0;

        /* Generic Summary */
//virtual bool getGroupSummary(      const uint32_t &token, std::list<RsGroupMetaData> &groupInfo) = 0;
//virtual bool getMsgSummary(        const uint32_t &token, std::list<RsMsgMetaData> &msgInfo) = 0;

	/* Specific Service Data */
virtual bool getGroupData(const uint32_t &token, RsForumV2Group &group) = 0;
virtual bool getMsgData(const uint32_t &token, RsForumV2Msg &msg) = 0;



	/* FUNCTIONS THAT HAVE BEEN COPIED FROM THE ORIGINAL FORUMS....
	 * -> TODO, Split into generic and specific functions!
	 */
        //////////////////////////////////////////////////////////////////////////////
        /* Functions from Forums -> need to be implemented generically */
//virtual bool groupsChanged(std::list<std::string> &groupIds) = 0;

        // Get Message Status - is retrived via MessageSummary.
//virtual bool setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask) = 0;

        // 
//virtual bool groupSubscribe(const std::string &groupId, bool subscribe)     = 0;

//virtual bool groupRestoreKeys(const std::string &groupId) = 0;
//virtual bool groupShareKeys(const std::string &groupId, std::list<std::string>& peers) = 0;


	// ONES THAT WE ARE NOT IMPLEMENTING. (YET!)

//virtual bool getMessageStatus(const std::string& fId, const std::string& mId, uint32_t& status) = 0;

// THINK WE CAN GENERALISE THIS TO: a list function, and you can just count the list entries...
// requestGroupList(groupId, UNREAD, ...)
//virtual bool getMessageCount(const std::string &groupId, unsigned int &newCount, unsigned int &unreadCount) = 0;



/* details are updated in group - to choose GroupID */
virtual bool createGroup(RsForumV2Group &group) = 0;
virtual bool createMsg(RsForumV2Msg &msg) = 0;

};



#endif
