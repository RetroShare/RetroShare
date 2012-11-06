/*
 * libretroshare/src/services p3idservice.cc
 *
 * Id interface for RetroShare.
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

#include "services/p3idservice.h"
#include "serialiser/rsgxsiditems.h"

#include "util/rsrandom.h"
#include "util/rsstring.h"

#include "pqi/authgpg.h"

#include <retroshare/rspeers.h>

#include <sstream>
#include <stdio.h>

/****
 * #define ID_DEBUG 1
 ****/

#define ID_REQUEST_LIST		0x0001
#define ID_REQUEST_IDENTITY	0x0002
#define ID_REQUEST_REPUTATION	0x0003
#define ID_REQUEST_OPINION	0x0004

RsIdentity *rsIdentity = NULL;


/******
 * Some notes:
 * Identity tasks:
 *   - Provide keys for signing / validating author signatures.
 *   - Reputations
 *   - Identify Known Friend's IDs.
 *   - Provide details to other services (nicknames, reputations, gpg ids, etc)
 *
 * Background services:
 *   - Lookup and cache keys / details of identities.
 *   - Check GPGHashes.
 *   - Calculate Reputations.
 *
 * We have a lot of information to store in Service Strings.
 *   - GPGId or last check ts.
 *   - Reputation stuff.
 */

#define RSGXSID_MAX_SERVICE_STRING 1024

#define BG_PGPHASH 	1
#define BG_REPUTATION 	2

/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3IdService::p3IdService(RsGeneralDataService *gds, RsNetworkExchangeService *nes)
	: RsGxsIdExchange(gds, nes, new RsGxsIdSerialiser(), RS_SERVICE_GXSV1_TYPE_GXSID), RsIdentity(this), 
	mIdMtx("p3IdService")
{
	mCacheTest_LastTs = 0;
	mCacheTest_Active = false;

	mCacheLoad_LastCycle = 0;
	mCacheLoad_Status = 0;

	mHashPgp_SearchActive = false;

	mBgSchedule_Mode = 0;
	mBgSchedule_Active = false;
}

void	p3IdService::service_tick()
{
	//std::cerr << "p3IdService::service_tick()";
	//std::cerr << std::endl;

	// Disable for now.
	// background_tick();

	cache_tick();

	// internal testing - request keys. (NOT FINISHED YET)
	cachetest_tick();

	// background stuff.
	//scheduling_tick();

	return;
}

void p3IdService::notifyChanges(std::vector<RsGxsNotify *> &changes)
{
	std::cerr << "p3IdService::notifyChanges()";
	std::cerr << std::endl;

	receiveChanges(changes);
}

/********************************************************************************/
/******************* RsIdentity Interface ***************************************/
/********************************************************************************/

bool p3IdService:: getNickname(const RsGxsId &id, std::string &nickname)
{
	return false;
}

bool p3IdService:: getIdDetails(const RsGxsId &id, RsIdentityDetails &details)
{
	return false;
}

bool p3IdService:: getOwnIds(std::list<RsGxsId> &ownIds)
{
	return false;
}


//
bool p3IdService::submitOpinion(uint32_t& token, RsIdOpinion &opinion)
{
	return false;
}

bool p3IdService::createIdentity(uint32_t& token, RsIdentityParameters &params)
{
	return false;
}


/********************************************************************************/
/******************* RsGixs Interface     ***************************************/
/********************************************************************************/

bool p3IdService::haveKey(const RsGxsId &id)
{
	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
	return mPublicKeyCache.is_cached(id);
}

bool p3IdService::havePrivateKey(const RsGxsId &id)
{
	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
	return mPrivateKeyCache.is_cached(id);
}

bool p3IdService::requestKey(const RsGxsId &id, const std::list<PeerId> &peers)
{
	if (haveKey(id))
		return true;
	return cache_request_load(id);
}

int  p3IdService::getKey(const RsGxsId &id, RsTlvSecurityKey &key)
{
	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
	RsGxsIdCache data;
	if (mPublicKeyCache.fetch(id, data))
	{
		key = data.pubkey;
		return 1;
	}
	return -1;
}

bool p3IdService::requestPrivateKey(const RsGxsId &id)
{
	if (havePrivateKey(id))
		return true;
	return cache_request_load(id);
}

int  p3IdService::getPrivateKey(const RsGxsId &id, RsTlvSecurityKey &key)
{
	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
	RsGxsIdCache data;
	if (mPrivateKeyCache.fetch(id, data))
	{
		key = data.pubkey;
		return 1;
	}
	return -1;
}


/********************************************************************************/
/******************* RsGixsReputation     ***************************************/
/********************************************************************************/

bool p3IdService::getReputation(const RsGxsId &id, const GixsReputation &rep)
{
	return false;
}


/********************************************************************************/
/******************* Get/Set Data      ******************************************/
/********************************************************************************/

bool p3IdService::getGroupData(const uint32_t &token, std::vector<RsGxsIdGroup> &groups)
{

        std::vector<RsGxsGrpItem*> grpData;
        bool ok = RsGenExchange::getGroupData(token, grpData);

        if(ok)
        {
                std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();

                for(; vit != grpData.end(); vit++)
                {
                        RsGxsIdGroupItem* item = dynamic_cast<RsGxsIdGroupItem*>(*vit);
                        RsGxsIdGroup group = item->group;
                        group.mMeta = item->meta;

			// Decode information from serviceString.
			SSGxsIdGroup ssdata;
			if (ssdata.load(group.mMeta.mServiceString))
			{
				group.mPgpKnown = ssdata.pgp.idKnown;
				group.mPgpId    = ssdata.pgp.pgpId;
			}
			else
			{
				group.mPgpKnown = false;
				group.mPgpId    = "";
			}

                        groups.push_back(group);
                }
        }

        return ok;
}


bool p3IdService::getMsgData(const uint32_t &token, std::vector<RsGxsIdOpinion> &opinions)
{
	GxsMsgDataMap msgData;
	bool ok = RsGenExchange::getMsgData(token, msgData);
	
	if(ok)
	{
		GxsMsgDataMap::iterator mit = msgData.begin();
		
		for(; mit != msgData.end();  mit++)
		{
			RsGxsGroupId grpId = mit->first;
			std::vector<RsGxsMsgItem*>& msgItems = mit->second;
			std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();
			
			for(; vit != msgItems.end(); vit++)
			{
				RsGxsIdOpinionItem* item = dynamic_cast<RsGxsIdOpinionItem*>(*vit);
				
				if (item)
				{
					RsGxsIdOpinion opinion = item->opinion;
					opinion.mMeta = item->meta;
					opinions.push_back(opinion);
					delete item;
				}
				else
				{
					std::cerr << "Not a IdOpinion Item, deleting!" << std::endl;
					delete *vit;
				}
			}
		}
	}
	return ok;
}

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

bool 	p3IdService::createGroup(uint32_t& token, RsGxsIdGroup &group)
{
    RsGxsIdGroupItem* item = new RsGxsIdGroupItem();
    item->group = group;
    item->meta = group.mMeta;
    RsGenExchange::publishGroup(token, item);
    return true;
}

bool 	p3IdService::createMsg(uint32_t& token, RsGxsIdOpinion &opinion)
{
    RsGxsIdOpinionItem* item = new RsGxsIdOpinionItem();
    item->opinion = opinion;
    item->meta = opinion.mMeta;
    RsGenExchange::publishMsg(token, item);
    return true;
}

/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

/* Encoding / Decoding Group Service String stuff
 *
 * Pgp stuff.
 *
 * If flagged as pgp id....
 *   then we need to know if its been matched, or when we last tried to match.
 *
 */

bool SSGxsIdPgp::load(const std::string &input)
{
	char pgpline[RSGXSID_MAX_SERVICE_STRING];
	int timestamp = 0;
	if (1 == sscanf(input.c_str(), "K:1 I:%[^)]", pgpline))
	{
		idKnown = true;
		pgpId = pgpline;
	}
	else if (1 == sscanf(input.c_str(), "K:0 T:%d", &timestamp))
	{
		lastCheckTs = timestamp;
		idKnown = false;
	}
	else
	{
		return false;
	}	
	return true;
}

std::string SSGxsIdPgp::save() const
{
	std::string output;
	if (idKnown)
	{
		output += "K:1 I:";
		output += pgpId;
	}
	else
	{
		rs_sprintf(output, "K:0 T:%d", lastCheckTs);
	}
	return output;
}

bool SSGxsIdScore::load(const std::string &input)
{
	return (1 == sscanf(input.c_str(), "%d", &score));
}

std::string SSGxsIdScore::save() const
{
	std::string output;
	rs_sprintf(output, "%d", score);
	return output;
}

bool SSGxsIdCumulator::load(const std::string &input)
{
	return (4 == sscanf(input.c_str(), "%d %d %lf %lf", &count, &nullcount, &sum, &sumsq));
}

std::string SSGxsIdCumulator::save() const
{
	std::string output;
	rs_sprintf(output, "%d %d %lf %lf", count, nullcount, sum, sumsq);
	return output;
}

bool SSGxsIdGroup::load(const std::string &input)
{
	char pgpstr[RSGXSID_MAX_SERVICE_STRING];
	char scorestr[RSGXSID_MAX_SERVICE_STRING];
	char opinionstr[RSGXSID_MAX_SERVICE_STRING];
	char repstr[RSGXSID_MAX_SERVICE_STRING];
	
	// split into two parts.
	if (4 != sscanf(input.c_str(), "v1 {%[^}]} {%[^}]} {%[^}]} {%[^}]}", pgpstr, scorestr, opinionstr, repstr))
	{
		std::cerr << "SSGxsIdGroup::load() Failed to extract 4 Parts";
		std::cerr << std::endl;
		return false;
	}

	bool ok = true;
	if (0 == strncmp(pgpstr, "P:", 2))
	{
		std::cerr << "SSGxsIdGroup::load() pgpstr: " << pgpstr;
		std::cerr << std::endl;
		ok &= pgp.load(pgpstr);
	}
	else
	{
		std::cerr << "SSGxsIdGroup::load() Invalid pgpstr: " << pgpstr;
		std::cerr << std::endl;
		ok = false;
	}

	if (0 == strncmp(scorestr, "Y:", 2))
	{
		std::cerr << "SSGxsIdGroup::load() scorestr: " << scorestr;
		std::cerr << std::endl;
		ok &= score.load(scorestr);
	}
	else
	{
		std::cerr << "SSGxsIdGroup::load() Invalid scorestr: " << scorestr;
		std::cerr << std::endl;
		ok = false;
	}

	if (0 == strncmp(opinionstr, "O:", 2))
	{
		std::cerr << "SSGxsIdGroup::load() opinionstr: " << opinionstr;
		std::cerr << std::endl;
		ok &= opinion.load(opinionstr);
	}
	else
	{
		std::cerr << "SSGxsIdGroup::load() Invalid opinionstr: " << opinionstr;
		std::cerr << std::endl;
		ok = false;
	}

	if (0 == strncmp(repstr, "R:", 2))
	{
		std::cerr << "SSGxsIdGroup::load() repstr: " << repstr;
		std::cerr << std::endl;
		ok &= reputation.load(repstr);
	}
	else
	{
		std::cerr << "SSGxsIdGroup::load() Invalid repstr: " << repstr;
		std::cerr << std::endl;
		ok = false;
	}

	return ok;
}

std::string SSGxsIdGroup::save() const
{
	std::string output = "v1 ";

	output += "{P:";
	output += pgp.save();
	output += "}";

	output += "{Y:";
	output += score.save();
	output += "}";

	output += "{O:";
	output += opinion.save();
	output += "}";

	output += "{R:";
	output += reputation.save();
	output += "}";

	std::cerr << "SSGxsIdGroup::save() output: " << output;
	std::cerr << std::endl;

	return output;
}

/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

/* Cache of recently used keys
 *
 * It is expensive to fetch the keys, so we want to keep them around if possible.
 * It only stores the immutable stuff.
 *
 * This is probably crude and crap to start with.
 * Want Least Recently Used (LRU) discard policy, without having to search whole cache.
 * Use two maps:
 *   - CacheMap[key] => data.
 *   - LRUMultiMap[AccessTS] => key
 *
 * NOTE: This could be moved to a seperate class and templated to make generic
 * as it might be generally useful.
 *
 */

RsGxsIdCache::RsGxsIdCache() 
	:reputation(0), lastUsedTs(0) 
{ 
	return; 
}

RsGxsIdCache::RsGxsIdCache(const RsGxsIdGroupItem *item, const RsTlvSecurityKey &in_pkey)
{
	id = item->meta.mGroupId;
	name = item->meta.mGroupName;
	pubkey = in_pkey;

	std::cerr << "RsGxsIdCache::RsGxsIdCache() for: " << id;
	std::cerr << std::endl;

        reputation = 0; /* TODO: extract from string - This will need to be refreshed!!! */
	lastUsedTs = 0;	

}

bool p3IdService::cache_store(const RsGxsIdGroupItem *item)
{
	std::cerr << "p3IdService::cache_store() Item: ";
	std::cerr << std::endl;
	//item->print(std::cerr, 0); NEEDS CONST!!!! TODO
	std::cerr << std::endl;

        /* extract key from keys */
    	RsTlvSecurityKeySet keySet;
    	RsTlvSecurityKey    pubkey;
    	RsTlvSecurityKey    fullkey;
	bool pub_key_ok = false;
	bool full_key_ok = false;

    	RsGxsId id = item->meta.mGroupId;
    	if (!getGroupKeys(id, keySet))
	{
		std::cerr << "p3IdService::cache_store() ERROR getting GroupKeys for: ";
		std::cerr << item->meta.mGroupId;
		std::cerr << std::endl;
		return false;
	}

	std::map<std::string, RsTlvSecurityKey>::iterator kit;

	//std::cerr << "p3IdService::cache_store() KeySet is:";
	//keySet.print(std::cerr, 10);

        for (kit = keySet.keys.begin(); kit != keySet.keys.end(); kit++)
        {
		if (kit->second.keyFlags | RSTLV_KEY_DISTRIB_ADMIN)
		{
			std::cerr << "p3IdService::cache_store() Found Admin Key";
			std::cerr << std::endl;

			/* save full key - if we have it */
			if (kit->second.keyFlags | RSTLV_KEY_TYPE_FULL)
			{
				fullkey = kit->second;
				full_key_ok = true;
			}

			/* cache public key always */
			pubkey = kit->second;
			pub_key_ok = true;
		}
	}

	if (!pub_key_ok)
	{
		std::cerr << "p3IdService::cache_store() ERROR No Public Key Found";
		std::cerr << std::endl;
		return false;
	}

	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

	// Create Cache Data.
	RsGxsIdCache pubcache(item, pubkey);
	mPublicKeyCache.store(id, pubcache);

	if (full_key_ok)
	{
		RsGxsIdCache fullcache(item, fullkey);
		mPrivateKeyCache.store(id, fullcache);
	}

	return true;
}



/***** BELOW LOADS THE CACHE FROM GXS DATASTORE *****/

#define MIN_CYCLE_GAP	2

int	p3IdService::cache_tick()
{
	//std::cerr << "p3IdService::cache_tick()";
	//std::cerr << std::endl;

	/* every minute - run a background check */
	time_t now = time(NULL);
	bool doCycle = false;
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		if (now -  mCacheLoad_LastCycle > MIN_CYCLE_GAP)
		{
			doCycle = true;
			mCacheLoad_LastCycle = now;
		}
	}

	if (doCycle)
	{
		cache_start_load();
	}

	cache_check_loading();

		
	return 0;
}

bool p3IdService::cache_request_load(const RsGxsId &id)
{
	std::cerr << "p3IdService::cache_request_load(" << id << ")";
	std::cerr << std::endl;

	bool start = false;
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

		mCacheLoad_ToCache.push_back(id);

		if (time(NULL) - mCacheLoad_LastCycle > MIN_CYCLE_GAP)
		{
			start = true;
		}
	}

	if (start)
		cache_start_load();

	return true;
}


bool p3IdService::cache_start_load()
{
	/* trigger request to load missing ids into cache */
	std::list<RsGxsGroupId> groupIds;
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

		/* now we process the modGroupList -> a map so we can use it easily later, and create id list too */
		std::list<RsGxsId>::iterator it;
		for(it = mCacheLoad_ToCache.begin(); it != mCacheLoad_ToCache.end(); it++)
		{
			groupIds.push_back(*it); // might need conversion?
		}

		mCacheLoad_ToCache.clear();
	}

	if (groupIds.size() > 0)
	{
		std::cerr << "p3IdService::cache_start_load() #Groups: " << groupIds.size();
		std::cerr << std::endl;

		mCacheLoad_LastCycle = time(NULL);
		mCacheLoad_Status = 1;

		uint32_t ansType = RS_TOKREQ_ANSTYPE_DATA; 
	        RsTokReqOptions opts;
        	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
		uint32_t token = 0;
	
		RsGenExchange::getTokenService()->requestGroupInfo(token, ansType, opts, groupIds);
	
		{
			RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
			mCacheLoad_Tokens.push_back(token);
		}
	}
	return 1;
}

bool p3IdService::cache_check_loading()
{
	/* check the status of all active tokens */
	std::list<uint32_t> toload;
	std::list<uint32_t>::iterator it;

	bool stuffToLoad = false;
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		for(it = mCacheLoad_Tokens.begin(); it != mCacheLoad_Tokens.end();)
		{
			std::cerr << "p3IdService::cache_check_loading() token: " << *it;
			std::cerr << std::endl;

			uint32_t token = *it;
			uint32_t status = RsGenExchange::getTokenService()->requestStatus(token);
			//checkRequestStatus(token, status, reqtype, anstype, ts);
	
			if (status == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
			{
				it = mCacheLoad_Tokens.erase(it);
				toload.push_back(token);
				stuffToLoad = true;
			}
			else
			{
				it++;
			}
		}
	}

	if (stuffToLoad)
	{
		for(it = toload.begin(); it != toload.end(); it++)
		{
			cache_load_for_token(*it);
		}

		// cleanup.
		mPrivateKeyCache.resize();
		mPublicKeyCache.resize();
	}
	return 1;

}

bool p3IdService::cache_load_for_token(uint32_t token)
{
	std::cerr << "p3IdService::cache_load_for_token() : " << token;
	std::cerr << std::endl;

        std::vector<RsGxsGrpItem*> grpData;
        bool ok = RsGenExchange::getGroupData(token, grpData);

        if(ok)
        {
                std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();

                for(; vit != grpData.end(); vit++)
                {
                        RsGxsIdGroupItem* item = dynamic_cast<RsGxsIdGroupItem*>(*vit);

			std::cerr << "p3IdService::cache_load_for_token() Loaded Id with Meta: ";
			std::cerr << item->meta;
			std::cerr << std::endl;

			/* cache the data */
			cache_store(item);
			delete item;
                }
        }
	else
	{
		std::cerr << "p3IdService::cache_load_for_token() ERROR no data";
		std::cerr << std::endl;

		return false;
	}
	return true;
}


/************************************************************************************/
/************************************************************************************/

#define TEST_PERIOD 60

bool p3IdService::cachetest_tick()
{
	/* every minute - run a background check */
	time_t now = time(NULL);
	bool doTest = false;
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		if (now -  mCacheTest_LastTs > TEST_PERIOD)
		{
			doTest = true;
			mCacheTest_LastTs = now;
		}
	}

	if (doTest)
	{
		std::cerr << "p3IdService::cachetest_tick() starting";
		std::cerr << std::endl;
		cachetest_getlist();
	}

	cachetest_request();
	return true;
}

bool p3IdService::cachetest_getlist()
{

	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		if (mCacheTest_Active)
		{
			std::cerr << "p3IdService::cachetest_getlist() Already active";
			std::cerr << std::endl;
			return false;
		}
	}

	std::cerr << "p3IdService::cachetest_getlist() making request";
	std::cerr << std::endl;

	uint32_t ansType = RS_TOKREQ_ANSTYPE_LIST; 
        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_IDS;
	uint32_t token = 0;
	
	RsGenExchange::getTokenService()->requestGroupInfo(token, ansType, opts);
	
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		mCacheTest_Token = token;
		mCacheTest_Active = true;
	}
	return true;
}


bool p3IdService::cachetest_request()
{
	uint32_t token = 0;
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		if (!mCacheTest_Active)
		{
			return false;
		}
		token = mCacheTest_Token;
	}

	std::cerr << "p3IdService::cachetest_request() checking request";
	std::cerr << std::endl;

	uint32_t status = RsGenExchange::getTokenService()->requestStatus(token);

	if (status == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
	{
		std::cerr << "p3IdService::cachetest_request() token ready: " << token;
		std::cerr << std::endl;

        	std::list<RsGxsId> grpIds;
        	bool ok = RsGenExchange::getGroupList(token, grpIds);

		{	
			RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
			mCacheTest_Active = false;
		}

	        if(ok)
	        {
	                std::list<RsGxsId>::iterator vit = grpIds.begin();
	                for(; vit != grpIds.end(); vit++)
	                {
				/* 5% chance of checking it! */
				if (RSRandom::random_f32() < 0.25)
				{
					std::cerr << "p3IdService::cachetest_request() Testing Id: " << *vit;
					std::cerr << std::endl;

					/* try the cache! */
					if (!haveKey(*vit))
					{
						std::list<PeerId> nullpeers;
						requestKey(*vit, nullpeers);

						std::cerr << "p3IdService::cachetest_request() Requested Key Id: " << *vit;
						std::cerr << std::endl;
					}
					else
					{
						RsTlvSecurityKey seckey;
						if (getKey(*vit, seckey))
						{
							std::cerr << "p3IdService::cachetest_request() Got Key OK Id: " << *vit;
							std::cerr << std::endl;

							// success!
        						seckey.print(std::cerr, 10);
							std::cerr << std::endl;


						}
						else
						{
							std::cerr << "p3IdService::cachetest_request() ERROR no Key for Id: " << *vit;
							std::cerr << std::endl;
						}
					}

					/* try private key too! */
					if (!havePrivateKey(*vit))
					{
						requestPrivateKey(*vit);
						std::cerr << "p3IdService::cachetest_request() Requested PrivateKey Id: " << *vit;
						std::cerr << std::endl;
					}
					else
					{
						RsTlvSecurityKey seckey;
						if (getPrivateKey(*vit, seckey))
						{
							// success!
							std::cerr << "p3IdService::cachetest_request() Got PrivateKey OK Id: " << *vit;
							std::cerr << std::endl;
						}
						else
						{
							std::cerr << "p3IdService::cachetest_request() ERROR no PrivateKey for Id: " << *vit;
							std::cerr << std::endl;
						}
					}
				}
			}
	        }
		else
		{
			std::cerr << "p3IdService::cache_load_for_token() ERROR no data";
			std::cerr << std::endl;
	
			return false;
		}
	}
	return true;
}

/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

/*
 * We have two background tasks that use the ServiceString: PGPHash & Reputation.
 *
 * Only one task can be run at a time - otherwise potential overwrite issues.
 * So this part coordinates that part of the code.
 *
 * 
 *
 *
 */

#define ID_BACKGROUND_PERIOD	60

int	p3IdService::scheduling_tick()
{
	std::cerr << "p3IdService::scheduling_tick()";
	std::cerr << std::endl;

	/*** MUTEX TODO ***/

	if (mBgSchedule_Active)
	{
		bool done = false;
		if (mBgSchedule_Mode == BG_PGPHASH)
		{
			done = pgphash_continue();
		}
		else
		{
			done = reputation_continue();
		}

		if (done)
		{
			mBgSchedule_Active = false;
		}
	}
	else 
	{
		if (pgphash_start())
		{
			mBgSchedule_Mode = BG_PGPHASH;
			mBgSchedule_Active = true;
		}
		else if (reputation_start())
		{
			mBgSchedule_Mode = BG_REPUTATION;
			mBgSchedule_Active = true;
		}
	}

	return 1;
}


/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

/* Task to determine GPGHash matches
 *
 * Info to be stored in GroupServiceString + Cache.
 *
 * Actually - it must be a Signature here - otherwise, you could 
 * put in a hash from someone else!
 *
 * Don't think that we need to match very often - maybe once a day?
 * Actually - we should scale the matching based on number of keys we have.
 *
 * imagine - 10^6 rsa keys + 10^3 gpg keys  => 10^9 combinations.
 *  -- far too many to check all quickly.
 *
 * Need to grab and cache data we need... then check over slowly.
 *
 * maybe grab a list of all gpgids - that we know of: store id list.
 * then big GroupRequest, and iterate through these.
 **/

//const int SHA_DIGEST_LENGTH = 20;

typedef t_RsGenericIdType<SHA_DIGEST_LENGTH> GxsIdPgpHash;

static void calcPGPHash(const RsGxsId &id, const PGPFingerprintType &pgp, GxsIdPgpHash &hash);


void p3IdService::service_CreateGroup(RsGxsGrpItem* grpItem, RsTlvSecurityKeySet& keySet)
{
	RsGxsIdGroupItem *item = dynamic_cast<RsGxsIdGroupItem *>(grpItem);
	if (!item)
	{
		std::cerr << "p3IdService::service_CreateGroup() ERROR invalid cast";
		std::cerr << std::endl;
		return;
	}

	std::cerr << "p3IdService::service_CreateGroup() for : " << item->group.mMeta.mGroupId;
	std::cerr << std::endl;

	if (item->group.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
	{
		/* create the hash */
		GxsIdPgpHash hash;

		/* */
		PGPFingerprintType ownFinger;
		PGPIdType ownId(AuthGPG::getAuthGPG()->getGPGOwnId());

		if (!AuthGPG::getAuthGPG()->getKeyFingerprint(ownId,ownFinger))
		{
			std::cerr << "p3IdService::service_CreateGroup() ERROR Own Finger is stuck";
			std::cerr << std::endl;
			return; // abandon attempt!
		}

		calcPGPHash(item->group.mMeta.mGroupId, ownFinger, hash);
		item->group.mPgpIdHash = hash.toStdString();

		/* do signature */

#if ENABLE_PGP_SIGNATURES
#define MAX_SIGN_SIZE 2048
		uint8_t signarray[MAX_SIGN_SIZE]; 
		unsigned int sign_size = MAX_SIGN_SIZE;
		if (!AuthGPG::getAuthGPG()->SignDataBin((void *) hash.toByteArray(), hash.SIZE_IN_BYTES, signarray, &sign_size))
		{
			/* error */
			std::cerr << "p3IdService::service_CreateGroup() ERROR Signing stuff";
			std::cerr << std::endl;
		}
		else
		{
			/* push binary into string -> really bad! */
			item->group.mPgpIdSign = "";
			for(unsigned int i = 0; i < sign_size; i++)
			{
				item->group.mPgpIdSign += signarray[i];
			}
		}
		/* done! */
#else
		item->group.mPgpIdSign = "";
#endif

	}
}


#define HASHPGP_PERIOD		180


bool p3IdService::pgphash_start()
{
	/* every minute - run a background check */
	time_t now = time(NULL);
	bool doHash = false;
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		if (now -  mHashPgp_LastTs > HASHPGP_PERIOD)
		{
			doHash = true;
			mHashPgp_LastTs = now;
		}
	}

	if (doHash)
	{
		std::cerr << "p3IdService::pgphash_tick() starting";
		std::cerr << std::endl;
		pgphash_getlist();
		return true;
	}
	return false;
}


bool p3IdService::pgphash_continue()
{
	if (mHashPgp_SearchActive)
	{
		pgphash_request();
	}
	else
	{
		return pgphash_process();
	}
	return false;
}



bool p3IdService::pgphash_getlist()
{

	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		if (mHashPgp_SearchActive)
		{
			std::cerr << "p3IdService::cachetest_getlist() Already active";
			std::cerr << std::endl;
			return false;
		}
	}

	std::cerr << "p3IdService::cachetest_getlist() making request";
	std::cerr << std::endl;

	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;
	uint32_t token = 0;
	
	RsGenExchange::getTokenService()->requestGroupInfo(token, ansType, opts);
	
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		mHashPgp_Token = token;
		mHashPgp_SearchActive = true;
	}
	return true;
}


bool p3IdService::pgphash_request()
{
	uint32_t token = 0;
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		if (!mHashPgp_SearchActive)
		{
			return false;
		}
		token = mHashPgp_Token;
	}

	std::cerr << "p3IdService::pgphash_request() checking request";
	std::cerr << std::endl;

	uint32_t status = RsGenExchange::getTokenService()->requestStatus(token);

	if (status == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
	{
		std::cerr << "p3IdService::pgphash_request() token ready: " << token;
		std::cerr << std::endl;

		// We need full data - for access to Hash & Signature.
		// Perhaps we will change this to an initial pass through Meta, 
		// and use this to discard lots of things.

		// Even better - we can set flags in the Meta Data, (IdType), 
		// And use GXS to filter out all the AnonIds, and only have to process
		// Proper Ids.

		// We Will do this later!

		std::vector<RsGxsIdGroup> groups;
		std::vector<RsGxsIdGroup> groupsToProcess;
		bool ok = getGroupData(token, groups);

		{	
			RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
			mHashPgp_SearchActive = false;
		}

	        if(ok)
	        {
			std::vector<RsGxsIdGroup>::iterator vit;
	                for(vit = groups.begin(); vit != groups.end(); vit++)
	                {
				/* Filter based on IdType */
				if (!(vit->mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID))
				{
					std::cerr << "p3IdService::pgphash_request() discarding AnonID";
					std::cerr << std::endl;
					continue;
				}

				/* now we need to decode the Service String - see what is saved there */
				SSGxsIdGroup ssdata;
				if (ssdata.load(vit->mMeta.mServiceString))
				{
					if (ssdata.pgp.idKnown)
					{
						std::cerr << "p3IdService::pgphash_request() discarding Already Known";
						std::cerr << std::endl;
						continue;
					}

					/* Have a linear attempt policy -	
					 * if zero checks - try now.
					 * if 1 check, at least a day.
					 * if 2 checks: 2days, etc.
					 */

#define SECS_PER_DAY (3600 * 24)
					time_t age = time(NULL) - ssdata.pgp.lastCheckTs;
					time_t wait_period = ssdata.pgp.checkAttempts * SECS_PER_DAY;
					if (wait_period > 30 * SECS_PER_DAY)
					{
						wait_period = 30 * SECS_PER_DAY;
					}

					if (age < wait_period)
					{
						std::cerr << "p3IdService::pgphash_request() discarding Recently Check";
						std::cerr << std::endl;
						continue;
					}
				}

				/* if we get here -> then its to be processed */
				std::cerr << "p3IdService::cachetest_request() Requested Key Id: " << *vit;
				std::cerr << std::endl;

				mGroupsToProcess.push_back(*vit);
			}
		}
	}
	return true;
}

bool p3IdService::pgphash_process()
{
	/* each time this is called - process one Id from mGroupsToProcess */
	if (mGroupsToProcess.empty())
	{
		return true;
	}

	RsGxsIdGroup pg = mGroupsToProcess.front();
	mGroupsToProcess.pop_front();

	SSGxsIdGroup ssdata;
	ssdata.load(pg.mMeta.mServiceString); // attempt load - okay if fails.

	PGPIdType pgpId;

	if (checkId(pg, pgpId))	
	{
		/* found a match - update everything */
		/* Consistency issues here - what if Reputation was recently updated? */


		/* update */
		ssdata.pgp.idKnown = true;
		ssdata.pgp.pgpId = pgpId.toStdString();

// SHOULD BE PUSHED TO CACHE!
#if 0
				id.mGpgIdKnown = true;
			
				id.mGpgId = *it;
				id.mGpgName = details.name;
				id.mGpgEmail = details.email;

				if (*it == ownId)
				{
					id.mIdType |= RSID_RELATION_YOURSELF;
				}
				else if (rsPeers->isGPGAccepted(*it))
				{
					id.mIdType |= RSID_RELATION_FRIEND;
				}
				else
				{
					id.mIdType |= RSID_RELATION_OTHER;
				}
				
#endif

	}
	else
	{
		ssdata.pgp.lastCheckTs = time(NULL);
		ssdata.pgp.checkAttempts++;
	}

	/* set new Group ServiceString */
	uint32_t dummyToken = 0;
	std::string serviceString = ssdata.save();
	setGroupServiceString(dummyToken, pg.mMeta.mGroupId, serviceString);

	return true;

}



bool p3IdService::checkId(const RsGxsIdGroup &grp, PGPIdType &pgpId)
{
	std::cerr << "p3IdService::checkId() Starting Match Check for RsGxsId: ";
	std::cerr << grp.mMeta.mGroupId;
	std::cerr << std::endl;

	/* iterate through and check hash */
	GxsIdPgpHash ans(grp.mPgpIdHash);

	std::map<PGPIdType, PGPFingerprintType>::iterator mit;
	for(mit = mPgpFingerprintMap.begin(); mit != mPgpFingerprintMap.end(); mit++)
	{
		GxsIdPgpHash hash;
		calcPGPHash(grp.mMeta.mGroupId, mit->second, hash);
		if (ans == hash)
		{
			std::cerr << "p3IdService::checkId() HASH MATCH!";
			std::cerr << std::endl;

#if ENABLE_PGP_SIGNATURES
			/* miracle match! */
			/* check signature too */
			if (AuthGPG::getAuthGPG()->VerifySignBin((void *) hash.toByteArray(), hash.SIZE_IN_BYTES, 
				(unsigned char *) grp.mPgpIdSign.c_str(), grp.mPgpIdSign.length(), 
				mit->second.toStdString()))
			{
				std::cerr << "p3IdService::checkId() Signature Okay too!";
				std::cerr << std::endl;

				pgpId = mit->first;
				return true;
			}

			/* error */
			std::cerr << "p3IdService::checkId() ERROR Signature Failed";
			std::cerr << std::endl;
#else
			std::cerr << "p3IdService::checkId() Skipping Signature check for now... Hash Okay";
			std::cerr << std::endl;
			return true;
#endif

		}
	}

	std::cerr << "p3IdService::checkId() Checked " << mPgpFingerprintMap.size() << " Hashes without Match";
	std::cerr << std::endl;

	return false;
}


/* worker functions */
void p3IdService::getPgpIdList()
{
	std::cerr << "p3IdService::getPgpIdList() Starting....";
	std::cerr << std::endl;

 	std::list<std::string> list;
	AuthGPG::getAuthGPG()->getGPGFilteredList(list);

 	std::list<std::string>::iterator it;
	for(it = list.begin(); it != list.end(); it++)
	{
 		PGPIdType pgpId(*it);
		PGPFingerprintType fp;
		AuthGPG::getAuthGPG()->getKeyFingerprint(pgpId, fp);

		mPgpFingerprintMap[pgpId] = fp;
	}

	std::cerr << "p3IdService::getPgpIdList() Items: " << mPgpFingerprintMap.size();
	std::cerr << std::endl;
}


void calcPGPHash(const RsGxsId &id, const PGPFingerprintType &pgp, GxsIdPgpHash &hash)
{
	unsigned char signature[SHA_DIGEST_LENGTH];
	/* hash id + pubkey => pgphash */
        SHA_CTX *sha_ctx = new SHA_CTX;
        SHA1_Init(sha_ctx);

        SHA1_Update(sha_ctx, id.c_str(), id.length()); // TO FIX ONE DAY.
        SHA1_Update(sha_ctx, pgp.toByteArray(), pgp.SIZE_IN_BYTES);
        SHA1_Final(signature, sha_ctx);
        hash = GxsIdPgpHash(signature);

        delete sha_ctx;
}


/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

std::string p3IdService::genRandomId()
{
	std::string randomId;
	for(int i = 0; i < 20; i++)
	{
		randomId += (char) ('a' + (RSRandom::random_u32() % 26));
	}
	
	return randomId;
}
	
void p3IdService::generateDummyData()
{
	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

	/* grab all the gpg ids... and make some ids */

	std::list<std::string> gpgids;
	std::list<std::string>::iterator it;
	
	rsPeers->getGPGAllList(gpgids);

	std::string ownId = rsPeers->getGPGOwnId();
	gpgids.push_back(ownId);

	int genCount = 0;
	int i;
	for(it = gpgids.begin(); it != gpgids.end(); it++)
	{
		/* create one or two for each one */
		int nIds = 1 + (RSRandom::random_u32() % 2);
		for(i = 0; i < nIds; i++)
		{
			RsGxsIdGroup id;

                	RsPeerDetails details;

			//id.mKeyId = genRandomId();
			id.mMeta.mGroupId = genRandomId();
			id.mMeta.mGroupFlags = RSGXSID_GROUPFLAG_REALID;
			id.mPgpIdHash = genRandomId();
			id.mPgpIdSign = genRandomId();

                	if (rsPeers->getPeerDetails(*it, details))
			{
				std::ostringstream out;
				out << details.name << "_" << i + 1;

				//id.mNickname = out.str();
				id.mMeta.mGroupName = out.str();
			
	
			}
			else
			{
				std::cerr << "p3IdService::generateDummyData() missing" << std::endl;
				std::cerr << std::endl;

				//id.mNickname = genRandomId();
				id.mMeta.mGroupName = genRandomId();
			}

			uint32_t dummyToken = 0;
			createGroup(dummyToken, id);

// LIMIT - AS GENERATION IS BROKEN.
#define MAX_TEST_GEN 50
			if (++genCount > MAX_TEST_GEN)
			{
				return;
			}
		}
	}
	return;

#define MAX_RANDOM_GPGIDS	10 //1000
#define MAX_RANDOM_PSEUDOIDS	50 //5000

	int nFakeGPGs = (RSRandom::random_u32() % MAX_RANDOM_GPGIDS);
	int nFakePseudoIds = (RSRandom::random_u32() % MAX_RANDOM_PSEUDOIDS);

	/* make some fake gpg ids */
	for(i = 0; i < nFakeGPGs; i++)
	{
		RsGxsIdGroup id;

                RsPeerDetails details;

		id.mMeta.mGroupName = genRandomId();

		id.mMeta.mGroupId = genRandomId();
		id.mMeta.mGroupFlags = RSGXSID_GROUPFLAG_REALID;
		id.mPgpIdHash = genRandomId();
		id.mPgpIdSign = genRandomId();

		uint32_t dummyToken = 0;
		createGroup(dummyToken, id);
	}

	/* make lots of pseudo ids */
	for(i = 0; i < nFakePseudoIds; i++)
	{
		RsGxsIdGroup id;

                RsPeerDetails details;

		id.mMeta.mGroupName = genRandomId();

		id.mMeta.mGroupId = genRandomId();
		id.mMeta.mGroupFlags = 0;
		id.mPgpIdHash = "";
		id.mPgpIdSign = "";


		uint32_t dummyToken = 0;
		createGroup(dummyToken, id);
	}

	//mUpdated = true;

	return;
}



std::string rsIdTypeToString(uint32_t idtype)
{
	std::string str;
	if (idtype & RSID_TYPE_REALID)
	{
		str += "GPGID ";
	}
	if (idtype & RSID_TYPE_PSEUDONYM)
	{
		str += "PSEUDO ";
	}
	if (idtype & RSID_RELATION_YOURSELF)
	{
		str += "YOURSELF ";
	}
	if (idtype & RSID_RELATION_FRIEND)
	{
		str += "FRIEND ";
	}
	if (idtype & RSID_RELATION_FOF)
	{
		str += "FOF ";
	}
	if (idtype & RSID_RELATION_OTHER)
	{
		str += "OTHER ";
	}
	if (idtype & RSID_RELATION_UNKNOWN)
	{
		str += "UNKNOWN ";
	}
	return str;
}








/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/


/* here we are running a background process that calculates the reputation scores
 * for each of the IDs....
 * 
 * As this class will be extensively used by many other threads... it is best
 * that we don't block at all. This should be in a background thread.
 * Perhaps a generic method to handle this will be advisable.... but we do that later.
 *
 * To start with we will work from the Posted service.
 *
 * 
 *
 * So Reputation....
 *   Three components:
 *     1) Your Opinion: Should override everything else.
 *     2) Implicit Factors: Know the associated GPG Key.
 *     3) Your Friends Opinions: 
 *     4) Your Friends Calculated Reputation Scores.
 *
 * Must make sure that there is no Feedback loop in the Reputation calculation.
 *
 * So: Our Score + Friends Scores => Local Reputation.
 *  Local Reputation + Friends Reputations => Final Reputation?
 *
 * Do we need to 'ignore' Non-scores?
 *   ---> This becomes like the "Best Comment" algorithm from Reddit...
 *   Use a statistical mechanism to work out a lower bound on Reputation.
 *
 * But what if your opinion is wrong?.... well likely your friends will
 * get their messages and reply... you'll see the missing message - request it - check reputation etc.
 *
 * So we are going to have three different scores (Own, Peers, (the neighbour) Hood)...
 *
 * So next question, when do we need to incrementally calculate the score?
 *  .... how often do we need to recalculate everything -> this could lead to a flux of messages. 
 *
 *
 * 
 * MORE NOTES:
 *
 *   The Opinion Messages will have to be signed by PGP or SSL Keys, to guarantee that we don't 
 * multiple votes per person... As the message system doesn't handle uniqueness in this respect, 
 * we might have to do FULL_CALC for everything - This bit TODO.
 *
 * This will make IdService quite different to the other GXS services.
 */

/************************************************************************************/
/*
 * Processing Algorithm:
 *  - Grab all Groups which have received messages. 
 *  (opt 1)-> grab latest msgs for each of these and process => score.
 *  (opt 2)-> try incremental system (people probably won't change opinions often -> just set them once)
 *      --> if not possible, fallback to full calculation.
 *
 * 
 */

bool 	p3IdService::reputation_start()
{
	return false;
}


bool 	p3IdService::reputation_continue()
{
	return true;
}


#define ID_BACKGROUND_PERIOD	60

int	p3IdService::background_tick()
{
	std::cerr << "p3IdService::background_tick()";
	std::cerr << std::endl;

	// Run Background Stuff.	
	background_checkTokenRequest();

	/* every minute - run a background check */
	time_t now = time(NULL);
	bool doCheck = false;
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		if (now -  mLastBgCheck > ID_BACKGROUND_PERIOD)
		{
			doCheck = true;
			mLastBgCheck = now;
		}
	}

	if (doCheck)
	{
		//addExtraDummyData();
		background_requestGroups();
	}



	// Add in new votes + comments.
	return 0;
}




/***** Background Processing ****
 *
 * Process Each Message - as it arrives.
 *
 * Update 
 *
 */
#define ID_BG_IDLE						0
#define ID_BG_REQUEST_GROUPS			1
#define ID_BG_REQUEST_UNPROCESSED		2
#define ID_BG_REQUEST_FULLCALC			3

bool p3IdService::background_checkTokenRequest()
{
	uint32_t token = 0;
	uint32_t phase = 0;
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		if (!mBgProcessing)
		{
			return false;
		}

		token = mBgToken;
		phase = mBgPhase;
	}


	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;

        
	status = RsGenExchange::getTokenService()->requestStatus(token);
	//checkRequestStatus(token, status, reqtype, anstype, ts);
	
	if (status == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
	{
		switch(phase)
		{
			case ID_BG_REQUEST_GROUPS:
				background_requestNewMessages();
				break;
			case ID_BG_REQUEST_UNPROCESSED:
				background_processNewMessages();
				break;
			case ID_BG_REQUEST_FULLCALC:
				background_processFullCalc();
				break;
			default:
				break;
		}
	}
	return true;
}


bool p3IdService::background_requestGroups()
{
	std::cerr << "p3IdService::background_requestGroups()";
	std::cerr << std::endl;

	// grab all the subscribed groups.
	uint32_t token = 0;

	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

		if (mBgProcessing)
		{
			std::cerr << "p3IdService::background_requestGroups() ERROR Already processing, Skip this cycle";
			std::cerr << std::endl;
			return false;
		}

		mBgProcessing = true;
		mBgPhase = ID_BG_REQUEST_GROUPS;
		mBgToken = 0;
	}

	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
        RsTokReqOptions opts;
	std::list<std::string> groupIds;

/**
	TODO
	opts.mStatusFilter = RSGXS_GROUP_STATUS_NEWMSG;
	opts.mStatusMask = RSGXS_GROUP_STATUS_NEWMSG;
**/

	RsGenExchange::getTokenService()->requestGroupInfo(token, ansType, opts, groupIds);
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		mBgToken = token;
	}

	return true;
}



bool p3IdService::background_requestNewMessages()
{
	std::cerr << "p3IdService::background_requestNewMessages()";
	std::cerr << std::endl;

	std::list<RsGroupMetaData> modGroupList;
	std::list<RsGroupMetaData>::iterator it;

	std::list<std::string> groupIds;
	uint32_t token = 0;

	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		token = mBgToken;
	}

	if (!getGroupSummary(token, modGroupList))
	{
		std::cerr << "p3IdService::background_requestNewMessages() ERROR No Group List";
		std::cerr << std::endl;
		background_cleanup();
		return false;
	}

	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		mBgPhase = ID_BG_REQUEST_UNPROCESSED;
		mBgToken = 0;

		/* now we process the modGroupList -> a map so we can use it easily later, and create id list too */
		for(it = modGroupList.begin(); it != modGroupList.end(); it++)
		{
			/*** TODO
                        uint32_t dummyToken = 0;
			setGroupStatusFlags(dummyToken, it->mGroupId, 0, RSGXS_GROUP_STATUS_NEWMSG);
			***/

			mBgGroupMap[it->mGroupId] = *it;
			groupIds.push_back(it->mGroupId);
		}
	}

	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY; 
        RsTokReqOptions opts;
	token = 0;

	/* TODO 
	opts.mStatusFilter = RSGXS_MSG_STATUS_UNPROCESSED;
	opts.mStatusMask = RSGXS_MSG_STATUS_UNPROCESSED;
	*/

	RsGenExchange::getTokenService()->requestMsgInfo(token, ansType, opts, groupIds);

	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		mBgToken = token;
	}
	return true;
}


bool p3IdService::background_processNewMessages()
{
	std::cerr << "p3IdService::background_processNewMessages()";
	std::cerr << std::endl;

	GxsMsgMetaMap newMsgMap;
	GxsMsgMetaMap::iterator it;
	uint32_t token = 0;

	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		token = mBgToken;
	}

	if (!getMsgSummary(token, newMsgMap))
	{
		std::cerr << "p3IdService::background_processNewMessages() ERROR No New Msgs";
		std::cerr << std::endl;
		background_cleanup();
		return false;
	}

	/* iterate through the msgs.. update the mBgGroupMap with new data, 
	 * and flag these items as modified - so we rewrite them to the db later.
	 *
	 * If a message is not an original -> store groupId for requiring full analysis later.
         */

	std::map<std::string, RsGroupMetaData>::iterator mit;

	for (it = newMsgMap.begin(); it != newMsgMap.end(); it++)
	{
		std::vector<RsMsgMetaData>::iterator vit;
		for(vit = it->second.begin(); vit != it->second.end(); vit++)
		{
			std::cerr << "p3IdService::background_processNewMessages() new MsgId: " << vit->mMsgId;
			std::cerr << std::endl;
	
			/* flag each new vote as processed */
			/**
				TODO
			uint32_t dummyToken = 0;
			setMsgStatusFlags(dummyToken, it->mMsgId, 0, RSGXS_MSG_STATUS_UNPROCESSED);
			**/
	
			RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
	
			mit = mBgGroupMap.find(it->first);
			if (mit == mBgGroupMap.end())
			{
				std::cerr << "p3IdService::background_processNewMessages() ERROR missing GroupId: ";
				std::cerr << vit->mGroupId;
				std::cerr << std::endl;
	
				/* error */
				continue;
			}
	
			if (mit->second.mGroupStatus & ID_LOCAL_STATUS_FULL_CALC_FLAG)
			{
				std::cerr << "p3IdService::background_processNewMessages() Group Already marked FULL_CALC";
				std::cerr << std::endl;
	
				/* already marked */
				continue;
			}
	
			if (vit->mMsgId != vit->mOrigMsgId)
			{
				/*
				 *  not original -> hard, redo calc (alt: could substract previous score)
				 */
	
				std::cerr << "p3IdService::background_processNewMessages() Update, mark for FULL_CALC";
				std::cerr << std::endl;
	
				mit->second.mGroupStatus |= ID_LOCAL_STATUS_FULL_CALC_FLAG;
			}
			else
			{
				/*
				 * Try incremental calculation.
				 * - extract parameters from group.
				 * - increment, & save back.
				 * - flag group as modified.
				 */
	
				std::cerr << "p3IdService::background_processNewMessages() NewOpt, Try Inc Calc";
				std::cerr << std::endl;
	
				mit->second.mGroupStatus |= ID_LOCAL_STATUS_INC_CALC_FLAG;
	
				SSGxsIdGroup ssdata;
				if (!ssdata.load(mit->second.mServiceString))
				{
					/* error */
					std::cerr << "p3IdService::background_processNewMessages() ERROR Extracting";
					std::cerr << std::endl;
				}
	
				/* do calcs */
				std::cerr << "p3IdService::background_processNewMessages() Extracted: ";
				std::cerr << std::endl;
	
				/* store it back in */
				std::cerr << "p3IdService::background_processNewMessages() Stored: ";
				std::cerr << std::endl;

				std::string serviceString = ssdata.save();	
				if (0)
				{
					/* error */
					std::cerr << "p3IdService::background_processNewMessages() ERROR Storing";
					std::cerr << std::endl;
				}
			}
		}
	}


	/* now iterate through groups again 
	 *  -> update status as we go
	 *  -> record one requiring a full analyssis
	 */

	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

		std::cerr << "p3IdService::background_processNewMessages() Checking Groups for Calc Type";
		std::cerr << std::endl;

		for(mit = mBgGroupMap.begin(); mit != mBgGroupMap.end(); mit++)
		{
			if (mit->second.mGroupStatus & ID_LOCAL_STATUS_FULL_CALC_FLAG)
			{
				std::cerr << "p3IdService::background_processNewMessages() FullCalc for: ";
				std::cerr << mit->second.mGroupId;
				std::cerr << std::endl;

				mBgFullCalcGroups.push_back(mit->second.mGroupId);
			}
			else if (mit->second.mGroupStatus & ID_LOCAL_STATUS_INC_CALC_FLAG)
			{
				std::cerr << "p3IdService::background_processNewMessages() IncCalc done for: ";
				std::cerr << mit->second.mGroupId;
				std::cerr << std::endl;

				/* set Cache */
                                uint32_t dummyToken = 0;
				setGroupServiceString(dummyToken, mit->second.mGroupId, mit->second.mServiceString);
			}
			else
			{
				/* why is it here? error. */
				std::cerr << "p3IdService::background_processNewMessages() ERROR for: ";
				std::cerr << mit->second.mGroupId;
				std::cerr << std::endl;
			}
		}
	}

	return background_FullCalcRequest();
}



bool p3IdService::background_FullCalcRequest()
{
	/* 
	 * grab an GroupId from List.
	 *  - If empty, we are finished.
	 *  - request all latest mesgs
	 */
	
	std::list<std::string> groupIds;
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		mBgPhase = ID_BG_REQUEST_FULLCALC;
		mBgToken = 0;
		mBgGroupMap.clear();

		if (mBgFullCalcGroups.empty())
		{
			/* finished! */
			background_cleanup();
			return true;
	
		}
	
		groupIds.push_back(mBgFullCalcGroups.front());
		mBgFullCalcGroups.pop_front();
		
	}

	/* request the summary info from the parents */
	uint32_t ansType = RS_TOKREQ_ANSTYPE_DATA; 
	uint32_t token = 0;
        RsTokReqOptions opts;
	opts.mOptions = RS_TOKREQOPT_MSG_LATEST;

	RsGenExchange::getTokenService()->requestMsgInfo(token, ansType, opts, groupIds);

	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		mBgToken = token;
	}
	return true;
}




bool p3IdService::background_processFullCalc()
{
	std::cerr << "p3IdService::background_processFullCalc()";
	std::cerr << std::endl;

	std::list<RsMsgMetaData> msgList;
	std::list<RsMsgMetaData>::iterator it;


	bool validmsgs = false;

	/* calc variables */
	uint32_t opinion_count = 0;
	uint32_t opinion_nullcount = 0;
	double   opinion_sum = 0;
	double   opinion_sumsq = 0;

	uint32_t rep_count = 0;
	uint32_t rep_nullcount = 0;
	double   rep_sum = 0;
	double   rep_sumsq = 0;

        std::vector<RsGxsIdOpinion> opinions;
        std::vector<RsGxsIdOpinion>::iterator vit;

        if (!getMsgData(mBgToken, opinions))
	{
		std::cerr << "p3IdService::background_processFullCalc() ERROR Failed to get Opinions";
		std::cerr << std::endl;

		return false;
	}

	std::string groupId;
        for(vit = opinions.begin(); vit != opinions.end(); vit++)
	{
		RsGxsIdOpinion &opinion = *vit;

		/* These should all be same group - should check for sanity! */
		if (groupId == "")
		{
			groupId = opinion.mMeta.mGroupId;
		}

		std::cerr << "p3IdService::background_processFullCalc() Msg:";
		std::cerr << opinion;
		std::cerr << std::endl;

		validmsgs = true;

		/* for each opinion.... extract score, and reputation */
		if (opinion.mOpinion != 0)
		{
			opinion_count++;
			opinion_sum += opinion.mOpinion;
			opinion_sum += (opinion.mOpinion * opinion.mOpinion);
		}
		else
		{
			opinion_nullcount++;
		}
		

		/* for each opinion.... extract score, and reputation */
		if (opinion.mReputation != 0)
		{
			rep_nullcount++;
			rep_sum += opinion.mReputation;
			rep_sum += (opinion.mReputation * opinion.mReputation);
		}
		else
		{
			rep_nullcount++;
		}
	}

	double opinion_avg = 0;
	double opinion_var = 0;
	double opinion_frac = 0;

	double rep_avg = 0;
	double rep_var = 0;
	double rep_frac = 0;


	if (opinion_count)
	{
		opinion_avg = opinion_sum / opinion_count;
		opinion_var = (opinion_sumsq  - opinion_count * opinion_avg * opinion_avg) / opinion_count;
		opinion_frac = opinion_count / ((float) (opinion_count + opinion_nullcount));
	}

	if (rep_count)
	{
		rep_avg = rep_sum / rep_count;
		rep_var = (rep_sumsq  - rep_count * rep_avg * rep_avg) / rep_count;
		rep_frac = rep_count / ((float) (rep_count + rep_nullcount));
	}


	if (validmsgs)
	{
		SSGxsIdGroup ssdata;
		std::string serviceString = ssdata.save();

		std::cerr << "p3IdService::background_updateVoteCounts() Encoded String: " << serviceString;
		std::cerr << std::endl;
		/* store new result */
		uint32_t dummyToken = 0;
		setGroupServiceString(dummyToken, groupId, serviceString);
	}

	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		mBgPhase = ID_BG_IDLE;
		mBgToken = 0;
	}

	return background_FullCalcRequest();
}


bool p3IdService::background_cleanup()
{
	std::cerr << "p3IdService::background_cleanup()";
	std::cerr << std::endl;

	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

	// Cleanup.
	mBgProcessing = false;
	mBgPhase = ID_BG_IDLE;
	mBgToken = 0;
	mBgGroupMap.clear();
	mBgFullCalcGroups.clear();
	
	return true;
}


std::ostream &operator<<(std::ostream &out, const RsGxsIdGroup &grp)
{
	out << "RsGxsIdGroup: Meta: " << grp.mMeta;
	out << " PgpIdHash: " << grp.mPgpIdHash;
	out << " PgpIdSign: [binary]"; // << grp.mPgpIdSign;
	out << std::endl;
	
	return out;
}

std::ostream &operator<<(std::ostream &out, const RsGxsIdOpinion &opinion)
{
	out << "RsGxsIdOpinion: Meta: " << opinion.mMeta;
	out << std::endl;
	
	return out;
}

