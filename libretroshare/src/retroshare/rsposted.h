/*******************************************************************************
 * libretroshare/src/retroshare: rsposted.h                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2008-2012 by Robert Fernie, Christopher Evi-Parker                *
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
#ifndef RETROSHARE_GXS_RSPOSTED_GUI_INTERFACE_H
#define RETROSHARE_GXS_RSPOSTED_GUI_INTERFACE_H

#include <inttypes.h>
#include <string>
#include <list>
#include <functional>

#include "retroshare/rstokenservice.h"
#include "retroshare/rsgxsifacehelper.h"
#include "retroshare/rsgxscommon.h"
#include "serialiser/rsserializable.h"

/* The Main Interface Class - for information about your Posted */
class RsPosted;
extern RsPosted *rsPosted;

class RsPostedPost;
class RsPostedGroup
{
	public:
	RsPostedGroup() { return; }

	RsGroupMetaData mMeta;
	std::string mDescription;

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


std::ostream &operator<<(std::ostream &out, const RsPostedGroup &group);
std::ostream &operator<<(std::ostream &out, const RsPostedPost &post);


class RsPosted : public RsGxsIfaceHelper, public RsGxsCommentService
{
	    public:

	enum RankType {TopRankType, HotRankType, NewRankType };

	//static const uint32_t FLAG_MSGTYPE_POST;
	//static const uint32_t FLAG_MSGTYPE_MASK;

	explicit RsPosted(RsGxsIface& gxs) : RsGxsIfaceHelper(gxs) {}
	virtual ~RsPosted() {}

	    /* Specific Service Data */
virtual bool getGroupData(const uint32_t &token, std::vector<RsPostedGroup> &groups) = 0;
virtual bool getPostData(const uint32_t &token, std::vector<RsPostedPost> &posts, std::vector<RsGxsComment> &cmts) = 0;
virtual bool getPostData(const uint32_t &token, std::vector<RsPostedPost> &posts) = 0;
//Not currently used
//virtual bool getRelatedPosts(const uint32_t &token, std::vector<RsPostedPost> &posts) = 0;

	    /* From RsGxsCommentService */
//virtual bool getCommentData(const uint32_t &token, std::vector<RsGxsComment> &comments) = 0;
//virtual bool getRelatedComments(const uint32_t &token, std::vector<RsGxsComment> &comments) = 0;
//virtual bool createComment(uint32_t &token, RsGxsComment &comment) = 0;
//virtual bool createVote(uint32_t &token, RsGxsVote &vote) = 0;

        //////////////////////////////////////////////////////////////////////////////
virtual void setMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read) = 0;

virtual bool createGroup(uint32_t &token, RsPostedGroup &group) = 0;
virtual bool createPost(uint32_t &token, RsPostedPost &post) = 0;

virtual bool updateGroup(uint32_t &token, RsPostedGroup &group) = 0;

    virtual bool groupShareKeys(const RsGxsGroupId& group,const std::set<RsPeerId>& peers) = 0 ;
};


class RsPostedPost
{
	public:
	RsPostedPost()
	{
		//mMeta.mMsgFlags = RsPosted::FLAG_MSGTYPE_POST;
		mUpVotes = 0;
		mDownVotes = 0;
		mComments = 0;
		mHaveVoted = false;

        mHotScore = 0;
        mTopScore = 0;
        mNewScore = 0;
	}

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


#endif // RETROSHARE_GXS_RSPOSTED_GUI_INTERFACE_H
