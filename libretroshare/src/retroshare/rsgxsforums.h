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
    virtual ~RsGxsForumGroup() {}

	RsGroupMetaData mMeta;
	std::string mDescription;
	std::string mColor;

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
		RS_SERIAL_PROCESS(mColor);
	}

    // utility functions

    bool canEditPosts(const RsGxsId& id) const { return mAdminList.ids.find(id) != mAdminList.ids.end() || id == mMeta.mAuthorId; }
};

struct RsGxsForumMsg : RsSerializable
{
    virtual ~RsGxsForumMsg() {}

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

	/**
	 * @brief Edit forum details.
	 * @jsonapi{development}
	 * @param[in] forum Forum data (name, description...) with modifications
	 * @return false on error, true otherwise
	 */
	virtual bool editForum(RsGxsForumGroup& forum) = 0;

	/**
	 * @brief Get forums summaries list. Blocking API.
	 * @jsonapi{development}
	 * @param[out] forums list where to store the forums summaries
	 * @return false if something failed, true otherwhise
	 */
	virtual bool getForumsSummaries(std::list<RsGroupMetaData>& forums) = 0;

	/**
	 * @brief Get forums information (description, thumbnail...).
	 * Blocking API.
	 * @jsonapi{development}
	 * @param[in] forumIds ids of the forums of which to get the informations
	 * @param[out] forumsInfo storage for the forums informations
	 * @return false if something failed, true otherwhise
	 */
	virtual bool getForumsInfo(
	        const std::list<RsGxsGroupId>& forumIds,
	        std::vector<RsGxsForumGroup>& forumsInfo ) = 0;

	/**
	 * @brief Get message metadatas for a specific forum. Blocking API
	 * @jsonapi{development}
	 * @param[in] forumId id of the forum of which the content is requested
	 * @param[out] msgMetas storage for the forum messages meta data
	 * @return false if something failed, true otherwhise
	 */
	virtual bool getForumMsgMetaData( const RsGxsGroupId& forumId,
	                                  std::vector<RsMsgMetaData>& msgMetas) = 0;

	/**
	 * @brief Get specific list of messages from a single forums. Blocking API
	 * @jsonapi{development}
	 * @param[in] forumId id of the forum of which the content is requested
	 * @param[in] msgsIds list of message ids to request
	 * @param[out] msgs storage for the forum messages
	 * @return false if something failed, true otherwhise
	 */
	virtual bool getForumContent(
	        const RsGxsGroupId& forumId,
	        std::set<RsGxsMessageId>& msgsIds,
	        std::vector<RsGxsForumMsg>& msgs) = 0;

	/**
	 * @brief Toggle message read status. Blocking API.
	 * @jsonapi{development}
	 * @param[in] messageId post identifier
	 * @param[in] read true to mark as read, false to mark as unread
	 * @return false on error, true otherwise
	 */
	virtual bool markRead(const RsGxsGrpMsgIdPair& messageId, bool read) = 0;

	/**
	 * @brief Subscrbe to a forum. Blocking API
	 * @jsonapi{development}
	 * @param[in] forumId Forum id
	 * @param[in] subscribe true to subscribe, false to unsubscribe
	 * @return false on error, true otherwise
	 */
	virtual bool subscribeToForum( const RsGxsGroupId& forumId,
	                               bool subscribe ) = 0;

	/* Specific Service Data */
	RS_DEPRECATED_FOR("getForumsSummaries, getForumsInfo")
	virtual bool getGroupData(const uint32_t &token, std::vector<RsGxsForumGroup> &groups) = 0;
	RS_DEPRECATED_FOR(getForumContent)
	virtual bool getMsgData(const uint32_t &token, std::vector<RsGxsForumMsg> &msgs) = 0;
	RS_DEPRECATED_FOR(markRead)
	virtual void setMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read) = 0;
	RS_DEPRECATED_FOR(createForum)
	virtual bool createGroup(uint32_t &token, RsGxsForumGroup &group) = 0;
	RS_DEPRECATED_FOR(createMessage)
	virtual bool createMsg(uint32_t &token, RsGxsForumMsg &msg) = 0;
	RS_DEPRECATED_FOR(editForum)
	virtual bool updateGroup(uint32_t &token, RsGxsForumGroup &group) = 0;
};

