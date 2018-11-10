/*******************************************************************************
 * libretroshare/src/retroshare: rsgxsforums.h                                 *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012 by Robert Fernie <retroshare@lunamutt.com>               *
 * Copyright (C) 2018  Gioacchino Mazzurco <gio@eigenlab.org>                  *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#pragma once

#include <cstdint>
#include <string>
#include <list>

#include "retroshare/rstokenservice.h"
#include "retroshare/rsgxsifacehelper.h"
#include "serialiser/rstlvidset.h"
#include "serialiser/rsserializable.h"


/* The Main Interface Class - for information about your Peers */
class RsGxsForums;

/**
 * Pointer to global instance of RsGxsChannels service implementation
 * @jsonapi{development}
 */
extern RsGxsForums* rsGxsForums;


/** Forum Service message flags, to be used in RsMsgMetaData::mMsgFlags
 *  Gxs imposes to use the first two bytes (lower bytes) of mMsgFlags for
 * private forum flags, the upper bytes being used for internal GXS stuff.
 */
static const uint32_t RS_GXS_FORUM_MSG_FLAGS_MASK      = 0x0000000f;
static const uint32_t RS_GXS_FORUM_MSG_FLAGS_MODERATED = 0x00000001;

#define IS_FORUM_MSG_MODERATION(flags) (flags & RS_GXS_FORUM_MSG_FLAGS_MODERATED)


struct RsGxsForumGroup : RsSerializable
{
	RsGroupMetaData mMeta;
	std::string mDescription;

	/* What's below is optional, and handled by the serialiser
	 * TODO: run away from TLV old serializables as those types are opaque to
	 *	JSON API! */
	RsTlvGxsIdSet mAdminList;
	RsTlvGxsMsgIdSet mPinnedPosts;

	/// @see RsSerializable
	virtual void serial_process( RsGenericSerializer::SerializeJob j,
	                             RsGenericSerializer::SerializeContext& ctx )
	{
		RS_SERIAL_PROCESS(mMeta);
		RS_SERIAL_PROCESS(mDescription);
		RS_SERIAL_PROCESS(mAdminList);
		RS_SERIAL_PROCESS(mPinnedPosts);
	}
};

struct RsGxsForumMsg : RsSerializable
{
	RsMsgMetaData mMeta;
	std::string mMsg; 

	/// @see RsSerializable
	virtual void serial_process( RsGenericSerializer::SerializeJob j,
	                             RsGenericSerializer::SerializeContext& ctx )
	{
		RS_SERIAL_PROCESS(mMeta);
		RS_SERIAL_PROCESS(mMsg);
	}
};


class RsGxsForums: public RsGxsIfaceHelper
{
public:
	explicit RsGxsForums(RsGxsIface& gxs) : RsGxsIfaceHelper(gxs) {}
	virtual ~RsGxsForums() {}

	/**
	 * @brief Create forum. Blocking API.
	 * @jsonapi{development}
	 * @param[inout] forum Forum data (name, description...)
	 * @return false on error, true otherwise
	 */
	virtual bool createForum(RsGxsForumGroup& forum) = 0;

	/**
	 * @brief Create forum message. Blocking API.
	 * @jsonapi{development}
	 * @param[inout] message
	 * @return false on error, true otherwise
	 */
	virtual bool createMessage(RsGxsForumMsg& message) = 0;


	/* Specific Service Data */
	virtual bool getGroupData(const uint32_t &token, std::vector<RsGxsForumGroup> &groups) = 0;
	virtual bool getMsgData(const uint32_t &token, std::vector<RsGxsForumMsg> &msgs) = 0;
	//Not currently used
	//virtual bool getRelatedMessages(const uint32_t &token, std::vector<RsGxsForumMsg> &msgs) = 0;

	//////////////////////////////////////////////////////////////////////////////
	virtual void setMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read) = 0;

	//virtual bool setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask);
	//virtual bool setGroupSubscribeFlags(const std::string &groupId, uint32_t subscribeFlags, uint32_t subscribeMask);

	//virtual bool groupRestoreKeys(const std::string &groupId);
	//virtual bool groupShareKeys(const std::string &groupId, std::list<std::string>& peers);

	RS_DEPRECATED_FOR(createForum)
	virtual bool createGroup(uint32_t &token, RsGxsForumGroup &group) = 0;

	RS_DEPRECATED_FOR(createMessage)
	virtual bool createMsg(uint32_t &token, RsGxsForumMsg &msg) = 0;

	/*!
 * To update forum group with new information
 * @param token the token used to check completion status of update
 * @param group group to be updated, groupId element must be set or will be rejected
 * @return false groupId not set, true if set and accepted (still check token for completion)
 */
	virtual bool updateGroup(uint32_t &token, RsGxsForumGroup &group) = 0;
};

