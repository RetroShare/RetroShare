/*******************************************************************************
 * libretroshare/src/services: p3idservice.cc                                  *
 *                                                                             *
 * Copyright (C) 2012-2014  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2017-2019  Gioacchino Mazzurco <gio@altermundi.net>           *
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

/// RetroShare GXS identities service


#include <unistd.h>
#include <algorithm>
#include <sstream>
#include <cstdio>

#include "services/p3idservice.h"
#include "pgp/pgpauxutils.h"
#include "rsitems/rsgxsiditems.h"
#include "rsitems/rsconfigitems.h"
#include "retroshare/rsgxsflags.h"
#include "util/rsrandom.h"
#include "util/rsstring.h"
#include "util/radix64.h"
#include "util/rsdir.h"
#include "util/rstime.h"
#include "crypto/hashstream.h"
#include "gxs/gxssecurity.h"
#include "retroshare/rspeers.h"
#include "retroshare/rsnotify.h"


/****
 * #define DEBUG_IDS	1
 * #define DEBUG_RECOGN	1
 * #define DEBUG_OPINION 1
 * #define GXSID_GEN_DUMMY_DATA	1
 ****/

#define ID_REQUEST_LIST		    0x0001
#define ID_REQUEST_IDENTITY	    0x0002
#define ID_REQUEST_REPUTATION	0x0003
#define ID_REQUEST_OPINION	    0x0004

#define GXSID_MAX_CACHE_SIZE 5000

// unused keys are deleted according to some heuristic that should favor known keys, signed keys etc. 

static const rstime_t MAX_KEEP_KEYS_BANNED_DEFAULT =     2 * 86400 ; // get rid of banned ids after 1 days. That gives a chance to un-ban someone before he gets definitely kicked out

static const rstime_t MAX_KEEP_KEYS_DEFAULT      =     5 * 86400 ; // default for unsigned identities: 5 days
static const rstime_t MAX_KEEP_KEYS_SIGNED       =     8 * 86400 ; // signed identities by unknown key
static const rstime_t MAX_KEEP_KEYS_SIGNED_KNOWN =    30 * 86400 ; // signed identities by known node keys

static const uint32_t MAX_DELAY_BEFORE_CLEANING=    1800 ; // clean old keys every 30 mins

static const uint32_t MAX_SERIALISED_IDENTITY_AGE  = 600 ; // after 10 mins, a serialised identity record must be renewed.

RsIdentity* rsIdentity = nullptr;

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


#define GXSIDREQ_CACHELOAD	         0x0001
#define GXSIDREQ_CACHEOWNIDS	     0x0002
#define GXSIDREQ_PGPHASH 	         0x0010
#define GXSIDREQ_RECOGN 	         0x0020
#define GXSIDREQ_OPINION 	         0x0030
#define GXSIDREQ_SERIALIZE_TO_MEMORY 0x0040

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

#define CACHETEST_PERIOD	            60
#define DELAY_BETWEEN_CONFIG_UPDATES   300
#define GXS_MAX_KEY_TS_USAGE_MAP_SIZE    5

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

p3IdService::p3IdService(
        RsGeneralDataService *gds, RsNetworkExchangeService *nes,
        PgpAuxUtils *pgpUtils ) :
    RsGxsIdExchange( gds, nes, new RsGxsIdSerialiser(),
                     RS_SERVICE_GXS_TYPE_GXSID, idAuthenPolicy() ),
    RsIdentity(static_cast<RsGxsIface&>(*this)), GxsTokenQueue(this),
    RsTickEvent(), mKeyCache(GXSID_MAX_CACHE_SIZE, "GxsIdKeyCache"),
    mIdMtx("p3IdService"), mNes(nes), mPgpUtils(pgpUtils)
{
	mBgSchedule_Mode = 0;
    mBgSchedule_Active = false;
    mLastKeyCleaningTime = time(NULL) - int(MAX_DELAY_BEFORE_CLEANING * 0.9) ;
    mLastConfigUpdate = 0 ;
    mOwnIdsLoaded = false ;
	mAutoAddFriendsIdentitiesAsContacts = true; // default
    mMaxKeepKeysBanned = MAX_KEEP_KEYS_BANNED_DEFAULT;

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

bool p3IdService::getIdentitiesInfo(
        const std::set<RsGxsId>& ids, std::vector<RsGxsIdGroup>& idsInfo )
{
	uint32_t token;

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	std::list<RsGxsGroupId> idsList;
	for (auto&& id : ids) idsList.push_back(RsGxsGroupId(id));

	if( !requestGroupInfo(token, opts, idsList)
	        || waitToken(token) != RsTokenService::COMPLETE ) return false;
	return getGroupData(token, idsInfo);
}

bool p3IdService::getIdentitiesSummaries(std::list<RsGroupMetaData>& ids)
{
	uint32_t token;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;
	if( !requestGroupInfo(token, opts)
	        || waitToken(token) != RsTokenService::COMPLETE ) return false;
	return getGroupSummary(token, ids);
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

uint32_t p3IdService::nbRegularContacts()
{
    RsStackMutex stack(mIdMtx);
    return mContacts.size();
}

bool p3IdService::isARegularContact(const RsGxsId& id)
{
    RsStackMutex stack(mIdMtx);
    return mContacts.find(id) != mContacts.end() ;
}

bool p3IdService::setAsRegularContact(const RsGxsId& id,bool b)
{
    RsStackMutex stack(mIdMtx);
    std::set<RsGxsId>::iterator it = mContacts.find(id) ;
    
    if(b && (it == mContacts.end()))
    {
        mContacts.insert(id) ;
        slowIndicateConfigChanged() ;
    }
    
    if( (!b) &&(it != mContacts.end()))
    {
        mContacts.erase(it) ;
        slowIndicateConfigChanged() ;
    }
    
    return true ;
}

void p3IdService::slowIndicateConfigChanged()
{
    rstime_t now = time(NULL) ;

    if(mLastConfigUpdate + DELAY_BETWEEN_CONFIG_UPDATES < now)
    {
        IndicateConfigChanged() ;
    mLastConfigUpdate = now ;
    }
}
rstime_t p3IdService::locked_getLastUsageTS(const RsGxsId& gxs_id)
{
    std::map<RsGxsId,keyTSInfo>::const_iterator it = mKeysTS.find(gxs_id) ;

    if(it == mKeysTS.end())
        return 0 ;
    else
        return it->second.TS ;
}
void p3IdService::timeStampKey(const RsGxsId& gxs_id, const RsIdentityUsage& reason)
{
    if(rsReputations->isIdentityBanned(gxs_id) )
    {
        std::cerr << "(II) p3IdService:timeStampKey(): refusing to time stamp key " << gxs_id << " because it is banned." << std::endl;
        return ;
    }
#ifdef DEBUG_IDS
    std::cerr << "(II) time stamping key " << gxs_id << " for the following reason: " << reason.mUsageCode << std::endl;
#endif

    RS_STACK_MUTEX(mIdMtx) ;

    rstime_t now = time(NULL) ;

    keyTSInfo& info(mKeysTS[gxs_id]) ;

    info.TS = now ;
    info.usage_map[reason] = now;

    while(info.usage_map.size() > GXS_MAX_KEY_TS_USAGE_MAP_SIZE)
    {
        // This is very costly, but normally the outerloop should never be rolled more than once.

        std::map<RsIdentityUsage,rstime_t>::iterator best_it ;
        rstime_t best_time = now+1;

        for(std::map<RsIdentityUsage,rstime_t>::iterator it(info.usage_map.begin());it!=info.usage_map.end();++it)
            if(it->second < best_time)
            {
                best_time = it->second ;
                best_it = it;
            }

        info.usage_map.erase(best_it) ;
    }

    slowIndicateConfigChanged() ;
}

bool p3IdService::loadList(std::list<RsItem*>& items)
{
    RS_STACK_MUTEX(mIdMtx) ;
    RsGxsIdLocalInfoItem *lii;

    for(std::list<RsItem*>::const_iterator it = items.begin();it!=items.end();++it)
    {
        if( (lii = dynamic_cast<RsGxsIdLocalInfoItem*>(*it)) != NULL)
        {
            for(std::map<RsGxsId,rstime_t>::const_iterator it2 = lii->mTimeStamps.begin();it2!=lii->mTimeStamps.end();++it2)
                mKeysTS[it2->first].TS = it2->second;

            mContacts = lii->mContacts ;
        }

	    RsConfigKeyValueSet *vitem = dynamic_cast<RsConfigKeyValueSet *>(*it);

	    if(vitem)
		    for(std::list<RsTlvKeyValue>::const_iterator kit = vitem->tlvkvs.pairs.begin(); kit != vitem->tlvkvs.pairs.end(); ++kit)
		    {
			    if(kit->key == "REMOVE_BANNED_IDENTITIES_DELAY")
			    {
				    int val ;
				    if (sscanf(kit->value.c_str(), "%d", &val) == 1)
				    {
					    mMaxKeepKeysBanned = val ;
					    std::cerr << "Setting mMaxKeepKeysBanned threshold to " << val << std::endl ;
				    }
			    };
                if(kit->key == "AUTO_SET_FRIEND_IDENTITIES_AS_CONTACT")
					mAutoAddFriendsIdentitiesAsContacts = (kit->value == "YES") ;
            }

        delete *it ;
    }

    items.clear() ;
    return true ;
}

void p3IdService::setDeleteBannedNodesThreshold(uint32_t days)
{
    RsStackMutex stack(mIdMtx); /****** LOCKED MUTEX *******/
    if(mMaxKeepKeysBanned != days*86400)
    {
        mMaxKeepKeysBanned = days*86400 ;
        IndicateConfigChanged();
    }
}
uint32_t p3IdService::deleteBannedNodesThreshold()
{
    RsStackMutex stack(mIdMtx); /****** LOCKED MUTEX *******/

    return mMaxKeepKeysBanned/86400;
}

void p3IdService::setAutoAddFriendIdsAsContact(bool b)
{
    RS_STACK_MUTEX(mIdMtx) ;
    if(b != mAutoAddFriendsIdentitiesAsContacts)
    {
        IndicateConfigChanged();
        mAutoAddFriendsIdentitiesAsContacts=b;
    }
}
bool p3IdService::autoAddFriendIdsAsContact()
{
    RS_STACK_MUTEX(mIdMtx) ;
    return mAutoAddFriendsIdentitiesAsContacts;
}

bool p3IdService::saveList(bool& cleanup,std::list<RsItem*>& items)
{
#ifdef DEBUG_IDS
    std::cerr << "p3IdService::saveList()" << std::endl;
#endif

    RS_STACK_MUTEX(mIdMtx) ;
    cleanup = true ;
    RsGxsIdLocalInfoItem *item = new RsGxsIdLocalInfoItem ;

    for(std::map<RsGxsId,keyTSInfo>::const_iterator it(mKeysTS.begin());it!=mKeysTS.end();++it)
		item->mTimeStamps[it->first] = it->second.TS;

    item->mContacts = mContacts ;

    items.push_back(item) ;

    RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet ;
	RsTlvKeyValue kv;

	kv.key = "REMOVE_BANNED_IDENTITIES_DELAY" ;
	rs_sprintf(kv.value, "%d", mMaxKeepKeysBanned);
	vitem->tlvkvs.pairs.push_back(kv) ;

	kv.key = "AUTO_SET_FRIEND_IDENTITIES_AS_CONTACT" ;
	kv.value = mAutoAddFriendsIdentitiesAsContacts?"YES":"NO";
	vitem->tlvkvs.pairs.push_back(kv) ;

    items.push_back(vitem) ;

    return true ;
}

class IdCacheEntryCleaner
{
public:
    IdCacheEntryCleaner(const std::map<RsGxsId,p3IdService::keyTSInfo>& last_usage_TSs,uint32_t m) : mLastUsageTS(last_usage_TSs),mMaxKeepKeysBanned(m) {}

    bool processEntry(RsGxsIdCache& entry)
    {
        rstime_t now = time(NULL);
        const RsGxsId& gxs_id = entry.details.mId ;

        bool is_id_banned = rsReputations->isIdentityBanned(gxs_id) ;
        bool is_own_id    = (bool)(entry.details.mFlags & RS_IDENTITY_FLAGS_IS_OWN_ID) ;
        bool is_known_id  = (bool)(entry.details.mFlags & RS_IDENTITY_FLAGS_PGP_KNOWN) ;
        bool is_signed_id = (bool)(entry.details.mFlags & RS_IDENTITY_FLAGS_PGP_LINKED) ;
        bool is_a_contact = (bool)(entry.details.mFlags & RS_IDENTITY_FLAGS_IS_A_CONTACT) ;

#ifdef DEBUG_IDS
        std::cerr << "Identity: " << gxs_id << ": banned: " << is_id_banned << ", own: " << is_own_id << ", contact: " << is_a_contact << ", signed: " << is_signed_id << ", known: " << is_known_id;
#endif

        if(is_own_id || is_a_contact)
        {
#ifdef DEBUG_IDS
            std::cerr << " => kept" << std::endl;
#endif
            return true ;
        }

        std::map<RsGxsId,p3IdService::keyTSInfo>::const_iterator it = mLastUsageTS.find(gxs_id) ;

        bool no_ts = (it == mLastUsageTS.end()) ;

        rstime_t last_usage_ts = no_ts?0:(it->second.TS);
        rstime_t max_keep_time = 0;
        bool should_check = true ;

        if(no_ts)
            max_keep_time = 0 ;
		else if(is_id_banned)
        {
			if(mMaxKeepKeysBanned == 0)
				should_check = false ;
			else
				max_keep_time = mMaxKeepKeysBanned ;
        }
		else if(is_known_id)
            max_keep_time = MAX_KEEP_KEYS_SIGNED_KNOWN ;
        else if(is_signed_id)
            max_keep_time = MAX_KEEP_KEYS_SIGNED ;
        else
            max_keep_time = MAX_KEEP_KEYS_DEFAULT ;

#ifdef DEBUG_IDS
        std::cerr << ". Max keep = " << max_keep_time/86400 << " days. Unused for " << (now - last_usage_ts + 86399)/86400 << " days " ;
#endif

        if(should_check && now > last_usage_ts + max_keep_time)
        {
#ifdef DEBUG_IDS
            std::cerr << " => delete " << std::endl;
#endif
            ids_to_delete.push_back(gxs_id) ;
        }
#ifdef DEBUG_IDS
        else
            std::cerr << " => keep " << std::endl;
#endif

        return true;
    }

    std::list<RsGxsId> ids_to_delete ;
    const std::map<RsGxsId,p3IdService::keyTSInfo>& mLastUsageTS;
    uint32_t mMaxKeepKeysBanned ;
};

void p3IdService::cleanUnusedKeys()
{
    std::list<RsGxsId> ids_to_delete ;

    std::cerr << "Cleaning unused keys:" << std::endl;

    // we need to stash all ids to delete into an off-mutex structure since deleteIdentity() will trigger the lock
    {
        RS_STACK_MUTEX(mIdMtx) ;

        if(!mOwnIdsLoaded)
        {
            std::cerr << "(EE) Own ids not loaded. Cannot clean unused keys." << std::endl;
            return ;
        }

        // grab at most 10 identities to delete. No need to send too many requests to the token queue at once.
        IdCacheEntryCleaner idcec(mKeysTS,mMaxKeepKeysBanned) ;

        mKeyCache.applyToAllCachedEntries(idcec,&IdCacheEntryCleaner::processEntry);

        ids_to_delete = idcec.ids_to_delete ;
    }
    std::cerr << "Collected " << ids_to_delete.size() << " keys to delete among " << mKeyCache.size() << std::endl;

    for(std::list<RsGxsId>::const_iterator it(ids_to_delete.begin());it!=ids_to_delete.end();++it)
    {
#ifdef DEBUG_IDS
        std::cerr << "Deleting identity " << *it << " which is too old." << std::endl;
#endif
        uint32_t token ;
        RsGxsIdGroup group;
        group.mMeta.mGroupId=RsGxsGroupId(*it);
        rsIdentity->deleteIdentity(token, group);

        {
            RS_STACK_MUTEX(mIdMtx) ;
            mKeysTS.erase(*it) ;

            // mPublicKeyCache.erase(*it) ; no need to do it now. It's done in p3IdService::deleteGroup()
        }
    }
}

void	p3IdService::service_tick()
{
    RsTickEvent::tick_events();
    GxsTokenQueue::checkRequests(); // GxsTokenQueue handles all requests.

    rstime_t now = time(NULL) ;

    if(mLastKeyCleaningTime + MAX_DELAY_BEFORE_CLEANING < now)
    {
        cleanUnusedKeys() ;
        mLastKeyCleaningTime = now ;
    }
    return;
}

bool p3IdService::acceptNewGroup(const RsGxsGrpMetaData *grpMeta)
{
    bool res = !rsReputations->isIdentityBanned(RsGxsId(grpMeta->mGroupId)) ;

#ifdef DEBUG_IDS
    std::cerr << "p3IdService::acceptNewGroup: ID=" << grpMeta->mGroupId << ": " << (res?"ACCEPTED":"DENIED") << std::endl;
#endif

    return res ;
}

void p3IdService::notifyChanges(std::vector<RsGxsNotify *> &changes)
{
#ifdef DEBUG_IDS
    std::cerr << "p3IdService::notifyChanges()";
    std::cerr << std::endl;
#endif

    /* iterate through and grab any new messages */
    std::list<RsGxsGroupId> unprocessedGroups;

    for(uint32_t i = 0;i<changes.size();++i)
    {
        RsGxsMsgChange *msgChange = dynamic_cast<RsGxsMsgChange *>(changes[i]);

        if (msgChange && !msgChange->metaChange())
        {
#ifdef DEBUG_IDS
            std::cerr << "p3IdService::notifyChanges() Found Message Change Notification";
            std::cerr << std::endl;
#endif

            std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &msgChangeMap = msgChange->msgChangeMap;

            for(auto mit = msgChangeMap.begin(); mit != msgChangeMap.end(); ++mit)
            {
#ifdef DEBUG_IDS
                std::cerr << "p3IdService::notifyChanges() Msgs for Group: " << mit->first;
                std::cerr << std::endl;
#endif
            }
        }

        RsGxsGroupChange *groupChange = dynamic_cast<RsGxsGroupChange *>(changes[i]);

        if (groupChange && !groupChange->metaChange())
        {
#ifdef DEBUG_IDS
            std::cerr << "p3IdService::notifyChanges() Found Group Change Notification";
            std::cerr << std::endl;
#endif
            std::list<RsGxsGroupId> &groupList = groupChange->mGrpIdList;

            for(auto git = groupList.begin(); git != groupList.end();++git)
            {
#ifdef DEBUG_IDS
                std::cerr << "p3IdService::notifyChanges() Auto Subscribe to Incoming Groups: " << *git;
                std::cerr << std::endl;
#endif
                if(!rsReputations->isIdentityBanned(RsGxsId(*git)))
                {
                    uint32_t token;
                    RsGenExchange::subscribeToGroup(token, *git, true);

                    // also time_stamp the key that this group represents

                    timeStampKey(RsGxsId(*git),RsIdentityUsage(serviceType(),RsIdentityUsage::IDENTITY_DATA_UPDATE)) ;

                    // notify that a new identity is received, if needed

                    switch(groupChange->getType())
                    {
                    case RsGxsNotify::TYPE_PUBLISHED:
                    case RsGxsNotify::TYPE_RECEIVED_NEW:
                    {
                        auto ev = std::make_shared<RsGxsIdentityEvent>();
                        ev->mIdentityId = *git;
                        ev->mIdentityEventCode = RsGxsIdentityEventCode::NEW_IDENTITY;
                        rsEvents->postEvent(ev);
                    }
                        break;

                    default:
                        break;
                    }
                }
            }
        }

        delete changes[i];
    }
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

rstime_t p3IdService::getLastUsageTS(const RsGxsId &id)
{
    RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
    return locked_getLastUsageTS(id) ;
}

bool p3IdService::getIdDetails(const RsGxsId &id, RsIdentityDetails &details)
{
#ifdef DEBUG_IDS
    std::cerr << "p3IdService::getIdDetails(" << id << ")";
    std::cerr << std::endl;
#endif

    {
        RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

        RsGxsIdCache data;

        if (mKeyCache.fetch(id, data))
        {
            bool is_a_contact = (mContacts.find(id) != mContacts.end());

            details = data.details;

            if(mAutoAddFriendsIdentitiesAsContacts && (!is_a_contact) && (details.mFlags & RS_IDENTITY_FLAGS_PGP_KNOWN) && rsPeers->isPgpFriend(details.mPgpId))
            {
				mContacts.insert(id) ;
				slowIndicateConfigChanged() ;

                is_a_contact = true;
            }

            // This step is needed, because p3GxsReputation does not know all identities, and might not have any data for
            // the ones in the contact list. So we change them on demand.

            if(is_a_contact && rsReputations->autoPositiveOpinionForContacts())
			{
				RsOpinion op;
				if( rsReputations->getOwnOpinion(id,op) &&
				        op == RsOpinion::NEUTRAL )
					rsReputations->setOwnOpinion(id, RsOpinion::POSITIVE);
			}

			std::map<RsGxsId,keyTSInfo>::const_iterator it = mKeysTS.find(id) ;

			if(it == mKeysTS.end())
				details.mLastUsageTS = 0 ;
            else
            {
				details.mLastUsageTS = it->second.TS ;
				details.mUseCases = it->second.usage_map ;
            }
            details.mPublishTS = data.mPublishTs;

            // one utf8 symbol can be at most 4 bytes long - would be better to measure real unicode length !!!
            if(details.mNickname.length() > RSID_MAXIMUM_NICKNAME_SIZE*4)
                details.mNickname = "[too long a name]" ;

            rsReputations->getReputationInfo(id,details.mPgpId,details.mReputation) ;

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


bool p3IdService::getOwnSignedIds(std::vector<RsGxsId>& ids)
{
	ids.clear();

	std::chrono::seconds maxWait(5);
	auto timeout = std::chrono::steady_clock::now() + maxWait;
	while( !ownIdsAreLoaded() && std::chrono::steady_clock::now() < timeout )
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

	if(ownIdsAreLoaded())
	{
		RS_STACK_MUTEX(mIdMtx);
		ids.resize(mOwnSignedIds.size());
		std::copy(mOwnSignedIds.begin(), mOwnSignedIds.end(), ids.begin());
		return true;
	}

	return false;
}

bool p3IdService::getOwnPseudonimousIds(std::vector<RsGxsId>& ids)
{
	ids.clear();
	std::vector<RsGxsId> signedV;

	// this implicitely ensure ids are already loaded ;)
	if(!getOwnSignedIds(signedV)) return false;

	std::set<RsGxsId> signedS(signedV.begin(), signedV.end());

	{
		RS_STACK_MUTEX(mIdMtx);
		ids.resize(mOwnIds.size() - signedV.size());
		std::copy_if( mOwnIds.begin(), mOwnIds.end(), ids.begin(),
		              [&](const RsGxsId& id) {return !signedS.count(id);} );
	}

	return true;
}

bool p3IdService::getOwnIds(std::list<RsGxsId> &ownIds,bool signed_only)
{
    RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

    if(!mOwnIdsLoaded)
    {
        std::cerr << "p3IdService::getOwnIds(): own identities are not loaded yet." << std::endl;
        return false ;
    }

	ownIds = signed_only ? mOwnSignedIds : mOwnIds;

    return true ;
}

bool p3IdService::isKnownId(const RsGxsId& id)
{
	RS_STACK_MUTEX(mIdMtx);
	return mKeyCache.is_cached(id) ||
	        std::find(mOwnIds.begin(), mOwnIds.end(),id) != mOwnIds.end();
}

bool p3IdService::serialiseIdentityToMemory( const RsGxsId& id,
                                             std::string& radix_string )
{
	RS_STACK_MUTEX(mIdMtx);

    // look into cache. If available, return the data. If not, request it.

    std::map<RsGxsId,SerialisedIdentityStruct>::const_iterator it = mSerialisedIdentities.find(id);

    if(it != mSerialisedIdentities.end())
    {
        Radix64::encode(it->second.mMem,it->second.mSize,radix_string) ;

        if(it->second.mLastUsageTS + MAX_SERIALISED_IDENTITY_AGE > time(NULL))
			return true ;

        std::cerr << "Identity " << id << " will be re-serialised, because the last record is too old." << std::endl;
    }

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
	uint32_t token = 0;
    std::list<RsGxsGroupId> groupIds;

    groupIds.push_back(RsGxsGroupId(id)) ;

	RsGenExchange::getTokenService()->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds);
	GxsTokenQueue::queueRequest(token, GXSIDREQ_SERIALIZE_TO_MEMORY);

	return false;
}

void p3IdService::handle_get_serialized_grp(uint32_t token)
{
    // store the serialized data in cache.

    unsigned char *mem = NULL;
    uint32_t size;
    RsGxsGroupId id ;

	if(!RsGenExchange::getSerializedGroupData(token,id, mem,size))
    {
        std::cerr << "(EE) call to RsGenExchage::getSerializedGroupData() failed." << std::endl;
        return ;
    }

    std::cerr << "Received serialised group from RsGenExchange." << std::endl;

    std::map<RsGxsId,SerialisedIdentityStruct>::const_iterator it = mSerialisedIdentities.find(RsGxsId(id));

    if(it != mSerialisedIdentities.end())
        free(it->second.mMem) ;

    SerialisedIdentityStruct s ;
    s.mMem = mem ;
    s.mSize = size ;
    s.mLastUsageTS = time(NULL) ;

    mSerialisedIdentities[RsGxsId(id)] = s ;
}

bool p3IdService::deserialiseIdentityFromMemory(const std::string& radix_string,
                                                RsGxsId* id /* = nullptr */)
{
	std::vector<uint8_t> mem = Radix64::decode(radix_string);

	if(mem.empty())
	{
		std::cerr << __PRETTY_FUNCTION__ << "Cannot decode radix string \""
		          << radix_string << "\"" << std::endl;
		return false;
	}

	if( !RsGenExchange::deserializeGroupData(
	            mem.data(), mem.size(), reinterpret_cast<RsGxsGroupId*>(id)) )
	{
		std::cerr << __PRETTY_FUNCTION__ << "Cannot load identity from radix "
		          << "string \"" << radix_string << "\"" << std::endl;
		return false;
	}

	return true;
}

bool p3IdService::createIdentity(
        RsGxsId& id,
        const std::string& name, const RsGxsImage& avatar,
        bool pseudonimous, const std::string& pgpPassword)
{
	bool ret = true;
	RsIdentityParameters params;
	uint32_t token = 0;
	RsGroupMetaData meta;
	RsTokenService::GxsRequestStatus wtStatus = RsTokenService::CANCELLED;

	if(!pseudonimous && !pgpPassword.empty())
	{
		if(!rsNotify->cachePgpPassphrase(pgpPassword))
		{
			RsErr() << __PRETTY_FUNCTION__ << " Failure caching password"
			        << std::endl;
			ret = false;
			goto LabelCreateIdentityCleanup;
		}

		if(!rsNotify->setDisableAskPassword(true))
		{
			RsErr() << __PRETTY_FUNCTION__ << " Failure disabling password user"
			        << " request" << std::endl;
			ret = false;
			goto LabelCreateIdentityCleanup;
		}
	}

	params.isPgpLinked = !pseudonimous;
	params.nickname = name;
	params.mImage = avatar;

	if(!createIdentity(token, params))
	{
		RsErr() << __PRETTY_FUNCTION__ << " Failed creating GXS group."
		        << std::endl;
		ret = false;
		goto LabelCreateIdentityCleanup;
	}

	/* Use custom timeout for waitToken because creating identities involves
	 * creating multiple signatures, which can take a lot of time expecially on
	 * slow hardware like phones or embedded devices */
	if( (wtStatus = waitToken(
	         token, std::chrono::seconds(10), std::chrono::milliseconds(20) ))
	        != RsTokenService::COMPLETE )
	{
		RsErr() << __PRETTY_FUNCTION__ << " waitToken("<< token
		        << ") failed with: " << wtStatus << std::endl;
		ret = false;
		goto LabelCreateIdentityCleanup;
	}

	if(!RsGenExchange::getPublishedGroupMeta(token, meta))
	{
		RsErr() << __PRETTY_FUNCTION__ << " Failure getting updated group data."
		        << std::endl;
		ret = false;
		goto LabelCreateIdentityCleanup;
	}

	id = RsGxsId(meta.mGroupId);

	{
		RS_STACK_MUTEX(mIdMtx);
		mOwnIds.push_back(id);
		if(!pseudonimous) mOwnSignedIds.push_back(id);
	}

LabelCreateIdentityCleanup:
	if(!pseudonimous && !pgpPassword.empty())
	{
		rsNotify->setDisableAskPassword(false);
		rsNotify->clearPgpPassphrase();
	}

	return ret;
}

bool p3IdService::createIdentity(uint32_t& token, RsIdentityParameters &params)
{

    RsGxsIdGroup id;

    id.mMeta.mGroupName = params.nickname;
    id.mMeta.mCircleType = GXS_CIRCLE_TYPE_PUBLIC ;
    id.mImage = params.mImage;

    if (params.isPgpLinked)
    {
#warning csoler 2017-02-07: Backward compatibility issue to fix here in v0.7.0

        // This is a hack, because a bad decision led to having RSGXSID_GROUPFLAG_REALID be equal to GXS_SERV::FLAG_PRIVACY_PRIVATE.
        // In order to keep backward compatibility, we'll also add the new value
        // When the ID is not PGP linked, the group flag cannot be let empty, so we use PUBLIC.
        //
        // The correct combination of flags should be:
        //		PGP-linked:		GXS_SERV::FLAGS_PRIVACY_PUBLIC | RSGXSID_GROUPFLAG_REALID
        //		Anonymous :		GXS_SERV::FLAGS_PRIVACY_PUBLIC

        id.mMeta.mGroupFlags |= GXS_SERV::FLAG_PRIVACY_PRIVATE;	// this is also equal to RSGXSID_GROUPFLAG_REALID_deprecated
        id.mMeta.mGroupFlags |= RSGXSID_GROUPFLAG_REALID;

        // The current version should be able to produce new identities that old peers will accept as well.
        // In the future, we need to:
        //     - set the current group flags here (see above)
        //	   - replace all occurences of RSGXSID_GROUPFLAG_REALID_deprecated by RSGXSID_GROUPFLAG_REALID in the code.
    }
    else
        id.mMeta.mGroupFlags |= GXS_SERV::FLAG_PRIVACY_PUBLIC;

    createGroup(token, id);

    return true;
}

bool p3IdService::updateIdentity(RsGxsIdGroup& identityData)
{
	uint32_t token;
	if(!updateGroup(token, identityData))
	{
		std::cerr << __PRETTY_FUNCTION__ << "Error! Failed updating group."
		          << std::endl;
		return false;
	}

	if(waitToken(token) != RsTokenService::COMPLETE)
	{
		std::cerr << __PRETTY_FUNCTION__ << "Error! GXS operation failed."
		          << std::endl;
		return false;
	}

	return true;
}

bool p3IdService::updateIdentity(uint32_t& token, RsGxsIdGroup &group)
{
#ifdef DEBUG_IDS
    std::cerr << "p3IdService::updateIdentity()";
    std::cerr << std::endl;
#endif
    group.mMeta.mCircleType = GXS_CIRCLE_TYPE_PUBLIC ;

    updateGroup(token, group);

    return false;
}

bool p3IdService::deleteIdentity(RsGxsId& id)
{
	uint32_t token;
	RsGxsGroupId grouId = RsGxsGroupId(id);
	if(!deleteGroup(token, grouId))
	{
		std::cerr << __PRETTY_FUNCTION__ << "Error! Failed deleting group."
		          << std::endl;
		return false;
	}

	if(waitToken(token) != RsTokenService::COMPLETE)
	{
		std::cerr << __PRETTY_FUNCTION__ << "Error! GXS operation failed."
		          << std::endl;
		return false;
	}

    if(rsEvents)
	{
		auto ev = std::make_shared<RsGxsIdentityEvent>();
		ev->mIdentityId = grouId;
		ev->mIdentityEventCode = RsGxsIdentityEventCode::DELETED_IDENTITY;
		rsEvents->postEvent(ev);
	}

	return true;
}

bool p3IdService::deleteIdentity(uint32_t& token, RsGxsIdGroup &group)
{
#ifdef DEBUG_IDS
    std::cerr << "p3IdService::deleteIdentity()";
    std::cerr << std::endl;
#endif

	deleteGroup(token, group.mMeta.mGroupId);

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
    if(!isOwnId(id))
    {
        std::cerr << "(EE) cannot retrieve own key to create tag request. KeyId=" << id << std::endl;
        return false ;
    }

    RsTlvPrivateRSAKey key;
    std::string nickname;
    RsGxsIdCache data ;

    {
        RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

        if(!mKeyCache.fetch(id, data))
            return false ;

        nickname = data.details.mNickname ;
        key = data.priv_key ;
    }

    return RsRecogn::createTagRequest(key, id, nickname, tag_class, tag_type, comment,  tag);
}



/********************************************************************************/
/******************* RsGixs Interface     ***************************************/
/********************************************************************************/

bool p3IdService::haveKey(const RsGxsId &id)
{
    RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
    return mKeyCache.is_cached(id);
}

bool p3IdService::havePrivateKey(const RsGxsId &id)
{
    if(! isOwnId(id))
        return false ;

	RS_STACK_MUTEX(mIdMtx);
	return mKeyCache.is_cached(id);
}

static void mergeIds(std::map<RsGxsId,std::list<RsPeerId> >& idmap,const RsGxsId& id,const std::list<RsPeerId>& peers)
{
	/* merge the two lists, use std::set to avoid duplicates efficiently */

	std::set<RsPeerId> new_peers(std::begin(peers), std::end(peers));

	std::list<RsPeerId>& stored_peers(idmap[id]);
	std::copy( std::begin(stored_peers), std::end(stored_peers),
	           std::inserter(new_peers, std::begin(new_peers)) );
	stored_peers.clear();
	std::copy( std::begin(new_peers), std::end(new_peers),
	           std::inserter(stored_peers, std::begin(stored_peers)) );
}

bool p3IdService::requestIdentity(
        const RsGxsId& id, const std::vector<RsPeerId>& peers )
{
	std::list<RsPeerId> askPeersList(peers.begin(), peers.end());

	// Empty list passed? Ask to all online peers.
	if(askPeersList.empty()) rsPeers->getOnlineList(askPeersList);

	if(askPeersList.empty()) // Still empty? Fail!
	{
		RsErr() << __PRETTY_FUNCTION__ << " failure retrieving peers list"
		        << std::endl;
		return false;
	}

	RsIdentityUsage usageInfo( RsServiceType::GXSID,
	                           RsIdentityUsage::IDENTITY_DATA_UPDATE );

	return requestKey(id, askPeersList, usageInfo);
}

bool p3IdService::requestKey(const RsGxsId &id, const std::list<RsPeerId>& peers,const RsIdentityUsage& use_info)
{
	Dbg3() << __PRETTY_FUNCTION__ << " id: " <<  id << std::endl;

	if(id.isNull())
	{
		RsErr() << __PRETTY_FUNCTION__ << " cannot request null id"
		        << std::endl;
		return false;
	}

	if(peers.empty())
	{
		RsErr() << __PRETTY_FUNCTION__ << " cannot request id: " << id
		        << " to empty lists of peers" << std::endl;
		return false;
	}

	if(isKnownId(id)) return true;

	/* Normally we should call getIdDetails(), but since the key is not known,
	 * we need to dig a possibly old information from the reputation system,
	 * which keeps its own list of banned keys.
	 * Of course, the owner ID is not known at this point.c*/

	RsReputationInfo info;
	rsReputations->getReputationInfo(id, RsPgpId(), info);

	if( info.mOverallReputationLevel == RsReputationLevel::LOCALLY_NEGATIVE )
	{
		RsInfo() << __PRETTY_FUNCTION__ << " not requesting Key " << id
		         << " because it has been banned." << std::endl;

		RS_STACK_MUTEX(mIdMtx);
		mIdsNotPresent.erase(id);

		return false;
	}

	{
		RS_STACK_MUTEX(mIdMtx);
		mergeIds(mIdsNotPresent, id, peers);
		mKeysTS[id].usage_map[use_info] = time(nullptr);
	}

    return cache_request_load(id, peers);
}

bool p3IdService::isPendingNetworkRequest(const RsGxsId& gxsId)
{
    RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
    // if ids has beens confirmed as not physically present return
    // immediately, id will be removed from list if found by auto nxs net search
    if(mIdsNotPresent.find(gxsId) != mIdsNotPresent.end())
        return true;

    return false;
}

bool p3IdService::getKey(const RsGxsId &id, RsTlvPublicRSAKey &key)
{
    {
        RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
        RsGxsIdCache data;

        if (mKeyCache.fetch(id, data))
        {
            key = data.pub_key;
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

bool p3IdService::getPrivateKey(const RsGxsId &id, RsTlvPrivateRSAKey &key)
{
    {
        RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
        RsGxsIdCache data;

        if (mKeyCache.fetch(id, data))
        {
            key = data.priv_key;
            return true;
        }
    }

    key.keyId.clear() ;
    cache_request_load(id);

    return false ;
}


bool p3IdService::signData(const uint8_t *data,uint32_t data_size,const RsGxsId& own_gxs_id,RsTlvKeySignature& signature,uint32_t& error_status)
{
    RsTlvPrivateRSAKey signature_key ;

    int i ;
    for(i=0;i<6;++i)
        if(!getPrivateKey(own_gxs_id,signature_key) || signature_key.keyData.bin_data == NULL)
        {
#ifdef DEBUG_IDS
            std::cerr << "  Cannot get key. Waiting for caching. try " << i << "/6" << std::endl;
#endif
            rstime::rs_usleep(500 * 1000) ;	// sleep for 500 msec.
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
    timeStampKey(own_gxs_id,RsIdentityUsage(serviceType(),RsIdentityUsage::IDENTITY_GENERIC_SIGNATURE_CREATION)) ;

    return true ;
}
bool p3IdService::validateData(const uint8_t *data,uint32_t data_size,const RsTlvKeySignature& signature,bool force_load,const RsIdentityUsage& info,uint32_t& signing_error)
{
    // RsIdentityDetails details ;
    // getIdDetails(signature.keyId,details);
    RsTlvPublicRSAKey signature_key ;

    for(int i=0;i< (force_load?6:1);++i)
        if(!getKey(signature.keyId,signature_key) || signature_key.keyData.bin_data == NULL)
        {
#ifdef DEBUG_IDS
            std::cerr << "  Cannot get key. Waiting for caching. try " << i << "/6" << std::endl;
#endif
            if(force_load) rstime::rs_usleep(500 * 1000) ;	// sleep for 500 msec.
        }
        else
            break ;

    if(signature_key.keyData.bin_data == NULL)
    {
#ifdef DEBUG_IDS
        std::cerr << "(EE) Cannot validate signature for unknown key " << signature.keyId << std::endl;
#endif
        signing_error = RS_GIXS_ERROR_KEY_NOT_AVAILABLE ;
        return false;
    }

    if(!GxsSecurity::validateSignature((char*)data,data_size,signature_key,signature))
    {
        std::cerr << "(SS) Signature was verified and it doesn't check! This is a security issue!" << std::endl;
        signing_error = RS_GIXS_ERROR_SIGNATURE_MISMATCH ;
        return false;
    }
    signing_error = RS_GIXS_ERROR_NO_ERROR ;

    timeStampKey(signature.keyId,info);
    return true ;
}

bool p3IdService::encryptData( const uint8_t *decrypted_data,
                               uint32_t decrypted_data_size,
                               uint8_t *& encrypted_data,
                               uint32_t& encrypted_data_size,
                               const RsGxsId& encryption_key_id,
                               uint32_t& error_status,
                               bool force_load )
{
    RsTlvPublicRSAKey encryption_key ;

    // get the key, and let the cache find it.
	for(int i=0; i<(force_load?6:1);++i)
		if(getKey(encryption_key_id,encryption_key))
            break ;
        else
            rstime::rs_usleep(500*1000) ; // sleep half a sec.

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
    timeStampKey(encryption_key_id,RsIdentityUsage(serviceType(),RsIdentityUsage::IDENTITY_GENERIC_ENCRYPTION)) ;

    return true ;
}

bool p3IdService::encryptData( const uint8_t* decrypted_data,
                               uint32_t decrypted_data_size,
                               uint8_t*& encrypted_data,
                               uint32_t& encrypted_data_size,
                               const std::set<RsGxsId>& encrypt_ids,
                               uint32_t& error_status, bool force_load )
{
	std::set<const RsGxsId*> keyNotYetFoundIds;

	for( std::set<RsGxsId>::const_iterator it = encrypt_ids.begin();
	     it != encrypt_ids.end(); ++it )
	{
		const RsGxsId& gId(*it);
		if(gId.isNull())
		{
			std::cerr << "p3IdService::encryptData(...) (EE) got null GXS id"
			          << std::endl;
			return false;
		}
		else keyNotYetFoundIds.insert(&gId);
	}

	if(keyNotYetFoundIds.empty())
	{
		std::cerr << "p3IdService::encryptData(...) (EE) got empty GXS ids set"
		          << std::endl;
		print_stacktrace();
		return false;
	}

	std::vector<RsTlvPublicRSAKey> encryption_keys;
	int maxRounds = force_load ? 6 : 1;
	for( int i=0; i < maxRounds; ++i )
	{
		for( std::set<const RsGxsId*>::iterator it = keyNotYetFoundIds.begin();
		     it !=keyNotYetFoundIds.end(); )
		{
			RsTlvPublicRSAKey encryption_key;
			if(getKey(**it, encryption_key) && !encryption_key.keyId.isNull())
			{
				encryption_keys.push_back(encryption_key);
				it = keyNotYetFoundIds.erase(it);
			}
			else
			{
				++it;
			}
		}

		if(keyNotYetFoundIds.empty()) break;
		else rstime::rs_usleep(500*1000);
	}

	if(!keyNotYetFoundIds.empty())
	{
		std::cerr << "p3IdService::encryptData(...) (EE) Cannot get "
		          << "encryption key for: ";
		for( std::set<const RsGxsId*>::iterator it = keyNotYetFoundIds.begin();
		     it !=keyNotYetFoundIds.end(); ++it )
			std::cerr << **it << " ";
		std::cerr << std::endl;
		print_stacktrace();

		error_status = RS_GIXS_ERROR_KEY_NOT_AVAILABLE;
		return false;
	}

	if(!GxsSecurity::encrypt( encrypted_data, encrypted_data_size,
	                          decrypted_data, decrypted_data_size,
	                          encryption_keys ))
	{
		std::cerr << "p3IdService::encryptData(...) (EE) Encryption failed."
		          << std::endl;
		print_stacktrace();

		error_status = RS_GIXS_ERROR_UNKNOWN;
		return false ;
	}

	for( std::set<RsGxsId>::const_iterator it = encrypt_ids.begin();
	     it != encrypt_ids.end(); ++it )
	{
		timeStampKey( *it,
		              RsIdentityUsage(
		                  serviceType(),
		                  RsIdentityUsage::IDENTITY_GENERIC_ENCRYPTION ) );
	}

	error_status = RS_GIXS_ERROR_NO_ERROR;
	return true;
}

bool p3IdService::decryptData( const uint8_t *encrypted_data,
                               uint32_t encrypted_data_size,
                               uint8_t *& decrypted_data,
                               uint32_t& decrypted_size,
                               const RsGxsId& key_id, uint32_t& error_status,
                               bool force_load )
{
    RsTlvPrivateRSAKey encryption_key ;

    // Get the key, and let the cache find it. It's our own key, so we should be able to find it, even if it takes
    // some seconds.

	int maxRounds = force_load ? 6 : 1;
	for(int i=0; i<maxRounds ;++i)
		if(getPrivateKey(key_id,encryption_key)) break;
		else rstime::rs_usleep(500*1000) ; // sleep half a sec.

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
	error_status = RS_GIXS_ERROR_NO_ERROR;
	timeStampKey( key_id,
	              RsIdentityUsage(
	                  serviceType(),
	                  RsIdentityUsage::IDENTITY_GENERIC_DECRYPTION) );

    return true ;
}

bool p3IdService::decryptData( const uint8_t* encrypted_data,
                               uint32_t encrypted_data_size,
                               uint8_t*& decrypted_data,
                               uint32_t& decrypted_data_size,
                               const std::set<RsGxsId>& decrypt_ids,
                               uint32_t& error_status,
                               bool force_load )
{
	std::set<const RsGxsId*> keyNotYetFoundIds;

	for( std::set<RsGxsId>::const_iterator it = decrypt_ids.begin();
	     it != decrypt_ids.end(); ++it )
	{
		const RsGxsId& gId(*it);
		if(gId.isNull())
		{
			std::cerr << "p3IdService::decryptData(...) (EE) got null GXS id"
			          << std::endl;
			print_stacktrace();
			return false;
		}
		else keyNotYetFoundIds.insert(&gId);
	}

	if(keyNotYetFoundIds.empty())
	{
		std::cerr << "p3IdService::decryptData(...) (EE) got empty GXS ids set"
		          << std::endl;
		print_stacktrace();
		return false;
	}

	std::vector<RsTlvPrivateRSAKey> decryption_keys;
	int maxRounds = force_load ? 6 : 1;
	for( int i=0; i < maxRounds; ++i )
	{
		for( std::set<const RsGxsId*>::iterator it = keyNotYetFoundIds.begin();
		     it !=keyNotYetFoundIds.end(); )
		{
			RsTlvPrivateRSAKey decryption_key;
			if( getPrivateKey(**it, decryption_key)
			        && !decryption_key.keyId.isNull() )
			{
				decryption_keys.push_back(decryption_key);
				it = keyNotYetFoundIds.erase(it);
			}
			else
			{
				++it;
			}
		}

		if(keyNotYetFoundIds.empty()) break;
		else rstime::rs_usleep(500*1000);
	}

	if(!keyNotYetFoundIds.empty())
	{
		std::cerr << "p3IdService::decryptData(...) (EE) Cannot get private key"
		          << " for: ";
		for( std::set<const RsGxsId*>::iterator it = keyNotYetFoundIds.begin();
		     it !=keyNotYetFoundIds.end(); ++it )
			std::cerr << **it << " ";
		std::cerr << std::endl;
		print_stacktrace();

		error_status = RS_GIXS_ERROR_KEY_NOT_AVAILABLE;
		return false;
	}

	if(!GxsSecurity::decrypt( decrypted_data, decrypted_data_size,
	                          encrypted_data, encrypted_data_size,
	                          decryption_keys ))
	{
		std::cerr << "p3IdService::decryptData(...) (EE) Decryption failed."
		          << std::endl;
		print_stacktrace();

		error_status = RS_GIXS_ERROR_UNKNOWN;
		return false ;
	}

	for( std::set<RsGxsId>::const_iterator it = decrypt_ids.begin();
	     it != decrypt_ids.end(); ++it )
	{
		timeStampKey( *it,
		              RsIdentityUsage(
		                  serviceType(),
		                  RsIdentityUsage::IDENTITY_GENERIC_DECRYPTION ) );
	}

	error_status = RS_GIXS_ERROR_NO_ERROR;
	return true;
}

#ifdef TO_BE_REMOVED
/********************************************************************************/
/******************* RsGixsReputation     ***************************************/
/********************************************************************************/

bool p3IdService::haveReputation(const RsGxsId &id)
{
    return haveKey(id);
}

bool p3IdService::loadReputation(const RsGxsId &id, const std::list<RsPeerId>& peers)
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

    if (mKeyCache.fetch(id, data))
    {
        rep.id = id;
        rep.score = 0;//data.details.mReputation.mOverallScore;
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
#endif

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

        updatePublicRequestStatus(req.mToken, RsTokenService::FAILED);
        return false;
    }

    if (groups.size() != 1)
    {
        std::cerr << "p3IdService::opinion_handlerequest() ERROR group.size() != 1";
        std::cerr << std::endl;

        // error.
        updatePublicRequestStatus(req.mToken, RsTokenService::FAILED);
        return false;
    }
    RsGroupMetaData &meta = *(groups.begin());

    if (meta.mGroupId != RsGxsGroupId(req.mId))
    {
        std::cerr << "p3IdService::opinion_handlerequest() ERROR Id mismatch";
        std::cerr << std::endl;

        // error.
        updatePublicRequestStatus(req.mToken, RsTokenService::FAILED);
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
    bool pgpId = (meta.mGroupFlags & RSGXSID_GROUPFLAG_REALID_kept_for_compatibility);
    ssdata.score.rep.updateIdScore(pgpId, ssdata.pgp.validatedSignature);
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

    updatePublicRequestStatus(req.mToken, RsTokenService::COMPLETE);
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
                    group.mPgpKnown = ssdata.pgp.validatedSignature;
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

					if(!group.mMeta.mServiceString.empty())
						std::cerr << "p3IdService::getGroupData() " << group.mMeta.mGroupId << " (" << group.mMeta.mGroupName << ") : Failed to decode, or no ServiceString \"" << group.mMeta.mServiceString << "\"" << std::endl;
                }

                group.mIsAContact =  (mContacts.find(RsGxsId(group.mMeta.mGroupId)) != mContacts.end());

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

bool p3IdService::getGroupSerializedData(const uint32_t &token, std::map<RsGxsId,std::string>& serialized_groups)
{
    unsigned char *mem = NULL;
    uint32_t size;
    RsGxsGroupId id ;

    serialized_groups.clear() ;

	if(!RsGenExchange::getSerializedGroupData(token,id, mem,size))
    {
        std::cerr << "(EE) call to RsGenExchage::getSerializedGroupData() failed." << std::endl;
        return false;
    }

    std::string radix ;

    Radix64::encode(mem,size,radix) ;

    serialized_groups[RsGxsId(id)] = radix ;

    return true;
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

bool p3IdService::updateGroup(uint32_t& token, RsGxsIdGroup &group)
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
        if (mKeyCache.erase(id))
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
    }

    return true;
}

bool p3IdService::deleteGroup(uint32_t& token, RsGxsGroupId& groupId)
{
	RsGxsId id(groupId);

#ifdef DEBUG_IDS
    std::cerr << "p3IdService::deleteGroup() Deleting RsGxsId: " << id;
    std::cerr << std::endl;
#endif

	RsGenExchange::deleteGroup(token, groupId);

    // if its in the cache - clear it.
    {
        RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
        if (mKeyCache.erase(id))
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
        validatedSignature = true;
        std::string str_line = pgpline;
        pgpId = RsPgpId(str_line);
        return true;
    }
    else if (3 == sscanf(input.c_str(), "K:0 T:%d C:%d I:%[^)]", &timestamp, &attempts,pgpline))
    {
        lastCheckTs = timestamp;
        checkAttempts = attempts;
        validatedSignature = false;
        std::string str_line = pgpline;
        pgpId = RsPgpId(str_line);
        return true;
    }
    else if (2 == sscanf(input.c_str(), "K:0 T:%d C:%d", &timestamp, &attempts))
    {
        lastCheckTs = timestamp;
        checkAttempts = attempts;
        validatedSignature = false;
        return true;
    }
    else if (1 == sscanf(input.c_str(), "K:0 T:%d", &timestamp))
    {
        lastCheckTs = timestamp;
        checkAttempts = 0;
        validatedSignature = false;
        return true;
    }
    else
    {
        lastCheckTs = 0;
        checkAttempts = 0;
        validatedSignature = false;
        return false;
    }
}

std::string SSGxsIdPgp::save() const
{
    std::string output;
    if (validatedSignature)
    {
        output += "K:1 I:";
        output += pgpId.toStdString();
    }
    else
    {
        rs_sprintf(output, "K:0 T:%d C:%d", lastCheckTs, checkAttempts);

        if(!pgpId.isNull())
            output += " I:"+pgpId.toStdString();
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

RsGxsIdCache::RsGxsIdCache() {}

RsGxsIdCache::RsGxsIdCache(const RsGxsIdGroupItem *item, const RsTlvPublicRSAKey& in_pkey, const std::list<RsRecognTag> &tagList)
{
    init(item,in_pkey,RsTlvPrivateRSAKey(),tagList) ;
}

RsGxsIdCache::RsGxsIdCache(const RsGxsIdGroupItem *item, const RsTlvPublicRSAKey& in_pkey, const RsTlvPrivateRSAKey& privkey, const std::list<RsRecognTag> &tagList)
{
    init(item,in_pkey,privkey,tagList) ;
}

void RsGxsIdCache::init(const RsGxsIdGroupItem *item, const RsTlvPublicRSAKey& in_pub_key, const RsTlvPrivateRSAKey& in_priv_key,const std::list<RsRecognTag> &tagList)
{
    // Save Keys.
    pub_key = in_pub_key;
    priv_key = in_priv_key;

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

    details.mFlags = 0 ;

    if(item->meta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN)		details.mFlags |= RS_IDENTITY_FLAGS_IS_OWN_ID;
    if(item->meta.mGroupFlags     & RSGXSID_GROUPFLAG_REALID_kept_for_compatibility)			details.mFlags |= RS_IDENTITY_FLAGS_PGP_LINKED;

    // do some tests
    if(details.mFlags & RS_IDENTITY_FLAGS_IS_OWN_ID)
    {
        if(!priv_key.checkKey())
            std::cerr << "(EE) Private key missing for own identity " << pub_key.keyId << std::endl;

    }
    if(!pub_key.checkKey())
        std::cerr << "(EE) Public key missing for identity " << pub_key.keyId << std::endl;

    if(!GxsSecurity::checkFingerprint(pub_key))
        details.mFlags |= RS_IDENTITY_FLAGS_IS_DEPRECATED;

    /* rest must be retrived from ServiceString */
    updateServiceString(item->meta.mServiceString);
}

void RsGxsIdCache::updateServiceString(std::string serviceString)
{
    details.mRecognTags.clear();

    SSGxsIdGroup ssdata;
    if (ssdata.load(serviceString))
    {
        if (details.mFlags & RS_IDENTITY_FLAGS_PGP_LINKED)
        {
            if(ssdata.pgp.validatedSignature) details.mFlags |= RS_IDENTITY_FLAGS_PGP_KNOWN ;

            if (details.mFlags & RS_IDENTITY_FLAGS_PGP_LINKED)
                details.mPgpId = ssdata.pgp.pgpId;
            else
                details.mPgpId.clear();
        }


        // process RecognTags.
        if (ssdata.recogntags.tagsProcessed())
        {
#ifdef DEBUG_RECOGN
            std::cerr << "RsGxsIdCache::updateServiceString() updating recogntags";
            std::cerr << std::endl;
#endif
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
#endif
                        details.mRecognTags.push_back(*it);
                    }
                    else
                    {
#ifdef DEBUG_RECOGN
                        std::cerr << "RsGxsIdCache::updateServiceString() Invalid Tag: " << it->tag_class << ":" << it->tag_type;
                        std::cerr << std::endl;
#endif
                    }
                }
            }
            else
            {
#ifdef DEBUG_RECOGN
                std::cerr << "RsGxsIdCache::updateServiceString() recogntags old publishTs";
                std::cerr << std::endl;
#endif
            }

        }
        else
        {
#ifdef DEBUG_RECOGN
            std::cerr << "RsGxsIdCache::updateServiceString() recogntags unprocessed";
            std::cerr << std::endl;
#endif
        }

        // copy over Reputation scores.
        //details.mReputation = ssdata.score.rep;
    }
    else
    {
        details.mFlags &= ~RS_IDENTITY_FLAGS_PGP_KNOWN ;
        details.mPgpId.clear();
        //details.mReputation.updateIdScore(false, false);
        //details.mReputation.update();
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

    //rstime_t now = time(NULL);
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

// Loads in the cache the group data from the given group item, retrieved from sqlite storage.

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

    RsTlvPublicRSAKey   pubkey;
    RsTlvPrivateRSAKey  fullkey;

    bool pub_key_ok = false;
    bool full_key_ok = false;

    RsGxsId id (item->meta.mGroupId.toStdString());

    if (!getGroupKeys(RsGxsGroupId(id.toStdString()), keySet))
    {
        std::cerr << "p3IdService::cache_store() ERROR getting GroupKeys for: "<< item->meta.mGroupId << std::endl;
        return false;
    }

    for (std::map<RsGxsId, RsTlvPrivateRSAKey>::iterator kit = keySet.private_keys.begin(); kit != keySet.private_keys.end(); ++kit)
        if (kit->second.keyFlags & RSTLV_KEY_DISTRIB_ADMIN)
        {
#ifdef DEBUG_IDS
            std::cerr << "p3IdService::cache_store() Found Admin Key" << std::endl;
#endif 
            fullkey = kit->second;
            full_key_ok = true;
        }
    for (std::map<RsGxsId, RsTlvPublicRSAKey>::iterator kit = keySet.public_keys.begin(); kit != keySet.public_keys.end(); ++kit)
        if (kit->second.keyFlags & RSTLV_KEY_DISTRIB_ADMIN)
        {
#ifdef DEBUG_IDS
            std::cerr << "p3IdService::cache_store() Found Admin public Key" << std::endl;
#endif
            pubkey = kit->second;
            pub_key_ok = true ;
        }

    assert(!( pubkey.keyFlags & RSTLV_KEY_TYPE_FULL)) ;
    assert(!full_key_ok || (fullkey.keyFlags & RSTLV_KEY_TYPE_FULL)) ;

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

    // Create Cache Data.
    RsGxsIdCache keycache(item, pubkey, fullkey,tagList);

    if(mContacts.find(id) != mContacts.end())
        keycache.details.mFlags |= RS_IDENTITY_FLAGS_IS_A_CONTACT;

    mKeyCache.store(id, keycache);
    mKeyCache.resize();

    return true;
}



/***** BELOW LOADS THE CACHE FROM GXS DATASTORE *****/

#define MIN_CYCLE_GAP	2

bool p3IdService::cache_request_load(const RsGxsId &id, const std::list<RsPeerId> &peers)
{
	Dbg4() << __PRETTY_FUNCTION__ << " id: " << id << std::endl;

	{
		RS_STACK_MUTEX(mIdMtx);
		// merge, even if peers is empty
		mergeIds(mCacheLoad_ToCache, id, peers);
	}

	if(RsTickEvent::event_count(GXSID_EVENT_CACHELOAD) > 0)
	{
		Dbg3() << __PRETTY_FUNCTION__ << " cache reload already scheduled "
		       << "skipping" << std::endl;
		return true;
	}

	int32_t age = 0;
	if( RsTickEvent::prev_event_ago(GXSID_EVENT_CACHELOAD, age) && age < MIN_CYCLE_GAP )
	{
		RsTickEvent::schedule_in(GXSID_EVENT_CACHELOAD, MIN_CYCLE_GAP - age);
		return true;
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
            groupIds.push_back(RsGxsGroupId(it->first)); // might need conversion?
        }

        for(std::map<RsGxsId,std::list<RsPeerId> >::const_iterator it(mCacheLoad_ToCache.begin());it!=mCacheLoad_ToCache.end();++it)
            mergeIds(mPendingCache,it->first,it->second) ;

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

            // No need to merge empty peers since the request would fail.

            for(std::map<RsGxsId,std::list<RsPeerId> >::const_iterator itt(mPendingCache.begin());itt!=mPendingCache.end();++itt)
                if(!itt->second.empty())
                    mergeIds(mIdsNotPresent,itt->first,itt->second) ;
#ifdef DEBUG_IDS
				else
                    std::cerr << "(WW) empty list of peers to request ID " << itt->first << ": cannot request" << std::endl;
#endif


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
	RS_STACK_MUTEX(mIdMtx);

	if(!mNes)
	{
		RsErr() << __PRETTY_FUNCTION__ << " Cannot request missing GXS IDs "
		        << "because network service is not present." << std::endl;
		return;
	}

    std::map<RsGxsId, std::list<RsPeerId> >::iterator cit;
    std::map<RsPeerId, std::list<RsGxsId> > requests;

	/* Transform to appropriate structure (<RsPeerId, std::list<RsGxsId> > map)
	 * to make request to nes per peer ID
	 * Only delete entries in mIdsNotPresent that can actually be performed, or
	 * that have empty peer list */

	for(cit = mIdsNotPresent.begin(); cit != mIdsNotPresent.end();)
	{
		Dbg2() << __PRETTY_FUNCTION__ << " Processing missing key RsGxsId: "
		       << cit->first << std::endl;

		const RsGxsId& gxsId = cit->first;
		const std::list<RsPeerId>& peers = cit->second;
		std::list<RsPeerId>::const_iterator cit2;

        bool request_can_proceed = false ;

		for(cit2 = peers.begin(); cit2 != peers.end(); ++cit2)
		{
			const RsPeerId& peer = *cit2;

			if(rsPeers->isOnline(peer) || mNes->isDistantPeer(peer))
			{
				/* make sure that the peer in online, so that we know that the
				 * request has some chance to succeed.*/
				requests[peer].push_back(cit->first);
				request_can_proceed = true ;

				Dbg2() << __PRETTY_FUNCTION__ << " Moving missing key RsGxsId:"
				       << gxsId << " to peer: " << peer << " requests queue"
				       << std::endl;
			}
		}

		const bool noPeersFound = peers.empty();
		if(noPeersFound)
			RsWarn() << __PRETTY_FUNCTION__ << " No peers supplied to request "
			         << "RsGxsId: " << gxsId << " dropping." << std::endl;

		if(request_can_proceed || noPeersFound)
		{
			std::map<RsGxsId, std::list<RsPeerId> >::iterator tmp(cit);
			++tmp;
			mIdsNotPresent.erase(cit);
			cit = tmp;
		}
		else
		{
			RsInfo() << __PRETTY_FUNCTION__ << " no online peers among supplied"
			         << " list in request for RsGxsId: " << gxsId
			         << ". Keeping it until peers show up."<< std::endl;
			++cit;
		}
	}

	for( std::map<RsPeerId, std::list<RsGxsId> >::const_iterator cit2(
	         requests.begin() ); cit2 != requests.end(); ++cit2 )
	{
		const RsPeerId& peer = cit2->first;
		std::list<RsGxsGroupId> grpIds;
		for( std::list<RsGxsId>::const_iterator gxs_id_it = cit2->second.begin();
		     gxs_id_it != cit2->second.end(); ++gxs_id_it )
		{
			Dbg2() << __PRETTY_FUNCTION__ << " passing RsGxsId: " << *gxs_id_it
			       << " request for peer: " << peer
			       << " to RsNetworkExchangeService " << std::endl;
			grpIds.push_back(RsGxsGroupId(*gxs_id_it));
		}

		mNes->requestGrp(grpIds, peer);
	}
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

	RsGxsIdCache updated_data;
    
	if(mKeyCache.fetch(id, updated_data))
	{
#ifdef DEBUG_IDS
		std::cerr << "p3IdService::cache_update_if_cached() Updating Public Cache";
		std::cerr << std::endl;
#endif // DEBUG_IDS

		updated_data.updateServiceString(serviceString);
        
		mKeyCache.store(id, updated_data);
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
	return true;
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

					SSGxsIdGroup ssdata;

                    std::cerr << "Adding own ID " << item->meta.mGroupId << " mGroupFlags=" << std::hex << item->meta.mGroupFlags << std::dec;

					if (ssdata.load(item->meta.mServiceString) && ssdata.pgp.validatedSignature) // (cyril) note: we cannot use if(item->meta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
                    {																			 // or we need to cmbine it with the deprecated value that overlaps with GXS_SERV::FLAG_PRIVACY_PRIVATE
                    	std::cerr << " signed = YES" << std::endl;								 // see comments line 799 in ::createIdentity();
						mOwnSignedIds.push_back(RsGxsId(item->meta.mGroupId));
                    }
                    else
                    	std::cerr << " signed = NO" << std::endl;

                    // This prevents automatic deletion to get rid of them.
                    // In other words, own ids are always used.

                    mKeysTS[RsGxsId(item->meta.mGroupId)].TS = time(NULL) ;
				}
				delete item ;
            }
            mOwnIdsLoaded = true ;

            std::cerr << mOwnIds.size() << " own Ids loaded, " << mOwnSignedIds.size() << " of which are signed" << std::endl;
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
                    std::list<RsPeerId> nullpeers;
					requestKey(*vit, nullpeers,RsIdentityUsage(serviceType(),RsIdentityUsage::UNKNOWN_USAGE));

#ifdef DEBUG_IDS
					std::cerr << "p3IdService::cachetest_request() Requested Key Id: " << *vit;
					std::cerr << std::endl;
#endif // DEBUG_IDS
				}
				else
				{
					RsTlvPublicRSAKey seckey;
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
					RsTlvPrivateRSAKey seckey;
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
RsGenExchange::ServiceCreate_Return p3IdService::service_CreateGroup(
        RsGxsGrpItem* grpItem, RsTlvSecurityKeySet& keySet )
{
	Dbg2() << __PRETTY_FUNCTION__ << std::endl;

    RsGxsIdGroupItem *item = dynamic_cast<RsGxsIdGroupItem *>(grpItem);
    if (!item)
    {
        std::cerr << "p3IdService::service_CreateGroup() ERROR invalid cast";
        std::cerr << std::endl;
        return SERVICE_CREATE_FAIL;
    }

    item->meta.mGroupId.clear();

    /********************* TEMP HACK UNTIL GXS FILLS IN GROUP_ID *****************/
	// find private admin key
	for( std::map<RsGxsId, RsTlvPrivateRSAKey>::iterator mit =
	     keySet.private_keys.begin(); mit != keySet.private_keys.end(); ++mit )
		if(mit->second.keyFlags == (RSTLV_KEY_DISTRIB_ADMIN | RSTLV_KEY_TYPE_FULL))
        {
            item->meta.mGroupId = RsGxsGroupId(mit->second.keyId);
            break;
        }

	if(item->meta.mGroupId.isNull())
	{
		RsErr() << __PRETTY_FUNCTION__ << " missing admin key!" << std::endl;
		return SERVICE_CREATE_FAIL;
	}
    mKeysTS[RsGxsId(item->meta.mGroupId)].TS = time(NULL) ;

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
    std::cerr << "p3IdService::service_CreateGroup() for : " << item->meta.mGroupId;
    std::cerr << std::endl;
    std::cerr << "p3IdService::service_CreateGroup() Alt GroupId : " << item->meta.mGroupId;
    std::cerr << std::endl;
#endif // DEBUG_IDS

    ServiceCreate_Return createStatus;

    if (item->meta.mGroupFlags & RSGXSID_GROUPFLAG_REALID_kept_for_compatibility)
    {
        /* create the hash */
        Sha1CheckSum hash;

		RsPgpFingerprint ownFinger;
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

		if(!mPgpUtils->getKeyFingerprint(ownId,ownFinger))
		{
			RsErr() << __PRETTY_FUNCTION__
			        << " failure retriving own PGP fingerprint" << std::endl;
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

        std::cerr << "p3IdService::service_CreateGroup() Calculated PgpIdHash : " << item->mPgpIdHash;
        std::cerr << std::endl;
#endif // DEBUG_IDS

        /* do signature */


#define MAX_SIGN_SIZE 2048
        uint8_t signarray[MAX_SIGN_SIZE];
		unsigned int sign_size = MAX_SIGN_SIZE;
        memset(signarray,0,MAX_SIGN_SIZE) ;	// just in case.

		/* -10 is never returned by askForDeferredSelfSignature therefore we can
		 * use it to properly detect and handle the case libretroshare is being
		 * used outside retroshare-gui */
		int result = -10;

		/* This method is DEPRECATED we call it only for retrocompatibility with
		 * retroshare-gui, when called from something different then
		 * retroshare-gui for example retroshare-service it miserably fail! */
		mPgpUtils->askForDeferredSelfSignature(
		            static_cast<const void*>(hash.toByteArray()),
		            hash.SIZE_IN_BYTES, signarray, &sign_size, result,
		            __PRETTY_FUNCTION__ );

		/* If askForDeferredSelfSignature left result untouched it means
		 * libretroshare is being used by something different then
		 * retroshare-gui so try calling AuthGPG::getAuthGPG()->SignDataBin
		 * directly */
		if( result == -10 )
			result = AuthGPG::getAuthGPG()->SignDataBin(
			            static_cast<const void*>(hash.toByteArray()),
			            hash.SIZE_IN_BYTES, signarray, &sign_size,
			            __PRETTY_FUNCTION__ )
			        ?
			            SELF_SIGNATURE_RESULT_SUCCESS :
			            SELF_SIGNATURE_RESULT_FAILED;

		switch(result)
		{
		case SELF_SIGNATURE_RESULT_PENDING:
			createStatus = SERVICE_CREATE_FAIL_TRY_LATER;
			Dbg1() << __PRETTY_FUNCTION__ << " signature still pending"
			       << std::endl;
			break;
		case SELF_SIGNATURE_RESULT_SUCCESS:
		{
			// Additional consistency checks.
			if(sign_size == MAX_SIGN_SIZE)
			{
				RsErr() << __PRETTY_FUNCTION__ << "Inconsistent result. "
				        << "Signature uses full buffer. This is probably an "
				        << "error." << std::endl;
				return SERVICE_CREATE_FAIL;
			}

			/* push binary into string -> really bad! */
			item->mPgpIdSign = "";
			for(unsigned int i = 0; i < sign_size; i++)
				item->mPgpIdSign += static_cast<char>(signarray[i]);

			createStatus = SERVICE_CREATE_SUCCESS;
			break;
		}
		case SELF_SIGNATURE_RESULT_FAILED: /* fall-through */
		default:
			RsErr() << __PRETTY_FUNCTION__ << " signature failed with: "
			        << result << std::endl;
			return SERVICE_CREATE_FAIL;
		}
	}
	else createStatus = SERVICE_CREATE_SUCCESS;

    // Enforce no AuthorId.
    item->meta.mAuthorId.clear() ;
    //item->mMeta.mAuthorId.clear() ;
    // copy meta data to be sure its all the same.
    //item->group.mMeta = item->meta;

    // do it like p3gxscircles: save the new grp id
    // this allows the user interface
    // to see the grp id on the list of ownIds immediately after the group was created
	{
		RS_STACK_MUTEX(mIdMtx);
        RsGxsId gxsId(item->meta.mGroupId);
        if (std::find(mOwnIds.begin(), mOwnIds.end(), gxsId) == mOwnIds.end())
        {
			mOwnIds.push_back(gxsId);
			mKeysTS[gxsId].TS = time(nullptr);
        }
    }

	Dbg2() << __PRETTY_FUNCTION__ << " returns: " << createStatus << std::endl;
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
			if (!(vit->mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID_kept_for_compatibility))
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
				if (ssdata.pgp.validatedSignature)
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
				rstime_t age = time(NULL) - ssdata.pgp.lastCheckTs;
				rstime_t wait_period = ssdata.pgp.checkAttempts * SECS_PER_DAY;
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
		ssdata.pgp.validatedSignature = true;
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
		ssdata.pgp.pgpId = pgpId;	// read from the signature, but not verified
	}

    if(!error)
    {
        // update IdScore too.
        ssdata.score.rep.updateIdScore(true, ssdata.pgp.validatedSignature);
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
    std::string esign ;
    Radix64::encode((unsigned char *) grp.mPgpIdSign.c_str(), grp.mPgpIdSign.length(),esign) ;
    std::cerr << "Checking group signature " << esign << std::endl;
#endif
    RsPgpId issuer_id ;

    if(mPgpUtils->parseSignature((unsigned char *) grp.mPgpIdSign.c_str(), grp.mPgpIdSign.length(),issuer_id))
    {
#ifdef DEBUG_IDS
	    std::cerr << "Issuer found: " << issuer_id << std::endl;
#endif
	    pgpId = issuer_id ;
    }
    else
    {
	    std::cerr << "Signature parsing failed!!" << std::endl;
	    pgpId.clear() ;
    }

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
	bool pgpId = (item->meta.mGroupFlags & RSGXSID_GROUPFLAG_REALID_kept_for_compatibility);
	ssdata.score.rep.updateIdScore(pgpId, ssdata.pgp.validatedSignature);
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
	
	rstime_t now = time(NULL);
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

	rstime_t age = 0;
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

	id.mMeta.mGroupFlags = RSGXSID_GROUPFLAG_REALID_kept_for_compatibility;

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
	id.mMeta.mGroupFlags = RSGXSID_GROUPFLAG_REALID_kept_for_compatibility;
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
	case GXSIDREQ_SERIALIZE_TO_MEMORY:
		handle_get_serialized_grp(token);
		break;
	default:
		std::cerr << "p3IdService::handleResponse() Unknown Request Type: "
		          << req_type << std::endl;
		break;
	}
}


	// Overloaded from RsTickEvent for Event callbacks.
void p3IdService::handle_event(uint32_t event_type, const std::string &/*elabel*/)
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
		RsErr() << __PRETTY_FUNCTION__ << " Unknown Event Type: "
		        << event_type << std::endl;
		print_stacktrace();
		break;
	}
}

/*static*/ const std::string RsIdentity::DEFAULT_IDENTITY_BASE_URL =
        "retroshare:///identities";
/*static*/ const std::string RsIdentity::IDENTITY_URL_NAME_FIELD = "identityName";
/*static*/ const std::string RsIdentity::IDENTITY_URL_ID_FIELD = "identityId";
/*static*/ const std::string RsIdentity::IDENTITY_URL_DATA_FIELD = "identityData";

bool p3IdService::exportIdentityLink(
        std::string& link, const RsGxsId& id, bool includeGxsData,
        const std::string& baseUrl, std::string& errMsg )
{
	constexpr auto fname = __PRETTY_FUNCTION__;
	const auto failure = [&](const std::string& err)
	{
		errMsg = err;
		RsErr() << fname << " " << err << std::endl;
		return false;
	};

	if(id.isNull()) return failure("id cannot be null");

	const bool outputRadix = baseUrl.empty();
	if(outputRadix && !includeGxsData) return
	        failure("includeGxsData must be true if format requested is base64");

	if( includeGxsData &&
	        !RsGenExchange::exportGroupBase64(
	            link, reinterpret_cast<const RsGxsGroupId&>(id), errMsg ) )
		return failure(errMsg);

	if(outputRadix) return true;

	 std::vector<RsGxsIdGroup> idsInfo;
	if( !getIdentitiesInfo(std::set<RsGxsId>({id}), idsInfo )
	        || idsInfo.empty() )
		return failure("failure retrieving identity information");

	RsUrl inviteUrl(baseUrl);
	inviteUrl.setQueryKV(IDENTITY_URL_ID_FIELD, id.toStdString());
	inviteUrl.setQueryKV(IDENTITY_URL_NAME_FIELD, idsInfo[0].mMeta.mGroupName);
	if(includeGxsData) inviteUrl.setQueryKV(IDENTITY_URL_DATA_FIELD, link);

	link = inviteUrl.toString();
	return true;
}

bool p3IdService::importIdentityLink(
        const std::string& link, RsGxsId& id, std::string& errMsg )
{
	constexpr auto fname = __PRETTY_FUNCTION__;
	const auto failure = [&](const std::string& err)
	{
		errMsg = err;
		RsErr() << fname << " " << err << std::endl;
		return false;
	};

	if(link.empty()) return failure("link is empty");

	const std::string* radixPtr(&link);

	RsUrl url(link);
	const auto& query = url.query();
	const auto qIt = query.find(IDENTITY_URL_DATA_FIELD);
	if(qIt != query.end()) radixPtr = &qIt->second;

	if(radixPtr->empty()) return failure(IDENTITY_URL_DATA_FIELD + " is empty");

	if(!RsGenExchange::importGroupBase64(
	            *radixPtr, reinterpret_cast<RsGxsGroupId&>(id), errMsg ))
		return failure(errMsg);

	return true;
}


void RsGxsIdGroup::serial_process(
        RsGenericSerializer::SerializeJob j,
        RsGenericSerializer::SerializeContext& ctx )
{
	RS_SERIAL_PROCESS(mMeta);
	RS_SERIAL_PROCESS(mPgpIdHash);
	RS_SERIAL_PROCESS(mPgpIdSign);
	RS_SERIAL_PROCESS(mImage);
	RS_SERIAL_PROCESS(mLastUsageTS);
	RS_SERIAL_PROCESS(mPgpKnown);
	RS_SERIAL_PROCESS(mIsAContact);
	RS_SERIAL_PROCESS(mPgpId);
	RS_SERIAL_PROCESS(mReputation);
}

RsIdentityUsage::RsIdentityUsage(
        RsServiceType service, RsIdentityUsage::UsageCode code,
        const RsGxsGroupId& gid, const RsGxsMessageId& mid,
        uint64_t additional_id, const std::string& comment ) :
    mServiceId(service), mUsageCode(code), mGrpId(gid), mMsgId(mid),
    mAdditionalId(additional_id), mComment(comment)
{
	/* This is a hack, since it will hash also mHash, but because it is
	 * initialized to 0, and only computed in the constructor here, it should
	 * be ok. */
	librs::crypto::HashStream hs(librs::crypto::HashStream::SHA1);

	hs << static_cast<uint32_t>(service); // G10h4ck: Why uint32 if it's 16 bits?
	hs << static_cast<uint8_t>(code);
	hs << gid;
	hs << mid;
	hs << static_cast<uint64_t>(additional_id);
	hs << comment;

	mHash = hs.hash();
}

RsIdentityUsage::RsIdentityUsage(
        uint16_t service, const RsIdentityUsage::UsageCode& code,
        const RsGxsGroupId& gid, const RsGxsMessageId& mid,
        uint64_t additional_id,const std::string& comment ) :
    mServiceId(static_cast<RsServiceType>(service)), mUsageCode(code),
    mGrpId(gid), mMsgId(mid), mAdditionalId(additional_id), mComment(comment)
{
#ifdef DEBUG_IDS
    std::cerr << "New identity usage: " << std::endl;
    std::cerr << "  service=" << std::hex << service << std::endl;
    std::cerr << "  code   =" << std::hex << code << std::endl;
    std::cerr << "  grpId  =" << std::hex << gid << std::endl;
    std::cerr << "  msgId  =" << std::hex << mid << std::endl;
    std::cerr << "  add id =" << std::hex << additional_id << std::endl;
    std::cerr << "  commnt =\"" << std::hex << comment << "\"" << std::endl;
#endif

	/* This is a hack, since it will hash also mHash, but because it is
	 * initialized to 0, and only computed in the constructor here, it should
	 * be ok. */
    librs::crypto::HashStream hs(librs::crypto::HashStream::SHA1) ;

	hs << (uint32_t)service ; // G10h4ck: Why uint32 if it's 16 bits?
    hs << (uint8_t)code ;
    hs << gid ;
    hs << mid ;
    hs << (uint64_t)additional_id ;
    hs << comment ;

	mHash = hs.hash();

#ifdef DEBUG_IDS
    std::cerr << "  hash   =\"" << std::hex << mHash << "\"" << std::endl;
#endif
}

RsIdentityUsage::RsIdentityUsage() :
    mServiceId(RsServiceType::NONE), mUsageCode(UNKNOWN_USAGE), mAdditionalId(0)
{}

RsIdentity::~RsIdentity() = default;
RsReputationInfo::~RsReputationInfo() = default;
RsGixs::~RsGixs() = default;
RsIdentityDetails::~RsIdentityDetails() = default;
GxsReputation::~GxsReputation() = default;
RsGxsIdGroup::~RsGxsIdGroup() = default;
