/*******************************************************************************
 * libretroshare/src/retroshare: rsgxsforums.h                                 *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012-2014  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2018-2021  Gioacchino Mazzurco <gio@eigenlab.org>             *
 * Copyright (C) 2019-2021  Asociación Civil Altermundi <info@altermundi.net>  *
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
#include <system_error>

#include "retroshare/rstokenservice.h"
#include "retroshare/rsgxsifacehelper.h"
#include "serialiser/rstlvidset.h"
#include "serialiser/rsserializable.h"
#include "retroshare/rsgxscircles.h"


class RsGxsForums;

/**
 * Pointer to global instance of RsGxsForums service implementation
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


struct RsGxsForumGroup : RsSerializable, RsGxsGenericGroupData
{
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


enum class RsForumEventCode: uint8_t
{
	UNKNOWN                  = 0x00,
	NEW_FORUM                = 0x01, /// emitted when new forum is received
	UPDATED_FORUM            = 0x02, /// emitted when existing forum is updated
	NEW_MESSAGE              = 0x03, /// new message reeived in a particular forum
	UPDATED_MESSAGE          = 0x04, /// existing message has been updated in a particular forum
	SUBSCRIBE_STATUS_CHANGED = 0x05, /// forum was subscribed or unsubscribed
	READ_STATUS_CHANGED      = 0x06, /// msg was read or marked unread
	STATISTICS_CHANGED       = 0x07, /// suppliers and how many messages they have changed
	MODERATOR_LIST_CHANGED   = 0x08, /// forum moderation list has changed.
	SYNC_PARAMETERS_UPDATED  = 0x0a, /// sync and storage times have changed
	PINNED_POSTS_CHANGED     = 0x0b, /// some posts where pinned or un-pinned
	DELETED_FORUM            = 0x0c, /// forum was deleted by cleaning
	DELETED_POST             = 0x0d,  /// Post deleted (usually by cleaning)

	/// Distant search result received
	DISTANT_SEARCH_RESULT    = 0x0e
};

struct RsGxsForumEvent: RsEvent
{
	RsGxsForumEvent()
	    : RsEvent(RsEventType::GXS_FORUMS),
	      mForumEventCode(RsForumEventCode::UNKNOWN) {}

	RsForumEventCode mForumEventCode;
	RsGxsGroupId mForumGroupId;
	RsGxsMessageId mForumMsgId;
	std::list<RsGxsId> mModeratorsAdded;
	std::list<RsGxsId> mModeratorsRemoved;

	///* @see RsEvent @see RsSerializable
	void serial_process(
	        RsGenericSerializer::SerializeJob j,
	        RsGenericSerializer::SerializeContext& ctx ) override
	{
		RsEvent::serial_process(j, ctx);
		RS_SERIAL_PROCESS(mForumEventCode);
		RS_SERIAL_PROCESS(mForumGroupId);
		RS_SERIAL_PROCESS(mForumMsgId);
		RS_SERIAL_PROCESS(mModeratorsAdded);
		RS_SERIAL_PROCESS(mModeratorsRemoved);
	}

	~RsGxsForumEvent() override;
};

/** This event is fired once distant search results are received */
struct RsGxsForumsDistantSearchEvent: RsEvent
{
	RsGxsForumsDistantSearchEvent():
	    RsEvent(RsEventType::GXS_FORUMS),
	    mForumEventCode(RsForumEventCode::DISTANT_SEARCH_RESULT) {}

	RsForumEventCode mForumEventCode;
	TurtleRequestId mSearchId;
	std::vector<RsGxsSearchResult> mSearchResults;

	///* @see RsEvent @see RsSerializable
	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx ) override
	{
		RsEvent::serial_process(j, ctx);

		RS_SERIAL_PROCESS(mForumEventCode);
		RS_SERIAL_PROCESS(mSearchId);
		RS_SERIAL_PROCESS(mSearchResults);
	}
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
     * @brief returns statistics for the forum service
	 * @jsonapi{development}
     * @param[out] stat     statistics struct
     * @return              false if the call fails
     */
	virtual bool getForumServiceStatistics(GxsServiceStatistic& stat) =0;

    /**
     * @brief returns statistics about a particular forum
	 * @jsonapi{development}
     * @param[in]  forumId  Id of the forum
     * @param[out] stat     statistics struct
     * @return              false when the object doesn't exist or when the timeout is reached requesting the data
     */
	virtual bool getForumStatistics(const RsGxsGroupId& forumId,GxsGroupStatistic& stat)=0;


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
	 * @brief Get specific list of messages from a single forum. Blocking API
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
	 * @see exportForumLink */
	static const std::string FORUM_URL_MSG_TITLE_FIELD;

	/// Link query field used to store forum message id @see exportForumLink
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
	 * @brief Get posts related to the given post.
	 * If the set is empty, nothing is returned.
	 * @jsonapi{development}
	 * @param[in] forumId id of the forum of which the content is requested
	 * @param[in] parentId id of the post of which child posts (aka replies)
	 *	are requested.
	 * @param[out] childPosts storage for the child posts
	 * @return Success or error details
	 */
	virtual std::error_condition getChildPosts(
	        const RsGxsGroupId& forumId, const RsGxsMessageId& parentId,
	        std::vector<RsGxsForumMsg>& childPosts ) = 0;

	/**
	 * @brief Set keep forever flag on a post so it is not deleted even if older
	 * then group maximum storage time
	 * @jsonapi{development}
	 * @param[in] forumId id of the forum of which the post pertain
	 * @param[in] postId id of the post on which to set the flag
	 * @param[in] keepForever true to set the flag, false to unset it
	 * @return Success or error details
	 */
	virtual std::error_condition setPostKeepForever(
	        const RsGxsGroupId& forumId, const RsGxsMessageId& postId,
	        bool keepForever ) = 0;

	/**
	 * @brief Get forum content summaries
	 * @jsonapi{development}
	 * @param[in] forumId id of the forum of which the content is requested
	 * @param[in] contentIds ids of requested contents, if empty summaries of
	 *	all messages are reqeusted
	 * @param[out] summaries storage for summaries
	 * @return success or error details if something failed
	 */
	virtual std::error_condition getContentSummaries(
	        const RsGxsGroupId& forumId,
	        const std::set<RsGxsMessageId>& contentIds,
	        std::vector<RsMsgMetaData>& summaries ) = 0;

	/**
	 * @brief Search the whole reachable network for matching forums and
	 *	posts
	 * @jsonapi{development}
	 * An @see RsGxsForumsDistantSearchEvent is emitted when matching results
	 *	arrives from the network
	 * @param[in] matchString string to search into the forum and posts
	 * @param[out] searchId storage for search id, useful to track search events
	 *	and retrieve search results
	 * @return success or error details
	 */
	virtual std::error_condition distantSearchRequest(
	        const std::string& matchString, TurtleRequestId& searchId ) = 0;

	/**
	 * @brief Search the local index for matching forums and posts
	 * @jsonapi{development}
	 * @param[in] matchString string to search into the index
	 * @param[out] searchResults storage for searchr esults
	 * @return success or error details
	 */
	virtual std::error_condition localSearch(
	        const std::string& matchString,
	        std::vector<RsGxsSearchResult>& searchResults ) = 0;


	////////////////////////////////////////////////////////////////////////////
	/* Following functions are deprecated and should not be considered a stable
	 * to use API */

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
	virtual bool updateGroup(uint32_t &token, const RsGxsForumGroup &group) = 0;
};
