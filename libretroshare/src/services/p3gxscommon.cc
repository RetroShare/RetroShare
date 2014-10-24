/*
 * libretroshare/src/services p3gxscommon.cc
 *
 * GxsChannels interface for RetroShare.
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

#include "retroshare/rsgxscommon.h"
#include "services/p3gxscommon.h"
#include "serialiser/rsgxscommentitems.h"
#include "util/rsstring.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


RsGxsComment::RsGxsComment()
{
	mUpVotes = 0;
	mDownVotes = 0;
	mScore = 0;
	mOwnVote = 0;
}

/********************************************************************************/

RsGxsImage::RsGxsImage()
{
	std::cerr << "RsGxsImage(" << this << ")";
	std::cerr << std::endl;
	mData = NULL;
	mSize = 0;
}

RsGxsImage::RsGxsImage(const RsGxsImage& a)
{
	std::cerr << "RsGxsImage(" << this << ") = RsGxsImage(" << (void *) &a << ")";
	std::cerr << std::endl;
	mData = NULL;
	mSize = 0;
	copy(a.mData, a.mSize);
}
	

RsGxsImage::~RsGxsImage()
{
	std::cerr << "~RsGxsImage(" << this << ")";
	std::cerr << std::endl;
	clear();
}


RsGxsImage &RsGxsImage::operator=(const RsGxsImage &a)
{
	copy(a.mData, a.mSize);
	return *this;
}


void RsGxsImage::take(uint8_t *data, uint32_t size)
{
	std::cerr << "RsGxsImage(" << this << ")::take(" << (void *) data << "," << size << ")";
	std::cerr << std::endl;
	// Copies Pointer.
	clear();
	mData = data;
	mSize = size;
}

// NB Must make sure that we always use malloc/free for this data.
uint8_t *RsGxsImage::allocate(uint32_t size)
{
	uint8_t *val = (uint8_t *) malloc(size);
	std::cerr << "RsGxsImage()::allocate(" << (void *) val << ")";
	std::cerr << std::endl;
	return val;
}

void RsGxsImage::release(void *data)
{
	std::cerr << "RsGxsImage()::release(" << (void *) data << ")";
	std::cerr << std::endl;
	free(data);
}

void RsGxsImage::copy(uint8_t *data, uint32_t size)
{
	std::cerr << "RsGxsImage(" << this << ")::copy(" << (void *) data << "," << size << ")";
	std::cerr << std::endl;
	// Allocates and Copies.
	clear(); 
	if (data && size)
	{
		mData = allocate(size);
		memcpy(mData, data, size);
		mSize = size;
	}
}

void RsGxsImage::clear()
{
	// Frees.
	if (mData)
	{
		release(mData);	       
	}
	mData = NULL;
	mSize = 0;
}

void RsGxsImage::shallowClear()
{
	// Clears Pointer.
	mData = NULL;
	mSize = 0;
}

/********************************************************************************/


RsGxsFile::RsGxsFile()
{
	mSize = 0;
}

RsGxsVote::RsGxsVote()
{
	mVoteType = 0;
}

/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

#define GXSCOMMENTS_VOTE_CHECK		0x0002
#define GXSCOMMENTS_VOTE_DONE		0x0003

p3GxsCommentService::p3GxsCommentService(RsGenExchange *exchange, uint16_t service_type)
    : GxsTokenQueue(exchange), mExchange(exchange), mServiceType(service_type)
{
	return;
}

void p3GxsCommentService::comment_tick()
{
        GxsTokenQueue::checkRequests();
}

bool p3GxsCommentService::getGxsCommentData(const uint32_t &token, std::vector<RsGxsComment> &comments)
{
	std::cerr << "p3GxsCommentService::getGxsCommentData()";
	std::cerr << std::endl;

	GxsMsgDataMap msgData;
	bool ok = mExchange->getMsgData(token, msgData);
		
	if(ok)
	{
		GxsMsgDataMap::iterator mit = msgData.begin();
		std::multimap<RsGxsMessageId, RsGxsVoteItem *> voteMap;
		
		for(; mit != msgData.end();  ++mit)
		{
			RsGxsGroupId grpId = mit->first;
			std::vector<RsGxsMsgItem*>& msgItems = mit->second;
			std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();

			/* now split into Comments and Votes */
		
			for(; vit != msgItems.end(); ++vit)
			{
				RsGxsCommentItem* item = dynamic_cast<RsGxsCommentItem*>(*vit);
		
				if(item)
				{
					RsGxsComment comment = item->mMsg;
					comment.mMeta = item->meta;
					comments.push_back(comment);
					delete item;
				}
				else
				{
					RsGxsVoteItem* vote = dynamic_cast<RsGxsVoteItem*>(*vit);
					if (vote)
					{
						voteMap.insert(std::make_pair(vote->meta.mParentId, vote));
					}
					else
					{
						std::cerr << "Not a Comment or Vote, deleting!" << std::endl;
						delete *vit;
					}
				}
			}
		}

		/* now iterate through comments - and set the vote counts */
		std::vector<RsGxsComment>::iterator cit;
		std::multimap<RsGxsMessageId, RsGxsVoteItem *>::iterator it;
		for(cit = comments.begin(); cit != comments.end(); ++cit)
		{
			for (it = voteMap.lower_bound(cit->mMeta.mMsgId); it != voteMap.upper_bound(cit->mMeta.mMsgId); ++it)
			{
				if (it->second->mMsg.mVoteType == GXS_VOTE_UP)
				{
					cit->mUpVotes++;
				}
				else
				{
					cit->mDownVotes++;
				}
			}
			cit->mScore = calculateBestScore(cit->mUpVotes, cit->mDownVotes);

			/* convert Status -> mHaveVoted */
			if (cit->mMeta.mMsgStatus & GXS_SERV::GXS_MSG_STATUS_VOTE_MASK)
			{
				if (cit->mMeta.mMsgStatus & GXS_SERV::GXS_MSG_STATUS_VOTE_UP)
				{
					cit->mOwnVote = GXS_VOTE_UP;
				}
				else
				{
					cit->mOwnVote = GXS_VOTE_DOWN;
				}
			}
			else
			{
				cit->mOwnVote = GXS_VOTE_NONE;
			}
		}

		std::cerr << "p3GxsCommentService::getGxsCommentData() Found " << comments.size() << " Comments";
		std::cerr << std::endl;
		std::cerr << "p3GxsCommentService::getGxsCommentData() Found " << voteMap.size() << " Votes";
		std::cerr << std::endl;

		/* delete the votes */
		for (it = voteMap.begin(); it != voteMap.end(); ++it)
		{
			delete it->second;
		}


		
	}
		
	return ok;
}




bool p3GxsCommentService::getGxsRelatedComments(const uint32_t &token, std::vector<RsGxsComment> &comments)
{
	std::cerr << "p3GxsCommentService::getGxsRelatedComments()";
	std::cerr << std::endl;

	GxsMsgRelatedDataMap msgData;
	bool ok = mExchange->getMsgRelatedData(token, msgData);
			
	if(ok)
	{
		GxsMsgRelatedDataMap::iterator mit = msgData.begin();
		std::multimap<RsGxsMessageId, RsGxsVoteItem *> voteMap;
		
		for(; mit != msgData.end();  ++mit)
		{
			std::vector<RsGxsMsgItem*>& msgItems = mit->second;
			std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();
			
			for(; vit != msgItems.end(); ++vit)
			{
				RsGxsCommentItem* item = dynamic_cast<RsGxsCommentItem*>(*vit);
		
				if(item)
				{
					RsGxsComment comment = item->mMsg;
					comment.mMeta = item->meta;
					comments.push_back(comment);
					delete item;
				}
				else
				{
					RsGxsVoteItem* vote = dynamic_cast<RsGxsVoteItem*>(*vit);
					if (vote)
					{
						voteMap.insert(std::make_pair(vote->meta.mParentId, vote));
					}
					else
					{
						std::cerr << "Not a Comment or Vote, deleting!" << std::endl;
						delete *vit;
					}
				}
			}
		}

		/* now iterate through comments - and set the vote counts */
		std::vector<RsGxsComment>::iterator cit;
		std::multimap<RsGxsMessageId, RsGxsVoteItem *>::iterator it;
		for(cit = comments.begin(); cit != comments.end(); ++cit)
		{
			for (it = voteMap.lower_bound(cit->mMeta.mMsgId); it != voteMap.upper_bound(cit->mMeta.mMsgId); ++it)
			{
				if (it->second->mMsg.mVoteType == GXS_VOTE_UP)
				{
					cit->mUpVotes++;
				}
				else
				{
					cit->mDownVotes++;
				}
			}
			cit->mScore = calculateBestScore(cit->mUpVotes, cit->mDownVotes);

			/* convert Status -> mHaveVoted */
			if (cit->mMeta.mMsgStatus & GXS_SERV::GXS_MSG_STATUS_VOTE_MASK)
			{
				if (cit->mMeta.mMsgStatus & GXS_SERV::GXS_MSG_STATUS_VOTE_UP)
				{
					cit->mOwnVote = GXS_VOTE_UP;
				}
				else
				{
					cit->mOwnVote = GXS_VOTE_DOWN;
				}
			}
			else
			{
				cit->mOwnVote = GXS_VOTE_NONE;
			}
		}

		std::cerr << "p3GxsCommentService::getGxsRelatedComments() Found " << comments.size() << " Comments";
		std::cerr << std::endl;
		std::cerr << "p3GxsCommentService::getGxsRelatedComments() Found " << voteMap.size() << " Votes";
		std::cerr << std::endl;

		/* delete the votes */
		for (it = voteMap.begin(); it != voteMap.end(); ++it)
		{
			delete it->second;
		}
	}
			
	return ok;
}



double p3GxsCommentService::calculateBestScore(int upVotes, int downVotes)
{
	static float z = 1.0;

	float score;
	int n = upVotes - downVotes;
	if(n==0)
	{
		score = 0.0;
	}
	else
	{
		float phat = upVotes;
		score = sqrt(phat+z*z/(2*n)-z*((phat*(1-phat)+z*z/(4*n))/n))/(1+z*z/n);
	}
	return score;
}



/********************************************************************************************/

bool p3GxsCommentService::createGxsComment(uint32_t &token, RsGxsComment &msg)
{
	std::cerr << "p3GxsCommentService::createGxsComment() GroupId: " << msg.mMeta.mGroupId;
	std::cerr << std::endl;

	RsGxsCommentItem* msgItem = new RsGxsCommentItem(mServiceType);
	msgItem->mMsg = msg;
	msgItem->meta = msg.mMeta;
	
	mExchange->publishMsg(token, msgItem);
	return true;
}



bool p3GxsCommentService::createGxsVote(uint32_t &token, RsGxsVote &vote)
{
	// NOTE Because we cannot do this operation immediately, we create a token, 
	// and monitor acknowledgeTokenMsg ... to return correct answer.

	std::cerr << "p3GxsCommentService::createGxsVote() GroupId: " << vote.mMeta.mGroupId;
	std::cerr << std::endl;

	/* vote must be associated with another item */
	if (vote.mMeta.mThreadId.isNull())
	{
		std::cerr << "p3GxsCommentService::createGxsVote() ERROR Missing Required ThreadId";
		std::cerr << std::endl;
		return false;
	}

	if (vote.mMeta.mParentId.isNull())
	{
		std::cerr << "p3GxsCommentService::createGxsVote() ERROR Missing Required ParentId";
		std::cerr << std::endl;
		return false;
	}

	if (vote.mMeta.mGroupId.isNull())
	{
		std::cerr << "p3GxsCommentService::createGxsVote() ERROR Missing Required GroupId";
		std::cerr << std::endl;
		return false;
	}

	if (vote.mMeta.mAuthorId.isNull())
	{
		std::cerr << "p3GxsCommentService::createGxsVote() ERROR Missing Required AuthorId";
		std::cerr << std::endl;
		return false;
	}


	/* now queue */

	RsGxsGrpMsgIdPair parentId(vote.mMeta.mGroupId, vote.mMeta.mParentId);

	std::map<RsGxsGrpMsgIdPair, VoteHolder>::iterator it;
	it = mPendingVotes.find(parentId);
	if (it != mPendingVotes.end())
	{
		std::cerr << "p3GxsCommentService::createGxsVote() ERROR Already a pending vote!";
		std::cerr << std::endl;
		return false;
	}

	token = mExchange->generatePublicToken();
	mPendingVotes[parentId] = VoteHolder(vote, token);

	// request parent, and queue for response.
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_META;

	GxsMsgReq msgIds;
	std::vector<RsGxsMessageId> &vect_msgIds = msgIds[parentId.first];
	vect_msgIds.push_back(parentId.second);

	uint32_t int_token;
	mExchange->getTokenService()->requestMsgInfo(int_token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, msgIds);
	GxsTokenQueue::queueRequest(int_token, GXSCOMMENTS_VOTE_CHECK);

	return true;
}







void p3GxsCommentService::load_PendingVoteParent(const uint32_t &token)
{
        std::cerr << "p3GxsCommentService::load_PendingVoteParent()";
	std::cerr << std::endl;
	GxsMsgMetaMap msginfo;
        if (!mExchange->getMsgMeta(token, msginfo))
	{
                std::cerr << "p3GxsCommentService::load_PendingVoteParent() ERROR Fetching Data";
		std::cerr << std::endl;
                std::cerr << "p3GxsCommentService::load_PendingVoteParent() Warning - this means LOST QUEUE ENTRY";
		std::cerr << std::endl;
                std::cerr << "p3GxsCommentService::load_PendingVoteParent() Need to track tokens - if this happens";
		std::cerr << std::endl;
                return;
        }

	GxsMsgMetaMap::iterator it;
	for(it = msginfo.begin(); it != msginfo.end(); ++it)
	{
		std::vector<RsMsgMetaData>::iterator mit;
		for(mit = it->second.begin(); mit != it->second.end(); ++mit)
		{
			/* find the matching Pending Vote */
			RsMsgMetaData &meta = *mit;

                	std::cerr << "p3GxsCommentService::load_PendingVoteParent() recv (groupId: " << meta.mGroupId;
			std::cerr << ", msgId: " << meta.mMsgId << ")";
			std::cerr << std::endl;

			RsGxsGrpMsgIdPair parentId(meta.mGroupId, meta.mMsgId);
			std::map<RsGxsGrpMsgIdPair, VoteHolder>::iterator pit;
			pit = mPendingVotes.find(parentId);
			if (pit == mPendingVotes.end())
			{
                		std::cerr << "p3GxsCommentService::load_PendingVoteParent() ERROR Finding Pending Vote";
				std::cerr << std::endl;
				continue;
			}

			RsGxsVote vote = pit->second.mVote;
			if (meta.mMsgStatus & GXS_SERV::GXS_MSG_STATUS_VOTE_MASK)
			{
                		std::cerr << "p3GxsCommentService::load_PendingVoteParent() ERROR Already Voted";
				std::cerr << std::endl;
                		std::cerr << "mGroupId: " << meta.mGroupId;
				std::cerr << std::endl;
                		std::cerr << "mMsgId: " << meta.mMsgId;
				std::cerr << std::endl;

				pit->second.mStatus = VoteHolder::VOTE_ERROR;
				uint32_t status = RsTokenService::GXS_REQUEST_V2_STATUS_FAILED;
				mExchange->updatePublicRequestStatus(pit->second.mReqToken, status);
				continue;
			}

                	std::cerr << "p3GxsCommentService::load_PendingVoteParent() submitting Vote";
			std::cerr << std::endl;

			uint32_t status_token;
			if (vote.mVoteType == GXS_VOTE_UP)
			{
				mExchange->setMsgStatusFlags(status_token, parentId, 
						GXS_SERV::GXS_MSG_STATUS_VOTE_UP, GXS_SERV::GXS_MSG_STATUS_VOTE_MASK);
			}
			else
			{
				mExchange->setMsgStatusFlags(status_token, parentId, 
						GXS_SERV::GXS_MSG_STATUS_VOTE_DOWN, GXS_SERV::GXS_MSG_STATUS_VOTE_MASK);
			}

			uint32_t vote_token;
			castVote(vote_token, vote);
			GxsTokenQueue::queueRequest(vote_token, GXSCOMMENTS_VOTE_DONE);

			pit->second.mVoteToken = vote_token;
			pit->second.mStatus = VoteHolder::VOTE_SUBMITTED;
		}
	}
}



void p3GxsCommentService::completeInternalVote(uint32_t &token)
{
	std::cerr << "p3GxsCommentService::completeInternalVote() token: " << token;
	std::cerr << std::endl;
	std::map<RsGxsGrpMsgIdPair, VoteHolder>::iterator it;
	for (it = mPendingVotes.begin(); it != mPendingVotes.end(); ++it)
	{
		if (it->second.mVoteToken == token)
		{

			uint32_t status = mExchange->getTokenService()->requestStatus(token);
			mExchange->updatePublicRequestStatus(it->second.mReqToken, status);

			std::cerr << "p3GxsCommentService::completeInternalVote() Matched to PendingVote. status: " << status;
			std::cerr << std::endl;

			it->second.mStatus = VoteHolder::VOTE_READY;
			return;
		}
	}

	std::cerr << "p3GxsCommentService::completeInternalVote() ERROR Failed to match PendingVote";
	std::cerr << std::endl;

	return;
}


bool p3GxsCommentService::acknowledgeVote(const uint32_t& token, RsGxsGrpMsgIdPair& msgId)
{
	std::cerr << "p3GxsCommentService::acknowledgeVote() token: " << token;
	std::cerr << std::endl;

	std::map<RsGxsGrpMsgIdPair, VoteHolder>::iterator it;
	for (it = mPendingVotes.begin(); it != mPendingVotes.end(); ++it)
	{
		if (it->second.mReqToken == token)
		{
			std::cerr << "p3GxsCommentService::acknowledgeVote() Matched to PendingVote";
			std::cerr << std::endl;

			bool ans = false;
			if (it->second.mStatus == VoteHolder::VOTE_READY)
			{
				std::cerr << "p3GxsCommentService::acknowledgeVote() PendingVote = READY";
				std::cerr << std::endl;

				// Finally finish this Vote off.
				ans = mExchange->acknowledgeTokenMsg(it->second.mVoteToken, msgId);
			}
			else if (it->second.mStatus == VoteHolder::VOTE_ERROR)
			{
				std::cerr << "p3GxsCommentService::acknowledgeVote() PendingVote = ERROR ???";
				std::cerr << std::endl;
			}
			else
			{
				std::cerr << "p3GxsCommentService::acknowledgeVote() PendingVote = OTHER STATUS";
				std::cerr << std::endl;
			}

			std::cerr << "p3GxsCommentService::acknowledgeVote() cleanup token & PendingVote";
			std::cerr << std::endl;
			mExchange->disposeOfPublicToken(it->second.mReqToken);
			mPendingVotes.erase(it);

			return ans;
		}
	}

	std::cerr << "p3GxsCommentService::acknowledgeVote() Failed to match PendingVote";
	std::cerr << std::endl;

	return false;
}


        // Overloaded from GxsTokenQueue for Request callbacks.
void p3GxsCommentService::handleResponse(uint32_t token, uint32_t req_type)
{
        std::cerr << "p3GxsCommentService::handleResponse(" << token << "," << req_type << ")";
        std::cerr << std::endl;

        // stuff.
        switch(req_type)
        {
		case GXSCOMMENTS_VOTE_CHECK:
			load_PendingVoteParent(token);
			break;

		case GXSCOMMENTS_VOTE_DONE:
			completeInternalVote(token);
			break;

                default:
                        /* error */
                        std::cerr << "p3GxsCommentService::handleResponse() Unknown Request Type: " << req_type;
                        std::cerr << std::endl;
                        break;
        }
}



	

bool p3GxsCommentService::castVote(uint32_t &token, RsGxsVote &msg)
{
	std::cerr << "p3GxsCommentService::castVote() GroupId: " << msg.mMeta.mGroupId;
	std::cerr << std::endl;

	RsGxsVoteItem* msgItem = new RsGxsVoteItem(mServiceType);
	msgItem->mMsg = msg;
	msgItem->meta = msg.mMeta;
	
	mExchange->publishMsg(token, msgItem);
	return true;
}







/********************************************************************************************/
/********************************************************************************************/

#if 0
void p3GxsCommentService::setMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read)
{
	uint32_t mask = GXS_SERV::GXS_MSG_STATUS_UNREAD | GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;
	uint32_t status = GXS_SERV::GXS_MSG_STATUS_UNREAD;
	if (read)
	{
		status = 0;
	}

	setMsgStatusFlags(token, msgId, status, mask);

}

#endif

