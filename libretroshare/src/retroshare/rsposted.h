#ifndef RSPOSTED_H
#define RSPOSTED_H


/*
 * libretroshare/src/retroshare: rsposted.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2008-2012 by Robert Fernie, Christopher Evi-Parker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
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
#include "gxs/rstokenservice.h"
#include "gxs/rsgxsifaceimpl.h"

class RsPosted;
extern RsPosted *rsPosted;

/* The Main Interface Class - for information about your Peers */

class RsPostedGroup
{
        public:
        RsGroupMetaData mMeta;
        std::string mDescription;
        RsPostedGroup() { return; }
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

class RsPostedPost;
class RsPostedComment;
class RsPostedVote;

typedef std::map<RsGxsGroupId, std::vector<RsPostedPost> > PostedPostResult;
typedef std::map<RsGxsGroupId, std::vector<RsPostedComment> > PostedCommentResult;
typedef std::map<RsGxsGroupId, std::vector<RsPostedVote> > PostedVoteResult;
typedef std::map<RsGxsGrpMsgIdPair, std::vector<RsPostedComment> > PostedRelatedCommentResult;
typedef std::pair<RsGxsGroupId, int32_t> GroupRank;
typedef std::map<RsGxsMessageId, uint32_t> PostedRanking;

std::ostream &operator<<(std::ostream &out, const RsPostedGroup &group);
std::ostream &operator<<(std::ostream &out, const RsPostedPost &post);
std::ostream &operator<<(std::ostream &out, const RsPostedVote &vote);
std::ostream &operator<<(std::ostream &out, const RsPostedComment &comment);



class RsPosted : public RsGxsIfaceImpl
{
        public:

    enum RankType {TopRankType, BestRankType, NewRankType };

    static const uint32_t FLAG_MSGTYPE_POST;
    static const uint32_t FLAG_MSGTYPE_VOTE;
    static const uint32_t FLAG_MSGTYPE_COMMENT;
    static const uint32_t FLAG_MSGTYPE_MASK;


    RsPosted(RsGenExchange* gxs) : RsGxsIfaceImpl(gxs) { return; }
virtual ~RsPosted() { return; }

        /* Specific Service Data */

    virtual bool getGroup(const uint32_t &token, std::vector<RsPostedGroup> &group) = 0;
    virtual bool getPost(const uint32_t &token, PostedPostResult &post) = 0;
    virtual bool getComment(const uint32_t &token, PostedCommentResult &comment) = 0;
    virtual bool getRelatedComment(const uint32_t& token, PostedRelatedCommentResult& comments) = 0;
    virtual bool getRanking(const uint32_t& token, PostedRanking& ranking) = 0;

    virtual bool submitGroup(uint32_t &token, RsPostedGroup &group) = 0;
    virtual bool submitPost(uint32_t &token, RsPostedPost &post) = 0;
    virtual bool submitVote(uint32_t &token, RsPostedVote &vote) = 0;
    virtual bool submitComment(uint32_t &token, RsPostedComment &comment) = 0;

    virtual bool retrieveScores(const std::string& serviceString, uint32_t& upVotes, uint32_t& downVotes, uint32_t& nComments) const = 0;

        // Special Ranking Request.
    /*!
     * Makes request for posts of a topic
     * @param token
     * @param rType
     * @param groupId
     */
    virtual bool requestMessageRankings(uint32_t &token, const RankType& rType, const RsGxsGroupId& groupId) = 0;

    /*!
     * Makes request for ranking of comments for a post
     * @param token
     * @param rType type of ranking to collect
     * @param msgId message id of post as groupid-messageid pair
     */
    virtual bool requestCommentRankings(uint32_t &token, const RankType& rType, const RsGxsGrpMsgIdPair& msgId) = 0;

};

class RsPostedPost
{
        public:
        RsPostedPost()
        {
            mMeta.mMsgFlags = RsPosted::FLAG_MSGTYPE_POST;
            mMeta.mServiceString = " 0 0 0";
            return;
        }

        RsMsgMetaData mMeta;
        std::string mLink;
        std::string mNotes;
};

class RsGxsPostedVoteItem;

class RsPostedVote
{
public:

    RsPostedVote(const RsGxsPostedVoteItem&);
    RsPostedVote()
    {
        mMeta.mMsgFlags = RsPosted::FLAG_MSGTYPE_VOTE;
            return;
    }
    uint8_t mDirection;
    RsMsgMetaData mMeta;
};

class RsGxsPostedCommentItem;

class RsPostedComment
{
        public:
        RsPostedComment()
        {
            mMeta.mMsgFlags = RsPosted::FLAG_MSGTYPE_COMMENT;
                return;
        }

        RsPostedComment(const RsGxsPostedCommentItem& );
        std::string mComment;
        RsMsgMetaData mMeta;
};

#endif // RSPOSTED_H
