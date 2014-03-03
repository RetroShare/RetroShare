/*
 * libretroshare/src/gxs: rsgxnetutils.cc
 *
 * Helper objects for the operation rsgxsnetservice
 *
 * Copyright 2012-2013 by Christopher Evi-Parker
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

#include "rsgxsnetutils.h"
#include "retroshare/rspeers.h"


 const time_t AuthorPending::EXPIRY_PERIOD_OFFSET = 30; // 30 seconds
 const int AuthorPending::MSG_PEND = 1;
 const int AuthorPending::GRP_PEND = 2;


AuthorPending::AuthorPending(RsGixsReputation* rep, time_t timeStamp)
 : mRep(rep), mTimeStamp(timeStamp) {
}

AuthorPending::~AuthorPending()
{

}

bool AuthorPending::expired() const
{
	return time(NULL) > (mTimeStamp + EXPIRY_PERIOD_OFFSET);
}

bool AuthorPending::getAuthorRep(GixsReputation& rep,
                                 const RsGxsId& authorId, const RsPeerId& peerId)
{
	if(mRep->haveReputation(authorId))
	{
		return mRep->getReputation(authorId, rep);
	}

        std::list<RsPeerId> peers;
        peers.push_back(peerId);
        mRep->loadReputation(authorId, peers);
	return false;

}

MsgAuthEntry::MsgAuthEntry()
 : mPassedVetting(false) {}


GrpAuthEntry::GrpAuthEntry()
 : mPassedVetting(false) {}



MsgRespPending::MsgRespPending(RsGixsReputation* rep, const RsPeerId& peerId, const MsgAuthorV& msgAuthV, int cutOff)
 : AuthorPending(rep, time(NULL)), mPeerId(peerId), mMsgAuthV(msgAuthV), mCutOff(cutOff)
{
}

GrpRespPending::GrpRespPending(RsGixsReputation* rep, const RsPeerId& peerId, const GrpAuthorV& grpAuthV, int cutOff)
 : AuthorPending(rep, time(NULL)), mPeerId(peerId), mGrpAuthV(grpAuthV), mCutOff(cutOff)
{
}

int MsgRespPending::getType() const
{
	return MSG_PEND;
}

bool MsgRespPending::accepted()
{
	MsgAuthorV::iterator cit = mMsgAuthV.begin();
	MsgAuthorV::size_type count = 0;
	for(; cit != mMsgAuthV.end(); cit++)
	{
		MsgAuthEntry& entry = *cit;

		if(!entry.mPassedVetting)
		{
			GixsReputation rep;
                        if(getAuthorRep(rep, entry.mAuthorId, mPeerId))
			{
				if(rep.score > mCutOff)
				{
					entry.mPassedVetting = true;
					count++;
				}
			}

		}else
		{
			count++;
		}
	}

	return count == mMsgAuthV.size();
}

int GrpRespPending::getType() const
{
	return GRP_PEND;
}

bool GrpRespPending::accepted()
{
	GrpAuthorV::iterator cit = mGrpAuthV.begin();
	GrpAuthorV::size_type count = 0;
	for(; cit != mGrpAuthV.end(); cit++)
	{
		GrpAuthEntry& entry = *cit;

		if(!entry.mPassedVetting)
		{
			GixsReputation rep;

                        if(getAuthorRep(rep, entry.mAuthorId, mPeerId))
			{
				if(rep.score > mCutOff)
				{
					entry.mPassedVetting = true;
					count++;
				}
			}

		}else
		{
			count++;
		}
	}

	return count == mGrpAuthV.size();
}



/** NxsTransaction definition **/

const uint8_t NxsTransaction::FLAG_STATE_STARTING = 0x0001; // when
const uint8_t NxsTransaction::FLAG_STATE_RECEIVING = 0x0002; // begin receiving items for incoming trans
const uint8_t NxsTransaction::FLAG_STATE_SENDING = 0x0004; // begin sending items for outgoing trans
const uint8_t NxsTransaction::FLAG_STATE_COMPLETED = 0x008;
const uint8_t NxsTransaction::FLAG_STATE_FAILED = 0x0010;
const uint8_t NxsTransaction::FLAG_STATE_WAITING_CONFIRM = 0x0020;


NxsTransaction::NxsTransaction()
    : mFlag(0), mTimeOut(0), mTransaction(NULL) {

}

NxsTransaction::~NxsTransaction(){

	std::list<RsNxsItem*>::iterator lit = mItems.begin();

	for(; lit != mItems.end(); lit++)
	{
		delete *lit;
		*lit = NULL;
	}

	if(mTransaction)
		delete mTransaction;

	mTransaction = NULL;
}


/* Net Manager */

RsNxsNetMgrImpl::RsNxsNetMgrImpl(p3LinkMgr *lMgr)
    : mLinkMgr(lMgr), mNxsNetMgrMtx("RsNxsNetMgrImpl")
{

}


const RsPeerId& RsNxsNetMgrImpl::getOwnId()
{
    RsStackMutex stack(mNxsNetMgrMtx);
    return mLinkMgr->getOwnId();
}

void RsNxsNetMgrImpl::getOnlineList(std::set<RsPeerId> &ssl_peers)
{
    ssl_peers.clear();

    std::list<RsPeerId> pList;
    {
        RsStackMutex stack(mNxsNetMgrMtx);
        mLinkMgr->getOnlineList(pList);
    }

    std::list<RsPeerId>::const_iterator lit = pList.begin();

    for(; lit != pList.end(); lit++)
        ssl_peers.insert(*lit);
}

const time_t GrpCircleVetting::EXPIRY_PERIOD_OFFSET = 5; // 10 seconds
const int GrpCircleVetting::GRP_ID_PEND = 1;
const int GrpCircleVetting::GRP_ITEM_PEND = 2;
const int GrpCircleVetting::MSG_ID_SEND_PEND = 3;
const int GrpCircleVetting::MSG_ID_RECV_PEND = 3;


GrpIdCircleVet::GrpIdCircleVet(const RsGxsGroupId& grpId, const RsGxsCircleId& circleId)
 : mGroupId(grpId), mCircleId(circleId), mCleared(false) {}

GrpCircleVetting::GrpCircleVetting(RsGcxs* const circles)
 : mCircles(circles) {}

GrpCircleVetting::~GrpCircleVetting() {}

bool GrpCircleVetting::expired()
{
	return  time(NULL) > (mTimeStamp + EXPIRY_PERIOD_OFFSET);
}
bool GrpCircleVetting::canSend(const SSLIdType& peerId, const RsGxsCircleId& circleId)
{
	if(mCircles->isLoaded(circleId))
	{
		const RsPgpId& pgpId = rsPeers->getGPGId(peerId);
		return mCircles->canSend(circleId, pgpId);
	}

	mCircles->loadCircle(circleId);

	return false;
}

GrpCircleIdRequestVetting::GrpCircleIdRequestVetting(
		RsGcxs* const circles, std::vector<GrpIdCircleVet> grpCircleV, const RsPeerId& peerId)
 : GrpCircleVetting(circles), mGrpCircleV(grpCircleV), mPeerId(peerId) {}

bool GrpCircleIdRequestVetting::cleared()
{
	std::vector<GrpIdCircleVet>::size_type i, count;
	for(i = 0; i < mGrpCircleV.size(); i++)
	{
		GrpIdCircleVet& gic = mGrpCircleV[i];

		if(!gic.mCleared)
		{
			if(canSend(mPeerId, gic.mCircleId))
			{
				gic.mCleared = true;
				count++;
			}
		}
		else
		{
			count++;
		}

	}

	return count == mGrpCircleV.size();
}

int GrpCircleIdRequestVetting::getType() const
{
	return GRP_ID_PEND;
}

MsgIdCircleVet::MsgIdCircleVet(const RsGxsMessageId& msgId,
		const RsGxsId& authorId)
 : mMsgId(msgId), mAuthorId(authorId) {
}

MsgCircleIdsRequestVetting::MsgCircleIdsRequestVetting(RsGcxs* const circles,
		std::vector<MsgIdCircleVet> msgs, const RsGxsGroupId& grpId,
		const RsPeerId& peerId, const RsGxsCircleId& circleId)
: GrpCircleVetting(circles), mMsgs(msgs), mGrpId(grpId), mPeerId(peerId), mCircleId(circleId) {}

bool MsgCircleIdsRequestVetting::cleared()
{

	return canSend(mPeerId, mCircleId);

}

int MsgCircleIdsRequestVetting::getType() const
{
	return MSG_ID_SEND_PEND;
}

