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
	return mTimeStamp > (time(NULL) + EXPIRY_PERIOD_OFFSET);
}

bool AuthorPending::getAuthorRep(GixsReputation& rep,
		const std::string& authorId)
{
	if(mRep->haveReputation(authorId))
	{
		return mRep->getReputation(authorId, rep);
	}

	mRep->loadReputation(authorId);
	return false;

}

MsgAuthEntry::MsgAuthEntry()
 : mPassedVetting(false) {}


GrpAuthEntry::GrpAuthEntry()
 : mPassedVetting(false) {}



MsgRespPending::MsgRespPending(RsGixsReputation* rep, const std::string& peerId, const MsgAuthorV& msgAuthV, int cutOff)
 : AuthorPending(rep, time(NULL)), mPeerId(peerId), mMsgAuthV(msgAuthV), mCutOff(cutOff)
{
}

GrpRespPending::GrpRespPending(RsGixsReputation* rep, const std::string& peerId, const GrpAuthorV& grpAuthV, int cutOff)
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

			if(getAuthorRep(rep, entry.mAuthorId))
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

			if(getAuthorRep(rep, entry.mAuthorId))
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

	delete mTransaction;
	mTransaction = NULL;
}


/* Net Manager */

RsNxsNetMgrImpl::RsNxsNetMgrImpl(p3LinkMgr *lMgr)
    : mLinkMgr(lMgr), mNxsNetMgrMtx("RsNxsNetMgrImpl")
{

}


std::string RsNxsNetMgrImpl::getOwnId()
{
    RsStackMutex stack(mNxsNetMgrMtx);
    return mLinkMgr->getOwnId();
}

void RsNxsNetMgrImpl::getOnlineList(std::set<std::string> &ssl_peers)
{
    ssl_peers.clear();

    std::list<std::string> pList;
    {
        RsStackMutex stack(mNxsNetMgrMtx);
        mLinkMgr->getOnlineList(pList);
    }

    std::list<std::string>::const_iterator lit = pList.begin();

    for(; lit != pList.end(); lit++)
        ssl_peers.insert(*lit);
}

