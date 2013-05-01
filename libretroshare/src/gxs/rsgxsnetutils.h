/*
 * libretroshare/src/gxs: rsgxnetutils.h
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

#ifndef RSGXSNETUTILS_H_
#define RSGXSNETUTILS_H_

#include <stdlib.h>
#include "retroshare/rsgxsifacetypes.h"
#include "pqi/p3linkmgr.h"
#include "serialiser/rsnxsitems.h"
#include "rsgixs.h"


/*!
 * This represents a transaction made
 * with the NxsNetService in all states
 * of operation until completion
 *
 */
class NxsTransaction
{

public:

    static const uint8_t FLAG_STATE_STARTING; // when
    static const uint8_t FLAG_STATE_RECEIVING; // begin receiving items for incoming trans
    static const uint8_t FLAG_STATE_SENDING; // begin sending items for outgoing trans
    static const uint8_t FLAG_STATE_COMPLETED;
    static const uint8_t FLAG_STATE_FAILED;
    static const uint8_t FLAG_STATE_WAITING_CONFIRM;

    NxsTransaction();
    ~NxsTransaction();

    uint32_t mFlag; // current state of transaction
    uint32_t mTimeOut;

    /*!
     * this contains who we
     * c what peer this transaction involves.
     * c The type of transaction
     * c transaction id
     * c timeout set for this transaction
     * c and itemCount
     */
    RsNxsTransac* mTransaction;
    std::list<RsNxsItem*> mItems; // items received or sent
};

/*!
 * An abstraction of the net manager
 * for retrieving Rs peers whom you will be synchronising
 * and also you own Id
 * Useful for testing also (abstracts away Rs's p3NetMgr)
 */
class RsNxsNetMgr
{

public:

	virtual ~RsNxsNetMgr(){};
    virtual std::string getOwnId() = 0;
    virtual void getOnlineList(std::set<std::string>& ssl_peers) = 0;

};

class RsNxsNetMgrImpl : public RsNxsNetMgr
{

public:

    RsNxsNetMgrImpl(p3LinkMgr* lMgr);
    virtual ~RsNxsNetMgrImpl(){};

    std::string getOwnId();
    void getOnlineList(std::set<std::string>& ssl_peers);

private:

    p3LinkMgr* mLinkMgr;
    RsMutex mNxsNetMgrMtx;

};

class AuthorPending
{
public:

	static const int MSG_PEND;
	static const int GRP_PEND;
	static const time_t EXPIRY_PERIOD_OFFSET;

	AuthorPending(RsGixsReputation* rep, time_t timeStamp);
	virtual ~AuthorPending();
	virtual int getType() const = 0 ;

	/*!
	 * @return true if all authors pass vetting and their messages
	 *         should be requested
	 */
	virtual bool accepted() = 0;

	/*!
	 * @return true if message is past set expiry date
	 */
	bool expired() const;

protected:

	/*!
	 * Convenience function to get author reputation
	 * @param rep reputation of author
	 * @param authorId reputation to get
	 * @return true if successfully retrieve repution
	 */
	bool getAuthorRep(GixsReputation& rep, const std::string& authorId);

private:

	RsGixsReputation* mRep;
	time_t mTimeStamp;
};

class MsgAuthEntry
{

public:

	MsgAuthEntry();

	RsGxsMessageId mMsgId;
	RsGxsGroupId mGrpId;
	std::string mAuthorId;
	bool mPassedVetting;

};

class GrpAuthEntry
{

public:

	GrpAuthEntry();

	RsGxsGroupId mGrpId;
	std::string mAuthorId;
	bool mPassedVetting;
};

typedef std::vector<MsgAuthEntry> MsgAuthorV;
typedef std::vector<GrpAuthEntry> GrpAuthorV;

class MsgRespPending : public AuthorPending
{
public:

	MsgRespPending(RsGixsReputation* rep, const std::string& peerId, const MsgAuthorV& msgAuthV, int cutOff = 0);

	int getType() const;
	bool accepted();
	std::string mPeerId;
	MsgAuthorV mMsgAuthV;
	int mCutOff;
};

class GrpRespPending : public AuthorPending
{
public:

	GrpRespPending(RsGixsReputation* rep, const std::string& peerId, const GrpAuthorV& grpAuthV, int cutOff = 0);
	int getType() const;
	bool accepted();
	std::string mPeerId;
	GrpAuthorV mGrpAuthV;
	int mCutOff;
};


#endif /* RSGXSNETUTILS_H_ */
