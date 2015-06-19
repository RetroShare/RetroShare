#ifndef RETROSHARE_GXS_RSPOSTED_GUI_INTERFACE_H
#define RETROSHARE_GXS_RSPOSTED_GUI_INTERFACE_H

/*
 * libretroshare/src/retroshare: rsposted.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2008-2012 by Robert Fernie, Christopher Evi-Parker
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
#include "retroshare/rsgxscommon.h"

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
#define RSPOSTED_PERIOD_DAY		4
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

	RsPosted(RsGxsIface* gxs) : RsGxsIfaceHelper(gxs) { return; }
virtual ~RsPosted() { return; }

	    /* Specific Service Data */
virtual bool getGroupData(const uint32_t &token, std::vector<RsPostedGroup> &groups) = 0;
virtual bool getPostData(const uint32_t &token, std::vector<RsPostedPost> &posts) = 0;
virtual bool getRelatedPosts(const uint32_t &token, std::vector<RsPostedPost> &posts) = 0;

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

	bool calculateScores(time_t ref_time);

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
};


#endif // RETROSHARE_GXS_RSPOSTED_GUI_INTERFACE_H
