/*******************************************************************************
 * libretroshare/src/retroshare: rsposted.h                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2008-2012  Robert Fernie, Christopher Evi-Parker              *
 * Copyright (C) 2020  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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

#include <inttypes.h>
#include <string>
#include <list>
#include <functional>

#include "retroshare/rstokenservice.h"
#include "retroshare/rsgxsifacehelper.h"
#include "retroshare/rsgxscommon.h"
#include "serialiser/rsserializable.h"

class RsPosted;

/**
 * Pointer to global instance of RsGxsChannels service implementation
 * @jsonapi{development}
 */
extern RsPosted* rsPosted;

struct RsPostedGroup: RsGxsGenericGroupData
{
	std::string mDescription;
	RsGxsImage mGroupImage;
};

struct RsPostedPost
{
	RsPostedPost(): mHaveVoted(false), mUpVotes(0), mDownVotes(0), mComments(0),
	    mHotScore(0), mTopScore(0), mNewScore(0) {}

	bool calculateScores(rstime_t ref_time);

	RsMsgMetaData mMeta;
	std::string mLink;
	std::string mNotes;

	bool     mHaveVoted;

	// Calculated.
	uint32_t mUpVotes;
	uint32_t mDownVotes;
	uint32_t mComments;


	// and Calculated Scores:???
	double  mHotScore;
	double  mTopScore;
	double  mNewScore;

	RsGxsImage mImage;

	/// @see RsSerializable
	/*virtual void serial_process( RsGenericSerializer::SerializeJob j,
								 RsGenericSerializer::SerializeContext& ctx )
	{
		RS_SERIAL_PROCESS(mImage);
		RS_SERIAL_PROCESS(mMeta);
		RS_SERIAL_PROCESS(mLink);
		RS_SERIAL_PROCESS(mHaveVoted);
		RS_SERIAL_PROCESS(mUpVotes);
		RS_SERIAL_PROCESS(mDownVotes);
		RS_SERIAL_PROCESS(mComments);
		RS_SERIAL_PROCESS(mHotScore);
		RS_SERIAL_PROCESS(mTopScore);
		RS_SERIAL_PROCESS(mNewScore);
	}*/
};


//#define RSPOSTED_MSGTYPE_POST		0x0001
//#define RSPOSTED_MSGTYPE_VOTE		0x0002
//#define RSPOSTED_MSGTYPE_COMMENT	0x0004

#define RSPOSTED_PERIOD_YEAR		1
#define RSPOSTED_PERIOD_MONTH		2
#define RSPOSTED_PERIOD_WEEK		3
#define RSPOSTED_PERIOD_DAY			4
#define RSPOSTED_PERIOD_HOUR		5

#define RSPOSTED_VIEWMODE_LATEST	1
#define RSPOSTED_VIEWMODE_TOP		2
#define RSPOSTED_VIEWMODE_HOT		3
#define RSPOSTED_VIEWMODE_COMMENTS	4


enum class RsPostedEventCode: uint8_t
{
	UNKNOWN                  = 0x00,
	NEW_POSTED_GROUP         = 0x01,
	NEW_MESSAGE              = 0x02,
	SUBSCRIBE_STATUS_CHANGED = 0x03,
	UPDATED_POSTED_GROUP     = 0x04,
	UPDATED_MESSAGE          = 0x05,
	READ_STATUS_CHANGED      = 0x06,
};


struct RsGxsPostedEvent: RsEvent
{
	RsGxsPostedEvent():
	    RsEvent(RsEventType::GXS_POSTED),
	    mPostedEventCode(RsPostedEventCode::UNKNOWN) {}

	RsPostedEventCode mPostedEventCode;
	RsGxsGroupId mPostedGroupId;
	RsGxsMessageId mPostedMsgId;

	///* @see RsEvent @see RsSerializable
	void serial_process( RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx) override
	{
		RsEvent::serial_process(j, ctx);
		RS_SERIAL_PROCESS(mPostedEventCode);
		RS_SERIAL_PROCESS(mPostedGroupId);
		RS_SERIAL_PROCESS(mPostedMsgId);
	}

	~RsGxsPostedEvent() override;
};

class RsPosted : public RsGxsIfaceHelper, public RsGxsCommentService
{
public:
	explicit RsPosted(RsGxsIface& gxs) : RsGxsIfaceHelper(gxs) {}

	virtual bool getBoardsInfo(
	        const std::list<RsGxsGroupId>& boardsIds,
	        std::vector<RsPostedGroup>& boardsInfo ) = 0;

	virtual bool getBoardsSummaries(std::list<RsGroupMetaData>& groupInfo) =0;

	virtual bool getBoardContent(
	        const RsGxsGroupId& boardId,
	        const std::set<RsGxsMessageId>& contentsIds,
	        std::vector<RsPostedPost>& posts,
	        std::vector<RsGxsComment>& comments ) = 0;

	virtual bool editBoard(RsPostedGroup& board) =0;

	virtual bool createBoard(RsPostedGroup& board) =0;

	virtual bool getBoardStatistics(const RsGxsGroupId& boardId,GxsGroupStatistic& stat) =0;

	enum RS_DEPRECATED RankType {TopRankType, HotRankType, NewRankType };

	RS_DEPRECATED_FOR(getBoardsInfo)
	virtual bool getGroupData( const uint32_t& token,
	                           std::vector<RsPostedGroup> &groups ) = 0;

	RS_DEPRECATED_FOR(getBoardsContent)
	virtual bool getPostData(
	        const uint32_t& token, std::vector<RsPostedPost>& posts,
	        std::vector<RsGxsComment>& cmts) = 0;

	RS_DEPRECATED_FOR(getBoardsContent)
	virtual bool getPostData(
	        const uint32_t& token, std::vector<RsPostedPost>& posts) = 0;

	//Not currently used
//virtual bool getRelatedPosts(const uint32_t &token, std::vector<RsPostedPost> &posts) = 0;

	    /* From RsGxsCommentService */
//virtual bool getCommentData(const uint32_t &token, std::vector<RsGxsComment> &comments) = 0;
//virtual bool getRelatedComments(const uint32_t &token, std::vector<RsGxsComment> &comments) = 0;
//virtual bool createNewComment(uint32_t &token, RsGxsComment &comment) = 0;
//virtual bool createNewVote(uint32_t &token, RsGxsVote &vote) = 0;

        //////////////////////////////////////////////////////////////////////////////
virtual void setMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read) = 0;

virtual bool createGroup(uint32_t &token, RsPostedGroup &group) = 0;
virtual bool createPost(uint32_t &token, RsPostedPost &post) = 0;

virtual bool updateGroup(uint32_t &token, RsPostedGroup &group) = 0;

    virtual bool groupShareKeys(const RsGxsGroupId& group,const std::set<RsPeerId>& peers) = 0 ;

	virtual ~RsPosted();
};
