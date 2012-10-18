/*
 * libretroshare/src/services: p3idservice.h
 *
 * Identity interface for RetroShare.
 *
 * Copyright 2012-2012 by Robert Fernie.
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

#ifndef P3_IDENTITY_SERVICE_HEADER
#define P3_IDENTITY_SERVICE_HEADER

#include "services/p3service.h"
#include "services/p3gxsservice.h"

#include "retroshare/rsidentity.h"

#include <map>
#include <string>

/* 
 * Identity Service
 *
 */

// INTERNAL DATA TYPES. 
// Describes data stored in GroupServiceString.
class IdRepCumulScore
{
public:
	uint32_t count;
	uint32_t nullcount;
	double   sum;
	double   sumsq;
	
	// derived parameters:
};


class IdGroupServiceStrData
{
public:
	IdGroupServiceStrData() { pgpIdKnown = false; }
	bool pgpIdKnown;
	std::string pgpId;
	
	uint32_t ownScore;
	IdRepCumulScore opinion;
	IdRepCumulScore reputation;
	
};

#define ID_LOCAL_STATUS_FULL_CALC_FLAG	0x00010000
#define ID_LOCAL_STATUS_INC_CALC_FLAG	0x00020000



// Not sure exactly what should be inherited here?
// Chris - please correct as necessary.

class p3IdService: public RsIdentity, public RsGxsIdExchange
{
	public:
	p3IdService(RsGeneralDataService* gds, RsNetworkExchangeService* nes);

	virtual int	internal_tick(); // needed for background processing.


	/* General Interface is provided by RsIdentity / RsGxsIfaceImpl. */

	/* Data Specific Interface */

	/* TODO */


	/**************** RsIdentity External Interface.
	 * Notes:
	 */


virtual bool  getNickname(const RsId &id, std::string &nickname);
virtual bool  getIdDetails(const RsId &id, RsIdentityDetails &details);
virtual bool  getOwnIds(std::list<RsId> &ownIds);

        // 
virtual bool submitOpinion(uint32_t& token, RsIdOpinion &opinion);
virtual bool createIdentity(uint32_t& token, RsIdentityParameters &params);


	/**************** RsGixs Implementation 
	 * Notes:
	 *   Interface is only suggestion at the moment, will be changed as necessary.
	 *   Results should be cached / preloaded for maximum speed.
	 *
	 */
virtual bool haveKey(const GxsId &id);
virtual bool havePrivateKey(const GxsId &id);
virtual bool requestKey(const GxsId &id, const std::list<PeerId> &peers);
virtual int  getKey(const GxsId &id, TlvSecurityKey &key);
virtual int  getPrivateKey(const GxsId &id, TlvSecurityKey &key);  

	/**************** RsGixsReputation Implementation 
	 * Notes:
	 *   Again should be cached if possible.
	 */

        // get Reputation.
virtual bool getReputation(const GxsId &id, const GixsReputation &rep);

	private:

/************************************************************************
 * Below is the background task for processing opinions => reputations 
 *
 */

virtual void generateDummyData();

std::string genRandomId();

	int	background_tick();
	bool background_checkTokenRequest();
	bool background_requestGroups();
	bool background_requestNewMessages();
	bool background_processNewMessages();
	bool background_FullCalcRequest();
	bool background_processFullCalc();
	
	bool background_cleanup();

	bool encodeIdGroupCache(std::string &str, const IdGroupServiceStrData &data);
	bool extractIdGroupCache(std::string &str, IdGroupServiceStrData &data);
	
	RsMutex mIdMtx;

	/***** below here is locked *****/
	bool mLastBgCheck;
	bool mBgProcessing;
	
	uint32_t mBgToken;
	uint32_t mBgPhase;
	
	std::map<std::string, RsGroupMetaData> mBgGroupMap;
	std::list<std::string> mBgFullCalcGroups;

/************************************************************************
 * Other Data that is protected by the Mutex.
 */

	std::vector<RsGxsGroupChange*> mGroupChange;
	std::vector<RsGxsMsgChange*> mMsgChange;

};

#endif // P3_IDENTITY_SERVICE_HEADER



