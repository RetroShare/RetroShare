/*******************************************************************************
 * libretroshare/src/retroshare: rsgxsforums.h                                 *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012-2014  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2018-2019  Gioacchino Mazzurco <gio@eigenlab.org>             *
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
#include "retroshare/rsgxscircles.h"


class RsGxsForums;

/**
 * Pointer to global instance of RsGxsChannels service implementation
 * @jsonapi{development}
 */
extern RsGxsForums* rsGxsForums;


/** Forum Service message flags, to be used in RsMsgMetaData::mMsgFlags
 * Gxs imposes to use the first two bytes (lower bytes) of mMsgFlags for
 * private forum flags, the upper bytes being used for internal GXS stuff.
 * @todo mixing service level flags and GXS level flag into the same member is
 * prone to confusion, use separated members for those things
 */
static const uint32_t RS_GXS_FORUM_MSG_FLAGS_MASK      = 0x0000000f;
static const uint32_t RS_GXS_FORUM_MSG_FLAGS_MODERATED = 0x00000001;

#define IS_FORUM_MSG_MODERATION(flags) (flags & RS_GXS_FORUM_MSG_FLAGS_MODERATED)


struct RsGxsForumGroup : RsSerializable
{
	/** Forum GXS metadata */
	RsGroupMetaData mMeta;

	/** @brief Forum desciption */
	std::string mDescription;

	/** @brief List of forum moderators ids
	 * @todo run away from TLV old serializables as those types are opaque to
	 * JSON API! */
	RsTlvGxsIdSet mAdminList;

	/** @brief List of forum pinned posts, those are usually displayed on top
	 * @todo run away from TLV old serializables as those types are opaque to
	 * JSON API! */
	RsTlvGxsMsgIdSet mPinnedPosts;

	/// @see RsSerializable
	virtual void serial_process(
	        RsGenericSerializer::SerializeJob j,
	        RsGenericSerializer::SerializeContext& ctx ) override;

	~RsGxsForumGroup() override;

	/* G10h4ck: We should avoid actual methods in this contexts as they are
	 * invisible to JSON API */
	bool canEditPosts(const RsGxsId& id) const;
};

struct RsGxsForumMsg : RsSerializable
{
	/** @brief Forum post GXS metadata */
	RsMsgMetaData mMeta;

	/** @brief Forum post content */
	std::string mMsg; 

	/// @see RsSerializable
	virtual void serial_process(
	        RsGenericSerializer::SerializeJob j,
	        RsGenericSerializer::SerializeContext& ctx ) override
	{
		RS_SERIAL_PROCESS(mMeta);
		RS_SERIAL_PROCESS(mMsg);
	}

	~RsGxsForumMsg() override;
};


class RsGxsForums: public RsGxsIfaceHelper
{
public:
	explicit RsGxsForums(RsGxsIface& gxs) : RsGxsIfaceHelper(gxs) {}
	virtual ~RsGxsForums();

	/**
	 * @brief Create forum.
	 * @jsonapi{development}
	 * @param[in]  name          Name of the forum
	 * @param[in]  description   Optional description of the forum
	 * @param[in]  authorId      Optional id of the froum owner author
	 * @param[in]  moderatorsIds Optional list of forum moderators
	 * @param[in]  circleType    Optional visibility rule, default public.
	 * @param[in]  circleId      If the forum is not public specify the id of
	 *                           the circle who can see the forum. Depending on
	 *                           the value you pass for circleType this should
	 *                           be a circle if EXTERNAL is passed, a local
	 *                           friends group id if NODES_GROUP is passed,
	 *                           empty otherwise.
	 * @param[out] forumId       Optional storage for the id of the created
	 *                           forum, meaningful only if creations succeeds.
	 * @param[out] errorMessage  Optional storage for error messsage, meaningful
	 *                           only if creation fail.
	 * @return False on error, true otherwise.
	 */
	virtual bool createForumV2(
	        const std::string& name, const std::string& description,
	        const RsGxsId& authorId = RsGxsId(),
	        const std::set<RsGxsId>& moderatorsIds = std::set<RsGxsId>(),
	        RsGxsCircleType circleType = RsGxsCircleType::PUBLIC,
	        const RsGxsCircleId& circleId = RsGxsCircleId(),
	        RsGxsGroupId& forumId = RS_DEFAULT_STORAGE_PARAM(RsGxsGroupId),
	        std::string& errorMessage = RS_DEFAULT_STORAGE_PARAM(std::string)
	        ) = 0;

	/**
	 * @brief Create a post on the given forum.
	 * @jsonapi{development}
	 * @param[in]  forumId   Id of the forum in which the post is to be
	 *                       submitted
	 * @param[in]  title     UTF-8 string containing the title of the post
	 * @param[in]  mBody     UTF-8 string containing the text of the post
	 * @param[in]  authorId  Id of the author of the comment
	 * @param[in]  parentId  Optional Id of the parent post if this post is a
	 *                       reply to another post, empty otherwise.
	 * @param[in]  origPostId  If this is supposed to replace an already
	 *                         existent post, the id of the old post.
	 *                         If left blank a new post will be created.
	 * @param[out] postMsgId Optional storage for the id of the created,
	 *                       meaningful only on success.
	 * @param[out] errorMessage Optional storage for error message, meaningful
	 *                          only on failure.
	 * @return false on error, true otherwise
	 */
	virtual bool createPost(
	        const RsGxsGroupId&   forumId,
	        const std::string&    title,
	        const std::string&    mBody,
	        const RsGxsId&        authorId,
	        const RsGxsMessageId& parentId = RsGxsMessageId(),
	        const RsGxsMessageId& origPostId = RsGxsMessageId(),
	        RsGxsMessageId&       postMsgId = RS_DEFAULT_STORAGE_PARAM(RsGxsMessageId),
	        std::string&          errorMessage     = RS_DEFAULT_STORAGE_PARAM(std::string)
	        ) = 0;

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
	        const std::set<RsGxsMessageId>& msgsIds,
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

	/// default base URL used for forums links @see exportForumLink
	static const std::string DEFAULT_FORUM_BASE_URL;

	/// Link query field used to store forum name @see exportForumLink
	static const std::string FORUM_URL_NAME_FIELD;

	/// Link query field used to store forum id @see exportForumLink
	static const std::string FORUM_URL_ID_FIELD;

	/// Link query field used to store forum data @see exportForumLink
	static const std::string FORUM_URL_DATA_FIELD;

	/** Link query field used to store forum message title
	 * @see exportChannelLink */
	static const std::string FORUM_URL_MSG_TITLE_FIELD;

	/// Link query field used to store forum message id @see exportChannelLink
	static const std::string FORUM_URL_MSG_ID_FIELD;

	/**
	 * @brief Get link to a forum
	 * @jsonapi{development}
	 * @param[out] link storage for the generated link
	 * @param[in] forumId Id of the forum of which we want to generate a link
	 * @param[in] includeGxsData if true include the forum GXS group data so
	 *	the receiver can subscribe to the forum even if she hasn't received it
	 *	through GXS yet
	 * @param[in] baseUrl URL into which to sneak in the RetroShare link
	 *	radix, this is primarly useful to induce applications into making the
	 *	link clickable, or to disguise the RetroShare link into a
	 *	"normal" looking web link. If empty the GXS data link will be outputted
	 *	in plain base64 format.
	 * @param[out] errMsg optional storage for error message, meaningful only in
	 *	case of failure
	 * @return false if something failed, true otherwhise
	 */
	virtual bool exportForumLink(
	        std::string& link, const RsGxsGroupId& forumId,
	        bool includeGxsData = true,
	        const std::string& baseUrl = RsGxsForums::DEFAULT_FORUM_BASE_URL,
	        std::string& errMsg = RS_DEFAULT_STORAGE_PARAM(std::string) ) = 0;

	/**
	 * @brief Import forum from full link
	 * @param[in] link forum link either in radix or URL format
	 * @param[out] forumId optional storage for parsed forum id
	 * @param[out] errMsg optional storage for error message, meaningful only in
	 *	case of failure
	 * @return false if some error occurred, true otherwise
	 */
	virtual bool importForumLink(
	        const std::string& link,
	        RsGxsGroupId& forumId = RS_DEFAULT_STORAGE_PARAM(RsGxsGroupId),
	        std::string& errMsg = RS_DEFAULT_STORAGE_PARAM(std::string) ) = 0;


	/**
	 * @brief Create forum. Blocking API.
	 * @jsonapi{development}
	 * @param[inout] forum Forum data (name, description...)
	 * @return false on error, true otherwise
	 * @deprecated @see createForumV2
	 */
	RS_DEPRECATED_FOR(createForumV2)
	virtual bool createForum(RsGxsForumGroup& forum) = 0;

	/**
	 * @brief Create forum message. Blocking API.
	 * @jsonapi{development}
	 * @param[inout] message
	 * @return false on error, true otherwise
	 * @deprecated @see createPost
	 */
	RS_DEPRECATED_FOR(createPost)
	virtual bool createMessage(RsGxsForumMsg& message) = 0;

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
