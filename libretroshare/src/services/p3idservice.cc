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

#include <unistd.h>

#include "services/p3idservice.h"
#include "pgp/pgpauxutils.h"
#include "serialiser/rsgxsiditems.h"
#include "serialiser/rsconfigitems.h"
#include "retroshare/rsgxsflags.h"
#include "util/rsrandom.h"
#include "util/rsstring.h"
#include "util/radix64.h"
#include "gxs/gxssecurity.h"


//#include "pqi/authgpg.h"

//#include <retroshare/rspeers.h>

#include <sstream>
#include <stdio.h>

/****
 * #define DEBUG_IDS	1
 * #define DEBUG_RECOGN	1
 * #define DEBUG_OPINION 1
 * #define GXSID_GEN_DUMMY_DATA	1
 ****/

#define ID_REQUEST_LIST		0x0001
#define ID_REQUEST_IDENTITY	0x0002
#define ID_REQUEST_REPUTATION	0x0003
#define ID_REQUEST_OPINION	0x0004

static const uint32_t MAX_KEEP_UNUSED_KEYS      = 30*86400 ; // remove unused keys after 30 days
static const uint32_t MAX_DELAY_BEFORE_CLEANING =      601 ; // clean old keys every 10 mins

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
#define BG_RECOGN 	2
#define BG_REPUTATION 	3


#define GXSIDREQ_CACHELOAD	0x0001
#define GXSIDREQ_CACHEOWNIDS	0x0002

#define GXSIDREQ_PGPHASH 	0x0010
#define GXSIDREQ_RECOGN 	0x0020

#define GXSIDREQ_OPINION 	0x0030

#define GXSIDREQ_CACHETEST 	0x1000

// Events.
#define GXSID_EVENT_CACHEOWNIDS		0x0001
#define GXSID_EVENT_CACHELOAD 		0x0002

#define GXSID_EVENT_PGPHASH 		0x0010
#define GXSID_EVENT_PGPHASH_PROC 	0x0011

#define GXSID_EVENT_RECOGN 		0x0020
#define GXSID_EVENT_RECOGN_PROC 	0x0021

#define GXSID_EVENT_REPUTATION 		0x0030

#define GXSID_EVENT_CACHETEST 		0x1000

#define GXSID_EVENT_DUMMYDATA		0x2000
#define GXSID_EVENT_DUMMY_OWNIDS	0x2001
#define GXSID_EVENT_DUMMY_PGPID		0x2002
#define GXSID_EVENT_DUMMY_UNKNOWN_PGPID	0x2003
#define GXSID_EVENT_DUMMY_PSEUDOID	0x2004
#define GXSID_EVENT_REQUEST_IDS     0x2005


/* delays */

#define CACHETEST_PERIOD	        60
#define DELAY_BETWEEN_CONFIG_UPDATES   300

#define OWNID_RELOAD_DELAY		10

#define PGPHASH_PERIOD			60
#define PGPHASH_RETRY_PERIOD		11
#define PGPHASH_PROC_PERIOD		1

#define RECOGN_PERIOD			90
#define RECOGN_RETRY_PERIOD		17
#define RECOGN_PROC_PERIOD		1

#define REPUTATION_PERIOD		60
#define REPUTATION_RETRY_PERIOD		13
#define REPUTATION_PROC_PERIOD		1

/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3IdService::p3IdService(RsGeneralDataService *gds, RsNetworkExchangeService *nes, PgpAuxUtils *pgpUtils)
	: RsGxsIdExchange(gds, nes, new RsGxsIdSerialiser(), RS_SERVICE_GXS_TYPE_GXSID, idAuthenPolicy()), 
	RsIdentity(this), GxsTokenQueue(this), RsTickEvent(), 
	mPublicKeyCache(DEFAULT_MEM_CACHE_SIZE, "GxsIdPublicKeyCache"), 
	mPrivateKeyCache(DEFAULT_MEM_CACHE_SIZE, "GxsIdPrivateKeyCache"), 
	mIdMtx("p3IdService"), mNes(nes),
	mPgpUtils(pgpUtils)
{
	mBgSchedule_Mode = 0;
    mBgSchedule_Active = false;
    mLastKeyCleaningTime = 0 ;
    mLastConfigUpdate = 0 ;
    mOwnIdsLoaded = false ;

	// Kick off Cache Testing, + Others.
	RsTickEvent::schedule_in(GXSID_EVENT_PGPHASH, PGPHASH_PERIOD);
	RsTickEvent::schedule_in(GXSID_EVENT_REPUTATION, REPUTATION_PERIOD);
	RsTickEvent::schedule_now(GXSID_EVENT_CACHEOWNIDS);

	//RsTickEvent::schedule_in(GXSID_EVENT_CACHETEST, CACHETEST_PERIOD);

#ifdef GXSID_GEN_DUMMY_DATA
	//RsTickEvent::schedule_now(GXSID_EVENT_DUMMYDATA);
#endif

	loadRecognKeys();
}

const std::string GXSID_APP_NAME = "gxsid";
const uint16_t GXSID_APP_MAJOR_VERSION  =       1;
const uint16_t GXSID_APP_MINOR_VERSION  =       0;
const uint16_t GXSID_MIN_MAJOR_VERSION  =       1;
const uint16_t GXSID_MIN_MINOR_VERSION  =       0;

RsServiceInfo p3IdService::getServiceInfo()
{
        return RsServiceInfo(RS_SERVICE_GXS_TYPE_GXSID,
                GXSID_APP_NAME,
                GXSID_APP_MAJOR_VERSION,
                GXSID_APP_MINOR_VERSION,
                GXSID_MIN_MAJOR_VERSION,
                GXSID_MIN_MINOR_VERSION);
}



void p3IdService::setNes(RsNetworkExchangeService *nes)
{
    RsStackMutex stack(mIdMtx);
    mNes = nes;
}

uint32_t p3IdService::idAuthenPolicy()
{
	uint32_t policy = 0;
	uint8_t flag = 0;

	// Messages are send reputations. normally not by ID holder - so need signatures.
	flag = GXS_SERV::MSG_AUTHEN_ROOT_AUTHOR_SIGN | GXS_SERV::MSG_AUTHEN_CHILD_AUTHOR_SIGN;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PUBLIC_GRP_BITS);
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::RESTRICTED_GRP_BITS);
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PRIVATE_GRP_BITS);

	// No ID required.
	flag = 0;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::GRP_OPTION_BITS);

	return policy;
}
void p3IdService::slowIndicateConfigChanged()
{
    time_t now = time(NULL) ;

    if(mLastConfigUpdate + DELAY_BETWEEN_CONFIG_UPDATES < now)
    {
        IndicateConfigChanged() ;
    mLastConfigUpdate = now ;
    }
}
time_t p3IdService::locked_getLastUsageTS(const RsGxsId& gxs_id)
{
    std::map<RsGxsId,time_t>::const_iterator it = mKeysTS.find(gxs_id) ;

    if(it == mKeysTS.end())
    {
        slowIndicateConfigChanged() ;
        return mKeysTS[gxs_id] = time(NULL) ;
    }
    else
        return it->second ;
}
void p3IdService::timeStampKey(const RsGxsId& gxs_id)
{
    RS_STACK_MUTEX(mIdMtx) ;

    mKeysTS[gxs_id] = time(NULL) ;

        slowIndicateConfigChanged() ;
}

bool p3IdService::loadList(std::list<RsItem*>& items)
{
    RS_STACK_MUTEX(mIdMtx) ;
    RsGxsIdLocalInfoItem *lii;

    for(std::list<RsItem*>::const_iterator it = items.begin();it!=items.end();++it)
        if( (lii = dynamic_cast<RsGxsIdLocalInfoItem*>(*it)) != NULL)
            for(std::map<RsGxsId,time_t>::const_iterator it2 = lii->mTimeStamps.begin();it2!=lii->mTimeStamps.end();++it2)
                mKeysTS.insert(*it2) ;
    return true ;
}

bool p3IdService::saveList(bool& cleanup,std::list<RsItem*>& items)
{
    std::cerr << "p3IdService::saveList()" << std::endl;

    RS_STACK_MUTEX(mIdMtx) ;
    cleanup = true ;
    RsGxsIdLocalInfoItem *item = new RsGxsIdLocalInfoItem ;
    item->mTimeStamps = mKeysTS ;

    items.push_back(item) ;
    return true ;
}
void p3IdService::cleanUnusedKeys()
{
    std::list<RsGxsId> ids_to_delete ;

    // we need to stash all ids to delete into an off-mutex structure since deleteIdentity() will trigger the lock
    {
        RS_STACK_MUTEX(mIdMtx) ;

        if(!mOwnIdsLoaded)
        {
            std::cerr << "(EE) Own ids not loaded. Cannot clean unused keys." << std::endl;
            return ;
        }

    // grab at most 10 identities to delete. No need to send too many requests to the token queue at once.
        time_t now = time(NULL) ;
    int n=0 ;

        for(std::map<RsGxsId,time_t>::iterator it(mKeysTS.begin());it!=mKeysTS.end() && n < 10;++it)
            if(it->second + MAX_KEEP_UNUSED_KEYS < now && std::find(mOwnIds.begin(),mOwnIds.end(),it->first) == mOwnIds.end())
                ids_to_delete.push_back(it->first),++n ;
    }

    for(std::list<RsGxsId>::const_iterator it(ids_to_delete.begin());it!=ids_to_delete.end();++it)
    {
        std::cerr << "Deleting identity " << *it << " which is too old." << std::endl;
        uint32_t token ;
        RsGxsIdGroup group;
        group.mMeta.mGroupId=RsGxsGroupId(*it);
        rsIdentity->deleteIdentity(token, group);

        {
            RS_STACK_MUTEX(mIdMtx) ;
            std::map<RsGxsId,time_t>::iterator tmp = mKeysTS.find(*it) ;

            if(mKeysTS.end() != tmp)
                mKeysTS.erase(tmp) ;
        }
    }
}

void	p3IdService::service_tick()
{
	RsTickEvent::tick_events();
    GxsTokenQueue::checkRequests(); // GxsTokenQueue handles all requests.

    time_t now = time(NULL) ;

    if(mLastKeyCleaningTime + MAX_DELAY_BEFORE_CLEANING < now)
    {
        cleanUnusedKeys() ;
        mLastKeyCleaningTime = now ;
    }
	return;
}

void p3IdService::notifyChanges(std::vector<RsGxsNotify *> &changes)
{
#ifdef DEBUG_IDS
	std::cerr << "p3IdService::notifyChanges()";
	std::cerr << std::endl;
#endif

	/* iterate through and grab any new messages */
	std::list<RsGxsGroupId> unprocessedGroups;

	std::vector<RsGxsNotify *>::iterator it;
	for(it = changes.begin(); it != changes.end(); ++it)
	{
	       RsGxsGroupChange *groupChange = dynamic_cast<RsGxsGroupChange *>(*it);
	       RsGxsMsgChange *msgChange = dynamic_cast<RsGxsMsgChange *>(*it);
	       if (msgChange && !msgChange->metaChange())
	       {
#ifdef DEBUG_IDS
			std::cerr << "p3IdService::notifyChanges() Found Message Change Notification";
			std::cerr << std::endl;
#endif

			std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &msgChangeMap = msgChange->msgChangeMap;
			std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >::iterator mit;
			for(mit = msgChangeMap.begin(); mit != msgChangeMap.end(); ++mit)
			{
#ifdef DEBUG_IDS
				std::cerr << "p3IdService::notifyChanges() Msgs for Group: " << mit->first;
				std::cerr << std::endl;
#endif
			}
	       }

	       /* shouldn't need to worry about groups - as they need to be subscribed to */
		if (groupChange && !groupChange->metaChange())
		{
#ifdef DEBUG_IDS
			std::cerr << "p3IdService::notifyChanges() Found Group Change Notification";
			std::cerr << std::endl;
#endif

			std::list<RsGxsGroupId> &groupList = groupChange->mGrpIdList;
			std::list<RsGxsGroupId>::iterator git;
			for(git = groupList.begin(); git != groupList.end(); ++git)
			{
#ifdef DEBUG_IDS
				std::cerr << "p3IdService::notifyChanges() Auto Subscribe to Incoming Groups: " << *git;
				std::cerr << std::endl;
#endif

				uint32_t token;
				RsGenExchange::subscribeToGroup(token, *git, true);

			}
		}
	}

	RsGxsIfaceHelper::receiveChanges(changes);
}

/********************************************************************************/
/******************* RsIdentity Interface ***************************************/
/********************************************************************************/

#if 0
bool p3IdService:: getNickname(const RsGxsId &id, std::string &nickname)
{
	return false;
}
#endif

bool p3IdService:: getIdDetails(const RsGxsId &id, RsIdentityDetails &details)
{
#ifdef DEBUG_IDS
	std::cerr << "p3IdService::getIdDetails(" << id << ")";
	std::cerr << std::endl;
#endif

    {
        RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
        RsGxsIdCache data;
        if (mPublicKeyCache.fetch(id, data))
        {
            details = data.details;
            details.mLastUsageTS = locked_getLastUsageTS(id) ;

        if(details.mNickname.length() > 200)
            details.mNickname = "[too long a name]" ;

            return true;
        }

        /* try private cache too */
        if (mPrivateKeyCache.fetch(id, data))
        {
            details = data.details;
            details.mLastUsageTS = locked_getLastUsageTS(id) ;
            return true;
        }
    }

	/* it isn't there - add to public requests */
    cache_request_load(id);

	return false;
}


bool p3IdService::isOwnId(const RsGxsId& id)
{
    RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

    return std::find(mOwnIds.begin(),mOwnIds.end(),id) != mOwnIds.end() ;
}
bool p3IdService::getOwnIds(std::list<RsGxsId> &ownIds)
{
    RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

    if(!mOwnIdsLoaded)
    {
        std::cerr << "p3IdService::getOwnIds(): own identities are not loaded yet." << std::endl;
        return false ;
    }

    ownIds = mOwnIds;
    return true ;
}


bool p3IdService::createIdentity(uint32_t& token, RsIdentityParameters &params)
{

	RsGxsIdGroup id;

    id.mMeta.mGroupName = params.nickname;
    id.mImage = params.mImage;

	if (params.isPgpLinked)
	{
		id.mMeta.mGroupFlags = RSGXSID_GROUPFLAG_REALID;
	}
	else
	{
		id.mMeta.mGroupFlags = 0;
	}

	createGroup(token, id);

	return true;
}

bool p3IdService::updateIdentity(uint32_t& token, RsGxsIdGroup &group)
{
#ifdef DEBUG_IDS
	std::cerr << "p3IdService::updateIdentity()";
	std::cerr << std::endl;
#endif

	updateGroup(token, group);

	return false;
}

bool p3IdService::deleteIdentity(uint32_t& token, RsGxsIdGroup &group)
{
#ifdef DEBUG_IDS
	std::cerr << "p3IdService::deleteIdentity()";
	std::cerr << std::endl;
#endif

	deleteGroup(token, group);

	return false;
}


bool p3IdService::parseRecognTag(const RsGxsId &id, const std::string &nickname,
			const std::string &tag, RsRecognTagDetails &details)
{
#ifdef DEBUG_RECOGN
	std::cerr << "p3IdService::parseRecognTag()";
	std::cerr << std::endl;
#endif

	RsGxsRecognTagItem *tagitem = RsRecogn::extractTag(tag);
	if (!tagitem)
	{
		return false;
	}

	bool isPending = false;
	bool isValid = recogn_checktag(id, nickname, tagitem, true, isPending);

	details.valid_from = tagitem->valid_from;
	details.valid_to = tagitem->valid_to;
	details.tag_class = tagitem->tag_class;
	details.tag_type = tagitem->tag_type;
	details.signer = tagitem->sign.keyId.toStdString();

	details.is_valid = isValid;
	details.is_pending = isPending;

	delete tagitem;

	return true;
}

bool p3IdService::getRecognTagRequest(const RsGxsId &id, const std::string &comment, uint16_t tag_class, uint16_t tag_type, std::string &tag)
{
#ifdef DEBUG_RECOGN
	std::cerr << "p3IdService::getRecognTagRequest()";
	std::cerr << std::endl;
#endif

	if (!havePrivateKey(id))
	{
#ifdef DEBUG_RECOGN
		std::cerr << "p3IdService::getRecognTagRequest() Dont have private key";
		std::cerr << std::endl;
#endif
		// attempt to load it.
		cache_request_load(id);
		return false;
	}

	RsTlvSecurityKey key;
	std::string nickname;

	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		RsGxsIdCache data;
		if (!mPrivateKeyCache.fetch(id, data))
		{
#ifdef DEBUG_RECOGN
			std::cerr << "p3IdService::getRecognTagRequest() Cache failure";
			std::cerr << std::endl;
#endif
			return false;
		}

		key = data.pubkey;
		nickname = data.details.mNickname;
	}

	return RsRecogn::createTagRequest(key, id, nickname, tag_class, tag_type, comment,  tag);
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
	else
	{
		if(isPendingNetworkRequest(id))
			return true;
	}


    return cache_request_load(id, peers);
}

bool p3IdService::isPendingNetworkRequest(const RsGxsId& gxsId) const
{
    RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
	// if ids has beens confirmed as not physically present return
	// immediately, id will be removed from list if found by auto nxs net search
	if(mIdsNotPresent.find(gxsId) != mIdsNotPresent.end())
		return true;

	return false;
}

bool p3IdService::getKey(const RsGxsId &id, RsTlvSecurityKey &key)
{
    {
	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
	RsGxsIdCache data;
	if (mPublicKeyCache.fetch(id, data))
	{
		key = data.pubkey;
        return true;
    }
    }

    cache_request_load(id);

    key.keyId.clear() ;
    return false;
}

bool p3IdService::requestPrivateKey(const RsGxsId &id)
{
	if (havePrivateKey(id))
        return true;

    return cache_request_load(id);
}

bool p3IdService::getPrivateKey(const RsGxsId &id, RsTlvSecurityKey &key)
{
    {
        RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
        RsGxsIdCache data;
        if (mPrivateKeyCache.fetch(id, data))
        {
            key = data.pubkey;
            return true;
        }
    }

    key.keyId.clear() ;
    cache_request_load(id);

    return false ;
}


bool p3IdService::signData(const uint8_t *data,uint32_t data_size,const RsGxsId& own_gxs_id,RsTlvKeySignature& signature,uint32_t& error_status)
{
    //RsIdentityDetails details  ;
    RsTlvSecurityKey signature_key ;

    //getIdDetails(own_gxs_id,details);

    int i ;
    for(i=0;i<6;++i)
        if(!getPrivateKey(own_gxs_id,signature_key) || signature_key.keyData.bin_data == NULL)
        {
#ifdef DEBUG_IDS
            std::cerr << "  Cannot get key. Waiting for caching. try " << i << "/6" << std::endl;
#endif
            usleep(500 * 1000) ;	// sleep for 500 msec.
        }
        else
            break ;

    if(i == 6)
    {
        std::cerr << "  (EE) Could not retrieve own private key for ID = " << own_gxs_id << ". Giging up sending DH session params. This should not happen." << std::endl;
    error_status = RS_GIXS_ERROR_KEY_NOT_AVAILABLE ;
        return false ;
    }

#ifdef DEBUG_IDS
    std::cerr << "  Signing..." << std::endl;
#endif

    if(!GxsSecurity::getSignature((char *)data,data_size,signature_key,signature))
    {
        std::cerr << "  (EE) Cannot sign for id " << own_gxs_id << ". Signature call failed." << std::endl;
    error_status = RS_GIXS_ERROR_UNKNOWN ;
        return false ;
    }
    error_status = RS_GIXS_ERROR_NO_ERROR ;
    timeStampKey(own_gxs_id) ;
    return true ;
}
bool p3IdService::validateData(const uint8_t *data,uint32_t data_size,const RsTlvKeySignature& signature,bool force_load,uint32_t& signing_error)
{
   // RsIdentityDetails details ;
   // getIdDetails(signature.keyId,details);
    RsTlvSecurityKey signature_key ;

    for(int i=0;i< (force_load?6:1);++i)
        if(!getKey(signature.keyId,signature_key) || signature_key.keyData.bin_data == NULL)
        {
#ifdef DEBUG_IDS
            std::cerr << "  Cannot get key. Waiting for caching. try " << i << "/6" << std::endl;
#endif
            usleep(500 * 1000) ;	// sleep for 500 msec.
        }
        else
            break ;

    if(signature_key.keyData.bin_data == NULL)
    {
        std::cerr << "(EE) Cannot validate signature for unknown key " << signature.keyId << std::endl;
        signing_error = RS_GIXS_ERROR_KEY_NOT_AVAILABLE ;
        return false;
    }

    if(!GxsSecurity::validateSignature((char*)data,data_size,signature_key,signature))
    {
        std::cerr << "(EE) Signature was verified and it doesn't check! This is a security issue!" << std::endl;
        signing_error = RS_GIXS_ERROR_SIGNATURE_MISMATCH ;
        return false;
    }
    signing_error = RS_GIXS_ERROR_NO_ERROR ;

    timeStampKey(signature.keyId) ;
    return true ;
}
bool p3IdService::encryptData(const uint8_t *decrypted_data,uint32_t decrypted_data_size,uint8_t *& encrypted_data,uint32_t& encrypted_data_size,const RsGxsId& encryption_key_id,bool force_load,uint32_t& error_status)
{
    RsTlvSecurityKey encryption_key ;

    // get the key, and let the cache find it.
    for(int i=0;i<(force_load?6:1);++i)
        if(getKey(encryption_key_id,encryption_key))
            break ;
        else
            usleep(500*1000) ; // sleep half a sec.

    if(encryption_key.keyId.isNull())
    {
        std::cerr << "    (EE) Cannot get encryption key for id " << encryption_key_id << std::endl;
        error_status = RS_GIXS_ERROR_KEY_NOT_AVAILABLE ;
        return false ;
    }

    if(!GxsSecurity::encrypt(encrypted_data,encrypted_data_size,decrypted_data,decrypted_data_size,encryption_key))
    {
        std::cerr << "    (EE) Encryption failed." << std::endl;
        error_status = RS_GIXS_ERROR_UNKNOWN ;
        return false ;
    }
    error_status = RS_GIXS_ERROR_NO_ERROR ;
    timeStampKey(encryption_key_id) ;

    return true ;
}

bool p3IdService::decryptData(const uint8_t *encrypted_data,uint32_t encrypted_data_size,uint8_t *& decrypted_data,uint32_t& decrypted_size,const RsGxsId& key_id,uint32_t& error_status)
{
    RsTlvSecurityKey encryption_key ;

    // Get the key, and let the cache find it. It's our own key, so we should be able to find it, even if it takes
    // some seconds.

    for(int i=0;i<4;++i)
        if(getPrivateKey(key_id,encryption_key))
            break ;
        else
            usleep(500*1000) ; // sleep half a sec.

    if(encryption_key.keyId.isNull())
    {
        std::cerr << "  (EE) Cannot get own encryption key for id " << key_id << " to decrypt data. This should not happen." << std::endl;
        error_status = RS_GIXS_ERROR_KEY_NOT_AVAILABLE;
        return false ;
    }

    if(!GxsSecurity::decrypt(decrypted_data,decrypted_size,encrypted_data,encrypted_data_size,encryption_key))
    {
        std::cerr << "  (EE) Decryption failed." << std::endl;
        error_status = RS_GIXS_ERROR_UNKNOWN ;
        return false ;
    }
    error_status = RS_GIXS_ERROR_NO_ERROR ;
    timeStampKey(key_id) ;

    return true ;
}


/********************************************************************************/
/******************* RsGixsReputation     ***************************************/
/********************************************************************************/

bool p3IdService::haveReputation(const RsGxsId &id)
{
	return haveKey(id);
}

bool p3IdService::loadReputation(const RsGxsId &id, const std::list<PeerId>& peers)
{
	if (haveKey(id))
		return true;
	else
	{
		if(isPendingNetworkRequest(id))
			return true;
	}


	return cache_request_load(id, peers);
}

bool p3IdService::getReputation(const RsGxsId &id, GixsReputation &rep)
{
	/* this is the key part for accepting messages */

	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
	RsGxsIdCache data;
	if (mPublicKeyCache.fetch(id, data))
	{
		rep.id = id;
                rep.score = data.details.mReputation.mOverallScore;
#ifdef DEBUG_IDS
                std::cerr << "p3IdService::getReputation() id: ";
                std::cerr << id.toStdString() << " score: " <<
  rep.score;
                std::cerr << std::endl;
#endif

		return true;
	}
	else
	{
#ifdef DEBUG_IDS
              std::cerr << "p3IdService::getReputation() id: ";
              std::cerr << id.toStdString() << " not cached";
              std::cerr << std::endl;
#endif
	}
	return false;
}

#if 0
class RegistrationRequest
{
	public:
	RegistrationRequest(uint32_t token, RsGxsId &id, int score)
	:m_extToken(token), m_id(id), m_score(score) { return; }

	uint32_t m_intToken;
	uint32_t m_extToken;
	RsGxsId m_id;
	int m_score;
};
#endif


bool p3IdService::submitOpinion(uint32_t& token, const RsGxsId &id, bool absOpinion, int score)
{
#ifdef DEBUG_OPINION
	std::cerr << "p3IdService::submitOpinion()";
	std::cerr << std::endl;
#endif

	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

	token = RsGenExchange::generatePublicToken();

	uint32_t intToken;
	std::list<RsGxsGroupId> groups;
	groups.push_back(RsGxsGroupId(id));

	RsGenExchange::getTokenService()->requestGroupInfo(intToken, ansType, opts, groups);
	GxsTokenQueue::queueRequest(intToken, GXSIDREQ_OPINION);

	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
	mPendingOpinion[intToken] = OpinionRequest(token, id, absOpinion, score);
	return true;
}



bool p3IdService::opinion_handlerequest(uint32_t token)
{
#ifdef DEBUG_OPINION
	std::cerr << "p3IdService::opinion_handlerequest()";
	std::cerr << std::endl;
#endif

	OpinionRequest req;
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

		/* find in pendingReputation */
		std::map<uint32_t, OpinionRequest>::iterator it;
		it = mPendingOpinion.find(token);
		if (it == mPendingOpinion.end())
		{
			std::cerr << "p3IdService::opinion_handlerequest() ERROR finding PendingOpinion";
			std::cerr << std::endl;

			return false;
		}
		req = it->second;
		mPendingOpinion.erase(it);
	}

#ifdef DEBUG_OPINION
	std::cerr << "p3IdService::opinion_handlerequest() Id: " << req.mId << " score: " << req.mScore;
	std::cerr << std::endl;
#endif

        std::list<RsGroupMetaData> groups;
        std::list<RsGxsGroupId> groupList;

        if (!getGroupMeta(token, groups))
	{
		std::cerr << "p3IdService::opinion_handlerequest() ERROR getGroupMeta()";
		std::cerr << std::endl;

		updatePublicRequestStatus(req.mToken, RsTokenService::GXS_REQUEST_V2_STATUS_FAILED);
		return false;
	}

	if (groups.size() != 1)
	{
		std::cerr << "p3IdService::opinion_handlerequest() ERROR group.size() != 1";
		std::cerr << std::endl;

		// error.
		updatePublicRequestStatus(req.mToken, RsTokenService::GXS_REQUEST_V2_STATUS_FAILED);
		return false;
	}
        RsGroupMetaData &meta = *(groups.begin());

	if (meta.mGroupId != RsGxsGroupId(req.mId))
	{
		std::cerr << "p3IdService::opinion_handlerequest() ERROR Id mismatch";
		std::cerr << std::endl;

		// error.
		updatePublicRequestStatus(req.mToken, RsTokenService::GXS_REQUEST_V2_STATUS_FAILED);
		return false;
	}

	/* get the string */
	SSGxsIdGroup ssdata;
	ssdata.load(meta.mServiceString); // attempt load - okay if fails.

	/* modify score */
	if (req.mAbsOpinion)
	{
		ssdata.score.rep.mOwnOpinion = req.mScore;
	}
	else
	{
		ssdata.score.rep.mOwnOpinion += req.mScore;
	}

	// update IdScore too.
	bool pgpId = (meta.mGroupFlags & RSGXSID_GROUPFLAG_REALID);
	ssdata.score.rep.updateIdScore(pgpId, ssdata.pgp.idKnown);
	ssdata.score.rep.update();

	/* save string */
	std::string serviceString = ssdata.save();
#ifdef DEBUG_OPINION
	std::cerr << "p3IdService::opinion_handlerequest() new service_string: " << serviceString;
	std::cerr << std::endl;
#endif

	/* set new Group ServiceString */
	uint32_t dummyToken = 0;
	setGroupServiceString(dummyToken, meta.mGroupId, serviceString);
	cache_update_if_cached(RsGxsId(meta.mGroupId), serviceString);

	updatePublicRequestStatus(req.mToken, RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE);
	return true;
}


/********************************************************************************/
/******************* Get/Set Data      ******************************************/
/********************************************************************************/

RsSerialiser *p3IdService::setupSerialiser()
{
        RsSerialiser *rss = new RsSerialiser ;

        rss->addSerialType(new RsGxsIdSerialiser()) ;
        rss->addSerialType(new RsGeneralConfigSerialiser());

        return rss ;
}

bool p3IdService::getGroupData(const uint32_t &token, std::vector<RsGxsIdGroup> &groups)
{
	std::vector<RsGxsGrpItem*> grpData;
	bool ok = RsGenExchange::getGroupData(token, grpData);

	if(ok)
	{
		std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();

		for(; vit != grpData.end(); ++vit)
		{
			RsGxsIdGroupItem* item = dynamic_cast<RsGxsIdGroupItem*>(*vit);
			if (item)
			{
#ifdef DEBUG_IDS
        std::cerr << "p3IdService::getGroupData() Item is:";
        std::cerr << std::endl;
        item->print(std::cerr);
        std::cerr << std::endl;
#endif // DEBUG_IDS
        RsGxsIdGroup group ;
    item->toGxsIdGroup(group,false) ;

    {
        RS_STACK_MUTEX(mIdMtx) ;
    group.mLastUsageTS = locked_getLastUsageTS(RsGxsId(group.mMeta.mGroupId)) ;
    }

        // Decode information from serviceString.
        SSGxsIdGroup ssdata;
        if (ssdata.load(group.mMeta.mServiceString))
        {
            group.mPgpKnown = ssdata.pgp.idKnown;
            group.mPgpId    = ssdata.pgp.pgpId;
            group.mReputation = ssdata.score.rep;
#ifdef DEBUG_IDS
            std::cerr << "p3IdService::getGroupData() Success decoding ServiceString";
            std::cerr << std::endl;
            std::cerr << "\t mGpgKnown: " << group.mPgpKnown;
            std::cerr << std::endl;
            std::cerr << "\t mGpgId: " << group.mPgpId;
            std::cerr << std::endl;
#endif // DEBUG_IDS
				}
				else
				{
					group.mPgpKnown = false;
					group.mPgpId.clear();

                    std::cerr << "p3IdService::getGroupData() Failed to decode ServiceString \""
                          << group.mMeta.mServiceString << "\"" ;
					std::cerr << std::endl;
				}

				groups.push_back(group);
                delete(item);
			}
			else
			{
				std::cerr << "Not a Id Item, deleting!" << std::endl;
				delete(*vit);
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

    item->meta = group.mMeta;
    item->mImage.binData.setBinData(group.mImage.mData, group.mImage.mSize);

    RsGenExchange::publishGroup(token, item);
    return true;
}

bool 	p3IdService::updateGroup(uint32_t& token, RsGxsIdGroup &group)
{
    RsGxsId id(group.mMeta.mGroupId);
    RsGxsIdGroupItem* item = new RsGxsIdGroupItem();

    item->fromGxsIdGroup(group,false) ;

#ifdef DEBUG_IDS
	std::cerr << "p3IdService::updateGroup() Updating RsGxsId: " << id;
	std::cerr << std::endl;
#endif
	
    RsGenExchange::updateGroup(token, item);

	// if its in the cache - clear it.
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		if (mPublicKeyCache.erase(id))
		{
#ifdef DEBUG_IDS
			std::cerr << "p3IdService::updateGroup() Removed from PublicKeyCache";
			std::cerr << std::endl;
#endif
		}
		else
		{
#ifdef DEBUG_IDS
			std::cerr << "p3IdService::updateGroup() Not in PublicKeyCache";
			std::cerr << std::endl;
#endif
		}
	
		if (mPrivateKeyCache.erase(id))
		{
#ifdef DEBUG_IDS
			std::cerr << "p3IdService::updateGroup() Removed from PrivateKeyCache";
			std::cerr << std::endl;
#endif
		}
		else
		{
#ifdef DEBUG_IDS
			std::cerr << "p3IdService::updateGroup() Not in PrivateKeyCache";
			std::cerr << std::endl;
#endif
		}
	}

	return true;
}

bool 	p3IdService::deleteGroup(uint32_t& token, RsGxsIdGroup &group)
{
	RsGxsId id = RsGxsId(group.mMeta.mGroupId.toStdString());
    RsGxsIdGroupItem* item = new RsGxsIdGroupItem();

    item->fromGxsIdGroup(group,false) ;

#ifdef DEBUG_IDS
	std::cerr << "p3IdService::deleteGroup() Deleting RsGxsId: " << id;
	std::cerr << std::endl;
#endif

	RsGenExchange::deleteGroup(token, item);

	// if its in the cache - clear it.
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		if (mPublicKeyCache.erase(id))
		{
#ifdef DEBUG_IDS
			std::cerr << "p3IdService::deleteGroup() Removed from PublicKeyCache";
			std::cerr << std::endl;
#endif
		}
		else
		{
#ifdef DEBUG_IDS
			std::cerr << "p3IdService::deleteGroup() Not in PublicKeyCache";
			std::cerr << std::endl;
#endif
		}

		if (mPrivateKeyCache.erase(id))
		{
#ifdef DEBUG_IDS
			std::cerr << "p3IdService::deleteGroup() Removed from PrivateKeyCache";
			std::cerr << std::endl;
#endif
		}
		else
		{
#ifdef DEBUG_IDS
			std::cerr << "p3IdService::deleteGroup() Not in PrivateKeyCache";
			std::cerr << std::endl;
#endif
		}

		std::list<RsGxsId>::iterator lit = std::find( mOwnIds.begin(), mOwnIds.end(), id);
		if (lit != mOwnIds.end())
		{
			mOwnIds.remove((RsGxsId)*lit);
	#ifdef DEBUG_IDS
			std::cerr << "p3IdService::deleteGroup() Removed from OwnIds";
			std::cerr << std::endl;
	#endif
		}
		else
		{
	#ifdef DEBUG_IDS
			std::cerr << "p3IdService::deleteGroup() Not in OwnIds";
			std::cerr << std::endl;
	#endif
		}
	}

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
	uint32_t attempts = 0;
	if (1 == sscanf(input.c_str(), "K:1 I:%[^)]", pgpline))
	{
		idKnown = true;
		std::string str_line = pgpline;
		pgpId = RsPgpId(str_line);
		return true;
	}
	else if (2 == sscanf(input.c_str(), "K:0 T:%d C:%d", &timestamp, &attempts))
	{
		lastCheckTs = timestamp;
		checkAttempts = attempts;
		idKnown = false;
		return true;
	}
	else if (1 == sscanf(input.c_str(), "K:0 T:%d", &timestamp))
	{
		lastCheckTs = timestamp;
		checkAttempts = 0;
		idKnown = false;
		return true;
	}
	else 
	{
		lastCheckTs = 0;
		checkAttempts = 0;
		idKnown = false;
		return false;
	}
}

std::string SSGxsIdPgp::save() const
{
	std::string output;
	if (idKnown)
	{
		output += "K:1 I:";
		output += pgpId.toStdString();
	}
	else
	{
		rs_sprintf(output, "K:0 T:%d C:%d", lastCheckTs, checkAttempts);
	}
	return output;
}


/* Encoding / Decoding Group Service String stuff
 *
 * RecognTags.
 */

bool SSGxsIdRecognTags::load(const std::string &input)
{
    //char pgpline[RSGXSID_MAX_SERVICE_STRING];
	int pubTs = 0;
	int lastTs = 0;
	uint32_t flags = 0;

	if (3 == sscanf(input.c_str(), "F:%u P:%d T:%d", &flags, &pubTs, &lastTs))
	{
		publishTs = pubTs;
		lastCheckTs = lastTs;
		tagFlags = flags;
	}
	else
	{
		return false;
	}	
	return true;
}

std::string SSGxsIdRecognTags::save() const
{
	std::string output;
	rs_sprintf(output, "F:%u P:%d T:%d", tagFlags, publishTs, lastCheckTs);
	return output;
}

bool SSGxsIdRecognTags::tagsProcessed() const
{
	return (tagFlags & 0x1000);
}

bool SSGxsIdRecognTags::tagsPending() const
{
	return (tagFlags & 0x2000);
}

bool SSGxsIdRecognTags::tagValid(int i) const
{
	uint32_t idx = 0x01 << i;

#ifdef DEBUG_RECOGN
	std::cerr << "SSGxsIdRecognTags::tagValid(" << i << ") idx: " << idx;
	std::cerr << " result: " << (tagFlags & idx);
	std::cerr << std::endl;
#endif // DEBUG_RECOGN

	return (tagFlags & idx);
}

void SSGxsIdRecognTags::setTags(bool processed, bool pending, uint32_t flags)
{
	flags &= 0x00ff; // clear top bits;
	if (processed)
	{
		flags |= 0x1000;
	}
	if (pending)
	{
		flags |= 0x2000;
	}

#ifdef DEBUG_RECOGN
	std::cerr << "SSGxsIdRecognTags::setTags(" << processed << "," << pending << "," << flags << ")";
	std::cerr << " tagFlags: " << tagFlags;
	std::cerr << std::endl;
#endif // DEBUG_RECOGN

	tagFlags = flags;
}


GxsReputation::GxsReputation()
:mOverallScore(0), mIdScore(0), mOwnOpinion(0), mPeerOpinion(0)
{
	updateIdScore(false, false);
	update();
}

static const int kIdReputationPgpKnownScore 	= 50;
static const int kIdReputationPgpUnknownScore 	= 20;
static const int kIdReputationAnonScore 	= 5;

bool GxsReputation::updateIdScore(bool pgpLinked, bool pgpKnown)
{
	if (pgpLinked)
	{
		if (pgpKnown)
		{
			mIdScore = kIdReputationPgpKnownScore;
		}
		else
		{
			mIdScore = kIdReputationPgpUnknownScore;
		}
	}
	else
	{
		mIdScore = kIdReputationAnonScore;
	}
	return true;
}

bool GxsReputation::update()
{
	mOverallScore = mIdScore + mOwnOpinion + mPeerOpinion;
	return true;
}


bool SSGxsIdReputation::load(const std::string &input)
{
	return (4 == sscanf(input.c_str(), "%d %d %d %d", 
		&(rep.mOverallScore), &(rep.mIdScore), &(rep.mOwnOpinion), &(rep.mPeerOpinion)));
}

std::string SSGxsIdReputation::save() const
{
	std::string output;
	rs_sprintf(output, "%d %d %d %d", rep.mOverallScore, rep.mIdScore, rep.mOwnOpinion, rep.mPeerOpinion);
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
	char recognstr[RSGXSID_MAX_SERVICE_STRING];
	char scorestr[RSGXSID_MAX_SERVICE_STRING];
	
	// split into parts.
	if (3 != sscanf(input.c_str(), "v2 {P:%[^}]} {T:%[^}]} {R:%[^}]}", pgpstr, recognstr, scorestr))
	{
#ifdef DEBUG_IDS
		std::cerr << "SSGxsIdGroup::load() Failed to extract 4 Parts";
		std::cerr << std::endl;
#endif // DEBUG_IDS
		return false;
	}

	bool ok = true;
	if (pgp.load(pgpstr))
	{
#ifdef DEBUG_IDS
		std::cerr << "SSGxsIdGroup::load() pgpstr: " << pgpstr;
		std::cerr << std::endl;
#endif // DEBUG_IDS
	}
	else
	{
#ifdef DEBUG_IDS
		std::cerr << "SSGxsIdGroup::load() Invalid pgpstr: " << pgpstr;
		std::cerr << std::endl;
#endif // DEBUG_IDS
		ok = false;
	}

	if (recogntags.load(recognstr))
	{
#ifdef DEBUG_RECOGN
		std::cerr << "SSGxsIdGroup::load() recognstr: " << recognstr;
		std::cerr << std::endl;
#endif // DEBUG_RECOGN
	}
	else
	{
#ifdef DEBUG_RECOGN
		std::cerr << "SSGxsIdGroup::load() Invalid recognstr: " << recognstr;
		std::cerr << std::endl;
#endif // DEBUG_RECOGN
		ok = false;
	}

	if (score.load(scorestr))
	{
#ifdef DEBUG_IDS
		std::cerr << "SSGxsIdGroup::load() scorestr: " << scorestr;
		std::cerr << std::endl;
#endif // DEBUG_IDS
	}
	else
	{
#ifdef DEBUG_IDS
		std::cerr << "SSGxsIdGroup::load() Invalid scorestr: " << scorestr;
		std::cerr << std::endl;
#endif // DEBUG_IDS
		ok = false;
	}

#if 0
	if (opinion.load(opinionstr))
	{
#ifdef DEBUG_IDS
		std::cerr << "SSGxsIdGroup::load() opinionstr: " << opinionstr;
		std::cerr << std::endl;
#endif // DEBUG_IDS
	}
	else
	{
#ifdef DEBUG_IDS
		std::cerr << "SSGxsIdGroup::load() Invalid opinionstr: " << opinionstr;
		std::cerr << std::endl;
#endif // DEBUG_IDS
		ok = false;
	}

	if (reputation.load(repstr))
	{
#ifdef DEBUG_IDS
		std::cerr << "SSGxsIdGroup::load() repstr: " << repstr;
		std::cerr << std::endl;
#endif // DEBUG_IDS
	}
	else
	{
#ifdef DEBUG_IDS
		std::cerr << "SSGxsIdGroup::load() Invalid repstr: " << repstr;
		std::cerr << std::endl;
#endif // DEBUG_IDS
		ok = false;
	}
#endif

#ifdef DEBUG_IDS
	std::cerr << "SSGxsIdGroup::load() regurgitated: " << save();
	std::cerr << std::endl;

	std::cerr << "SSGxsIdGroup::load() isOkay?: " << ok;
	std::cerr << std::endl;
#endif // DEBUG_IDS
	return ok;
}

std::string SSGxsIdGroup::save() const
{
	std::string output = "v2 ";

	output += "{P:";
	output += pgp.save();
	output += "}";

	output += "{T:";
	output += recogntags.save();
	output += "}";

	output += "{R:";
	output += score.save();
	output += "}";

	//std::cerr << "SSGxsIdGroup::save() output: " << output;
	//std::cerr << std::endl;

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
{ 
	return; 
}

RsGxsIdCache::RsGxsIdCache(const RsGxsIdGroupItem *item, const RsTlvSecurityKey &in_pkey, const std::list<RsRecognTag> &tagList)
{
    // Save Keys.
    pubkey = in_pkey;

    // Save Time for ServiceString comparisions.
    mPublishTs = item->meta.mPublishTs;

    // Save RecognTags.
    mRecognTags = tagList;

    details.mAvatar.copy((uint8_t *) item->mImage.binData.bin_data, item->mImage.binData.bin_len);

    // Fill in Details.
    details.mNickname = item->meta.mGroupName;
    details.mId = RsGxsId(item->meta.mGroupId);

#ifdef DEBUG_IDS
    std::cerr << "RsGxsIdCache::RsGxsIdCache() for: " << details.mId;
    std::cerr << std::endl;
#endif // DEBUG_IDS

    details.mIsOwnId   = (item->meta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN);
    details.mPgpLinked = (item->meta.mGroupFlags & RSGXSID_GROUPFLAG_REALID);

    /* rest must be retrived from ServiceString */
    updateServiceString(item->meta.mServiceString);
}

void RsGxsIdCache::updateServiceString(std::string serviceString)
{
	details.mRecognTags.clear();

	SSGxsIdGroup ssdata;
	if (ssdata.load(serviceString))
	{
		if (details.mPgpLinked)
		{
			details.mPgpKnown = ssdata.pgp.idKnown;
			if (details.mPgpKnown)
			{
				details.mPgpId = ssdata.pgp.pgpId;
			}
			else
			{
				details.mPgpId.clear();
			}
		}


		// process RecognTags.
		if (ssdata.recogntags.tagsProcessed())
		{
#ifdef DEBUG_RECOGN
			std::cerr << "RsGxsIdCache::updateServiceString() updating recogntags";
			std::cerr << std::endl;
#endif // DEBUG_RECOGN
			if (ssdata.recogntags.publishTs == mPublishTs)
			{
				std::list<RsRecognTag>::iterator it;
				int i = 0;
				for(it = mRecognTags.begin(); it != mRecognTags.end(); ++it, i++)
				{
					if (ssdata.recogntags.tagValid(i) && it->valid)
					{
#ifdef DEBUG_RECOGN
						std::cerr << "RsGxsIdCache::updateServiceString() Valid Tag: " << it->tag_class << ":" << it->tag_type;
						std::cerr << std::endl;
#endif // DEBUG_RECOGN
						details.mRecognTags.push_back(*it);
					}
					else
					{
#ifdef DEBUG_RECOGN
						std::cerr << "RsGxsIdCache::updateServiceString() Invalid Tag: " << it->tag_class << ":" << it->tag_type;
						std::cerr << std::endl;
#endif // DEBUG_RECOGN
					}
				}
			}
			else
			{
#ifdef DEBUG_RECOGN
				std::cerr << "RsGxsIdCache::updateServiceString() recogntags old publishTs";
				std::cerr << std::endl;
#endif // DEBUG_RECOGN
			}

		}
		else
		{
#ifdef DEBUG_RECOGN
				std::cerr << "RsGxsIdCache::updateServiceString() recogntags unprocessed";
				std::cerr << std::endl;
#endif // DEBUG_RECOGN
		}

		// copy over Reputation scores.
		details.mReputation = ssdata.score.rep;
	}
	else
	{
		details.mPgpKnown = false;
		details.mPgpId.clear();
		details.mReputation.updateIdScore(false, false);
		details.mReputation.update();
	}
}


bool p3IdService::recogn_extract_taginfo(const RsGxsIdGroupItem *item, std::list<RsGxsRecognTagItem *> &tagItems)
{
#ifdef DEBUG_RECOGN
	std::cerr << "p3IdService::recogn_extract_taginfo()";
	std::cerr << std::endl;
#endif // DEBUG_RECOGN

	/* process Recogn Tags */

	std::list<std::string>::const_iterator rit;
	int count = 0;
    for(rit = item->mRecognTags.begin(); rit != item->mRecognTags.end(); ++rit)
	{
		if (++count > RSRECOGN_MAX_TAGINFO)
		{
#ifdef DEBUG_RECOGN
			std::cerr << "p3IdService::recogn_extract_taginfo() Too many tags.";
			std::cerr << std::endl;
#endif // DEBUG_RECOGN

			return true;
		}

		RsGxsRecognTagItem *tagitem = RsRecogn::extractTag(*rit);

		if (!tagitem)
		{
			continue;
		}

#ifdef DEBUG_RECOGN
		std::cerr << "p3IdService::recogn_extract_taginfo() Got TagItem: ";
		std::cerr << std::endl;
		tagitem->print(std::cerr);
#endif // DEBUG_RECOGN

		tagItems.push_back(tagitem);
	}
	return true;
}


bool p3IdService::cache_process_recogntaginfo(const RsGxsIdGroupItem *item, std::list<RsRecognTag> &tagList)
{
	/* ServiceString decode */
	SSGxsIdGroup ssdata;
	bool recognProcess = false;
	if (ssdata.load(item->meta.mServiceString))
	{
		if (!ssdata.recogntags.tagsProcessed())
		{
#ifdef DEBUG_RECOGN
			std::cerr << "p3IdService::cache_process_recogntaginfo() tags not processed";
			std::cerr << std::endl;
#endif // DEBUG_RECOGN

			/* we need to reprocess it */
			recognProcess = true;
		}
		else 
		{
			if (item->meta.mPublishTs != ssdata.recogntags.publishTs)
			{
#ifdef DEBUG_RECOGN
				std::cerr << "p3IdService::cache_process_recogntaginfo() publishTs old";
				std::cerr << std::endl;
#endif // DEBUG_RECOGN
				recognProcess = true;
			}
			else if (ssdata.recogntags.tagsPending())
			{
#ifdef DEBUG_RECOGN
				std::cerr << "p3IdService::cache_process_recogntaginfo() tagsPending";
				std::cerr << std::endl;
#endif // DEBUG_RECOGN
				/* reprocess once a day */
				recognProcess = true;
			}
		}
	}
	else
	{
#ifdef DEBUG_RECOGN
		std::cerr << "p3IdService::cache_process_recogntaginfo() ServiceString invalid";
		std::cerr << std::endl;
#endif // DEBUG_RECOGN
		recognProcess = true;
	}

	std::list<RsGxsRecognTagItem *> tagItems;
	std::list<RsGxsRecognTagItem *>::iterator it;

	recogn_extract_taginfo(item, tagItems);

    //time_t now = time(NULL);
	for(it = tagItems.begin(); it != tagItems.end(); ++it)
	{
		RsRecognTag info((*it)->tag_class, (*it)->tag_type, false);
		bool isPending = false;
		if (recogn_checktag(RsGxsId(item->meta.mGroupId.toStdString()), item->meta.mGroupName, *it, false, isPending))
		{
			info.valid = true;
		}
#ifdef DEBUG_RECOGN
		std::cerr << "p3IdService::cache_process_recogntaginfo() Adding Tag: ";
		std::cerr << info.tag_class << ":";
		std::cerr << info.tag_type << ":";
		std::cerr << info.valid;
		std::cerr << std::endl;
#endif // DEBUG_RECOGN

		tagList.push_back(info);
		delete *it;
	}


	if (recognProcess)
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		mRecognGroupIds.push_back(item->meta.mGroupId);

#ifdef DEBUG_RECOGN
		std::cerr << "p3IdService::cache_process_recogntaginfo() Reprocessing groupId: ";
		std::cerr << item->meta.mGroupId;
		std::cerr << std::endl;
#endif // DEBUG_RECOGN

		recogn_schedule();
	}

	return true;
}


bool p3IdService::cache_store(const RsGxsIdGroupItem *item)
{
#ifdef DEBUG_IDS
	std::cerr << "p3IdService::cache_store() Item: " << item->meta.mGroupId;
	std::cerr << std::endl;
#endif // DEBUG_IDS
	//item->print(std::cerr, 0); NEEDS CONST!!!! TODO
	//std::cerr << std::endl;

	/* extract key from keys */
    	RsTlvSecurityKeySet keySet;
    	RsTlvSecurityKey    pubkey;
    	RsTlvSecurityKey    fullkey;
	bool pub_key_ok = false;
	bool full_key_ok = false;

    	RsGxsId id (item->meta.mGroupId.toStdString());
    	if (!getGroupKeys(RsGxsGroupId(id.toStdString()), keySet))
	{
		std::cerr << "p3IdService::cache_store() ERROR getting GroupKeys for: ";
		std::cerr << item->meta.mGroupId;
		std::cerr << std::endl;
		return false;
	}

	std::map<RsGxsId, RsTlvSecurityKey>::iterator kit;

	//std::cerr << "p3IdService::cache_store() KeySet is:";
	//keySet.print(std::cerr, 10);

	for (kit = keySet.keys.begin(); kit != keySet.keys.end(); ++kit)
	{
		if (kit->second.keyFlags & RSTLV_KEY_DISTRIB_ADMIN)
		{
#ifdef DEBUG_IDS
			std::cerr << "p3IdService::cache_store() Found Admin Key";
			std::cerr << std::endl;
#endif // DEBUG_IDS

			/* save full key - if we have it */
			if (kit->second.keyFlags & RSTLV_KEY_TYPE_FULL)
			{
				fullkey = kit->second;
				full_key_ok = true;

				if(GxsSecurity::extractPublicKey(fullkey,pubkey))
					pub_key_ok = true ;
			}
			else
			{
				pubkey = kit->second;
				pub_key_ok = true ;
			}

			/* cache public key always 
			 * we don't need to check the keyFlags, 
			 * as both FULL and PUBLIC_ONLY keys contain the PUBLIC key
			 */

		}
	}

	if (!pub_key_ok)
	{
		std::cerr << "p3IdService::cache_store() ERROR No Public Key Found";
		std::cerr << std::endl;
		return false;
	}

	// extract tags.
	std::list<RsRecognTag> tagList;
	cache_process_recogntaginfo(item, tagList);

	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

    assert(!(pubkey.keyFlags & RSTLV_KEY_TYPE_FULL)) ;

	// Create Cache Data.
	RsGxsIdCache pubcache(item, pubkey, tagList);
	mPublicKeyCache.store(id, pubcache);
	mPublicKeyCache.resize();

	if (full_key_ok)
	{
		RsGxsIdCache fullcache(item, fullkey, tagList);
		mPrivateKeyCache.store(id, fullcache);
		mPrivateKeyCache.resize();
	}

	return true;
}



/***** BELOW LOADS THE CACHE FROM GXS DATASTORE *****/

#define MIN_CYCLE_GAP	2

bool p3IdService::cache_request_load(const RsGxsId &id, const std::list<PeerId>& peers)
{
#ifdef DEBUG_IDS
	std::cerr << "p3IdService::cache_request_load(" << id << ")";
	std::cerr << std::endl;
#endif // DEBUG_IDS

	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		mCacheLoad_ToCache.insert(std::make_pair(id, peers));
	}

	if (RsTickEvent::event_count(GXSID_EVENT_CACHELOAD) > 0)
	{
		/* its already scheduled */
		return true;
	}

	int32_t age = 0;
	if (RsTickEvent::prev_event_ago(GXSID_EVENT_CACHELOAD, age))
	{
		if (age < MIN_CYCLE_GAP)
		{
			RsTickEvent::schedule_in(GXSID_EVENT_CACHELOAD, MIN_CYCLE_GAP - age);
			return true;
		}
	}

	RsTickEvent::schedule_now(GXSID_EVENT_CACHELOAD);
	return true;
}


bool p3IdService::cache_start_load()
{
	/* trigger request to load missing ids into cache */
	std::list<RsGxsGroupId> groupIds;
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

		/* now we process the modGroupList -> a map so we can use it easily later, and create id list too */
		std::map<RsGxsId, std::list<RsPeerId> >::iterator it;
		for(it = mCacheLoad_ToCache.begin(); it != mCacheLoad_ToCache.end(); ++it)
		{
#ifdef DEBUG_IDS
			std::cerr << "p3IdService::cache_start_load() GroupId: " << it->first;
			std::cerr << std::endl;
#endif // DEBUG_IDS
			groupIds.push_back(RsGxsGroupId(it->first.toStdString())); // might need conversion?
		}

                mPendingCache.insert(mCacheLoad_ToCache.begin(), mCacheLoad_ToCache.end());
		mCacheLoad_ToCache.clear();
	}

	if (groupIds.size() > 0)
	{
#ifdef DEBUG_IDS
		std::cerr << "p3IdService::cache_start_load() #Groups: " << groupIds.size();
		std::cerr << std::endl;
#endif // DEBUG_IDS

		uint32_t ansType = RS_TOKREQ_ANSTYPE_DATA; 
		RsTokReqOptions opts;
		opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
		uint32_t token = 0;
	
		RsGenExchange::getTokenService()->requestGroupInfo(token, ansType, opts, groupIds);
		GxsTokenQueue::queueRequest(token, GXSIDREQ_CACHELOAD);
	}
	return 1;
}


bool p3IdService::cache_load_for_token(uint32_t token)
{
#ifdef DEBUG_IDS
	std::cerr << "p3IdService::cache_load_for_token() : " << token;
	std::cerr << std::endl;
#endif // DEBUG_IDS

	std::vector<RsGxsGrpItem*> grpData;
	bool ok = RsGenExchange::getGroupData(token, grpData);

	if(ok)
	{
		std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();

		for(; vit != grpData.end(); ++vit)
		{
			RsGxsIdGroupItem* item = dynamic_cast<RsGxsIdGroupItem*>(*vit);
			if (!item)
			{
				std::cerr << "Not a RsGxsIdGroupItem Item, deleting!" << std::endl;
				delete(*vit);
				continue;
			}

#ifdef DEBUG_IDS
			std::cerr << "p3IdService::cache_load_for_token() Loaded Id with Meta: ";
			std::cerr << item->meta;
			std::cerr << std::endl;
#endif // DEBUG_IDS


			{
                // remove identities that are present
				RsStackMutex stack(mIdMtx);
				mPendingCache.erase(RsGxsId(item->meta.mGroupId.toStdString()));
			}

			/* cache the data */
			cache_store(item);
			delete item;
		}

		{
			// now store identities that aren't present

			RsStackMutex stack(mIdMtx);
			mIdsNotPresent.insert(mPendingCache.begin(), mPendingCache.end());
			mPendingCache.clear();

                        if(!mIdsNotPresent.empty())
                            schedule_now(GXSID_EVENT_REQUEST_IDS);
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

void p3IdService::requestIdsFromNet()
{
	RsStackMutex stack(mIdMtx);

	std::map<RsGxsId, std::list<RsPeerId> >::const_iterator cit;
	std::map<RsPeerId, std::list<RsGxsId> > requests;

	// transform to appropriate structure (<peer, std::list<RsGxsId> > map) to make request to nes
	for(cit = mIdsNotPresent.begin(); cit != mIdsNotPresent.end(); ++cit)
	{
		{
#ifdef DEBUG_IDS
				std::cerr << "p3IdService::requestIdsFromNet() Id not found, deferring for net request: ";
							std::cerr << cit->first;
							std::cerr << std::endl;
#endif // DEBUG_IDS

		}

		const std::list<RsPeerId>& peers = cit->second;
		std::list<RsPeerId>::const_iterator cit2;
		for(cit2 = peers.begin(); cit2 != peers.end(); ++cit2)
			requests[*cit2].push_back(cit->first);
	}

	std::map<RsPeerId, std::list<RsGxsId> >::const_iterator cit2;

	for(cit2 = requests.begin(); cit2 != requests.end(); ++cit2)
        {

            if(mNes)
            {
        		std::list<RsGxsId>::const_iterator gxs_id_it = cit2->second.begin();
        		std::list<RsGxsGroupId> grpIds;
        		for(; gxs_id_it != cit2->second.end(); ++gxs_id_it)
        			grpIds.push_back(RsGxsGroupId(gxs_id_it->toStdString()));

            	mNes->requestGrp(grpIds, cit2->first);
            }
        }

	mIdsNotPresent.clear();
}

bool p3IdService::cache_update_if_cached(const RsGxsId &id, std::string serviceString)
{
	/* if these entries are cached - update with new info */
#ifdef DEBUG_IDS
	std::cerr << "p3IdService::cache_update_if_cached(" << id << ")";
	std::cerr << std::endl;
#endif // DEBUG_IDS

	/* retrieve - update, save */
	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

	RsGxsIdCache pub_data;
	if (mPublicKeyCache.fetch(id, pub_data))
	{
#ifdef DEBUG_IDS
		std::cerr << "p3IdService::cache_update_if_cached() Updating Public Cache";
		std::cerr << std::endl;
#endif // DEBUG_IDS

        assert(!(pub_data.pubkey.keyFlags & RSTLV_KEY_TYPE_FULL)) ;

		pub_data.updateServiceString(serviceString);
		mPublicKeyCache.store(id, pub_data);
	}


	RsGxsIdCache priv_data;
	if (mPrivateKeyCache.fetch(id, priv_data))
	{
#ifdef DEBUG_IDS
		std::cerr << "p3IdService::cache_update_if_cached() Updating Private Cache";
		std::cerr << std::endl;
#endif // DEBUG_IDS

		priv_data.updateServiceString(serviceString);
		mPrivateKeyCache.store(id, priv_data);
	}

	return true;
}

/************************************************************************************/
/************************************************************************************/

bool p3IdService::cache_request_ownids()
{
	/* trigger request to load missing ids into cache */
	std::list<RsGxsGroupId> groupIds;
#ifdef DEBUG_IDS
	std::cerr << "p3IdService::cache_request_ownids()";
	std::cerr << std::endl;
#endif // DEBUG_IDS

	uint32_t ansType = RS_TOKREQ_ANSTYPE_DATA; 
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
	//opts.mSubscribeFlags = GXS_SERV::GROUP_SUBSCRIBE_ADMIN;

	uint32_t token = 0;
	
	RsGenExchange::getTokenService()->requestGroupInfo(token, ansType, opts);
	GxsTokenQueue::queueRequest(token, GXSIDREQ_CACHEOWNIDS);	
	return 1;
}


bool p3IdService::cache_load_ownids(uint32_t token)
{
#ifdef DEBUG_IDS
	std::cerr << "p3IdService::cache_load_ownids() : " << token;
	std::cerr << std::endl;
#endif // DEBUG_IDS

	std::vector<RsGxsGrpItem*> grpData;
	bool ok = RsGenExchange::getGroupData(token, grpData);

	if(ok)
	{
		std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();

		// Save List
		{
			RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

			mOwnIds.clear();
			for(vit = grpData.begin(); vit != grpData.end(); ++vit)
			{
				RsGxsIdGroupItem* item = dynamic_cast<RsGxsIdGroupItem*>(*vit);
				if (!item)
				{
					std::cerr << "Not a IdOpinion Item, deleting!" << std::endl;
					delete(*vit);
					continue;
				}

				if (item->meta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN)
				{
                    mOwnIds.push_back(RsGxsId(item->meta.mGroupId));

                    // This prevents automatic deletion to get rid of them.
                    // In other words, own ids are always used.
                    mKeysTS[RsGxsId(item->meta.mGroupId)] = time(NULL) ;
				}
				delete item ;
            }
            mOwnIdsLoaded = true ;
		}

		// No need to cache these items...
		// as it just causes the cache to be flushed.
#if 0
		// Cache Items too.
		for(vit = grpData.begin(); vit != grpData.end(); ++vit)
		{
			RsGxsIdGroupItem* item = dynamic_cast<RsGxsIdGroupItem*>(*vit);
			if (item->meta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN)
			{

				std::cerr << "p3IdService::cache_load_ownids() Loaded Id with Meta: ";
				std::cerr << item->meta;
				std::cerr << std::endl;

				/* cache the data */
				cache_store(item);
			}
			delete item;
		}
#endif

	}
	else
	{
		std::cerr << "p3IdService::cache_load_ownids() ERROR no data";
		std::cerr << std::endl;

		return false;
	}
	return true;
}

/************************************************************************************/
/************************************************************************************/

bool p3IdService::cachetest_getlist()
{
#ifdef DEBUG_IDS
	std::cerr << "p3IdService::cachetest_getlist() making request";
	std::cerr << std::endl;
#endif // DEBUG_IDS

	uint32_t ansType = RS_TOKREQ_ANSTYPE_LIST; 
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_IDS;
	uint32_t token = 0;
	
	RsGenExchange::getTokenService()->requestGroupInfo(token, ansType, opts);
	GxsTokenQueue::queueRequest(token, GXSIDREQ_CACHETEST);	

	// Schedule Next Event.
	RsTickEvent::schedule_in(GXSID_EVENT_CACHETEST, CACHETEST_PERIOD);
	return true;
}

bool p3IdService::cachetest_handlerequest(uint32_t token)
{
#ifdef DEBUG_IDS
	std::cerr << "p3IdService::cachetest_handlerequest() token: " << token;
	std::cerr << std::endl;
#endif // DEBUG_IDS

       	std::list<RsGxsId> grpIds;
       	std::list<RsGxsGroupId> grpIdsC;
       	bool ok = RsGenExchange::getGroupList(token, grpIdsC);

       	std::list<RsGxsGroupId>::const_iterator cit = grpIdsC.begin();
       	for(; cit != grpIdsC.end(); ++cit)
       		grpIds.push_back(RsGxsId(cit->toStdString()));

	if(ok)
	{
		std::list<RsGxsId>::iterator vit = grpIds.begin();
		for(; vit != grpIds.end(); ++vit)
		{
			/* 5% chance of checking it! */
			if (RSRandom::random_f32() < 0.25)
			{
#ifdef DEBUG_IDS
				std::cerr << "p3IdService::cachetest_request() Testing Id: " << *vit;
				std::cerr << std::endl;
#endif // DEBUG_IDS

				/* try the cache! */
				if (!haveKey(*vit))
				{
					std::list<PeerId> nullpeers;
					requestKey(*vit, nullpeers);

#ifdef DEBUG_IDS
					std::cerr << "p3IdService::cachetest_request() Requested Key Id: " << *vit;
					std::cerr << std::endl;
#endif // DEBUG_IDS
				}
				else
				{
					RsTlvSecurityKey seckey;
                    if (getKey(*vit, seckey))
					{
#ifdef DEBUG_IDS
						std::cerr << "p3IdService::cachetest_request() Got Key OK Id: " << *vit;
						std::cerr << std::endl;
#endif // DEBUG_IDS

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
#ifdef DEBUG_IDS
					std::cerr << "p3IdService::cachetest_request() Requested PrivateKey Id: " << *vit;
					std::cerr << std::endl;
#endif // DEBUG_IDS
				}
				else
				{
					RsTlvSecurityKey seckey;
					if (getPrivateKey(*vit, seckey))
					{
						// success!
#ifdef DEBUG_IDS
						std::cerr << "p3IdService::cachetest_request() Got PrivateKey OK Id: " << *vit;
						std::cerr << std::endl;
#endif // DEBUG_IDS
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
	return true;
}

/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

/*
 * We have three background tasks that use the ServiceString: PGPHash & Reputation & Recogn
 *
 * Only one task can be run at a time - otherwise potential overwrite issues.
 * So this part coordinates that part of the code.
 *
 * We are going to have a "fetcher task", which gets all the UNPROCESSED / UPDATED GROUPS.
 * and sets the CHECK_PGP, CHECK_RECOGN, etc... this will reduce the "Get All" calls.
 *
 */


bool	p3IdService::CacheArbitration(uint32_t mode)
{
	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

	if (!mBgSchedule_Active)
	{
#ifdef DEBUG_IDS
		std::cerr << "p3IdService::CacheArbitration() Okay: mode " << mode;
		std::cerr << std::endl;
#endif // DEBUG_IDS

		mBgSchedule_Active = true;
		mBgSchedule_Mode = mode;
		return true;
	}

#ifdef DEBUG_IDS
	std::cerr << "p3IdService::CacheArbitration() Is busy in mode: " << mBgSchedule_Mode;
	std::cerr << std::endl;
#endif // DEBUG_IDS

	return false;
}

void	p3IdService::CacheArbitrationDone(uint32_t mode)
{
	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

	if (mBgSchedule_Mode != mode)
	{
		/* issues */
		std::cerr << "p3IdService::CacheArbitrationDone() ERROR Wrong Current Mode";
		std::cerr << std::endl;
		return;
	}

#ifdef DEBUG_IDS
	std::cerr << "p3IdService::CacheArbitrationDone()";
	std::cerr << std::endl;
#endif // DEBUG_IDS

	mBgSchedule_Active = false;
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

static void calcPGPHash(const RsGxsId &id, const PGPFingerprintType &pgp, Sha1CheckSum &hash);


// Must Use meta.
RsGenExchange::ServiceCreate_Return p3IdService::service_CreateGroup(RsGxsGrpItem* grpItem, RsTlvSecurityKeySet& keySet)
{

#ifdef DEBUG_IDS
	std::cerr << "p3IdService::service_CreateGroup()";
	std::cerr << std::endl;
#endif // DEBUG_IDS

	RsGxsIdGroupItem *item = dynamic_cast<RsGxsIdGroupItem *>(grpItem);
	if (!item)
	{
		std::cerr << "p3IdService::service_CreateGroup() ERROR invalid cast";
		std::cerr << std::endl;
		return SERVICE_CREATE_FAIL;
	}

#ifdef DEBUG_IDS
	std::cerr << "p3IdService::service_CreateGroup() Item is:";
	std::cerr << std::endl;
	item->print(std::cerr);
	std::cerr << std::endl;
#endif // DEBUG_IDS

	/********************* TEMP HACK UNTIL GXS FILLS IN GROUP_ID *****************/	
	// find private admin key
	std::map<RsGxsId, RsTlvSecurityKey>::iterator mit = keySet.keys.begin();
	for(; mit != keySet.keys.end(); ++mit)
	{
		RsTlvSecurityKey& pk = mit->second;
	
		if(pk.keyFlags == (RSTLV_KEY_DISTRIB_ADMIN | RSTLV_KEY_TYPE_FULL))
		{
            item->meta.mGroupId = RsGxsGroupId(pk.keyId);
			break;
		}
	}
	
	if(mit == keySet.keys.end())
	{
		std::cerr << "p3IdService::service_CreateGroup() ERROR no admin key";
		std::cerr << std::endl;
		return SERVICE_CREATE_FAIL;
    }
    mKeysTS[RsGxsId(item->meta.mGroupId)] = time(NULL) ;
		
	/********************* TEMP HACK UNTIL GXS FILLS IN GROUP_ID *****************/	

	// SANITY CHECK.
//    if (item->mMeta.mAuthorId != item->meta.mAuthorId)
//	{
//		std::cerr << "p3IdService::service_CreateGroup() AuthorId mismatch(";
//        std::cerr << item->mMeta.mAuthorId;
//		std::cerr << " vs ";
//		std::cerr << item->meta.mAuthorId;
//		std::cerr << std::endl;
//	}
//
//	if (item->group.mMeta.mGroupId != item->meta.mGroupId)
//	{
//		std::cerr << "p3IdService::service_CreateGroup() GroupId mismatch(";
//        std::cerr << item->mMeta.mGroupId;
//		std::cerr << " vs ";
//		std::cerr << item->meta.mGroupId;
//		std::cerr << std::endl;
//	}
//
//
//	if (item->group.mMeta.mGroupFlags != item->meta.mGroupFlags)
//	{
//		std::cerr << "p3IdService::service_CreateGroup() GroupFlags mismatch(";
//        std::cerr << item->mMeta.mGroupFlags;
//		std::cerr << " vs ";
//		std::cerr << item->meta.mGroupFlags;
//		std::cerr << std::endl;
//	}
		
#ifdef DEBUG_IDS
    std::cerr << "p3IdService::service_CreateGroup() for : " << item->mMeta.mGroupId;
	std::cerr << std::endl;
	std::cerr << "p3IdService::service_CreateGroup() Alt GroupId : " << item->meta.mGroupId;
	std::cerr << std::endl;
#endif // DEBUG_IDS

	ServiceCreate_Return createStatus;

    if (item->meta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
	{
		/* create the hash */
		Sha1CheckSum hash;

		/* */
		PGPFingerprintType ownFinger;
		RsPgpId ownId(mPgpUtils->getPGPOwnId());

#ifdef DEBUG_IDS
		std::cerr << "p3IdService::service_CreateGroup() OwnPgpID: " << ownId.toStdString();
		std::cerr << std::endl;
#endif

#ifdef GXSID_GEN_DUMMY_DATA
//		if (item->group.mMeta.mAuthorId != "")
//		{
//			ownId = RsPgpId(item->group.mMeta.mAuthorId);
//		}
#endif

		if (!mPgpUtils->getKeyFingerprint(ownId,ownFinger))
		{
			std::cerr << "p3IdService::service_CreateGroup() ERROR Own Finger is stuck";
			std::cerr << std::endl;
			return SERVICE_CREATE_FAIL; // abandon attempt!
		}

#ifdef DEBUG_IDS
		std::cerr << "p3IdService::service_CreateGroup() OwnFingerprint: " << ownFinger.toStdString();
		std::cerr << std::endl;
#endif

        RsGxsId gxsId(item->meta.mGroupId.toStdString());
		calcPGPHash(gxsId, ownFinger, hash);
        item->mPgpIdHash = hash;

#ifdef DEBUG_IDS

		std::cerr << "p3IdService::service_CreateGroup() Calculated PgpIdHash : " << item->group.mPgpIdHash;
		std::cerr << std::endl;
#endif // DEBUG_IDS

		/* do signature */


#define MAX_SIGN_SIZE 2048
		uint8_t signarray[MAX_SIGN_SIZE]; 
		unsigned int sign_size = MAX_SIGN_SIZE;
        int result ;

        memset(signarray,0,MAX_SIGN_SIZE) ;	// just in case.

		if (!mPgpUtils->askForDeferredSelfSignature((void *) hash.toByteArray(), hash.SIZE_IN_BYTES, signarray, &sign_size,result))
		{
			/* error */
			std::cerr << "p3IdService::service_CreateGroup() ERROR Signing stuff";
			std::cerr << std::endl;
			createStatus = SERVICE_CREATE_FAIL_TRY_LATER;
		}
		else
        {
            // Additional consistency checks.

            if(sign_size == MAX_SIGN_SIZE)
            {
                std::cerr << "Inconsistent result. Signature uses full buffer. This is probably an error." << std::endl;
                return SERVICE_CREATE_FAIL; // abandon attempt!
            }
#ifdef DEBUG_IDS
			std::cerr << "p3IdService::service_CreateGroup() Signature: ";
			std::string strout;
#endif
			/* push binary into string -> really bad! */
            item->mPgpIdSign = "";
			for(unsigned int i = 0; i < sign_size; i++)
			{
#ifdef DEBUG_IDS
				rs_sprintf_append(strout, "%02x", (uint32_t) signarray[i]);
#endif
                item->mPgpIdSign += signarray[i];
			}
			createStatus = SERVICE_CREATE_SUCCESS;

#ifdef DEBUG_IDS
			std::cerr << strout;
			std::cerr << std::endl;
#endif
		}
		/* done! */
	}
	else
	{
		createStatus = SERVICE_CREATE_SUCCESS;
	}

	// Enforce no AuthorId.
	item->meta.mAuthorId.clear() ;
    //item->mMeta.mAuthorId.clear() ;
	// copy meta data to be sure its all the same.
	//item->group.mMeta = item->meta;

    // do it like p3gxscircles: save the new grp id
    // this allows the user interface
    // to see the grp id on the list of ownIds immediately after the group was created
    {
        RsStackMutex stack(mIdMtx);
        RsGxsId gxsId(item->meta.mGroupId);
        if (std::find(mOwnIds.begin(), mOwnIds.end(), gxsId) == mOwnIds.end())
        {
            mOwnIds.push_back(gxsId);
        mKeysTS[gxsId] = time(NULL) ;
        }
    }

	return createStatus;
}


#define HASHPGP_PERIOD		180


bool p3IdService::pgphash_start()
{
	if (!CacheArbitration(BG_PGPHASH))
	{
#ifdef DEBUG_IDS
		std::cerr << "p3IdService::pgphash_start() Other Events running... Rescheduling";
		std::cerr << std::endl;
#endif // DEBUG_IDS

		/* reschedule in a bit */
		RsTickEvent::schedule_in(GXSID_EVENT_PGPHASH, PGPHASH_RETRY_PERIOD);
		return false;
	}

	// SCHEDULE NEXT ONE.
	RsTickEvent::schedule_in(GXSID_EVENT_PGPHASH, PGPHASH_PERIOD);

#ifdef DEBUG_IDS
	std::cerr << "p3IdService::pgphash_start() making request";
	std::cerr << std::endl;
#endif // DEBUG_IDS

	// ACTUALLY only need summary - but have written code for data.
	// Also need to use opts.groupFlags to filter stuff properly to REALID's only.
	// TODO

	uint32_t ansType = RS_TOKREQ_ANSTYPE_DATA;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
	uint32_t token = 0;
	
	RsGenExchange::getTokenService()->requestGroupInfo(token, ansType, opts);
	GxsTokenQueue::queueRequest(token, GXSIDREQ_PGPHASH);	
	return true;
}


bool p3IdService::pgphash_handlerequest(uint32_t token)
{
#ifdef DEBUG_IDS
	std::cerr << "p3IdService::pgphash_handlerequest(" << token << ")";
	std::cerr << std::endl;
#endif // DEBUG_IDS

	// We need full data - for access to Hash & Signature.
	// Perhaps we will change this to an initial pass through Meta, 
	// and use this to discard lots of things.

	// Even better - we can set flags in the Meta Data, (IdType), 
	// And use GXS to filter out all the AnonIds, and only have to process
	// Proper Ids.

	// We Will do this later!

	std::vector<RsGxsIdGroup> groups;
	bool groupsToProcess = false;
	bool ok = getGroupData(token, groups);

	if(ok)
	{
#ifdef DEBUG_IDS
		std::cerr << "p3IdService::pgphash_request() Have " << groups.size() << " Groups";
		std::cerr << std::endl;
#endif // DEBUG_IDS

		std::vector<RsGxsIdGroup>::iterator vit;
		for(vit = groups.begin(); vit != groups.end(); ++vit)
		{
#ifdef DEBUG_IDS
			std::cerr << "p3IdService::pgphash_request() Group Id: " << vit->mMeta.mGroupId;
			std::cerr << std::endl;
#endif // DEBUG_IDS

			/* Filter based on IdType */
			if (!(vit->mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID))
			{
#ifdef DEBUG_IDS
				std::cerr << "p3IdService::pgphash_request() discarding AnonID";
				std::cerr << std::endl;
#endif // DEBUG_IDS
				continue;
			}

			/* now we need to decode the Service String - see what is saved there */
			SSGxsIdGroup ssdata;
			if (ssdata.load(vit->mMeta.mServiceString))
			{
				if (ssdata.pgp.idKnown)
				{
#ifdef DEBUG_IDS
					std::cerr << "p3IdService::pgphash_request() discarding Already Known";
					std::cerr << std::endl;
#endif // DEBUG_IDS
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
#ifdef DEBUG_IDS
                std::cerr << "p3IdService: group " << *vit << " age=" << age << ", attempts=" << ssdata.pgp.checkAttempts  << ", wait period = " << wait_period ;
#endif

                if (age < wait_period)
                {
#ifdef DEBUG_IDS
                    std::cerr << " => discard." << std::endl;
#endif // DEBUG_IDS
                    continue;
                }
#ifdef DEBUG_IDS
                std::cerr << " => recheck!" << std::endl;
#endif
			}

			/* if we get here -> then its to be processed */
#ifdef DEBUG_IDS
			std::cerr << "p3IdService::pgphash_request() ToProcess Group: " << vit->mMeta.mGroupId;
			std::cerr << std::endl;
#endif // DEBUG_IDS

			RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
			mGroupsToProcess.push_back(*vit);
			groupsToProcess = true;
		}
	}
	else
	{
		std::cerr << "p3IdService::pgphash_request() getGroupData ERROR";
		std::cerr << std::endl;
	}

	if (groupsToProcess)
	{
		// update PgpIdList -> if there are groups to process.
		getPgpIdList();
	}

	// Schedule Processing.
	RsTickEvent::schedule_in(GXSID_EVENT_PGPHASH_PROC, PGPHASH_PROC_PERIOD);
	return true;
}

bool p3IdService::pgphash_process()
{
	/* each time this is called - process one Id from mGroupsToProcess */
	RsGxsIdGroup pg;
	bool isDone = false;
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		if (!mGroupsToProcess.empty())
		{
			pg = mGroupsToProcess.front();
			mGroupsToProcess.pop_front();

#ifdef DEBUG_IDS
			std::cerr << "p3IdService::pgphash_process() Popped Group: " << pg.mMeta.mGroupId;
			std::cerr << std::endl;
#endif // DEBUG_IDS
		}
		else
		{
			isDone = true;
		}
	}

	if (isDone)
	{
#ifdef DEBUG_IDS
		std::cerr << "p3IdService::pgphash_process() List Empty... Done";
		std::cerr << std::endl;
#endif // DEBUG_IDS
		// FINISHED.
		CacheArbitrationDone(BG_PGPHASH);
		return true;
	}
	

	SSGxsIdGroup ssdata;
	ssdata.load(pg.mMeta.mServiceString); // attempt load - okay if fails.

    RsPgpId pgpId;
    bool error = false ;

    if (checkId(pg, pgpId,error))
	{
		/* found a match - update everything */
		/* Consistency issues here - what if Reputation was recently updated? */

#ifdef DEBUG_IDS
		std::cerr << "p3IdService::pgphash_process() CheckId Success for Group: " << pg.mMeta.mGroupId;
        std::cerr << " PgpId: " << pgpId;
		std::cerr << std::endl;
#endif // DEBUG_IDS

		/* update */
		ssdata.pgp.idKnown = true;
		ssdata.pgp.pgpId = pgpId;

	}
    else if(error)
    {
            std::cerr << "Identity has an invalid signature. It will be deleted." << std::endl;

            uint32_t token ;
            deleteIdentity(token,pg) ;
    }
    else
	{
#ifdef DEBUG_IDS
		std::cerr << "p3IdService::pgphash_process() No Match for Group: " << pg.mMeta.mGroupId;
		std::cerr << std::endl;
#endif // DEBUG_IDS

		ssdata.pgp.lastCheckTs = time(NULL);
		ssdata.pgp.checkAttempts++;
	}

    if(!error)
    {
        // update IdScore too.
        ssdata.score.rep.updateIdScore(true, ssdata.pgp.idKnown);
        ssdata.score.rep.update();

        /* set new Group ServiceString */
        uint32_t dummyToken = 0;
        std::string serviceString = ssdata.save();
        setGroupServiceString(dummyToken, pg.mMeta.mGroupId, serviceString);

        cache_update_if_cached(RsGxsId(pg.mMeta.mGroupId), serviceString);
    }

	// Schedule Next Processing.
	RsTickEvent::schedule_in(GXSID_EVENT_PGPHASH_PROC, PGPHASH_PROC_PERIOD);
	return false; // as there are more items on the queue to process.
}



bool p3IdService::checkId(const RsGxsIdGroup &grp, RsPgpId &pgpId,bool& error)
{
#ifdef DEBUG_IDS
	std::cerr << "p3IdService::checkId() Starting Match Check for RsGxsId: ";
	std::cerr << grp.mMeta.mGroupId;
	std::cerr << std::endl;
#endif // DEBUG_IDS

    error = false ;

	/* some sanity checking... make sure hash is the right size */

#ifdef DEBUG_IDS
	std::cerr << "p3IdService::checkId() PgpIdHash is: " << grp.mPgpIdHash;
	std::cerr << std::endl;
#endif // DEBUG_IDS

	/* iterate through and check hash */
	Sha1CheckSum ans = grp.mPgpIdHash;

#ifdef DEBUG_IDS
	std::cerr << "\tExpected Answer: " << ans.toStdString();
	std::cerr << std::endl;
#endif // DEBUG_IDS

	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

	std::map<RsPgpId, PGPFingerprintType>::iterator mit;
	for(mit = mPgpFingerprintMap.begin(); mit != mPgpFingerprintMap.end(); ++mit)
	{
		Sha1CheckSum hash;
        calcPGPHash(RsGxsId(grp.mMeta.mGroupId), mit->second, hash);

		if (ans == hash)
		{
#ifdef DEBUG_IDS
			std::cerr << "p3IdService::checkId() HASH MATCH!";
			std::cerr << std::endl;
			std::cerr << "p3IdService::checkId() Hash : " << hash.toStdString();
			std::cerr << std::endl;
#endif

			/* miracle match! */
			/* check signature too */
			if (mPgpUtils->VerifySignBin((void *) hash.toByteArray(), hash.SIZE_IN_BYTES, 
				(unsigned char *) grp.mPgpIdSign.c_str(), grp.mPgpIdSign.length(), 
				mit->second))
			{
#ifdef DEBUG_IDS
				std::cerr << "p3IdService::checkId() Signature Okay too!";
				std::cerr << std::endl;
#endif

				pgpId = mit->first;
				return true;
			}

			/* error */
			std::cerr << "p3IdService::checkId() ERROR Signature Failed";
			std::cerr << std::endl;

			std::cerr << "p3IdService::checkId() Matched PGPID: " << mit->first.toStdString();
			std::cerr << " Fingerprint: " << mit->second.toStdString();
			std::cerr << std::endl;

			std::cerr << "p3IdService::checkId() Signature: ";
			std::string strout;

			/* push binary into string -> really bad! */
			for(unsigned int i = 0; i < grp.mPgpIdSign.length(); i++)
			{
				rs_sprintf_append(strout, "%02x", (uint32_t) ((uint8_t) grp.mPgpIdSign[i]));
			}
			std::cerr << strout;
            std::cerr << std::endl;

            error = true ;
            return false ;
		}
	}

#ifdef DEBUG_IDS
	std::cerr << "p3IdService::checkId() Checked " << mPgpFingerprintMap.size() << " Hashes without Match";
	std::cerr << std::endl;
#endif // DEBUG_IDS

	return false;
}


/* worker functions */
void p3IdService::getPgpIdList()
{
#ifdef DEBUG_IDS
	std::cerr << "p3IdService::getPgpIdList() Starting....";
	std::cerr << std::endl;
#endif // DEBUG_IDS

 	std::list<RsPgpId> list;
	mPgpUtils->getGPGAllList(list);

	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

	mPgpFingerprintMap.clear();

 	std::list<RsPgpId>::iterator it;
	for(it = list.begin(); it != list.end(); ++it)
	{
 		RsPgpId pgpId(*it);
		PGPFingerprintType fp;
		mPgpUtils->getKeyFingerprint(pgpId, fp);

#ifdef DEBUG_IDS
		std::cerr << "p3IdService::getPgpIdList() Id: " << pgpId.toStdString() << " => " << fp.toStdString();
		std::cerr << std::endl;
#endif // DEBUG_IDS

		mPgpFingerprintMap[pgpId] = fp;
	}

#ifdef DEBUG_IDS
	std::cerr << "p3IdService::getPgpIdList() Items: " << mPgpFingerprintMap.size();
	std::cerr << std::endl;
#endif // DEBUG_IDS
}


void calcPGPHash(const RsGxsId &id, const PGPFingerprintType &pgp, Sha1CheckSum &hash)
{
	unsigned char signature[SHA_DIGEST_LENGTH];
	/* hash id + pubkey => pgphash */
	SHA_CTX *sha_ctx = new SHA_CTX;
	SHA1_Init(sha_ctx);

	SHA1_Update(sha_ctx, id.toStdString().c_str(), id.toStdString().length()); // TO FIX ONE DAY.
	SHA1_Update(sha_ctx, pgp.toByteArray(), pgp.SIZE_IN_BYTES);
	SHA1_Final(signature, sha_ctx);
	hash = Sha1CheckSum(signature);

#ifdef DEBUG_IDS
	std::cerr << "calcPGPHash():";
	std::cerr << std::endl;
	std::cerr << "\tRsGxsId: " << id;
	std::cerr << std::endl;
	std::cerr << "\tFingerprint: " << pgp.toStdString();
	std::cerr << std::endl;
	std::cerr << "\tFinal Hash: " << hash.toStdString();
	std::cerr << std::endl;
#endif // DEBUG_IDS

	delete sha_ctx;
}

/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

/* Task to validate Recogn Tags.
 *
 * Info to be stored in GroupServiceString + Cache.
 **/

bool p3IdService::recogn_schedule()
{
#ifdef DEBUG_RECOGN
	std::cerr << "p3IdService::recogn_schedule()";
	std::cerr << std::endl;
#endif

	int32_t age = 0;
	int32_t next_event = 0;

	if (RsTickEvent::event_count(GXSID_EVENT_RECOGN) > 0)
	{
#ifdef DEBUG_RECOGN
		std::cerr << "p3IdService::recogn_schedule() Skipping GXSIS_EVENT_RECOGN already scheduled";
		std::cerr << std::endl;
#endif
		return false;
	}

	if (RsTickEvent::prev_event_ago(GXSID_EVENT_RECOGN, age))
	{
#ifdef DEBUG_RECOGN
		std::cerr << "p3IdService::recogn_schedule() previous event " << age << " secs ago";
		std::cerr << std::endl;
#endif

		next_event = RECOGN_PERIOD - age;
		if (next_event < 0)
		{
			next_event = 0;
		}
	}

	RsTickEvent::schedule_in(GXSID_EVENT_RECOGN, next_event);
	return true;
}


bool p3IdService::recogn_start()
{
	if (!CacheArbitration(BG_RECOGN))
	{
#ifdef DEBUG_RECOGN
		std::cerr << "p3IdService::recogn_start() Other Events running... Rescheduling";
		std::cerr << std::endl;
#endif // DEBUG_RECOGN

		/* reschedule in a bit */
		RsTickEvent::schedule_in(GXSID_EVENT_RECOGN, RECOGN_RETRY_PERIOD);
		return false;
	}

	// NEXT EVENT is scheduled via recogn_schedule.

#ifdef DEBUG_RECOGN
	std::cerr << "p3IdService::recogn_start() making request";
	std::cerr << std::endl;
#endif // DEBUG_RECOGN

	std::list<RsGxsGroupId> recognList;
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

		recognList = mRecognGroupIds;
		mRecognGroupIds.clear();
	}

	if (recognList.empty())
	{
#ifdef DEBUG_RECOGN
		std::cerr << "p3IdService::recogn_start() List is Empty, cancelling";
		std::cerr << std::endl;
#endif // DEBUG_RECOGN

		// FINISHED.
		CacheArbitrationDone(BG_RECOGN);
		return false;
	}

	uint32_t ansType = RS_TOKREQ_ANSTYPE_DATA;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
	uint32_t token = 0;
	
	RsGenExchange::getTokenService()->requestGroupInfo(token, ansType, opts, recognList);
	GxsTokenQueue::queueRequest(token, GXSIDREQ_RECOGN);	
	return true;

}


bool p3IdService::recogn_handlerequest(uint32_t token)
{
#ifdef DEBUG_RECOGN
	std::cerr << "p3IdService::recogn_handlerequest(" << token << ")";
	std::cerr << std::endl;
#endif // DEBUG_RECOGN

	std::vector<RsGxsGrpItem*> grpData;
	bool ok = RsGenExchange::getGroupData(token, grpData);
	
	if(ok)
	{
#ifdef DEBUG_RECOGN
		std::cerr << "p3IdService::recogn_request() Have " << grpData.size() << " Groups";
		std::cerr << std::endl;
#endif // DEBUG_RECOGN
		
		std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();
		
		for(; vit != grpData.end(); ++vit)
		{
			RsGxsIdGroupItem* item = dynamic_cast<RsGxsIdGroupItem*>(*vit);
			if (item)
			{

#ifdef DEBUG_RECOGN
				std::cerr << "p3IdService::recogn_request() Group Id: " << item->meta.mGroupId;
				std::cerr << std::endl;
#endif // DEBUG_RECOGN

				RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
				mRecognGroupsToProcess.push_back(item);
			}
			else 
			{
				delete (*vit);
			}
		}
	}
	else
	{
		std::cerr << "p3IdService::recogn_request() getGroupData ERROR";
		std::cerr << std::endl;
	}

	// Schedule Processing.
	RsTickEvent::schedule_in(GXSID_EVENT_RECOGN_PROC, RECOGN_PROC_PERIOD);
	return true;
}


bool p3IdService::recogn_process()
{
	/* each time this is called - process one Id from mGroupsToProcess */
	RsGxsIdGroupItem *item;
	bool isDone = false;
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		if (!mRecognGroupsToProcess.empty())
		{
			item = mRecognGroupsToProcess.front();
			mRecognGroupsToProcess.pop_front();

#ifdef DEBUG_RECOGN
			std::cerr << "p3IdService::recogn_process() Popped Group: " << item->meta.mGroupId;
			std::cerr << std::endl;
#endif // DEBUG_RECOGN
		}
		else
		{
			isDone = true;
		}
	}

	if (isDone)
	{
#ifdef DEBUG_RECOGN
		std::cerr << "p3IdService::recogn_process() List Empty... Done";
		std::cerr << std::endl;
#endif // DEBUG_RECOGN
		// FINISHED.
		CacheArbitrationDone(BG_RECOGN);
		return true;
	}
	


	std::list<RsGxsRecognTagItem *> tagItems;
	std::list<RsGxsRecognTagItem *>::iterator it;

	recogn_extract_taginfo(item, tagItems);

	bool isPending = false;
	int i = 1;
	uint32_t tagValidFlags = 0;
	for(it = tagItems.begin(); it != tagItems.end(); ++it)
	{
		bool isTagPending = false;
		bool isTagOk = recogn_checktag(RsGxsId(item->meta.mGroupId.toStdString()), item->meta.mGroupName, *it, true, isPending);
		if (isTagOk)
		{
			tagValidFlags |= i;
		}
		else 
		{
			isPending |= isTagPending;
		}

		delete *it;
		i *= 2;		
	}

#ifdef DEBUG_RECOGN
	std::cerr << "p3IdService::recogn_process() Tags Checked, saving";
	std::cerr << std::endl;
#endif // DEBUG_RECOGN

	SSGxsIdGroup ssdata;
	ssdata.load(item->meta.mServiceString); // attempt load - okay if fails.

	ssdata.recogntags.setTags(true, isPending, tagValidFlags);
	ssdata.recogntags.lastCheckTs = time(NULL);
	ssdata.recogntags.publishTs = item->meta.mPublishTs;

	// update IdScore too.
	bool pgpId = (item->meta.mGroupFlags & RSGXSID_GROUPFLAG_REALID);
	ssdata.score.rep.updateIdScore(pgpId, ssdata.pgp.idKnown);
	ssdata.score.rep.update();

	/* set new Group ServiceString */
	uint32_t dummyToken = 0;
	std::string serviceString = ssdata.save();
	setGroupServiceString(dummyToken, item->meta.mGroupId, serviceString);

	cache_update_if_cached(RsGxsId(item->meta.mGroupId.toStdString()), serviceString);

	delete item;
	
	// Schedule Next Processing.
	RsTickEvent::schedule_in(GXSID_EVENT_RECOGN_PROC, RECOGN_PROC_PERIOD);
	return false; // as there are more items on the queue to process.
}


bool p3IdService::recogn_checktag(const RsGxsId &id, const std::string &nickname, RsGxsRecognTagItem *item, bool doSignCheck, bool &isPending)
{

#ifdef DEBUG_RECOGN
		std::cerr << "p3IdService::recogn_checktag() groupId: " << id;
		std::cerr << std::endl;
		std::cerr << "p3IdService::recogn_checktag() nickname: " << nickname;
		std::cerr << std::endl;
		std::cerr << "p3IdService::recogn_checktag() item: ";
		std::cerr << std::endl;
		((RsGxsRecognTagItem *) item)->print(std::cerr);
#endif // DEBUG_RECOGN

	// To check:
	// -------------------
	// date range.
	// id matches.
	// nickname matches.
	// signer is valid.
	// ------ 
	// signature is valid.  (only if doSignCheck == true)
	
	time_t now = time(NULL);
	isPending = false;
	
	// check date range.
	if ((item->valid_from > now) || (item->valid_to < now))
	{
#ifdef DEBUG_RECOGN
		std::cerr << "p3IdService::recogn_checktag() failed timestamp";
		std::cerr << std::endl;
#endif // DEBUG_RECOGN

		return false;
	}
	
	// id match.
	if (id != item->identity)
	{
#ifdef DEBUG_RECOGN
		std::cerr << "p3IdService::recogn_checktag() failed identity";
		std::cerr << std::endl;
#endif // DEBUG_RECOGN
		return false;
	}
	
	// nickname match.
	if (nickname != item->nickname)
	{
#ifdef DEBUG_RECOGN
		std::cerr << "p3IdService::recogn_checktag() failed nickname";
		std::cerr << std::endl;
#endif // DEBUG_RECOGN
		return false;
	}
	
	
	
	{
		/* check they validity of the Tag */
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

		
		std::map<RsGxsId, RsGxsRecognSignerItem *>::iterator it;
		it = mRecognSignKeys.find(item->sign.keyId);
		if (it == mRecognSignKeys.end())
		{
#ifdef DEBUG_RECOGN
			std::cerr << "p3IdService::recogn_checktag() failed to find signkey";
			std::cerr << std::endl;
#endif // DEBUG_RECOGN

			// If OldKey, then we don't want to reprocess.
			if (mRecognOldSignKeys.end() != 
				mRecognOldSignKeys.find(item->sign.keyId))
			{
				isPending = true; // need to reprocess later with new key
			}
			return false;
		}
		
		// Check tag_class is okay for signer.
		if (it->second->signing_classes.ids.end() == 
			std::find(it->second->signing_classes.ids.begin(), it->second->signing_classes.ids.end(), item->tag_class))
		{
#ifdef DEBUG_RECOGN
			std::cerr << "p3IdService::recogn_checktag() failed signing_class check";
			std::cerr << std::endl;
#endif // DEBUG_RECOGN
			return false;
		}
		
		// ALL Okay, just signature to check.
		if (!doSignCheck)
		{
			return true;
		}

		return RsRecogn::validateTagSignature(it->second, item);
	}
}


void p3IdService::loadRecognKeys()
{
	RsStackMutex stack(mIdMtx); /**** LOCKED MUTEX ****/

	RsRecogn::loadSigningKeys(mRecognSignKeys);
}



/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

#define MAX_KNOWN_PGPIDS	20 
#define MAX_UNKNOWN_PGPIDS	20 
#define MAX_PSEUDOIDS		20

#define DUMMY_GXSID_DELAY	5

void p3IdService::generateDummyData()
{

	generateDummy_OwnIds();

	time_t age = 0;
	for(int i = 0; i < MAX_KNOWN_PGPIDS; i++)
	{
		age += DUMMY_GXSID_DELAY;
		RsTickEvent::schedule_in(GXSID_EVENT_DUMMY_PGPID, age);
	}

	for(int i = 0; i < MAX_PSEUDOIDS; i++)
	{
		age += DUMMY_GXSID_DELAY;
		RsTickEvent::schedule_in(GXSID_EVENT_DUMMY_PSEUDOID, age);
	}

	for(int i = 0; i < MAX_UNKNOWN_PGPIDS; i++)
	{
		age += DUMMY_GXSID_DELAY;
		RsTickEvent::schedule_in(GXSID_EVENT_DUMMY_UNKNOWN_PGPID, age);
	}
}





void p3IdService::generateDummy_OwnIds()
{
	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

	/* grab all the gpg ids... and make some ids */

	RsPgpId ownId = mPgpUtils->getPGPOwnId();

#if 0
	// generate some ownIds.
    //int genCount = 0;
	int i;

	int nIds = 2 + (RSRandom::random_u32() % 2);
	for(i = 0; i < nIds; i++)
	{
		RsGxsIdGroup id;
	       	RsPeerDetails details;

		id.mMeta.mGroupFlags = RSGXSID_GROUPFLAG_REALID;

//		// HACK FOR DUMMY GENERATION.
//		id.mMeta.mAuthorId = ownId.toStdString();
//	       	if (rsPeers->getPeerDetails(ownId, details))
//		{
//			std::ostringstream out;
//			out << details.name << "_" << i + 1;
//
//			id.mMeta.mGroupName = out.str();
//		}

		uint32_t dummyToken = 0;
		createGroup(dummyToken, id);
	}
#endif
}


void p3IdService::generateDummy_FriendPGP()
{
	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

	// Now Generate for friends.
	std::list<RsPgpId> gpgids;
	std::list<RsPgpId>::const_iterator it;
	mPgpUtils->getGPGAllList(gpgids);

	RsGxsIdGroup id;

	id.mMeta.mGroupFlags = RSGXSID_GROUPFLAG_REALID;

	int idx = RSRandom::random_f32() * (gpgids.size() - 1);
	it = gpgids.begin();
	for(int j = 0; j < idx; j++, ++it) ;

#if 0
	// HACK FOR DUMMY GENERATION.
	id.mMeta.mAuthorId = RsGxsId::random() ;

	RsPeerDetails details;
	if (/*rsPeers->getPeerDetails(*it, details)*/false)
	{
		std::ostringstream out;
		out << details.name << "_" << RSRandom::random_u32() % 1000;
		id.mMeta.mGroupName = out.str();
	}
	else
	{
		std::cerr << "p3IdService::generateDummy_FriendPGP() missing" << std::endl;
		std::cerr << std::endl;
		id.mMeta.mGroupName = RSRandom::random_alphaNumericString(10) ;
	}

	uint32_t dummyToken = 0;
	createGroup(dummyToken, id);
#endif
}


void p3IdService::generateDummy_UnknownPGP()
{
	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

	RsGxsIdGroup id;

	// FAKE DATA.
	id.mMeta.mGroupFlags = RSGXSID_GROUPFLAG_REALID;
	id.mPgpIdHash = Sha1CheckSum::random() ;
	id.mPgpIdSign = RSRandom::random_alphaNumericString(20) ;
	id.mMeta.mGroupName = RSRandom::random_alphaNumericString(10) ;

	uint32_t dummyToken = 0;
	createGroup(dummyToken, id);
}


void p3IdService::generateDummy_UnknownPseudo()
{
	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

	RsGxsIdGroup id;

	// FAKE DATA.
	id.mMeta.mGroupFlags = 0;
	id.mMeta.mGroupName = RSRandom::random_alphaNumericString(10) ;

	uint32_t dummyToken = 0;
	createGroup(dummyToken, id);
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
 *
 * So Reputation....
 *   4 components:
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


/************************************************************************************/
/*
 * Scoring system.
 * -100 to 100 is expected range.
 * 
 *
 * Each Lobby has a publish threshold.
 *   - As part of Lobby definition. ???
 *   - Locally Set.
 *
 * Threshold:
 *   50 VIP List.
 *   20 Dress Code
 *   10 Limit Riffraff.
 *   0 Accept All.
 *
 * Implicit Scores:
 *   +50 for known PGP
 *   +10 for unknown PGP  (want to encourage usage).
 *   +5 for Anon ID.
 *
 * Own Scores:
 *   +1000 Accepted
 *   +50 Friend
 *   +10 Interesting
 *   0 Mostly Harmless
 *   -10 Annoying.
 *   -50 Troll
 *   -1000 Total Banned
 *
 *
 * 



Processing Algorithm:
 *  - Grab all Groups which have received messages. 
 *  (opt 1)-> grab latest msgs for each of these and process => score.
 *  (opt 2)-> try incremental system (people probably won't change opinions often -> just set them once)
 *      --> if not possible, fallback to full calculation.
 *
 * 
 */




std::ostream &operator<<(std::ostream &out, const RsGxsIdGroup &grp)
{
	out << "RsGxsIdGroup: Meta: " << grp.mMeta;
	out << " PgpIdHash: " << grp.mPgpIdHash;
	out << " PgpIdSign: [binary]"; // << grp.mPgpIdSign;
	out << std::endl;
	
	return out;
}


void p3IdService::checkPeerForIdentities()
{
	RsStackMutex stack(mIdMtx);

	// crud, i needed peers instead!
	mGroupNotPresent.clear();
}


// Overloaded from GxsTokenQueue for Request callbacks.
void p3IdService::handleResponse(uint32_t token, uint32_t req_type)
{
#ifdef DEBUG_IDS
	std::cerr << "p3IdService::handleResponse(" << token << "," << req_type << ")";
	std::cerr << std::endl;
#endif // DEBUG_IDS

	// stuff.
	switch(req_type)
	{
		case GXSIDREQ_CACHEOWNIDS:
			cache_load_ownids(token);
			break;
		case GXSIDREQ_CACHELOAD:
			cache_load_for_token(token);
			break;
		case GXSIDREQ_PGPHASH:
			pgphash_handlerequest(token);
			break;
		case GXSIDREQ_RECOGN:
			recogn_handlerequest(token);
			break;
		case GXSIDREQ_CACHETEST:
			cachetest_handlerequest(token);
			break;
		case GXSIDREQ_OPINION:
			opinion_handlerequest(token);
			break;
		default:
			/* error */
			std::cerr << "p3IdService::handleResponse() Unknown Request Type: " << req_type;
			std::cerr << std::endl;
			break;
	}
}


	// Overloaded from RsTickEvent for Event callbacks.
void p3IdService::handle_event(uint32_t event_type, const std::string &elabel)
{
#ifdef DEBUG_IDS
	std::cerr << "p3IdService::handle_event(" << event_type << ")";
	std::cerr << std::endl;
#endif // DEBUG_IDS

	// stuff.
	switch(event_type)
	{
		case GXSID_EVENT_CACHEOWNIDS:
			cache_request_ownids();
			break;

		case GXSID_EVENT_CACHELOAD:
			cache_start_load();
			break;

		case GXSID_EVENT_CACHETEST:
			cachetest_getlist();
			break;

		case GXSID_EVENT_PGPHASH:
			pgphash_start();
			break;

		case GXSID_EVENT_PGPHASH_PROC:
			pgphash_process();
			break;

		case GXSID_EVENT_RECOGN:
			recogn_start();
			break;

		case GXSID_EVENT_RECOGN_PROC:
			recogn_process();
			break;

		case GXSID_EVENT_DUMMYDATA:
			generateDummyData();
			break;

		case GXSID_EVENT_DUMMY_OWNIDS:
			generateDummy_OwnIds();
			break;

		case GXSID_EVENT_DUMMY_PGPID:
			generateDummy_FriendPGP();
			break;

		case GXSID_EVENT_DUMMY_UNKNOWN_PGPID:
			generateDummy_UnknownPGP();
			break;

		case GXSID_EVENT_DUMMY_PSEUDOID:
			generateDummy_UnknownPseudo();
			break;
		case GXSID_EVENT_REQUEST_IDS:
			requestIdsFromNet();
			break;


		default:
			/* error */
			std::cerr << "p3IdService::handle_event() Unknown Event Type: " << event_type;
			std::cerr << std::endl;
			break;
	}
}



