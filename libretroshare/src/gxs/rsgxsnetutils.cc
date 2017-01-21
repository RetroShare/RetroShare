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
#include "pqi/p3servicecontrol.h"
#include "pgp/pgpauxutils.h"

 const time_t AuthorPending::EXPIRY_PERIOD_OFFSET = 30; // 30 seconds
 const int AuthorPending::MSG_PEND = 1;
 const int AuthorPending::GRP_PEND = 2;

AuthorPending::AuthorPending(RsGixsReputation* rep, time_t timeStamp) : mRep(rep), mTimeStamp(timeStamp) {}

AuthorPending::~AuthorPending() {}

bool AuthorPending::expired() const
{
	return time(NULL) > (mTimeStamp + EXPIRY_PERIOD_OFFSET);
}

bool AuthorPending::getAuthorRep(GixsReputation& rep, const RsGxsId& authorId, const RsPeerId& /*peerId*/)
{
    rep.id = authorId ;
    rep.reputation_level = mRep->overallReputationLevel(authorId);

#warning can it happen that reputations do not have the info yet?
    return true ;
#ifdef TO_BE_REMOVED
	{
		return mRep->getReputation(authorId, rep);
	}

        std::list<RsPeerId> peers;
        peers.push_back(peerId);
        mRep->loadReputation(authorId, peers);
	return false;
#endif
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
	for(; cit != mMsgAuthV.end(); ++cit)
	{
		MsgAuthEntry& entry = *cit;

		if(!entry.mPassedVetting)
		{
			GixsReputation rep;
                        if(getAuthorRep(rep, entry.mAuthorId, mPeerId))
			{
				if(rep.reputation_level >= (uint32_t)mCutOff)
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
	for(; cit != mGrpAuthV.end(); ++cit)
	{
		GrpAuthEntry& entry = *cit;

		if(!entry.mPassedVetting)
		{
			GixsReputation rep;

                        if(getAuthorRep(rep, entry.mAuthorId, mPeerId))
			{
				if(rep.reputation_level >= (uint32_t)mCutOff)
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

const uint8_t NxsTransaction::FLAG_STATE_STARTING        = 0x0001; // when
const uint8_t NxsTransaction::FLAG_STATE_RECEIVING       = 0x0002; // begin receiving items for incoming trans
const uint8_t NxsTransaction::FLAG_STATE_SENDING         = 0x0004; // begin sending items for outgoing trans
const uint8_t NxsTransaction::FLAG_STATE_FAILED          = 0x0010;
const uint8_t NxsTransaction::FLAG_STATE_WAITING_CONFIRM = 0x0020;
const uint8_t NxsTransaction::FLAG_STATE_COMPLETED       = 0x0080;	// originaly 0x008, but probably a typo, but we cannot change it since it would break backward compatibility.


NxsTransaction::NxsTransaction()
    : mFlag(0), mTimeOut(0), mTransaction(NULL) {

}

NxsTransaction::~NxsTransaction(){

	std::list<RsNxsItem*>::iterator lit = mItems.begin();

	for(; lit != mItems.end(); ++lit)
	{
		delete *lit;
		*lit = NULL;
	}

	if(mTransaction)
		delete mTransaction;

	mTransaction = NULL;
}


/* Net Manager */

RsNxsNetMgrImpl::RsNxsNetMgrImpl(p3ServiceControl *sc)
    : mServiceCtrl(sc)
{

}


const RsPeerId& RsNxsNetMgrImpl::getOwnId()
{
    return mServiceCtrl->getOwnId();
}

void RsNxsNetMgrImpl::getOnlineList(const uint32_t serviceId, std::set<RsPeerId> &ssl_peers)
{
    mServiceCtrl->getPeersConnected(serviceId, ssl_peers);
}

const time_t GrpCircleVetting::EXPIRY_PERIOD_OFFSET = 5; // 10 seconds
const int GrpCircleVetting::GRP_ID_PEND = 1;
const int GrpCircleVetting::GRP_ITEM_PEND = 2;
const int GrpCircleVetting::MSG_ID_SEND_PEND = 3;
const int GrpCircleVetting::MSG_ID_RECV_PEND = 3;


GrpIdCircleVet::GrpIdCircleVet(const RsGxsGroupId& grpId, const RsGxsCircleId& circleId, const RsGxsId& authId)
 : mGroupId(grpId), mCircleId(circleId), mAuthorId(authId), mCleared(false) {}

GrpCircleVetting::GrpCircleVetting(RsGcxs* const circles, PgpAuxUtils *pgpUtils)
 : mCircles(circles), mPgpUtils(pgpUtils), mTimeStamp(time(NULL)) {}

GrpCircleVetting::~GrpCircleVetting() {}

bool GrpCircleVetting::expired()
{
	return  time(NULL) > (mTimeStamp + EXPIRY_PERIOD_OFFSET);
}
bool GrpCircleVetting::canSend(const SSLIdType& peerId, const RsGxsCircleId& circleId,bool& should_encrypt)
{
	if(mCircles->isLoaded(circleId))
	{
		const RsPgpId& pgpId = mPgpUtils->getPGPId(peerId);
		return mCircles->canSend(circleId, pgpId,should_encrypt);
	}

	mCircles->loadCircle(circleId);

	return false;
}

GrpCircleIdRequestVetting::GrpCircleIdRequestVetting(
		RsGcxs* const circles, 
		PgpAuxUtils *pgpUtils,
		std::vector<GrpIdCircleVet> grpCircleV, const RsPeerId& peerId)
 : GrpCircleVetting(circles, pgpUtils), mGrpCircleV(grpCircleV), mPeerId(peerId) {}

bool GrpCircleIdRequestVetting::cleared()
{
    std::vector<GrpIdCircleVet>::size_type i, count=0;
	for(i = 0; i < mGrpCircleV.size(); ++i)
	{
		GrpIdCircleVet& gic = mGrpCircleV[i];

		if(!gic.mCleared)
		{
			if(canSend(mPeerId, gic.mCircleId,gic.mShouldEncrypt))
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
		PgpAuxUtils *pgpUtils,
		std::vector<MsgIdCircleVet> msgs, const RsGxsGroupId& grpId,
		const RsPeerId& peerId, const RsGxsCircleId& circleId)
: GrpCircleVetting(circles, pgpUtils), mMsgs(msgs), mGrpId(grpId), mPeerId(peerId), mCircleId(circleId) {}

bool MsgCircleIdsRequestVetting::cleared()
{
    if(!mCircles->isLoaded(mCircleId))
    {
	    mCircles->loadCircle(mCircleId);
	    return false ;
    }
    
    for(uint32_t i=0;i<mMsgs.size();)
        if(!mCircles->isRecipient(mCircleId,mGrpId,mMsgs[i].mAuthorId))
        {
            std::cerr << "(WW) MsgCircleIdsRequestVetting::cleared() filtering out message " << mMsgs[i].mMsgId << " because it's signed by author " << mMsgs[i].mAuthorId << " which is not in circle " << mCircleId << std::endl;
            
            mMsgs[i] = mMsgs[mMsgs.size()-1] ;
            mMsgs.pop_back();
        }
        else
            ++i ;
    
    RsPgpId pgpId = mPgpUtils->getPGPId(mPeerId);
    bool can_send_res = mCircles->canSend(mCircleId, pgpId,mShouldEncrypt);
    
    if(mShouldEncrypt)		// that means the circle is external
        return true ;
    else
        return can_send_res ;
}

int MsgCircleIdsRequestVetting::getType() const
{
	return MSG_ID_SEND_PEND;
}


