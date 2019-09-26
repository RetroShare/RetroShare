/*******************************************************************************
 * libretroshare/src/services: p3idservice.h                                   *
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
#pragma once

/// RetroShare GXS identities service


#include <map>
#include <string>

#include "retroshare/rsidentity.h"	// External Interfaces.
#include "gxs/rsgenexchange.h"		// GXS service.
#include "gxs/rsgixs.h"			// Internal Interfaces.
#include "util/rsdebug.h"
#include "gxs/gxstokenqueue.h"		
#include "rsitems/rsgxsiditems.h"
#include "util/rsmemcache.h"
#include "util/rstickevent.h"
#include "util/rsrecogn.h"
#include "pqi/authgpg.h"
#include "rsitems/rsgxsrecognitems.h"

class PgpAuxUtils;

class OpinionRequest
{
	public:
	OpinionRequest(uint32_t token, RsGxsId id, bool absOpinion, int32_t score)
	:mToken(token), mId(id), mAbsOpinion(absOpinion), mScore(score)
	{ return; }
	OpinionRequest()
	:mToken(0), mId(), mAbsOpinion(false), mScore(0)
	{ return; }

	uint32_t mToken;
	RsGxsId    mId;
	bool     mAbsOpinion;
	int32_t	 mScore;
};


// INTERNAL DATA TYPES. 
// Describes data stored in GroupServiceString.

class SSBit
{
	public:
virtual	bool load(const std::string &input) = 0;
virtual	std::string save() const = 0;
};



class SSGxsIdPgp: public SSBit 
{
	public:
	SSGxsIdPgp()
	:validatedSignature(false), lastCheckTs(0), checkAttempts(0) { return; }

virtual	bool load(const std::string &input);
virtual	std::string save() const;

	bool validatedSignature;
	rstime_t lastCheckTs;
	uint32_t checkAttempts;
	RsPgpId pgpId;
};

class SSGxsIdRecognTags: public SSBit 
{
	public:
	SSGxsIdRecognTags()
	:tagFlags(0), publishTs(0), lastCheckTs(0) { return; }

virtual	bool load(const std::string &input);
virtual	std::string save() const;

	void setTags(bool processed, bool pending, uint32_t flags);

	bool tagsProcessed() const;  // have we processed?
	bool tagsPending() const;    // should we reprocess?
	bool tagValid(int i) const;

	uint32_t tagFlags;
	rstime_t publishTs;
	rstime_t lastCheckTs;
};


class SSGxsIdReputation: public SSBit 
{
	public:
	SSGxsIdReputation()
	:rep() { return; }

virtual	bool load(const std::string &input);
virtual	std::string save() const;

	GxsReputation rep;
};

class SSGxsIdCumulator: public SSBit
{
public:
	SSGxsIdCumulator()
	:count(0), nullcount(0), sum(0), sumsq(0) { return; }

virtual	bool load(const std::string &input);
virtual	std::string save() const;

	uint32_t count;
	uint32_t nullcount;
	double   sum;
	double   sumsq;
	
	// derived parameters:
};

class SSGxsIdGroup: public SSBit
{
public:
	SSGxsIdGroup() { return; }

virtual	bool load(const std::string &input);
virtual	std::string save() const;

	// pgphash status
	SSGxsIdPgp pgp;

	// recogTags.
	SSGxsIdRecognTags recogntags;	

	// reputation score.	
	SSGxsIdReputation  score;

	// These are depreciated (will load, but not save)
	SSGxsIdCumulator opinion;
	SSGxsIdCumulator reputation;

};

#define ID_LOCAL_STATUS_FULL_CALC_FLAG	0x00010000
#define ID_LOCAL_STATUS_INC_CALC_FLAG	0x00020000

class RsGxsIdGroupItem;

class RsGxsIdCache
{
public:
    RsGxsIdCache();

    RsGxsIdCache(const RsGxsIdGroupItem *item, const RsTlvPublicRSAKey& in_pkey, const RsTlvPrivateRSAKey& privkey, const std::list<RsRecognTag> &tagList);
    RsGxsIdCache(const RsGxsIdGroupItem *item, const RsTlvPublicRSAKey& in_pkey, const std::list<RsRecognTag> &tagList);
    
    void updateServiceString(std::string serviceString);

    rstime_t mPublishTs;
    std::list<RsRecognTag> mRecognTags; // Only partially validated.

    RsIdentityDetails details;

    RsTlvPublicRSAKey pub_key;
    RsTlvPrivateRSAKey priv_key;
    
private:
    void init(const RsGxsIdGroupItem *item, const RsTlvPublicRSAKey& in_pub_key, const RsTlvPrivateRSAKey& in_priv_key,const std::list<RsRecognTag> &tagList);
};

struct SerialisedIdentityStruct
{
    unsigned char *mMem ;
    uint32_t mSize ;
    rstime_t mLastUsageTS;
};

// We cache all identities, and provide alternative (instantaneous)
// functions to extract info, rather than the horrible Token system.
class p3IdService: public RsGxsIdExchange, public RsIdentity,  public GxsTokenQueue, public RsTickEvent, public p3Config
{
public:
	p3IdService(RsGeneralDataService* gds, RsNetworkExchangeService* nes, PgpAuxUtils *pgpUtils);

	virtual RsServiceInfo getServiceInfo();
	static	uint32_t idAuthenPolicy();

	virtual void service_tick(); // needed for background processing.


	/*!
	 * Design hack, id service must be constructed first as it
	 * is need for construction of subsequent net services
	 */
	void setNes(RsNetworkExchangeService* nes);

	/* General Interface is provided by RsIdentity / RsGxsIfaceImpl. */

	/* Data Specific Interface */

	/// @see RsIdentity
	bool getIdentitiesInfo(const std::set<RsGxsId>& ids,
	        std::vector<RsGxsIdGroup>& idsInfo ) override;

	/// @see RsIdentity
	bool getIdentitiesSummaries(std::list<RsGroupMetaData>& ids) override;

	// These are exposed via RsIdentity.
	virtual bool getGroupData(const uint32_t &token, std::vector<RsGxsIdGroup> &groups);
	virtual bool getGroupSerializedData(const uint32_t &token, std::map<RsGxsId,std::string>& serialized_groups);

	//virtual bool getMsgData(const uint32_t &token, std::vector<RsGxsIdOpinion> &opinions);

	// These are local - and not exposed via RsIdentity.
	virtual bool createGroup(uint32_t& token, RsGxsIdGroup &group);
	virtual bool updateGroup(uint32_t& token, RsGxsIdGroup &group);
	virtual bool deleteGroup(uint32_t& token, RsGxsGroupId& group);
	//virtual bool createMsg(uint32_t& token, RsGxsIdOpinion &opinion);

	/**************** RsIdentity External Interface.
	 * Notes:
	 * 
	 * All the data is cached together for the moment - We should probably
	 * seperate and sort this out.
	 * 
	 * Also need to handle Cache updates / invalidation from internal changes.
	 * 
	 */
	//virtual bool  getNickname(const RsGxsId &id, std::string &nickname);
	virtual bool  getIdDetails(const RsGxsId &id, RsIdentityDetails &details);

	RS_DEPRECATED_FOR(RsReputations)
	virtual bool submitOpinion(uint32_t& token, const RsGxsId &id, 
	                           bool absOpinion, int score);

	/// @see RsIdentity
	virtual bool createIdentity(
	        RsGxsId& id,
	        const std::string& name, const RsGxsImage& avatar = RsGxsImage(),
	        bool pseudonimous = true, const std::string& pgpPassword = "" ) override;

	virtual bool createIdentity(uint32_t& token, RsIdentityParameters &params);

	/// @see RsIdentity
	bool updateIdentity(RsGxsIdGroup& identityData) override;

	RS_DEPRECATED
	virtual bool updateIdentity(uint32_t& token, RsGxsIdGroup &group);

	/// @see RsIdentity
	bool deleteIdentity(RsGxsId& id) override;

	RS_DEPRECATED
	virtual bool deleteIdentity(uint32_t& token, RsGxsIdGroup &group);

    virtual void setDeleteBannedNodesThreshold(uint32_t days) ;
    virtual uint32_t deleteBannedNodesThreshold() ;

	virtual bool parseRecognTag(const RsGxsId &id, const std::string &nickname,
	                            const std::string &tag, RsRecognTagDetails &details);
	virtual bool getRecognTagRequest(const RsGxsId &id, const std::string &comment, 
	                                 uint16_t tag_class, uint16_t tag_type, std::string &tag);

	virtual bool setAsRegularContact(const RsGxsId& id,bool is_a_contact) override;
	virtual bool isARegularContact(const RsGxsId& id) override;
	virtual void setAutoAddFriendIdsAsContact(bool b) override;
	virtual bool autoAddFriendIdsAsContact() override;

	virtual uint32_t nbRegularContacts() ;
	virtual rstime_t getLastUsageTS(const RsGxsId &id) ;

	/**************** RsGixs Implementation ***************/

	/// @see RsIdentity
	bool getOwnSignedIds(std::vector<RsGxsId>& ids) override;

	/// @see RsIdentity
	bool getOwnPseudonimousIds(std::vector<RsGxsId>& ids) override;

	virtual bool getOwnIds(std::list<RsGxsId> &ownIds, bool signed_only = false);

	//virtual bool getPublicKey(const RsGxsId &id, RsTlvSecurityKey &key) ;
	//virtual void networkRequestPublicKey(const RsGxsId& key_id,const std::list<RsPeerId>& peer_ids) ;

	inline bool isKnownId(const RsGxsId& id) override { return haveKey(id); }

	bool isOwnId(const RsGxsId& key_id) override;

	virtual bool signData( const uint8_t* data,
	                       uint32_t data_size,
	                       const RsGxsId& signer_id,
	                       RsTlvKeySignature& signature,
	                       uint32_t& signing_error);

	virtual bool validateData( const uint8_t *data, uint32_t data_size,
	                           const RsTlvKeySignature& signature,
	                           bool force_load, const RsIdentityUsage &info,
	                           uint32_t& signing_error );

	virtual bool encryptData( const uint8_t* decrypted_data,
	                          uint32_t decrypted_data_size,
	                          uint8_t*& encrypted_data,
	                          uint32_t& encrypted_data_size,
	                          const RsGxsId& encryption_key_id,
	                          uint32_t& error_status,
	                          bool force_load = true );

	bool encryptData( const uint8_t* decrypted_data,
	                  uint32_t decrypted_data_size,
	                  uint8_t*& encrypted_data,
	                  uint32_t& encrypted_data_size,
	                  const std::set<RsGxsId>& encrypt_ids,
	                  uint32_t& error_status, bool force_loa = true );

	virtual bool decryptData( const uint8_t* encrypted_data,
	                          uint32_t encrypted_data_size,
	                          uint8_t*& decrypted_data,
	                          uint32_t& decrypted_data_size,
	                          const RsGxsId& decryption_key_id,
	                          uint32_t& error_status,
	                          bool force_load = true );

	virtual bool decryptData(const uint8_t* encrypted_data,
	                          uint32_t encrypted_data_size,
	                          uint8_t*& decrypted_data,
	                          uint32_t& decrypted_data_size,
	                          const std::set<RsGxsId>& decrypt_ids,
	                          uint32_t& error_status,
	                          bool force_load = true );


	virtual bool haveKey(const RsGxsId &id);
	virtual bool havePrivateKey(const RsGxsId &id);

	virtual bool getKey(const RsGxsId &id, RsTlvPublicRSAKey &key);
	virtual bool getPrivateKey(const RsGxsId &id, RsTlvPrivateRSAKey &key);

	virtual bool requestKey( const RsGxsId &id,
	                         const std::list<RsPeerId> &peers,
	                         const RsIdentityUsage &use_info );
	virtual bool requestPrivateKey(const RsGxsId &id);


	/// @see RsIdentity
	bool identityToBase64( const RsGxsId& id,
	                       std::string& base64String ) override;

	/// @see RsIdentity
	bool identityFromBase64( const std::string& base64String,
	                         RsGxsId& id ) override;

	virtual bool serialiseIdentityToMemory(const RsGxsId& id,
	                                       std::string& radix_string);
	virtual bool deserialiseIdentityFromMemory(const std::string& radix_string,
	                                           RsGxsId* id = nullptr);

	/// @see RsIdentity
	bool requestIdentity(const RsGxsId& id) override;

	/**************** RsGixsReputation Implementation ****************/

	// get Reputation.
#ifdef TO_BE_REMOVED
	virtual bool haveReputation(const RsGxsId &id);
	virtual bool loadReputation(const RsGxsId &id, const std::list<RsPeerId>& peers);
	virtual bool getReputation(const RsGxsId &id, GixsReputation &rep);
#endif


protected:
	/** Notifications **/
	virtual void notifyChanges(std::vector<RsGxsNotify*>& changes);

	/** Overloaded to add PgpIdHash to Group Definition **/
	virtual ServiceCreate_Return service_CreateGroup(RsGxsGrpItem* grpItem, RsTlvSecurityKeySet& keySet);

    // Overloads RsGxsGenExchange
    virtual bool acceptNewGroup(const RsGxsGrpMetaData *grpMeta) ;

    // Overloaded from GxsTokenQueue for Request callbacks.
	virtual void handleResponse(uint32_t token, uint32_t req_type);

	// Overloaded from RsTickEvent.
	virtual void handle_event(uint32_t event_type, const std::string &elabel);

	//===================================================//
	//                  p3Config methods                 //
	//===================================================//

	// Load/save the routing info, the pending items in transit, and the config variables.
	//
	virtual bool loadList(std::list<RsItem*>& items) ;
	virtual bool saveList(bool& cleanup,std::list<RsItem*>& items) ;

	virtual RsSerialiser *setupSerialiser() ;


private:

	/************************************************************************
 * This is the Cache for minimising calls to the DataStore.
 *
 */
	int  cache_tick();

    bool cache_request_load(const RsGxsId &id, const std::list<RsPeerId>& peers = std::list<RsPeerId>());
	bool cache_start_load();
	bool cache_load_for_token(uint32_t token);

	bool cache_store(const RsGxsIdGroupItem *item);
	bool cache_update_if_cached(const RsGxsId &id, std::string serviceString);

	bool isPendingNetworkRequest(const RsGxsId& gxsId);
    void requestIdsFromNet();

	// Mutex protected.

	//std::list<RsGxsId> mCacheLoad_ToCache;
	std::map<RsGxsId, std::list<RsPeerId> > mCacheLoad_ToCache, mPendingCache;

	// Switching to RsMemCache for Key Caching.
	RsMemCache<RsGxsId, RsGxsIdCache> mKeyCache;

	/************************************************************************
 * Refreshing own Ids.
 *
 */
	bool cache_request_ownids();
	bool cache_load_ownids(uint32_t token);

	std::list<RsGxsId> mOwnIds;
	std::list<RsGxsId> mOwnSignedIds;

	/************************************************************************
 * Test fns for Caching.
 *
 */
	bool cachetest_tick();
	bool cachetest_getlist();
	bool cachetest_handlerequest(uint32_t token);

	/************************************************************************
 * for processing background tasks that use the serviceString.
 * - must be mutually exclusive to avoid clashes.
 */
	bool CacheArbitration(uint32_t mode);
	void CacheArbitrationDone(uint32_t mode);

	bool mBgSchedule_Active;
	uint32_t mBgSchedule_Mode;

    /***********************************8
     * Fonction to receive and handle group serialisation to memory
     */

	virtual void handle_get_serialized_grp(uint32_t token);

	/************************************************************************
 * pgphash processing.
 *
 */
	bool pgphash_start();
	bool pgphash_handlerequest(uint32_t token);
	bool pgphash_process();

	bool checkId(const RsGxsIdGroup &grp, RsPgpId &pgp_id, bool &error);
	void getPgpIdList();

	/* MUTEX PROTECTED DATA (mIdMtx - maybe should use a 2nd?) */

	std::map<RsPgpId, PGPFingerprintType> mPgpFingerprintMap;
	std::list<RsGxsIdGroup> mGroupsToProcess;

	/************************************************************************
 * recogn processing.
 *
 */
	bool recogn_schedule();
	bool recogn_start();
	bool recogn_handlerequest(uint32_t token);
	bool recogn_process();

	// helper functions.
	bool recogn_extract_taginfo(const RsGxsIdGroupItem *item, std::list<RsGxsRecognTagItem *> &tagItems);
	bool cache_process_recogntaginfo(const RsGxsIdGroupItem *item, std::list<RsRecognTag> &tagList);

	bool recogn_checktag(const RsGxsId &id, const std::string &nickname, RsGxsRecognTagItem *item, bool doSignCheck, bool &isPending);

	void loadRecognKeys();


	/************************************************************************
 * opinion processing.
 *
 */

	bool opinion_handlerequest(uint32_t token);

	/* MUTEX PROTECTED DATA */
	std::map<uint32_t, OpinionRequest> mPendingOpinion;


	/************************************************************************
	 * for getting identities that are not present
	 *
	 */
	void checkPeerForIdentities();


	/* MUTEX PROTECTED DATA (mIdMtx - maybe should use a 2nd?) */

	bool checkRecognSignature_locked(std::string encoded, RSA &key, std::string signature);
	bool getRecognKey_locked(std::string signer, RSA &key);

	std::list<RsGxsGroupId> mRecognGroupIds;
	std::list<RsGxsIdGroupItem *> mRecognGroupsToProcess;
	std::map<RsGxsId, RsGxsRecognSignerItem *> mRecognSignKeys;
	std::map<RsGxsId, uint32_t> mRecognOldSignKeys;

	/************************************************************************
 * Below is the background task for processing opinions => reputations 
 *
 */

	virtual void generateDummyData();
	void generateDummy_OwnIds();
	void generateDummy_FriendPGP();
	void generateDummy_UnknownPGP();
	void generateDummy_UnknownPseudo();

	void cleanUnusedKeys() ;
	void slowIndicateConfigChanged() ;

	virtual void timeStampKey(const RsGxsId& id, const RsIdentityUsage& reason) ;
	rstime_t locked_getLastUsageTS(const RsGxsId& gxs_id);

	std::string genRandomId(int len = 20);

#if 0
	bool reputation_start();
	bool reputation_continue();

	int	background_tick();
	bool background_checkTokenRequest();
	bool background_requestGroups();
	bool background_requestNewMessages();
	bool background_processNewMessages();
	bool background_FullCalcRequest();
	bool background_processFullCalc();

	bool background_cleanup();
#endif

	RsMutex mIdMtx;

#if 0
	/***** below here is locked *****/
	bool mLastBgCheck;
	bool mBgProcessing;

	uint32_t mBgToken;
	uint32_t mBgPhase;

	std::map<RsGxsGroupId, RsGroupMetaData> mBgGroupMap;
	std::list<RsGxsGroupId> mBgFullCalcGroups;
#endif

	/************************************************************************
 * Other Data that is protected by the Mutex.
 */

private:

    struct keyTSInfo
    {
        keyTSInfo() : TS(0) {}

        rstime_t TS ;
        std::map<RsIdentityUsage,rstime_t> usage_map ;
    };
	friend class IdCacheEntryCleaner;

	std::map<uint32_t, std::set<RsGxsGroupId> > mIdsPendingCache;
	std::map<uint32_t, std::list<RsGxsGroupId> > mGroupNotPresent;
	std::map<RsGxsId, std::list<RsPeerId> > mIdsNotPresent;
	std::map<RsGxsId,keyTSInfo> mKeysTS ;

    std::map<RsGxsId,SerialisedIdentityStruct> mSerialisedIdentities ;

	// keep a list of regular contacts. This is useful to sort IDs, and allow some services to priviledged ids only.
	std::set<RsGxsId> mContacts;
	RsNetworkExchangeService* mNes;

	/**************************
	 * AuxUtils provides interface to Security Function (e.g. GPGAuth(), notify etc.)
	 * without depending directly on all these classes.
	 */

	PgpAuxUtils *mPgpUtils;

	rstime_t mLastKeyCleaningTime ;
	rstime_t mLastConfigUpdate ;

	bool mOwnIdsLoaded;
	bool ownIdsAreLoaded() { RS_STACK_MUTEX(mIdMtx); return mOwnIdsLoaded; }

	bool mAutoAddFriendsIdentitiesAsContacts;
	uint32_t mMaxKeepKeysBanned;

	RS_SET_CONTEXT_DEBUG_LEVEL(1)
};
