/*******************************************************************************
 * libretroshare/src/services: p3posted.h                                      *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2013 Robert Fernie <retroshare@lunamutt.com>                 *
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

#ifndef P3_POSTED_SERVICE_HEADER
#define P3_POSTED_SERVICE_HEADER


#include "retroshare/rsposted.h"
#include "services/p3postbase.h"

#include <retroshare/rsidentity.h>

#include <map>
#include <string>
#include <list>

/* 
 *
 */

class p3Posted: public p3PostBase, public RsPosted
{
public:

	p3Posted(RsGeneralDataService* gds, RsNetworkExchangeService* nes, RsGixs* gixs);
	virtual RsServiceInfo getServiceInfo();

protected:

virtual void notifyChanges(std::vector<RsGxsNotify*>& changes)
{
	return p3PostBase::notifyChanges(changes);
}

	public:

#ifdef TO_REMOVE
virtual void receiveHelperChanges(std::vector<RsGxsNotify*>& changes)
{
	return RsGxsIfaceHelper::receiveChanges(changes);
}
#endif

	bool getBoardsInfo(const std::list<RsGxsGroupId>& boardsIds,
	        std::vector<RsPostedGroup>& groupsInfo ) override;

	bool getBoardAllContent(const RsGxsGroupId& groupId,
	                        std::vector<RsPostedPost>& posts,
	                        std::vector<RsGxsComment>& comments,
	                        std::vector<RsGxsVote>& votes ) override;

	bool getBoardContent(const RsGxsGroupId& groupId,
	                     const std::set<RsGxsMessageId>& contentsIds,
	                     std::vector<RsPostedPost>& posts,
	                     std::vector<RsGxsComment>& comments,
	                     std::vector<RsGxsVote>& votes ) override;

	bool getBoardsSummaries(std::list<RsGroupMetaData>& groupInfo) override;

	bool getBoardStatistics(const RsGxsGroupId& boardId,GxsGroupStatistic& stat) override;

	bool getBoardsServiceStatistics(GxsServiceStatistic& stat) override;

	bool editBoard(RsPostedGroup& board) override;

	bool createBoard(RsPostedGroup& board) override;

    bool voteForPost(bool up,const RsGxsGroupId& postGrpId,const RsGxsMessageId& postMsgId,const RsGxsId& voterId) override;

    bool setPostReadStatus(const RsGxsGrpMsgIdPair& msgId, bool read) override;

    virtual bool getGroupData(const uint32_t &token, std::vector<RsPostedGroup> &groups) override;
	virtual bool getPostData(const uint32_t &token, std::vector<RsPostedPost> &posts, std::vector<RsGxsComment> &cmts, std::vector<RsGxsVote> &vots) override;
	virtual bool getPostData(const uint32_t &token, std::vector<RsPostedPost> &posts, std::vector<RsGxsComment> &cmts) override;
	virtual bool getPostData(const uint32_t &token, std::vector<RsPostedPost> &posts) override;

//Not currently used
//virtual bool getRelatedPosts(const uint32_t &token, std::vector<RsPostedPost> &posts);

virtual bool createGroup(uint32_t &token, RsPostedGroup &group) override;
virtual bool createPost(uint32_t &token, RsPostedPost &post) override;

virtual bool updateGroup(uint32_t &token, RsPostedGroup &group) override;
virtual bool groupShareKeys(const RsGxsGroupId &group, const std::set<RsPeerId>& peers) override;

        //////////////////////////////////////////////////////////////////////////////
	// WRAPPERS due to the separate Interface.

virtual void setMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read) override
	{
		return p3PostBase::setMessageReadStatus(token, msgId, read);
	}

    virtual bool setCommentAsRead(uint32_t& token,const RsGxsGroupId& gid,const RsGxsMessageId& comment_msg_id) override
    {
        p3PostBase::setMessageReadStatus(token,RsGxsGrpMsgIdPair(gid,comment_msg_id),true);
        return true;
    }



	/** Comment service - Provide RsGxsCommentService -
	 * redirect to p3GxsCommentService */
    virtual bool getCommentData(uint32_t token, std::vector<RsGxsComment> &msgs) override
	{ return mCommentService->getGxsCommentData(token, msgs); }

	virtual bool getRelatedComments( uint32_t token,
                                     std::vector<RsGxsComment> &msgs ) override
	{ return mCommentService->getGxsRelatedComments(token, msgs); }

	virtual bool createNewComment(uint32_t &token, const RsGxsComment &msg) override
	{
		return mCommentService->createGxsComment(token, msg);
	}
	virtual bool createComment(RsGxsComment& msg) override
	{
        uint32_t token;

		return mCommentService->createGxsComment(token, msg) && waitToken(token) == RsTokenService::COMPLETE ;
	}

    virtual bool createNewVote(uint32_t &token, RsGxsVote &msg) override
	{
		return mCommentService->createGxsVote(token, msg);
	}

	virtual bool acknowledgeComment(
            uint32_t token, std::pair<RsGxsGroupId, RsGxsMessageId>& msgId ) override
	{ return acknowledgeMsg(token, msgId); }

	virtual bool acknowledgeVote(
            uint32_t token, std::pair<RsGxsGroupId, RsGxsMessageId>& msgId ) override
	{
		if (mCommentService->acknowledgeVote(token, msgId)) return true;
		return acknowledgeMsg(token, msgId);
	}
};

#endif 
