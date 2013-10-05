#ifndef RETROSHARE_GXS_FORUM_GUI_INTERFACE_H
#define RETROSHARE_GXS_FORUM_GUI_INTERFACE_H

/*
 * libretroshare/src/retroshare: rsgxsforum.h
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

#include "retroshare/rstokenservice.h"
#include "retroshare/rsgxsifacehelper.h"



/* The Main Interface Class - for information about your Peers */
class RsGxsForums;
extern RsGxsForums *rsGxsForums;

class RsGxsForumGroup
{
	public:
	RsGroupMetaData mMeta;
	std::string mDescription;
};

class RsGxsForumMsg
{
	public:
	RsMsgMetaData mMeta;
	std::string mMsg; 
};


//typedef std::map<RsGxsGroupId, std::vector<RsGxsForumMsg> > GxsForumMsgResult;

std::ostream &operator<<(std::ostream &out, const RsGxsForumGroup &group);
std::ostream &operator<<(std::ostream &out, const RsGxsForumMsg &msg);

class RsGxsForums: public RsGxsIfaceHelper
{
	public:

	RsGxsForums(RsGxsIface *gxs)
	:RsGxsIfaceHelper(gxs)  { return; }
virtual ~RsGxsForums() { return; }

	/* Specific Service Data */
virtual bool getGroupData(const uint32_t &token, std::vector<RsGxsForumGroup> &groups) = 0;
virtual bool getMsgData(const uint32_t &token, std::vector<RsGxsForumMsg> &msgs) = 0;
virtual bool getRelatedMessages(const uint32_t &token, std::vector<RsGxsForumMsg> &msgs) = 0;

        //////////////////////////////////////////////////////////////////////////////
virtual void setMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read) = 0;

//virtual bool setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask);
//virtual bool setGroupSubscribeFlags(const std::string &groupId, uint32_t subscribeFlags, uint32_t subscribeMask);

//virtual bool groupRestoreKeys(const std::string &groupId);
//virtual bool groupShareKeys(const std::string &groupId, std::list<std::string>& peers);

virtual bool createGroup(uint32_t &token, RsGxsForumGroup &group) = 0;
virtual bool createMsg(uint32_t &token, RsGxsForumMsg &msg) = 0;

/*!
 * To update forum group with new information
 * @param token the token used to check completion status of update
 * @param group group to be updated, groupId element must be set or will be rejected
 * @return false groupId not set, true if set and accepted (still check token for completion)
 */
virtual bool updateGroup(uint32_t &token, RsGxsGroupUpdateMeta&, RsGxsForumGroup &group);

};



#endif
