#pragma once
/*******************************************************************************
 * libretroshare/src/retroshare: rsgxschannels.h                               *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012  Robert Fernie <retroshare@lunamutt.com>                 *
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

#include <inttypes.h>
#include <string>
#include <list>
#include <functional>

#include "retroshare/rstokenservice.h"
#include "retroshare/rsgxsifacehelper.h"
#include "retroshare/rsgxscommon.h"
#include "serialiser/rsserializable.h"
#include "retroshare/rsturtle.h"

class RsGxsChannels;

/**
 * Pointer to global instance of RsGxsChannels service implementation
 * @jsonapi{development}
 */
extern RsGxsChannels* rsGxsChannels;

// These should be in rsgxscommon.h
struct RsGxsChannelGroup : RsSerializable
{
	RsGroupMetaData mMeta;
	std::string mDescription;
	RsGxsImage mImage;

	bool mAutoDownload;

	/// @see RsSerializable
	virtual void serial_process( RsGenericSerializer::SerializeJob j,
	                             RsGenericSerializer::SerializeContext& ctx )
	{
		RS_SERIAL_PROCESS(mMeta);
		RS_SERIAL_PROCESS(mDescription);
		RS_SERIAL_PROCESS(mImage);
		RS_SERIAL_PROCESS(mAutoDownload);
	}
};

std::ostream &operator<<(std::ostream& out, const RsGxsChannelGroup& group);


struct RsGxsChannelPost : RsSerializable
{
	RsGxsChannelPost() : mCount(0), mSize(0) {}

	RsMsgMetaData mMeta;

	std::set<RsGxsMessageId> mOlderVersions;
	std::string mMsg;  // UTF8 encoded.

	std::list<RsGxsFile> mFiles;
	uint32_t mCount;   // auto calced.
	uint64_t mSize;    // auto calced.

	RsGxsImage mThumbnail;

	/// @see RsSerializable
	virtual void serial_process( RsGenericSerializer::SerializeJob j,
	                             RsGenericSerializer::SerializeContext& ctx )
	{
		RS_SERIAL_PROCESS(mMeta);
		RS_SERIAL_PROCESS(mOlderVersions);

		RS_SERIAL_PROCESS(mMsg);
		RS_SERIAL_PROCESS(mFiles);
		RS_SERIAL_PROCESS(mCount);
		RS_SERIAL_PROCESS(mSize);
		RS_SERIAL_PROCESS(mThumbnail);
	}
};

std::ostream &operator<<(std::ostream& out, const RsGxsChannelPost& post);


class RsGxsChannels: public RsGxsIfaceHelper, public RsGxsCommentService
{
public:

	explicit RsGxsChannels(RsGxsIface& gxs) : RsGxsIfaceHelper(gxs) {}
	virtual ~RsGxsChannels() {}

	/**
	 * @brief Get channels summaries list. Blocking API.
	 * @jsonapi{development}
	 * @param[out] channels list where to store the channels
	 * @return false if something failed, true otherwhise
	 */
	virtual bool getChannelsSummaries(std::list<RsGroupMetaData>& channels) = 0;

	/**
	 * @brief Get channels information (description, thumbnail...).
	 * Blocking API.
	 * @jsonapi{development}
	 * @param[in] chanIds ids of the channels of which to get the informations
	 * @param[out] channelsInfo storage for the channels informations
	 * @return false if something failed, true otherwhise
	 */
	virtual bool getChannelsInfo(
	        const std::list<RsGxsGroupId>& chanIds,
	        std::vector<RsGxsChannelGroup>& channelsInfo ) = 0;

	/**
	 * @brief Get content of specified channels. Blocking API
	 * @jsonapi{development}
	 * @param[in] chanIds id of the channels of which the content is requested
	 * @param[out] posts storage for the posts
	 * @param[out] comments storage for the comments
	 * @return false if something failed, true otherwhise
	 */
	virtual bool getChannelsContent(
	        const std::list<RsGxsGroupId>& chanIds,
	        std::vector<RsGxsChannelPost>& posts,
	        std::vector<RsGxsComment>& comments ) = 0;

	/**
	 * @brief Create channel. Blocking API.
	 * @jsonapi{development}
	 * @param[inout] channel Channel data (name, description...)
	 * @return false on error, true otherwise
	 */
	virtual bool createChannel(RsGxsChannelGroup& channel) = 0;

	/**
	 * @brief Create channel post. Blocking API.
	 * @jsonapi{development}
	 * @param[inout] post
	 * @return false on error, true otherwise
	 */
	virtual bool createPost(RsGxsChannelPost& post) = 0;


	/* Specific Service Data
	 * TODO: change the orrible const uint32_t &token to uint32_t token
	 * TODO: create a new typedef for token so code is easier to read
	 */

	virtual bool getGroupData(const uint32_t &token, std::vector<RsGxsChannelGroup> &groups) = 0;
	virtual bool getPostData(const uint32_t &token, std::vector<RsGxsChannelPost> &posts, std::vector<RsGxsComment> &cmts) = 0;
	virtual bool getPostData(const uint32_t &token, std::vector<RsGxsChannelPost> &posts) = 0;

	/**
	 * @brief toggle message read status
	 * @jsonapi{development}
	 * @param[out] token GXS token queue token
	 * @param[in] msgId
	 * @param[in] read
	 */
	virtual void setMessageReadStatus(
	        uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read) = 0;

	/**
	 * @brief Enable or disable auto-download for given channel
	 * @jsonapi{development}
	 * @param[in] groupId channel id
	 * @param[in] enable true to enable, false to disable
	 * @return false if something failed, true otherwhise
	 */
	virtual bool setChannelAutoDownload(
	        const RsGxsGroupId &groupId, bool enable) = 0;

	/**
	 * @brief Get auto-download option value for given channel
	 * @jsonapi{development}
	 * @param[in] groupId channel id
	 * @param[in] enabled storage for the auto-download option value
	 * @return false if something failed, true otherwhise
	 */
	virtual bool getChannelAutoDownload(
	        const RsGxsGroupId &groupId, bool& enabled) = 0;

	/**
	 * @brief Set download directory for the given channel
	 * @jsonapi{development}
	 * @param[in] channelId id of the channel
	 * @param[in] directory path
	 * @return false on error, true otherwise
	 */
	virtual bool setChannelDownloadDirectory(
	        const RsGxsGroupId& channelId, const std::string& directory) = 0;

	/**
	 * @brief Get download directory for the given channel
	 * @jsonapi{development}
	 * @param[in] channelId id of the channel
	 * @param[out] directory reference to string where to store the path
	 * @return false on error, true otherwise
	 */
	virtual bool getChannelDownloadDirectory( const RsGxsGroupId& channelId,
	                                          std::string& directory ) = 0;

	/**
	 * @brief Share channel publishing key
	 * This can be used to authorize other peers to post on the channel
	 * @jsonapi{development}
	 * @param[in] groupId Channel id
	 * @param[in] peers peers to which share the key
	 * @return false on error, true otherwise
	 */
	virtual bool groupShareKeys(
	        const RsGxsGroupId& groupId, const std::set<RsPeerId>& peers ) = 0;

	/**
	 * @brief Request subscription to a group.
	 * The action is performed asyncronously, so it could fail in a subsequent
	 * phase even after returning true.
	 * @jsonapi{development}
	 * @param[out] token Storage for RsTokenService token to track request
	 * status.
	 * @param[in] groupId Channel id
	 * @param[in] subscribe
	 * @return false on error, true otherwise
	 */
	virtual bool subscribeToGroup( uint32_t& token, const RsGxsGroupId &groupId,
	                               bool subscribe ) = 0;

	/**
	 * @brief Request channel creation.
	 * The action is performed asyncronously, so it could fail in a subsequent
	 * phase even after returning true.
	 * @param[out] token Storage for RsTokenService token to track request
	 * status.
	 * @param[in] group Channel data (name, description...)
	 * @return false on error, true otherwise
	 */
	virtual bool createGroup(uint32_t& token, RsGxsChannelGroup& group) = 0;

	/**
	 * @brief Request post creation.
	 * The action is performed asyncronously, so it could fail in a subsequent
	 * phase even after returning true.
	 * @param[out] token Storage for RsTokenService token to track request
	 * status.
	 * @param[in] post
	 * @return false on error, true otherwise
	 */
	virtual bool createPost(uint32_t& token, RsGxsChannelPost& post) = 0;

	/**
	 * @brief Request channel change.
	 * The action is performed asyncronously, so it could fail in a subsequent
	 * phase even after returning true.
	 * @jsonapi{development}
	 * @param[out] token Storage for RsTokenService token to track request
	 * status.
	 * @param[in] group Channel data (name, description...) with modifications
	 * @return false on error, true otherwise
	 */
	virtual bool updateGroup(uint32_t& token, RsGxsChannelGroup& group) = 0;

	/**
	 * @brief Share extra file
	 * Can be used to share extra file attached to a channel post
	 * @jsonapi{development}
	 * @param[in] path file path
	 * @return false on error, true otherwise
	 */
	virtual bool ExtraFileHash(const std::string& path) = 0;

	/**
	 * @brief Remove extra file from shared files
	 * @jsonapi{development}
	 * @param[in] hash hash of the file to remove
	 * @return false on error, true otherwise
	 */
	virtual bool ExtraFileRemove(const RsFileHash& hash) = 0;

	/**
	 * @brief Request remote channels search
	 * @jsonapi{development}
	 * @param[in] matchString string to look for in the search
	 * @param multiCallback function that will be called each time a search
	 * result is received
	 * @param[in] maxWait maximum wait time in seconds for search results
	 * @return false on error, true otherwise
	 */
	virtual bool turtleSearchRequest(
	        const std::string& matchString,
	        const std::function<void (const RsGxsGroupSummary& result)>& multiCallback,
	        rstime_t maxWait = 300 ) = 0;

	//////////////////////////////////////////////////////////////////////////////
    ///                     Distant synchronisation methods                    ///
    //////////////////////////////////////////////////////////////////////////////
    ///
	virtual TurtleRequestId turtleGroupRequest(const RsGxsGroupId& group_id)=0;
	virtual TurtleRequestId turtleSearchRequest(const std::string& match_string)=0;
	virtual bool retrieveDistantSearchResults(TurtleRequestId req, std::map<RsGxsGroupId, RsGxsGroupSummary> &results) =0;
	virtual bool clearDistantSearchResults(TurtleRequestId req)=0;
	virtual bool retrieveDistantGroup(const RsGxsGroupId& group_id,RsGxsChannelGroup& distant_group)=0;

	//////////////////////////////////////////////////////////////////////////////
};
