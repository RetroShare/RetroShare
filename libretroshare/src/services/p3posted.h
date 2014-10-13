/*
 * libretroshare/src/services: p3posted.h
 *
 * GxsChannel interface for RetroShare.
 *
 * Copyright 2012-2013 by Robert Fernie.
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

virtual void receiveHelperChanges(std::vector<RsGxsNotify*>& changes)
{
	return RsGxsIfaceHelper::receiveChanges(changes);
}

	// Posted Specific DataTypes.
virtual bool getGroupData(const uint32_t &token, std::vector<RsPostedGroup> &groups);
virtual bool getPostData(const uint32_t &token, std::vector<RsPostedPost> &posts);
virtual bool getRelatedPosts(const uint32_t &token, std::vector<RsPostedPost> &posts);

virtual bool createGroup(uint32_t &token, RsPostedGroup &group);
virtual bool createPost(uint32_t &token, RsPostedPost &post);

virtual bool updateGroup(uint32_t &token, RsPostedGroup &group);
virtual bool groupShareKeys(const RsGxsGroupId &group, const std::list<RsPeerId>& peers);

        //////////////////////////////////////////////////////////////////////////////
	// WRAPPERS due to the separate Interface.

virtual void setMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read)
	{
		return p3PostBase::setMessageReadStatus(token, msgId, read);
	}


	/* Comment service - Provide RsGxsCommentService - redirect to p3GxsCommentService */
virtual bool getCommentData(const uint32_t &token, std::vector<RsGxsComment> &msgs)
	{
        	return mCommentService->getGxsCommentData(token, msgs);
	}

virtual bool getRelatedComments(const uint32_t &token, std::vector<RsGxsComment> &msgs)
	{
		return mCommentService->getGxsRelatedComments(token, msgs);
	}

virtual bool createComment(uint32_t &token, RsGxsComment &msg)
	{
		return mCommentService->createGxsComment(token, msg);
	}

virtual bool createVote(uint32_t &token, RsGxsVote &msg)
	{
		return mCommentService->createGxsVote(token, msg);
	}

virtual bool acknowledgeComment(const uint32_t& token, std::pair<RsGxsGroupId, RsGxsMessageId>& msgId)
	{
		return acknowledgeMsg(token, msgId);
	}

virtual bool acknowledgeVote(const uint32_t& token, std::pair<RsGxsGroupId, RsGxsMessageId>& msgId)
	{
		if (mCommentService->acknowledgeVote(token, msgId))
		{
			return true;
		}
		return acknowledgeMsg(token, msgId);
	}
};

#endif 
