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

#include <retroshare/rsidentityVEG.h>

/* The Main Interface Class - for information about your Peers */
class RsForumsVEG;
extern RsForumsVEG *rsForumsVEG;

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

class RsForumsVEG: public RsTokenServiceVEG
{
	public:

	RsForumsVEG()  { return; }
virtual ~RsForumsVEG() { return; }

	/* Specific Service Data */
virtual bool getGroupData(const uint32_t &token, RsForumV2Group &group) = 0;
virtual bool getMsgData(const uint32_t &token, RsForumV2Msg &msg) = 0;


	// ONES THAT WE ARE NOT IMPLEMENTING. (YET!)
//virtual bool getMessageStatus(const std::string& fId, const std::string& mId, uint32_t& status) = 0;

// THINK WE CAN GENERALISE THIS TO: a list function, and you can just count the list entries...
// requestGroupList(groupId, UNREAD, ...)
//virtual bool getMessageCount(const std::string &groupId, unsigned int &newCount, unsigned int &unreadCount) = 0;


/* details are updated in group - to choose GroupID */
virtual bool createGroup(uint32_t &token, RsForumV2Group &group, bool isNew) = 0;
virtual bool createMsg(uint32_t &token, RsForumV2Msg &msg, bool isNew) = 0;

};



#endif
