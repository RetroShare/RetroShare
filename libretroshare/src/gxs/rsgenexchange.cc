/*******************************************************************************
 * libretroshare/src/gxs: rsgenexchange.cc                                     *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012  Christopher Evi-Parker                                  *
 * Copyright (C) 2019  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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
#include <unistd.h>
#include <algorithm>

#include "pqi/pqihash.h"
#include "rsgenexchange.h"
#include "gxssecurity.h"
#include "util/contentvalue.h"
#include "util/rsprint.h"
#include "util/rstime.h"
#include "retroshare/rsgxsflags.h"
#include "retroshare/rsgxscircles.h"
#include "retroshare/rsgrouter.h"
#include "retroshare/rsidentity.h"
#include "retroshare/rspeers.h"
#include "rsitems/rsnxsitems.h"
#include "rsgixs.h"
#include "rsgxsutil.h"
#include "rsserver/p3face.h"
#include "retroshare/rsevents.h"
#include "util/radix64.h"

#define PUB_GRP_MASK     0x000f
#define RESTR_GRP_MASK   0x00f0
#define PRIV_GRP_MASK    0x0f00
#define GRP_OPTIONS_MASK 0xf000

#define PUB_GRP_OFFSET        0
#define RESTR_GRP_OFFSET      8
#define PRIV_GRP_OFFSET      16
#define GRP_OPTIONS_OFFSET   24

// Authentication key indices. Used to store them in a map 
// these where originally flags, but used as indexes. Still, we need
// to keep their old values to ensure backward compatibility.

static const uint32_t INDEX_AUTHEN_IDENTITY     = 0x00000010; // identity
static const uint32_t INDEX_AUTHEN_PUBLISH      = 0x00000020; // publish key
static const uint32_t INDEX_AUTHEN_ADMIN        = 0x00000040; // admin key

#define GXS_MASK "GXS_MASK_HACK"

/*
 *  #define GEN_EXCH_DEBUG	1
 */

// Data flow in RsGenExchange
//
//     publishGroup()
//       |
//       +--- stores in mGrpsToPublish
//
//     updateGroup()
//       |
//       +--- stores in mGroupUpdatePublish
//
//     tick()
//       |
//       +--- processRecvdData();
//       |          |
//       |          +-- processRecvdGroups();
//       |          |          |
//       |          |          +---- mGroupPendingValidate
//       |          |                        |
//       |          |                        +---- acceptance => validateGrp() => check existance => store in mGroupUpdates
//       |          |                                                 |
//       |          |                                                 +--- validateNxsGrp()
//       |          |
//       |          +-- processRecvdMessages();
//       |          |
//       |          +-- performUpdateValidation();  // validates group updates in mGroupUpdates received from friends
//       |                     |
//       |                     +---- mGroupUpdates => updateValid() => store new groups
//       |                     |                          |
//       |                     |                          +--- stamp keys, check admin keys are the same, validateNxsGrp()
//       |                     |                                                                                |
//       |                     +---- stores RsGxsGroupChange in mNotifications                                  +--- data signature check
//       |
//       +--- publishGrps();  // creates signatures and data for locally created groups
//       |          |
//       |          +-- reads mGrpsToPublish, creates the group, clears mGrpsToPublish
//       |                     |
//       |                     +--- generateGroupKeys()
//       |                     |
//       |                     +--- serviceCreateGroup()
//       |                     |
//       |                     +--- createGroup()
//       |                              |
//       |                              +--- createGroupSignatures()
//       |
//       +--- processGroupUpdatePublish(); // goes through list of group updates to perform locally
//       |          |
//       |          +-- reads/deletes mGroupUpdatePublish, stores in mGrpsToPublish
//       |
//       +--- processGrpMetaChanges();
//       |          |
//       |          +-- updates mNotifications, calls mDataAccess
//       |
//       +--- processGroupDelete();
//       |
//       +--- publishMsgs();
//       |
//       +--- processMsgMetaChanges();
//       |          |
//       |          +-- updates mNotifications, calls mDataAccess
//       |
//       +--- processMessageDelete();
//       |
//       +--- mDataAccess->processRequests();
//       |
//       +--- processRoutingClues() ;
//

static const uint32_t MSG_CLEANUP_PERIOD     = 60*59; // 59 minutes
static const uint32_t INTEGRITY_CHECK_PERIOD = 60*31; // 31 minutes

RsGenExchange::RsGenExchange(
        RsGeneralDataService* gds, RsNetworkExchangeService* ns,
        RsSerialType* serviceSerialiser, uint16_t servType, RsGixs* gixs,
        uint32_t authenPolicy ) :
    mGenMtx("GenExchange"),
    mDataStore(gds),
    mNetService(ns),
    mSerialiser(serviceSerialiser),
  mServType(servType),
  mGixs(gixs),
  mAuthenPolicy(authenPolicy),
  mCleaning(false),
  mLastClean((int)time(NULL) - (int)(RSRandom::random_u32() % MSG_CLEANUP_PERIOD)),	// this helps unsynchronising the checks for the different services
  mChecking(false),
  mCheckStarted(false),
  mLastCheck((int)time(NULL) - (int)(RSRandom::random_u32() % INTEGRITY_CHECK_PERIOD) + 120),	// this helps unsynchronising the checks for the different services, with 2 min security to avoid checking right away before statistics come up.
  mIntegrityCheck(NULL),
  SIGN_MAX_WAITING_TIME(60),
  SIGN_FAIL(0),
  SIGN_SUCCESS(1),
  SIGN_FAIL_TRY_LATER(2),
  VALIDATE_FAIL(0),
  VALIDATE_SUCCESS(1),
  VALIDATE_FAIL_TRY_LATER(2),
  VALIDATE_MAX_WAITING_TIME(60)
{
    mDataAccess = new RsGxsDataAccess(gds);
}

void RsGenExchange::setNetworkExchangeService(RsNetworkExchangeService *ns)
{
    if(mNetService != NULL)
        std::cerr << "(EE) Cannot override existing network exchange service. Make sure it has been deleted otherwise." << std::endl;
    else
	{
        mNetService = ns ;
	}
}

RsGenExchange::~RsGenExchange()
{
    // need to destruct in a certain order (bad thing, TODO: put down instance ownership rules!)
    delete mNetService;

    delete mDataAccess;
    mDataAccess = NULL;

    delete mDataStore;
    mDataStore = NULL;

	for(uint32_t i=0;i<mNotifications.size();++i)
		delete mNotifications[i] ;

	for(uint32_t i=0;i<mGrpsToPublish.size();++i)
		delete mGrpsToPublish[i].mItem ;

	mNotifications.clear();
	mGrpsToPublish.clear();
}

bool RsGenExchange::getGroupServerUpdateTS(const RsGxsGroupId& gid, rstime_t& grp_server_update_TS, rstime_t& msg_server_update_TS) 
{
    return mNetService->getGroupServerUpdateTS(gid,grp_server_update_TS,msg_server_update_TS) ;
}

void RsGenExchange::threadTick()
{
	static const double timeDelta = 0.1; // slow tick in sec

	tick();
	rstime::rs_usleep((int) (timeDelta * 1000 *1000)); // timeDelta sec
}

void RsGenExchange::tick()
{
	// Meta Changes should happen first.
	// This is important, as services want to change Meta, then get results.
	// Services shouldn't rely on this ordering - but some do.
	processGrpMetaChanges();
	processMsgMetaChanges();

	mDataAccess->processRequests();

	publishGrps();

	publishMsgs();

	processGroupUpdatePublish();

	processGroupDelete();
	processMessageDelete();

	processRecvdData();

	processRoutingClues() ;

	{
		std::vector<RsGxsNotify*> mNotifications_copy;

        // make a non-deep copy of mNotifications so that it can be passed off-mutex to notifyChanged()
        // that will delete it. The potential high cost of notifyChanges() makes it preferable to call off-mutex.
		{
			RS_STACK_MUTEX(mGenMtx);
			if(!mNotifications.empty())
			{
				mNotifications_copy = mNotifications;
				mNotifications.clear();
			}
		}

        // Calling notifyChanges() calls back RsGxsIfaceHelper::receiveChanges() that deletes the pointers in the array
        // and the array itself.  This is pretty bad and we should normally delete the changes here.

		if(!mNotifications_copy.empty())
			notifyChanges(mNotifications_copy);
	}

	// implemented service tick function
	service_tick();

	rstime_t now = time(NULL);
    
    // Cleanup unused data. This is only needed when auto-synchronization is needed, which is not the case
    // of identities. This is why idendities do their own cleaning.
    now = time(NULL);

    if( (mNetService && (mNetService->msgAutoSync() || mNetService->grpAutoSync())) && (mLastClean + MSG_CLEANUP_PERIOD < now) )
	{
        GxsMsgReq msgs_to_delete;
        std::vector<RsGxsGroupId> grps_to_delete;

        RsGxsCleanUp(mDataStore,this,1).clean(mNextGroupToCheck,grps_to_delete,msgs_to_delete);	// no need to lock here, because all access below (RsGenExchange, RsDataStore) are properly mutexed

        uint32_t token1=0;
        deleteMsgs(token1,msgs_to_delete);

        for(auto& grpId: grps_to_delete)
        {
            uint32_t token2=0;
            deleteGroup(token2,grpId);
        }

        RS_STACK_MUTEX(mGenMtx) ;
        mLastClean = now;
    }

	if(mChecking || (mLastCheck + INTEGRITY_CHECK_PERIOD < now))
	{
		mLastCheck = time(NULL);

		{
			RS_STACK_MUTEX(mGenMtx) ;

			if(!mIntegrityCheck)
			{
				mIntegrityCheck = new RsGxsIntegrityCheck( mDataStore, this,
				                                           *mSerialiser, mGixs);
				mIntegrityCheck->start("gxs integrity");
				mChecking = true;
			}
		}

		if(mIntegrityCheck->isDone())
		{
			RS_STACK_MUTEX(mGenMtx) ;

            std::vector<RsGxsGroupId> grpIds;
            GxsMsgReq msgIds;

			mIntegrityCheck->getDeletedIds(grpIds, msgIds);

            if(!msgIds.empty())
            {
                uint32_t token1=0;
                deleteMsgs(token1,msgIds);
            }

            if(!grpIds.empty())
                for(auto& grpId: grpIds)
                {
                    uint32_t token2=0;
                    deleteGroup(token2,grpId);
                }

			delete mIntegrityCheck;
			mIntegrityCheck = NULL;
			mChecking = false;
		}
	}
}

bool RsGenExchange::messagePublicationTest(const RsGxsMsgMetaData& meta)
{
	if(!mNetService)
	{
#ifdef GEN_EXCH_DEBUG
		std::cerr << "(EE) No network service in service " << std::hex  << serviceType() << std::dec << ": cannot read message storage time." << std::endl;
#endif
		return false ;
	}

	uint32_t st = mNetService->getKeepAge(meta.mGroupId);

	rstime_t storageTimeLimit = meta.mPublishTs + st;

	return meta.mMsgStatus & GXS_SERV::GXS_MSG_STATUS_KEEP_FOREVER || st == 0 || storageTimeLimit >= time(NULL);
}

bool RsGenExchange::acknowledgeTokenMsg(const uint32_t& token,
                RsGxsGrpMsgIdPair& msgId)
{
	RS_STACK_MUTEX(mGenMtx) ;

#ifdef GEN_EXCH_DEBUG
	std::cerr << "RsGenExchange::acknowledgeTokenMsg(). token=" << token << std::endl;
#endif
	std::map<uint32_t, RsGxsGrpMsgIdPair >::iterator mit = mMsgNotify.find(token);

	if(mit == mMsgNotify.end())
	{
#ifdef GEN_EXCH_DEBUG
		std::cerr << "  no notification found for this token." << std::endl;
#endif
		return false;
	}


	msgId = mit->second;

	// no dump token as client has ackowledged its completion
	mDataAccess->disposeOfPublicToken(token);

#ifdef GEN_EXCH_DEBUG
	std::cerr << "  found grpId=" << msgId.first <<", msgId=" << msgId.second << std::endl;
	std::cerr << "  disposing token from mDataAccess" << std::endl;
#endif
	return true;
}



bool RsGenExchange::acknowledgeTokenGrp(const uint32_t& token, RsGxsGroupId& grpId)
{
	RS_STACK_MUTEX(mGenMtx) ;

#ifdef GEN_EXCH_DEBUG
	std::cerr << "RsGenExchange::acknowledgeTokenGrp(). token=" << token << std::endl;
#endif
	std::map<uint32_t, RsGxsGroupId >::iterator mit =
                        mGrpNotify.find(token);

	if(mit == mGrpNotify.end())
	{
#ifdef GEN_EXCH_DEBUG
		std::cerr << "  no notification found for this token." << std::endl;
#endif
		return false;
	}

	grpId = mit->second;

	// no dump token as client has ackowledged its completion
	mDataAccess->disposeOfPublicToken(token);

#ifdef GEN_EXCH_DEBUG
	std::cerr << "  found grpId=" << grpId << std::endl;
	std::cerr << "  disposing token from mDataAccess" << std::endl;
#endif
	return true;
}

void RsGenExchange::generateGroupKeys(RsTlvSecurityKeySet& keySet, bool genPublishKeys)
{
	/* create Keys */
	RsTlvPublicRSAKey  pubAdminKey ;
	RsTlvPrivateRSAKey privAdminKey;

	GxsSecurity::generateKeyPair(pubAdminKey,privAdminKey) ;

	// for now all public
	pubAdminKey.keyFlags  |= RSTLV_KEY_DISTRIB_ADMIN ;
	privAdminKey.keyFlags |= RSTLV_KEY_DISTRIB_ADMIN ;

	keySet.public_keys[pubAdminKey.keyId] = pubAdminKey;
	keySet.private_keys[privAdminKey.keyId] = privAdminKey;

	if(genPublishKeys)
	{
		/* set publish keys */
		RsTlvPublicRSAKey  pubPublishKey ;
		RsTlvPrivateRSAKey privPublishKey;

		GxsSecurity::generateKeyPair(pubPublishKey,privPublishKey) ;

		// for now all public
		pubPublishKey.keyFlags  |= RSTLV_KEY_DISTRIB_PUBLISH ;
		privPublishKey.keyFlags |= RSTLV_KEY_DISTRIB_PUBLISH ;

		keySet.public_keys[pubPublishKey.keyId] = pubPublishKey;
		keySet.private_keys[privPublishKey.keyId] = privPublishKey;
	}
}

uint8_t RsGenExchange::createGroup(RsNxsGrp *grp, RsTlvSecurityKeySet& keySet)
{
#ifdef GEN_EXCH_DEBUG
    std::cerr << "RsGenExchange::createGroup()";
    std::cerr << std::endl;
#endif

    RsGxsGrpMetaData* meta = grp->metaData;

    /* add public admin and publish keys to grp */

    // find private admin key
    RsTlvPrivateRSAKey privAdminKey;
    bool privKeyFound = false; // private admin key

    for( std::map<RsGxsId, RsTlvPrivateRSAKey>::iterator mit = keySet.private_keys.begin(); mit != keySet.private_keys.end(); ++mit)
    {
        RsTlvPrivateRSAKey& key = mit->second;

        if((key.keyFlags & RSTLV_KEY_DISTRIB_ADMIN) && (key.keyFlags & RSTLV_KEY_TYPE_FULL))
        {
            privAdminKey = key;
            privKeyFound = true;
	    break ;
        }
    }

    if(!privKeyFound)
    {
        std::cerr << "RsGenExchange::createGroup() Missing private ADMIN Key";
	std::cerr << std::endl;

    	return false;
    }

    // only public keys are included to be transported. The 2nd line below is very important.
    
    meta->keys = keySet; 
    meta->keys.private_keys.clear() ; 

    // group is self signing
    // for the creation of group signature
    // only public admin and publish keys are present in meta
    uint32_t metaDataLen = meta->serial_size(RS_GXS_GRP_META_DATA_CURRENT_API_VERSION);
    uint32_t allGrpDataLen = metaDataLen + grp->grp.bin_len;
    
    char* metaData = new char[metaDataLen];
    char* allGrpData = new char[allGrpDataLen]; // msgData + metaData

    meta->serialise(metaData, metaDataLen,RS_GXS_GRP_META_DATA_CURRENT_API_VERSION);

    // copy msg data and meta in allMsgData buffer
    memcpy(allGrpData, grp->grp.bin_data, grp->grp.bin_len);
    memcpy(allGrpData+(grp->grp.bin_len), metaData, metaDataLen);

    RsTlvKeySignature adminSign;
    bool ok = GxsSecurity::getSignature(allGrpData, allGrpDataLen, privAdminKey, adminSign);

    // add admin sign to grpMeta
    meta->signSet.keySignSet[INDEX_AUTHEN_ADMIN] = adminSign;

    RsTlvBinaryData grpData(mServType);
	grpData.setBinData(allGrpData, allGrpDataLen);

    uint8_t ret = createGroupSignatures(meta->signSet, grpData, *(grp->metaData));

    // clean up
    delete[] allGrpData;
    delete[] metaData;

    if (!ok)
    {
        std::cerr << "RsGenExchange::createGroup() ERROR !okay (getSignature error)";
		std::cerr << std::endl;
		return CREATE_FAIL;
    }

    if(ret == SIGN_FAIL)
    {
    	return CREATE_FAIL;
    }else if(ret == SIGN_FAIL_TRY_LATER)
    {
    	return CREATE_FAIL_TRY_LATER;
    }else if(ret == SIGN_SUCCESS)
    {
    	return CREATE_SUCCESS;
    }else{
    	return CREATE_FAIL;
    }
}

int RsGenExchange::createGroupSignatures(RsTlvKeySignatureSet& signSet, RsTlvBinaryData& grpData,
    							RsGxsGrpMetaData& grpMeta)
{
	bool needIdentitySign = false;
    int id_ret;

    uint8_t author_flag = GXS_SERV::GRP_OPTION_AUTHEN_AUTHOR_SIGN;

    PrivacyBitPos pos = GRP_OPTION_BITS;

    // Check required permissions, and allow them to sign it - if they want too - as well!
    if ((!grpMeta.mAuthorId.isNull()) || checkAuthenFlag(pos, author_flag))
    {
        needIdentitySign = true;
#ifdef GEN_EXCH_DEBUG
        std::cerr << "Needs Identity sign! (Service Flags)";
        std::cerr << std::endl;
#endif
    }

    if (needIdentitySign)
    {
        if(grpMeta.mAuthorId.isNull())
        {
            std::cerr << "RsGenExchange::createGroupSignatures() ";
            std::cerr << "Group signature is required by service, but the author id is null." << std::endl;
            id_ret = SIGN_FAIL;
        }
        else if(mGixs)
        {
            bool haveKey = mGixs->havePrivateKey(grpMeta.mAuthorId);

            if(haveKey)
            {
                RsTlvPrivateRSAKey authorKey;
                mGixs->getPrivateKey(grpMeta.mAuthorId, authorKey);
                RsTlvKeySignature sign;

                if(GxsSecurity::getSignature((char*)grpData.bin_data, grpData.bin_len, authorKey, sign))
                {
                	id_ret = SIGN_SUCCESS;
					mGixs->timeStampKey(grpMeta.mAuthorId,RsIdentityUsage(RsServiceType(mServType),RsIdentityUsage::GROUP_AUTHOR_SIGNATURE_CREATION,grpMeta.mGroupId)) ;
					signSet.keySignSet[INDEX_AUTHEN_IDENTITY] = sign;
                }
                else
                	id_ret = SIGN_FAIL;
            }
            else
            {
            	mGixs->requestPrivateKey(grpMeta.mAuthorId);

#ifdef GEN_EXCH_DEBUG
                std::cerr << "RsGenExchange::createGroupSignatures(): ";
                std::cerr << " ERROR AUTHOR KEY: " <<  grpMeta.mAuthorId
                		  << " is not Cached / available for Message Signing\n";
                std::cerr << "RsGenExchange::createGroupSignatures():  Requestiong AUTHOR KEY";
                std::cerr << std::endl;
#endif

                id_ret = SIGN_FAIL_TRY_LATER;
            }
        }
        else
        {
#ifdef GEN_EXCH_DEBUG
            std::cerr << "RsGenExchange::createGroupSignatures()";
            std::cerr << "Gixs not enabled while request identity signature validation!" << std::endl;
#endif
            id_ret = SIGN_FAIL;
        }
    }
    else
    {
    	id_ret = SIGN_SUCCESS;
    }

	return id_ret;
}

int RsGenExchange::createMsgSignatures(RsTlvKeySignatureSet& signSet, RsTlvBinaryData& msgData,
                                        const RsGxsMsgMetaData& msgMeta, const RsGxsGrpMetaData& grpMeta)
{
    bool needPublishSign = false, needIdentitySign = false;
    uint32_t grpFlag = grpMeta.mGroupFlags;

    bool publishSignSuccess = false;

#ifdef GEN_EXCH_DEBUG
    std::cerr << "RsGenExchange::createMsgSignatures() for Msg.mMsgName: " << msgMeta.mMsgName;
    std::cerr << std::endl;
#endif


    // publish signature is determined by whether group is public or not
    // for private group signature is not needed as it needs decrypting with
    // the private publish key anyways

    // restricted is a special case which heeds whether publish sign needs to be checked or not
    // one may or may not want

    uint8_t author_flag = GXS_SERV::MSG_AUTHEN_ROOT_AUTHOR_SIGN;
    uint8_t publish_flag = GXS_SERV::MSG_AUTHEN_ROOT_PUBLISH_SIGN;

    if(!msgMeta.mParentId.isNull())
    {
        // Child Message.
        author_flag = GXS_SERV::MSG_AUTHEN_CHILD_AUTHOR_SIGN;
        publish_flag = GXS_SERV::MSG_AUTHEN_CHILD_PUBLISH_SIGN;
    }

    PrivacyBitPos pos = PUBLIC_GRP_BITS;
    if (grpFlag & GXS_SERV::FLAG_PRIVACY_RESTRICTED)
    {
        pos = RESTRICTED_GRP_BITS;
    }
    else if (grpFlag & GXS_SERV::FLAG_PRIVACY_PRIVATE)
    {
        pos = PRIVATE_GRP_BITS;
    }
    
    needIdentitySign = false;
    needPublishSign = false;
    if (checkAuthenFlag(pos, publish_flag))
    {
        needPublishSign = true;
#ifdef GEN_EXCH_DEBUG
        std::cerr << "Needs Publish sign! (Service Flags)";
        std::cerr << std::endl;
#endif
    }

    // Check required permissions, and allow them to sign it - if they want too - as well!
    if (checkAuthenFlag(pos, author_flag))
    {
        needIdentitySign = true;
#ifdef GEN_EXCH_DEBUG
        std::cerr << "Needs Identity sign! (Service Flags)";
        std::cerr << std::endl;
#endif
    }

    if (!msgMeta.mAuthorId.isNull())
    {
        needIdentitySign = true;
#ifdef GEN_EXCH_DEBUG
        std::cerr << "Needs Identity sign! (AuthorId Exists)";
        std::cerr << std::endl;
#endif
    }

    if(needPublishSign)
    {
        // public and shared is publish key
        const RsTlvSecurityKeySet& keys = grpMeta.keys;
        const RsTlvPrivateRSAKey  *publishKey;

        std::map<RsGxsId, RsTlvPrivateRSAKey>::const_iterator mit = keys.private_keys.begin(), mit_end = keys.private_keys.end();
        bool publish_key_found = false;
        
        for(; mit != mit_end; ++mit)
        {

                publish_key_found = mit->second.keyFlags == (RSTLV_KEY_DISTRIB_PUBLISH | RSTLV_KEY_TYPE_FULL);
                if(publish_key_found)
                        break;
        }

        if (publish_key_found)
        {
            // private publish key
            publishKey = &(mit->second);

            RsTlvKeySignature publishSign = signSet.keySignSet[INDEX_AUTHEN_PUBLISH];

            publishSignSuccess = GxsSecurity::getSignature((char*)msgData.bin_data, msgData.bin_len, *publishKey, publishSign);

            //place signature in msg meta
            signSet.keySignSet[INDEX_AUTHEN_PUBLISH] = publishSign;
        }else
        {
        	std::cerr << "RsGenExchange::createMsgSignatures()";
			std::cerr << " ERROR Cannot find PUBLISH KEY for Message Signing!";
			std::cerr << " ERROR Publish Sign failed!";
			std::cerr << std::endl;
        }

    }
    else // publish sign not needed so set as successful
    {
    	publishSignSuccess = true;
    }

    int id_ret;

    if (needIdentitySign)
    {
        if(mGixs)
        {
            bool haveKey = mGixs->havePrivateKey(msgMeta.mAuthorId);

            if(haveKey)
	    {
		    RsTlvPrivateRSAKey authorKey;
		    mGixs->getPrivateKey(msgMeta.mAuthorId, authorKey);
		    RsTlvKeySignature sign;

		    if(GxsSecurity::getSignature((char*)msgData.bin_data, msgData.bin_len, authorKey, sign))
		    {
			    id_ret = SIGN_SUCCESS;
			    mGixs->timeStampKey(msgMeta.mAuthorId,RsIdentityUsage(RsServiceType(mServType),RsIdentityUsage::MESSAGE_AUTHOR_SIGNATURE_CREATION,msgMeta.mGroupId,msgMeta.mMsgId,msgMeta.mParentId,msgMeta.mThreadId)) ;
			    signSet.keySignSet[INDEX_AUTHEN_IDENTITY] = sign;
		    }
		    else
			    id_ret = SIGN_FAIL;
	    }
            else
            {
            	mGixs->requestPrivateKey(msgMeta.mAuthorId);

#ifdef GEN_EXCH_DEBUG
                std::cerr << "RsGenExchange::createMsgSignatures(): ";
                std::cerr << " ERROR AUTHOR KEY: " <<  msgMeta.mAuthorId
                		  << " is not Cached / available for Message Signing\n";
                std::cerr << "RsGenExchange::createMsgSignatures():  Requestiong AUTHOR KEY";
                std::cerr << std::endl;
#endif

                id_ret = SIGN_FAIL_TRY_LATER;
            }
        }
        else
        {
#ifdef GEN_EXCH_DEBUG
            std::cerr << "RsGenExchange::createMsgSignatures()";
            std::cerr << "Gixs not enabled while request identity signature validation!" << std::endl;
#endif
            id_ret = SIGN_FAIL;
        }
    }
    else
    {
    	id_ret = SIGN_SUCCESS;
    }

    if(publishSignSuccess)
    {
    	return id_ret;
    }
    else
    {
    	return SIGN_FAIL;
    }
}

int RsGenExchange::createMessage(RsNxsMsg* msg)
{
	const RsGxsGroupId& id = msg->grpId;

#ifdef GEN_EXCH_DEBUG
	std::cerr << "RsGenExchange::createMessage() " << std::endl;
#endif
	RsGxsGrpMetaTemporaryMap  metaMap ;
	metaMap.insert(std::make_pair(id, (RsGxsGrpMetaData*)(NULL)));
	mDataStore->retrieveGxsGrpMetaData(metaMap);

	RsGxsMsgMetaData &meta = *(msg->metaData);

	int ret_val;

	if(!metaMap[id])
	{
		return CREATE_FAIL;
	}
	else
	{
		// get publish key
		const RsGxsGrpMetaData* grpMeta = metaMap[id];

		uint32_t metaDataLen = meta.serial_size();
		uint32_t allMsgDataLen = metaDataLen + msg->msg.bin_len;
		char* metaData = new char[metaDataLen];
		char* allMsgData = new char[allMsgDataLen]; // msgData + metaData

		meta.serialise(metaData, &metaDataLen);

		// copy msg data and meta in allmsgData buffer
		memcpy(allMsgData, msg->msg.bin_data, msg->msg.bin_len);
		memcpy(allMsgData+(msg->msg.bin_len), metaData, metaDataLen);

		RsTlvBinaryData msgData(0);

		msgData.setBinData(allMsgData, allMsgDataLen);

		// create signatures
	   ret_val = createMsgSignatures(meta.signSet, msgData, meta, *grpMeta);


		// get hash of msg data to create msg id
		pqihash hash;
		hash.addData(allMsgData, allMsgDataLen);
		RsFileHash hashId;
		hash.Complete(hashId);
		msg->msgId = RsGxsMessageId(hashId);

		// assign msg id to msg meta
		msg->metaData->mMsgId = msg->msgId;

		delete[] metaData;
		delete[] allMsgData;
	}

	if(ret_val == SIGN_FAIL)
		return CREATE_FAIL;
	else if(ret_val == SIGN_FAIL_TRY_LATER)
		return CREATE_FAIL_TRY_LATER;
	else if(ret_val == SIGN_SUCCESS)
		return CREATE_SUCCESS;
	else
	{
		std::cerr << "Unknown return value from signature attempt!";
		return CREATE_FAIL;
	}
}

int RsGenExchange::validateMsg(RsNxsMsg *msg, const uint32_t& grpFlag, const uint32_t& /*signFlag*/, RsTlvSecurityKeySet& grpKeySet)
{
    bool needIdentitySign = false;
    bool needPublishSign = false;
    bool publishValidate = true, idValidate = true;

    uint8_t author_flag = GXS_SERV::MSG_AUTHEN_ROOT_AUTHOR_SIGN;
    uint8_t publish_flag = GXS_SERV::MSG_AUTHEN_ROOT_PUBLISH_SIGN;

    if(!msg->metaData->mParentId.isNull())
    {
        // Child Message.
        author_flag = GXS_SERV::MSG_AUTHEN_CHILD_AUTHOR_SIGN;
        publish_flag = GXS_SERV::MSG_AUTHEN_CHILD_PUBLISH_SIGN;
    }

    PrivacyBitPos pos = PUBLIC_GRP_BITS;
    if (grpFlag & GXS_SERV::FLAG_PRIVACY_RESTRICTED)
    {
        pos = RESTRICTED_GRP_BITS;
    }
    else if (grpFlag & GXS_SERV::FLAG_PRIVACY_PRIVATE)
    {
        pos = PRIVATE_GRP_BITS;
    }
    
    if (checkAuthenFlag(pos, publish_flag))
        needPublishSign = true;

    // Check required permissions, if they have signed it anyway - we need to validate it.
    if ((checkAuthenFlag(pos, author_flag)) || (!msg->metaData->mAuthorId.isNull()))
        needIdentitySign = true;

#ifdef GEN_EXCH_DEBUG	
    std::cerr << "Validate message: msgId=" << msg->msgId << ", grpId=" << msg->grpId << " grpFlags=" << std::hex << grpFlag << std::dec
              << ". Need publish=" << needPublishSign << ", needIdentitySign=" << needIdentitySign ;
#endif

    RsGxsMsgMetaData& metaData = *(msg->metaData);

    if(needPublishSign)
	{
		RsTlvKeySignature sign = metaData.signSet.keySignSet[INDEX_AUTHEN_PUBLISH];

		std::map<RsGxsId, RsTlvPublicRSAKey>& keys = grpKeySet.public_keys;
		std::map<RsGxsId, RsTlvPublicRSAKey>::iterator mit = keys.begin();

		RsGxsId keyId;
		for(; mit != keys.end() ; ++mit)
		{
			RsTlvPublicRSAKey& key = mit->second;

			if(key.keyFlags & RSTLV_KEY_DISTRIB_PUBLIC_deprecated)
			{
				keyId = key.keyId;
				std::cerr << "WARNING: old style publish key with flags " << key.keyFlags << std::endl;
				std::cerr << "         this cannot be fixed, but RS will deal with it." << std::endl;
				break ;
			}
			if(key.keyFlags & RSTLV_KEY_DISTRIB_PUBLISH) // we might have the private key, but we still should be able to check the signature
			{
				keyId = key.keyId;
				break;
			}
		}

		if(!keyId.isNull())
		{
			RsTlvPublicRSAKey& key = keys[keyId];
			publishValidate &= GxsSecurity::validateNxsMsg(*msg, sign, key);
		}
		else
		{
            std::cerr << "(EE) public publish key not found in group that require publish key validation. This should not happen! msgId=" << metaData.mMsgId << ", grpId=" << metaData.mGroupId << std::endl;
            std::cerr << "(EE) public keys available for this group are: " << std::endl;

            for(std::map<RsGxsId, RsTlvPublicRSAKey>::const_iterator it(grpKeySet.public_keys.begin());it!=grpKeySet.public_keys.end();++it)
				std::cerr << "(EE) " << it->first << std::endl;

            std::cerr << "(EE) private keys available for this group are: " << std::endl;

            for(std::map<RsGxsId, RsTlvPrivateRSAKey>::const_iterator it(grpKeySet.private_keys.begin());it!=grpKeySet.private_keys.end();++it)
				std::cerr << "(EE) " << it->first << std::endl;

			publishValidate = false;
		}
	}
    else
    {
    	publishValidate = true;
    }



    if(needIdentitySign)
    {
        if(mGixs)
        {
            bool haveKey = mGixs->haveKey(metaData.mAuthorId);

            if(haveKey)
	    {

		    RsTlvPublicRSAKey authorKey;
		    bool auth_key_fetched = mGixs->getKey(metaData.mAuthorId, authorKey) ;

		    if (auth_key_fetched)
		    {
			    RsTlvKeySignature sign = metaData.signSet.keySignSet[INDEX_AUTHEN_IDENTITY];
			    idValidate &= GxsSecurity::validateNxsMsg(*msg, sign, authorKey);
			    mGixs->timeStampKey(metaData.mAuthorId,RsIdentityUsage(RsServiceType(mServType),RsIdentityUsage::MESSAGE_AUTHOR_SIGNATURE_VALIDATION,
                                                                       metaData.mGroupId,
                                                                       metaData.mMsgId,
                                                                       metaData.mParentId,
                                                                       metaData.mThreadId)) ;
		    }
		    else
		    {
			    std::cerr << "RsGenExchange::validateMsg()";
			    std::cerr << " ERROR Cannot Retrieve AUTHOR KEY for Message Validation";
			    std::cerr << std::endl;
			    idValidate = false;
		    }

            	if(idValidate)
		{
			// get key data and check that the key is actually PGP-linked. If not, reject the post.

			RsIdentityDetails details ;

			if(!mGixs->getIdDetails(metaData.mAuthorId,details))
			{
				// the key cannot ke reached, although it's in cache. Weird situation.
				std::cerr << "RsGenExchange::validateMsg(): cannot get key data for ID=" << metaData.mAuthorId << ", although it's supposed to be already in cache. Cannot validate." << std::endl;
				idValidate = false ;
			}
			else 
			{

				// now check reputation of the message author. The reputation will need to be at least as high as this value for the msg to validate.
                // At validation step, we accept all messages, except the ones signed by locally rejected identities.

				if( details.mReputation.mOverallReputationLevel ==
				        RsReputationLevel::LOCALLY_NEGATIVE )
				{
#ifdef GEN_EXCH_DEBUG	
					std::cerr << "RsGenExchange::validateMsg(): message from " << metaData.mAuthorId << ", rejected because reputation level (" << static_cast<int>(details.mReputation.mOverallReputationLevel) <<") indicate that you banned this ID." << std::endl;
#endif
					idValidate = false ;
				}
			}

				}
	    }
            else
            {
                std::list<RsPeerId> peers;
                peers.push_back(msg->PeerId());
                mGixs->requestKey(metaData.mAuthorId, peers, RsIdentityUsage((RsServiceType)serviceType(),
                                                                             RsIdentityUsage::MESSAGE_AUTHOR_SIGNATURE_VALIDATION,
                                                                             metaData.mGroupId,
                                                                             metaData.mMsgId,
                                                                             metaData.mParentId,
                                                                             metaData.mThreadId));
                
#ifdef GEN_EXCH_DEBUG
                std::cerr << ", Key missing. Retry later." << std::endl;
#endif
                return VALIDATE_FAIL_TRY_LATER;
            }
        }
        else
        {
#ifdef GEN_EXCH_DEBUG
            std::cerr << "Gixs not enabled while request identity signature validation!" << std::endl;
#endif
            idValidate = false;
        }
    }
    else
    {
    	idValidate = true;
    }

#ifdef GEN_EXCH_DEBUG
    std::cerr << ", publish val=" << publishValidate << ", idValidate=" << idValidate << ". Result=" << (publishValidate && idValidate) << std::endl;
#endif
    
    if(publishValidate && idValidate)
    	return VALIDATE_SUCCESS;
    else
    	return VALIDATE_FAIL;

}

int RsGenExchange::validateGrp(RsNxsGrp* grp)
{
    bool needIdentitySign = false, idValidate = false;
    RsGxsGrpMetaData& metaData = *(grp->metaData);

    uint8_t author_flag = GXS_SERV::GRP_OPTION_AUTHEN_AUTHOR_SIGN;

    PrivacyBitPos pos = GRP_OPTION_BITS;

#ifdef GEN_EXCH_DEBUG
	std::cerr << "Validating group " << grp->grpId << ", authorId = " << metaData.mAuthorId << std::endl;
#endif
    // Check required permissions, and allow them to sign it - if they want too - as well!
    if ((!metaData.mAuthorId.isNull()) || checkAuthenFlag(pos, author_flag))
    {
	    needIdentitySign = true;
#ifdef GEN_EXCH_DEBUG
	    std::cerr << "  Needs Identity sign! (Service Flags). Identity signing key is " << metaData.mAuthorId << std::endl;
#endif
    }

    if(needIdentitySign)
    {
	    if(mGixs)
	    {
		    bool haveKey = mGixs->haveKey(metaData.mAuthorId);

		    if(haveKey)
		    {
#ifdef GEN_EXCH_DEBUG
			    std::cerr << "  have ID key in cache: yes" << std::endl;
#endif

			    RsTlvPublicRSAKey authorKey;
			    bool auth_key_fetched = mGixs->getKey(metaData.mAuthorId, authorKey) ;

			    if (auth_key_fetched)
			    {

				    RsTlvKeySignature sign = metaData.signSet.keySignSet[INDEX_AUTHEN_IDENTITY];
				    idValidate = GxsSecurity::validateNxsGrp(*grp, sign, authorKey);

#ifdef GEN_EXCH_DEBUG
				    std::cerr << "  key ID validation result: " << idValidate << std::endl;
#endif
					mGixs->timeStampKey(metaData.mAuthorId,RsIdentityUsage(RsServiceType(mServType),RsIdentityUsage::GROUP_AUTHOR_SIGNATURE_VALIDATION,metaData.mGroupId));
			    }
			    else
			    {
				    std::cerr << "RsGenExchange::validateGrp()";
				    std::cerr << " ERROR Cannot Retrieve AUTHOR KEY for Group Sign Validation";
				    std::cerr << std::endl;
				    idValidate = false;
			    }

		    }else
		    {
#ifdef GEN_EXCH_DEBUG
			    std::cerr << "  have key in cache: no. Return VALIDATE_LATER" << std::endl;
			    std::cerr << "  requesting key " << metaData.mAuthorId << " to origin peer " << grp->PeerId() << std::endl;
#endif
			    std::list<RsPeerId> peers;
			    peers.push_back(grp->PeerId());
			    mGixs->requestKey(metaData.mAuthorId, peers,RsIdentityUsage(RsServiceType(mServType),RsIdentityUsage::GROUP_AUTHOR_SIGNATURE_VALIDATION,metaData.mGroupId));
			    return VALIDATE_FAIL_TRY_LATER;
		    }
	    }
	    else
	    {
#ifdef GEN_EXCH_DEBUG
		    std::cerr << "  (EE) Gixs not enabled while request identity signature validation!" << std::endl;
#endif
		    idValidate = false;
	    }
    }
    else
    {
	    idValidate = true;
    }

    if(idValidate)
	    return VALIDATE_SUCCESS;
    else
	    return VALIDATE_FAIL;

}

bool RsGenExchange::checkAuthenFlag(const PrivacyBitPos& pos, const uint8_t& flag) const
{
#ifdef GEN_EXCH_DEBUG
    std::cerr << "RsGenExchange::checkMsgAuthenFlag(pos: " << pos << " flag: ";
    std::cerr << (int) flag << " mAuthenPolicy: " << mAuthenPolicy << ")";
    std::cerr << std::endl;
#endif

    switch(pos)
    {
        case PUBLIC_GRP_BITS:
            return mAuthenPolicy & flag;
            break;
        case RESTRICTED_GRP_BITS:
            return flag & (mAuthenPolicy >> RESTR_GRP_OFFSET);
            break;
        case PRIVATE_GRP_BITS:
            return  flag & (mAuthenPolicy >> PRIV_GRP_OFFSET);
            break;
        case GRP_OPTION_BITS:
            return  flag & (mAuthenPolicy >> GRP_OPTIONS_OFFSET);
            break;
        default:
            std::cerr << "pos option not recognised";
            return false;
    }
}

static void addMessageChanged(std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &msgs, const std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &msgChanged)
{
    if (msgs.empty()) {
        msgs = msgChanged;
    } else {
        for (auto mapIt = msgChanged.begin(); mapIt != msgChanged.end(); ++mapIt)
        {
            const RsGxsGroupId &grpId = mapIt->first;
            const std::set<RsGxsMessageId> &srcMsgIds = mapIt->second;
            std::set<RsGxsMessageId> &destMsgIds = msgs[grpId];

            for (auto msgIt = srcMsgIds.begin(); msgIt != srcMsgIds.end(); ++msgIt)
                destMsgIds.insert(*msgIt) ;
        }
    }
}

#ifdef TO_REMOVE
void RsGenExchange::receiveChanges(std::vector<RsGxsNotify*>& changes)
{
    std::cerr << "***********************************  RsGenExchange::receiveChanges()" << std::endl;
#ifdef GEN_EXCH_DEBUG
    std::cerr << "RsGenExchange::receiveChanges()" << std::endl;
#endif
	std::unique_ptr<RsGxsChanges> evt(new RsGxsChanges);
	evt->mServiceType = static_cast<RsServiceType>(mServType);

	RsGxsChanges& out = *evt;
    out.mService = getTokenService();

    // collect all changes in one GxsChanges object
    std::vector<RsGxsNotify*>::iterator vit = changes.begin();
    for(; vit != changes.end(); ++vit)
    {
        RsGxsNotify* n = *vit;
        RsGxsGroupChange* gc;
        RsGxsMsgChange* mc;
        RsGxsDistantSearchResultChange *gt;

		if((mc = dynamic_cast<RsGxsMsgChange*>(n)) != nullptr)
        {
            if (mc->metaChange())
                addMessageChanged(out.mMsgsMeta, mc->msgChangeMap);
            else
                addMessageChanged(out.mMsgs, mc->msgChangeMap);
        }
		else if((gc = dynamic_cast<RsGxsGroupChange*>(n)) != nullptr)
        {
            if(gc->metaChange())
                out.mGrpsMeta.splice(out.mGrpsMeta.end(), gc->mGrpIdList);
            else
                out.mGrps.splice(out.mGrps.end(), gc->mGrpIdList);
        }
		else if(( gt = dynamic_cast<RsGxsDistantSearchResultChange*>(n) ) != nullptr)
            out.mDistantSearchReqs.push_back(gt->mRequestId);
        else
			RsErr() << __PRETTY_FUNCTION__ << " Unknown changes type!" << std::endl;
        delete n;
    }
    changes.clear() ;

	if(rsEvents) rsEvents->postEvent(std::move(evt));
}
#endif

bool RsGenExchange::subscribeToGroup(uint32_t& token, const RsGxsGroupId& grpId, bool subscribe)
{
    if(subscribe)
        setGroupSubscribeFlags(token, grpId, GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED,
                               (GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED | GXS_SERV::GROUP_SUBSCRIBE_NOT_SUBSCRIBED));
    else
        setGroupSubscribeFlags(token, grpId, GXS_SERV::GROUP_SUBSCRIBE_NOT_SUBSCRIBED,
                               (GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED | GXS_SERV::GROUP_SUBSCRIBE_NOT_SUBSCRIBED));

	if(mNetService) mNetService->subscribeStatusChanged(grpId,subscribe);
#ifdef GEN_EXCH_DEBUG
    else
        std::cerr << "(EE) No mNetService in RsGenExchange for service 0x" << std::hex << mServType << std::dec << std::endl;
#endif

    return true;
}

bool RsGenExchange::getGroupStatistic(const uint32_t& token, GxsGroupStatistic& stats)
{
    return mDataAccess->getGroupStatistic(token, stats);
}

bool RsGenExchange::getServiceStatistic(const uint32_t& token, GxsServiceStatistic& stats)
{
    return mDataAccess->getServiceStatistic(token, stats);
}

bool RsGenExchange::getGroupList(const uint32_t &token, std::list<RsGxsGroupId> &groupIds)
{
	return mDataAccess->getGroupList(token, groupIds);

}

bool RsGenExchange::getMsgList(const uint32_t &token,
                               GxsMsgIdResult &msgIds)
{
	return mDataAccess->getMsgIdList(token, msgIds);
}

bool RsGenExchange::getMsgRelatedList(const uint32_t &token, MsgRelatedIdResult &msgIds)
{
    return mDataAccess->getMsgRelatedList(token, msgIds);
}
bool RsGenExchange::getPublishedMsgMeta(const uint32_t& token,RsMsgMetaData& meta)
{
    auto it = mPublishedMsgs.find(token);

    if(it == mPublishedMsgs.end())
        return false ;

    meta = it->second;
    return true;
}
bool RsGenExchange::getPublishedGroupMeta(const uint32_t& token,RsGroupMetaData& meta)
{
    auto it = mPublishedGrps.find(token);

    if(it == mPublishedGrps.end())
        return false ;

    meta = it->second;
    return true;
}

bool RsGenExchange::getGroupMeta(const uint32_t &token, std::list<RsGroupMetaData> &groupInfo)
{
	std::list<const RsGxsGrpMetaData*> metaL;
	bool ok = mDataAccess->getGroupSummary(token, metaL);

	RsGroupMetaData m;

	for( auto lit = metaL.begin(); lit != metaL.end(); ++lit)
	{
		const RsGxsGrpMetaData& gMeta = *(*lit);

        m = gMeta;
        RsGroupNetworkStats sts ;

		if(mNetService != NULL && mNetService->getGroupNetworkStats(gMeta.mGroupId,sts))
		{
			m.mPop = sts.mSuppliers ;
			m.mVisibleMsgCount = sts.mMaxVisibleCount ;

			if((!(IS_GROUP_SUBSCRIBED(gMeta.mSubscribeFlags))) || gMeta.mLastPost == 0)
				m.mLastPost = sts.mLastGroupModificationTS ;
		}
		else
		{
			m.mPop= 0 ;
			m.mVisibleMsgCount = 0 ;
		}

		groupInfo.push_back(m);
	}

	return ok;
}

bool RsGenExchange::getMsgMeta(const uint32_t &token,
                               GxsMsgMetaMap &msgInfo)
{
#ifdef GEN_EXCH_DEBUG
	std::cerr << "RsGenExchange::getMsgMeta(): retrieving meta data for token " << token << std::endl;
#endif
	//std::list<RsGxsMsgMetaData*> metaL;
	GxsMsgMetaResult result;
	bool ok = mDataAccess->getMsgSummary(token, result);

	GxsMsgMetaResult::iterator mit = result.begin();

	for(; mit != result.end(); ++mit)
	{
		std::vector<const RsGxsMsgMetaData*>& metaV = mit->second;

		std::vector<RsMsgMetaData>& msgInfoV = msgInfo[mit->first];

		std::vector<const RsGxsMsgMetaData*>::iterator vit = metaV.begin();
		RsMsgMetaData meta;
		for(; vit != metaV.end(); ++vit)
		{
			const RsGxsMsgMetaData& m = *(*vit);
			meta = m;
			msgInfoV.push_back(meta);
			//delete *vit;
		}
		metaV.clear();
	}

	return ok;
}

bool RsGenExchange::getMsgRelatedMeta(const uint32_t &token, GxsMsgRelatedMetaMap &msgMeta)
{
        MsgRelatedMetaResult result;
        bool ok = mDataAccess->getMsgRelatedSummary(token, result);

        MsgRelatedMetaResult::iterator mit = result.begin();

        for(; mit != result.end(); ++mit)
        {
                std::vector<const RsGxsMsgMetaData*>& metaV = mit->second;

                std::vector<RsMsgMetaData>& msgInfoV = msgMeta[mit->first];

                std::vector<const RsGxsMsgMetaData*>::iterator vit = metaV.begin();
                RsMsgMetaData meta;
                for(; vit != metaV.end(); ++vit)
                {
                        const RsGxsMsgMetaData& m = *(*vit);
                        meta = m;
                        msgInfoV.push_back(meta);
                        //delete *vit;
                }
                metaV.clear();
        }

        return ok;
}

bool RsGenExchange::getSerializedGroupData(uint32_t token, RsGxsGroupId& id,
                                           unsigned char *& data,
                                           uint32_t& size)
{
	RS_STACK_MUTEX(mGenMtx) ;

	std::list<RsNxsGrp*> nxsGrps;

	if(!mDataAccess->getGroupData(token, nxsGrps))
        return false ;

    if(nxsGrps.size() != 1)
    {
        std::cerr << "(EE) getSerializedGroupData() got multiple groups in single request. This is unexpected." << std::endl;

        for(std::list<RsNxsGrp*>::const_iterator it(nxsGrps.begin());it!=nxsGrps.end();++it)
            delete *it ;

        return false ;
    }
	RsNxsGrp *nxs_grp = *(nxsGrps.begin());

    size = RsNxsSerialiser(mServType).size(nxs_grp);
    id = nxs_grp->metaData->mGroupId ;

    if(size > 1024*1024 || NULL==(data = (unsigned char *)rs_malloc(size)))
    {
        std::cerr << "(EE) getSerializedGroupData() cannot allocate mem chunk of size " << size << ". Too big, or no room." << std::endl;
        delete nxs_grp ;
        return false ;
    }

    return RsNxsSerialiser(mServType).serialise(nxs_grp,data,&size) ;
}

bool RsGenExchange::retrieveNxsIdentity(const RsGxsGroupId& group_id,RsNxsGrp *& identity_grp)
{
    RS_STACK_MUTEX(mGenMtx) ;

    std::map<RsGxsGroupId, RsNxsGrp*> grp;
    grp[group_id]=nullptr;
    std::map<RsGxsGroupId, RsNxsGrp*>::const_iterator grp_it;

    if(! mDataStore->retrieveNxsGrps(grp, true,true) || grp.end()==(grp_it=grp.find(group_id)) || !grp_it->second)
    {
        std::cerr << "(EE) Cannot retrieve group data for group " << group_id << " in service " << mServType << std::endl;
        return false;
    }

    identity_grp = grp_it->second;
    return true;
}

bool RsGenExchange::deserializeGroupData(unsigned char *data, uint32_t size,
                                         RsGxsGroupId* gId /*= nullptr*/)
{
	RS_STACK_MUTEX(mGenMtx) ;

	RsItem *item = RsNxsSerialiser(mServType).deserialise(data, &size);

	RsNxsGrp *nxs_grp = dynamic_cast<RsNxsGrp*>(item);

	if(item == NULL)
	{
		std::cerr << "(EE) RsGenExchange::deserializeGroupData(): cannot "
		          << "deserialise this data. Something's wrong." << std::endl;
		delete item;
		return false;
	}

	if(mGrpPendingValidate.find(nxs_grp->grpId) != mGrpPendingValidate.end())
	{
		std::cerr << "(WW) Group " << nxs_grp->grpId << " is already pending validation. Not adding again." << std::endl;
		return true;
	}

	if(gId)
		*gId = nxs_grp->grpId;

	mGrpPendingValidate.insert(std::make_pair(nxs_grp->grpId, GxsPendingItem<RsNxsGrp*, RsGxsGroupId>(nxs_grp, nxs_grp->grpId,time(NULL))));

	return true;
}

bool RsGenExchange::getGroupData(const uint32_t &token, std::vector<RsGxsGrpItem *>& grpItem)
{
	std::list<RsNxsGrp*> nxsGrps;
	bool ok = mDataAccess->getGroupData(token, nxsGrps);

	std::list<RsNxsGrp*>::iterator lit = nxsGrps.begin();
#ifdef GEN_EXCH_DEBUG
	std::cerr << "RsGenExchange::getGroupData() RsNxsGrp::len: " << nxsGrps.size();
    std::cerr << std::endl;
#endif

	if(ok)
	{
		for(; lit != nxsGrps.end(); ++lit)
		{
			RsTlvBinaryData& data = (*lit)->grp;
			RsItem* item = NULL;

			if(data.bin_len != 0)
				item = mSerialiser->deserialise(data.bin_data, &data.bin_len);

			if(item)
			{
				RsGxsGrpItem* gItem = dynamic_cast<RsGxsGrpItem*>(item);
				if (gItem)
				{
					gItem->meta = *((*lit)->metaData);

					RsGroupNetworkStats sts ;
					if(mNetService && mNetService->getGroupNetworkStats(gItem->meta.mGroupId,sts))
					{
						gItem->meta.mPop = sts.mSuppliers;
						gItem->meta.mVisibleMsgCount  = sts.mMaxVisibleCount;

						// When the group is not subscribed, the last post value is not updated, because there's no message stored. As a consequence,
						// we rely on network statistics to give this value, but it is not as accurate as if it was locally computed, because of blocked
						// posts, friends not available, sync delays, etc. Similarly if the group has just been subscribed, the last post info is probably
						// uninitialised, so we will it too.

						if((!(IS_GROUP_SUBSCRIBED(gItem->meta.mSubscribeFlags))) || gItem->meta.mLastPost == 0)
							gItem->meta.mLastPost = sts.mLastGroupModificationTS ;
					}
					else
					{
						gItem->meta.mPop = 0;
						gItem->meta.mVisibleMsgCount = 0;
					}


                    // Also check the group privacy flags. A while ago, it as possible to publish a group without privacy flags. Now it is not possible anymore.
                    // As a consequence, it's important to supply a correct value in this flag before the data can be edited/updated.

					if((gItem->meta.mGroupFlags & GXS_SERV::FLAG_PRIVACY_MASK) == 0)
                    {
#ifdef GEN_EXCH_DEBUG
						std::cerr << "(WW) getGroupData(): mGroupFlags for group " << gItem->meta.mGroupId << " has incorrect value " << std::hex << gItem->meta.mGroupFlags << std::dec << ". Setting value to GXS_SERV::FLAG_PRIVACY_PUBLIC." << std::endl;
#endif
                        gItem->meta.mGroupFlags |=  GXS_SERV::FLAG_PRIVACY_PUBLIC;
					}

					grpItem.push_back(gItem);
				}
				else
				{
					std::cerr << "RsGenExchange::getGroupData() deserialisation/dynamic_cast ERROR";
					std::cerr << std::endl;
					delete item;
				}
			}
			else if(data.bin_len > 0)
				//std::cerr << "(EE) RsGenExchange::getGroupData() Item type is probably not handled. Data is: " << RsUtil::BinToHex((unsigned char*)data.bin_data,std::min(50u,data.bin_len)) << ((data.bin_len>50)?"...":"") << std::endl;
				std::cerr << "(EE) RsGenExchange::getGroupData() Item type is probably not handled. Data is: " << RsUtil::BinToHex((unsigned char*)data.bin_data,data.bin_len) << std::endl;

			delete *lit;
		}
	}
	return ok;
}

bool RsGenExchange::getMsgData(uint32_t token, GxsMsgDataMap &msgItems)
{
	RS_STACK_MUTEX(mGenMtx) ;
	NxsMsgDataResult msgResult;
	bool ok = mDataAccess->getMsgData(token, msgResult);

	if(ok)
	{
		NxsMsgDataResult::iterator mit = msgResult.begin();
		for(; mit != msgResult.end(); ++mit)
		{
			const RsGxsGroupId& grpId = mit->first;
			std::vector<RsGxsMsgItem*>& gxsMsgItems = msgItems[grpId];
			std::vector<RsNxsMsg*>& nxsMsgsV = mit->second;
			std::vector<RsNxsMsg*>::iterator vit = nxsMsgsV.begin();
			for(; vit != nxsMsgsV.end(); ++vit)
			{
				RsNxsMsg*& msg = *vit;
				RsItem* item = NULL;

				if(msg->msg.bin_len != 0)
					item = mSerialiser->deserialise(msg->msg.bin_data, &msg->msg.bin_len);

				if (item)
				{
					RsGxsMsgItem* mItem = dynamic_cast<RsGxsMsgItem*>(item);
					if (mItem)
					{
						mItem->meta = *((*vit)->metaData); // get meta info from nxs msg
						gxsMsgItems.push_back(mItem);
					}
					else
					{
						std::cerr << "RsGenExchange::getMsgData() deserialisation/dynamic_cast ERROR";
						std::cerr << std::endl;
						delete item;
					}
				}
				else
				{
					std::cerr << "RsGenExchange::getMsgData() deserialisation ERROR";
					std::cerr << std::endl;
				}
				delete msg;
			}
		}
	}
	return ok;
}

bool RsGenExchange::getMsgRelatedData( uint32_t token,
                                       GxsMsgRelatedDataMap &msgItems )
{
	RS_STACK_MUTEX(mGenMtx) ;
    NxsMsgRelatedDataResult msgResult;
    bool ok = mDataAccess->getMsgRelatedData(token, msgResult);

    if(ok)
    {
    	NxsMsgRelatedDataResult::iterator mit = msgResult.begin();
        for(; mit != msgResult.end(); ++mit)
        {
            const RsGxsGrpMsgIdPair& msgId = mit->first;
            std::vector<RsGxsMsgItem*> &gxsMsgItems = msgItems[msgId];
            std::vector<RsNxsMsg*>& nxsMsgsV = mit->second;
            std::vector<RsNxsMsg*>::iterator vit = nxsMsgsV.begin();
            for(; vit != nxsMsgsV.end(); ++vit)
            {
                RsNxsMsg*& msg = *vit;
                RsItem* item = NULL;

                if(msg->msg.bin_len != 0)
                	item = mSerialiser->deserialise(msg->msg.bin_data,
                                &msg->msg.bin_len);
				if (item)
				{
					RsGxsMsgItem* mItem = dynamic_cast<RsGxsMsgItem*>(item);

					if (mItem)
					{
							mItem->meta = *((*vit)->metaData); // get meta info from nxs msg
							gxsMsgItems.push_back(mItem);
					}
					else
					{
						std::cerr << "RsGenExchange::getMsgRelatedData() deserialisation/dynamic_cast ERROR";
						std::cerr << std::endl;
						delete item;
					}
				}
				else
				{
					std::cerr << "RsGenExchange::getMsgRelatedData() deserialisation ERROR";
					std::cerr << std::endl;
				}

                delete msg;
            }
        }
    }
    return ok;
}

RsTokenService* RsGenExchange::getTokenService()
{
    return mDataAccess;
}


bool RsGenExchange::setAuthenPolicyFlag(const uint8_t &msgFlag, uint32_t& authenFlag, const PrivacyBitPos &pos)
{
    uint32_t temp = 0;
    temp = msgFlag;

    switch(pos)
    {
        case PUBLIC_GRP_BITS:
            authenFlag &= ~PUB_GRP_MASK;
            authenFlag |= temp;
            break;
        case RESTRICTED_GRP_BITS:
            authenFlag &= ~RESTR_GRP_MASK;
            authenFlag |= (temp << RESTR_GRP_OFFSET);
            break;
        case PRIVATE_GRP_BITS:
            authenFlag &= ~PRIV_GRP_MASK;
            authenFlag |= (temp << PRIV_GRP_OFFSET);
            break;
        case GRP_OPTION_BITS:
            authenFlag &= ~GRP_OPTIONS_MASK;
            authenFlag |= (temp << GRP_OPTIONS_OFFSET);
            break;
        default:
            std::cerr << "pos option not recognised";
            return false;
    }
    return true;
}

void RsGenExchange::receiveNewGroups(const std::vector<RsNxsGrp *> &groups)
{
	RS_STACK_MUTEX(mGenMtx) ;

    auto vit = groups.begin();

    // store these for tick() to pick them up
    for(; vit != groups.end(); ++vit)
    {
    	RsNxsGrp* grp = *vit;
    	NxsGrpPendValidVect::iterator received = mGrpPendingValidate.find(grp->grpId);

    	// drop group if you already have them
    	// TODO: move this to nxs layer to save bandwidth
    	if(received == mGrpPendingValidate.end())
    	{
#ifdef GEN_EXCH_DEBUG
		std::cerr << "RsGenExchange::notifyNewGroups() Received GrpId: " << grp->grpId;
		std::cerr << std::endl;
#endif

    		mGrpPendingValidate.insert(std::make_pair(grp->grpId, GxsPendingItem<RsNxsGrp*, RsGxsGroupId>(grp, grp->grpId,time(NULL))));
    	}
    	else
    	{
    		delete grp;
    	}
    }

}


void RsGenExchange::receiveNewMessages(const std::vector<RsNxsMsg *>& messages)
{
	RS_STACK_MUTEX(mGenMtx) ;

	// store these for tick() to pick them up

	for(uint32_t i=0;i<messages.size();++i)
	{
		RsNxsMsg* msg = messages[i];
		NxsMsgPendingVect::iterator it = mMsgPendingValidate.find(msg->msgId) ;

		// if we have msg already just delete it
		if(it == mMsgPendingValidate.end())
		{
#ifdef GEN_EXCH_DEBUG
			std::cerr << "RsGenExchange::notifyNewMessages() Received Msg: ";
			std::cerr << " GrpId: " << msg->grpId;
			std::cerr << " MsgId: " << msg->msgId;
			std::cerr << std::endl;
#endif
			RsGxsGrpMsgIdPair id;
			id.first = msg->grpId;
			id.second = msg->msgId;

			mMsgPendingValidate.insert(std::make_pair(msg->msgId,GxsPendingItem<RsNxsMsg*, RsGxsGrpMsgIdPair>(msg, id,time(NULL))));
		}
		else
		{
#ifdef GEN_EXCH_DEBUG
			std::cerr << "  message is already in pending validation list. dropping." << std::endl;
#endif
			delete msg;
		}
	}
}

void RsGenExchange::receiveDistantSearchResults(TurtleRequestId id,const RsGxsGroupId &grpId)
{
	std::cerr << __PRETTY_FUNCTION__ << " received result for request " << std::hex << id << std::dec << ": this method should be overloaded in the client service, but it is not. This is a bug!" << std::endl;
}

void RsGenExchange::notifyReceivePublishKey(const RsGxsGroupId &grpId)
{
	RS_STACK_MUTEX(mGenMtx);

	RsGxsGroupChange* gc = new RsGxsGroupChange(RsGxsNotify::TYPE_RECEIVED_PUBLISHKEY,grpId, true);
	mNotifications.push_back(gc);
}

void RsGenExchange::notifyChangedGroupSyncParams(const RsGxsGroupId &grpId)
{
    RS_STACK_MUTEX(mGenMtx);

    RsGxsGroupChange* gc = new RsGxsGroupChange(RsGxsNotify::TYPE_GROUP_SYNC_PARAMETERS_UPDATED,grpId, false);

    mNotifications.push_back(gc);
}
void RsGenExchange::notifyChangedGroupStats(const RsGxsGroupId &grpId)
{
	RS_STACK_MUTEX(mGenMtx);

	RsGxsGroupChange* gc = new RsGxsGroupChange(RsGxsNotify::TYPE_STATISTICS_CHANGED,grpId, false);

	mNotifications.push_back(gc);
}

bool RsGenExchange::checkGroupMetaConsistency(const RsGroupMetaData& meta)
{
    std::cerr << "Checking group consistency:" << std::endl;

    if(meta.mGroupName.empty())
    {
        std::cerr << "(EE) cannot create a group with no name." << std::endl;
        return false;
    }

    uint32_t gf = meta.mGroupFlags & GXS_SERV::FLAG_PRIVACY_MASK ;

    if(gf != GXS_SERV::FLAG_PRIVACY_PUBLIC && gf != GXS_SERV::FLAG_PRIVACY_RESTRICTED && gf != GXS_SERV::FLAG_PRIVACY_PRIVATE)
    {
        std::cerr << "(EE) mGroupFlags has incorrect value " << std::hex << meta.mGroupFlags << std::dec << ". A value among GXS_SERV::FLAG_PRIVACY_{PUBLIC,RESTRICTED,PRIVATE} is expected." << std::endl;
        return false ;
    }

    if(meta.mCircleType < GXS_CIRCLE_TYPE_PUBLIC || meta.mCircleType > GXS_CIRCLE_TYPE_YOUR_EYES_ONLY)
    {
        std::cerr << "(EE) mCircleType has incorrect value " << std::hex << meta.mCircleType << std::dec << ". A single value among GXS_CIRCLE_TYPE_{PUBLIC,EXTERNAL,YOUR_FRIENDS_ONLY,LOCAL,EXT_SELF,YOUR_EYES_ONLY} is expected." << std::endl;
        return false ;
    }

    if(meta.mCircleType == GXS_CIRCLE_TYPE_EXTERNAL)
    {
		if(!meta.mInternalCircle.isNull())
        {
            std::cerr << "(EE) Group circle type is EXTERNAL, but an internal circle ID " << meta.mInternalCircle << " was supplied. This is an error." << std::endl;
            return false ;
        }
		if(meta.mCircleId.isNull())
        {
            std::cerr << "(EE) Group circle type is EXTERNAL, but no external circle ID was supplied. meta.mCircleId is indeed empty. This is an error." << std::endl;
            return false ;
        }
    }

    if(meta.mCircleType == GXS_CIRCLE_TYPE_YOUR_FRIENDS_ONLY)
    {
        if(!meta.mCircleId.isNull())
        {
            std::cerr << "(EE) Group circle type is YOUR_FRIENDS_ONLY, but an external circle ID " << meta.mCircleId << " was supplied. This is an error." << std::endl;
            return false ;
        }
		if(meta.mInternalCircle.isNull())
        {
            std::cerr << "(EE) Group circle type is YOUR_FRIENDS_ONLY, but no internal circle ID was supplied. meta.mInternalCircle is indeed empty. This is an error." << std::endl;
            return false ;
        }
    }

    if(meta.mCircleType == GXS_CIRCLE_TYPE_EXT_SELF)
    {
        if(!meta.mCircleId.isNull())
        {
            std::cerr << "(EE) Group circle type is EXT_SELF, but an external circle ID " << meta.mCircleId << " was supplied. This is an error." << std::endl;
            return false ;
        }
		if(!meta.mInternalCircle.isNull())
        {
            std::cerr << "(EE) Group circle type is EXT_SELF, but an internal circle ID " << meta.mInternalCircle << " was supplied. This is an error." << std::endl;
            return false ;
        }
    }

    std::cerr << "Group is clean." << std::endl;
    return true ;
}

void RsGenExchange::publishGroup(uint32_t& token, RsGxsGrpItem *grpItem)
{
    if(!checkGroupMetaConsistency(grpItem->meta))
    {
        std::cerr << "(EE) Cannot publish group. Some information was not supplied." << std::endl;
       return ;
    }

	RS_STACK_MUTEX(mGenMtx) ;
    token = mDataAccess->generatePublicToken();
    GxsGrpPendingSign ggps(grpItem, token);
    mGrpsToPublish.push_back(ggps);

#ifdef GEN_EXCH_DEBUG
    std::cerr << "RsGenExchange::publishGroup() token: " << token;
    std::cerr << std::endl;
#endif

}


void RsGenExchange::updateGroup(uint32_t& token, RsGxsGrpItem* grpItem)
{
	if(!checkGroupMetaConsistency(grpItem->meta))
	{
		std::cerr << "(EE) Cannot update group. Some information was not supplied." << std::endl;
		delete grpItem;
		return ;
	}

	RS_STACK_MUTEX(mGenMtx) ;
	token = mDataAccess->generatePublicToken();
	mGroupUpdatePublish.push_back(GroupUpdatePublish(grpItem, token));

#ifdef GEN_EXCH_DEBUG
	std::cerr << "RsGenExchange::updateGroup() token: " << token;
	std::cerr << std::endl;
#endif
}

void RsGenExchange::deleteGroup(uint32_t& token, const RsGxsGroupId& grpId)
{
	RS_STACK_MUTEX(mGenMtx) ;
	token = mDataAccess->generatePublicToken();
	mGroupDeletePublish.push_back(GroupDeletePublish(grpId, token));

#ifdef GEN_EXCH_DEBUG
	std::cerr << "RsGenExchange::deleteGroup() token: " << token;
	std::cerr << std::endl;
#endif
}
void RsGenExchange::deleteMsgs(uint32_t& token, const GxsMsgReq& msgs)
{
	RS_STACK_MUTEX(mGenMtx) ;

	token = mDataAccess->generatePublicToken();
	mMsgDeletePublish.push_back(MsgDeletePublish(msgs, token));

	// This code below will suspend any requests of the deleted messages for 24 hrs. This of course only works
	// if all friend nodes consistently delete the messages in the mean time.

	if(mNetService != NULL)
		for(GxsMsgReq::const_iterator it(msgs.begin());it!=msgs.end();++it)
			for(auto it2(it->second.begin());it2!=it->second.end();++it2)
				mNetService->rejectMessage(*it2);
}

void RsGenExchange::publishMsg(uint32_t& token, RsGxsMsgItem *msgItem)
{
					RS_STACK_MUTEX(mGenMtx) ;
    token = mDataAccess->generatePublicToken();
    mMsgsToPublish.insert(std::make_pair(token, msgItem));

#ifdef GEN_EXCH_DEBUG	
    std::cerr << "RsGenExchange::publishMsg() token: " << token;
    std::cerr << std::endl;
#endif

}

uint32_t RsGenExchange::getDefaultSyncPeriod()
{
	RS_STACK_MUTEX(mGenMtx) ;

	if(mNetService != NULL)
        return mNetService->getDefaultSyncAge();
    else
    {
        std::cerr << "(EE) No network service available. Cannot get default sync period. " << std::endl;
        return 0;
    }
}

RsReputationLevel RsGenExchange::minReputationForForwardingMessages(
        uint32_t group_sign_flags, uint32_t identity_sign_flags )
{
	return RsNetworkExchangeService::minReputationForForwardingMessages(group_sign_flags,identity_sign_flags);
}
uint32_t RsGenExchange::getSyncPeriod(const RsGxsGroupId& grpId)
{
	RS_STACK_MUTEX(mGenMtx) ;

	if(mNetService != NULL)
        return mNetService->getSyncAge(grpId);
    else
        return RS_GXS_DEFAULT_MSG_REQ_PERIOD;
}

bool     RsGenExchange::getGroupNetworkStats(const RsGxsGroupId& grpId,RsGroupNetworkStats& stats)
{
	return (!mNetService) || mNetService->getGroupNetworkStats(grpId,stats) ;
}

void     RsGenExchange::setSyncPeriod(const RsGxsGroupId& grpId,uint32_t age_in_secs)
{
	if(mNetService != NULL)
        return mNetService->setSyncAge(grpId,age_in_secs) ;
    else
        std::cerr << "(EE) No network service available. Cannot set storage period. " << std::endl;
}

uint32_t RsGenExchange::getStoragePeriod(const RsGxsGroupId& grpId)
{
	RS_STACK_MUTEX(mGenMtx) ;

	if(!mNetService)
	{
#ifdef GEN_EXCH_DEBUG
		std::cerr << "(EE) No network service in service " << std::hex  << serviceType() << std::dec << ": cannot read message storage time. Returning infinity." << std::endl;
#endif
		return false ;
	}
	return mNetService->getKeepAge(grpId) ;
}
void     RsGenExchange::setStoragePeriod(const RsGxsGroupId& grpId,uint32_t age_in_secs)
{
	if(mNetService != NULL)
        return mNetService->setKeepAge(grpId,age_in_secs) ;
    else
        std::cerr << "(EE) No network service available. Cannot set storage period. " << std::endl;
}

void RsGenExchange::setGroupSubscribeFlags(uint32_t& token, const RsGxsGroupId& grpId, const uint32_t& flag, const uint32_t& mask)
{
	/* TODO APPLY MASK TO FLAGS */
					RS_STACK_MUTEX(mGenMtx) ;
    token = mDataAccess->generatePublicToken();

    GrpLocMetaData g;
    g.grpId = grpId;
    g.val.put(RsGeneralDataService::GRP_META_SUBSCRIBE_FLAG, (int32_t)flag);
    g.val.put(RsGeneralDataService::GRP_META_SUBSCRIBE_FLAG+GXS_MASK, (int32_t)mask); // HACK, need to perform mask operation in a non-blocking location
    mGrpLocMetaMap.insert(std::make_pair(token, g));
}

void RsGenExchange::setGroupStatusFlags(uint32_t& token, const RsGxsGroupId& grpId, const uint32_t& status, const uint32_t& mask)
{
	/* TODO APPLY MASK TO FLAGS */
					RS_STACK_MUTEX(mGenMtx) ;
    token = mDataAccess->generatePublicToken();

    GrpLocMetaData g;
    g.grpId = grpId;
    g.val.put(RsGeneralDataService::GRP_META_STATUS, (int32_t)status);
    g.val.put(RsGeneralDataService::GRP_META_STATUS+GXS_MASK, (int32_t)mask); // HACK, need to perform mask operation in a non-blocking location
    mGrpLocMetaMap.insert(std::make_pair(token, g));
}


void RsGenExchange::setGroupServiceString(uint32_t& token, const RsGxsGroupId& grpId, const std::string& servString)
{
	RS_STACK_MUTEX(mGenMtx);
    token = mDataAccess->generatePublicToken();

    GrpLocMetaData g;
    g.grpId = grpId;
    g.val.put(RsGeneralDataService::GRP_META_SERV_STRING, servString);
    mGrpLocMetaMap.insert(std::make_pair(token, g));
}

void RsGenExchange::setMsgStatusFlags(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, const uint32_t& status, const uint32_t& mask)
{
	/* TODO APPLY MASK TO FLAGS */
					RS_STACK_MUTEX(mGenMtx) ;
    token = mDataAccess->generatePublicToken();

    MsgLocMetaData m;
    m.val.put(RsGeneralDataService::MSG_META_STATUS, (int32_t)status);
    m.val.put(RsGeneralDataService::MSG_META_STATUS+GXS_MASK, (int32_t)mask); // HACK, need to perform mask operation in a non-blocking location
    m.msgId = msgId;
    mMsgLocMetaMap.insert(std::make_pair(token, m));
}

void RsGenExchange::setMsgServiceString(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, const std::string& servString )
{
					RS_STACK_MUTEX(mGenMtx) ;
    token = mDataAccess->generatePublicToken();

    MsgLocMetaData m;
    m.val.put(RsGeneralDataService::MSG_META_SERV_STRING, servString);
    m.msgId = msgId;
    mMsgLocMetaMap.insert(std::make_pair(token, m));
}

void RsGenExchange::processMsgMetaChanges()
{
    std::map<uint32_t,  MsgLocMetaData> metaMap;

    {
        RS_STACK_MUTEX(mGenMtx);
        if (mMsgLocMetaMap.empty())
        {
            return;
        }
        metaMap = mMsgLocMetaMap;
        mMsgLocMetaMap.clear();
    }

    GxsMsgReq msgIds;

    std::map<uint32_t, MsgLocMetaData>::iterator mit;
    for (mit = metaMap.begin(); mit != metaMap.end(); ++mit)
    {
        MsgLocMetaData& m = mit->second;

		int32_t value, mask;
        bool ok = true;
        bool changed = false;

        // for meta flag changes get flag to apply mask
        if(m.val.getAsInt32(RsGeneralDataService::MSG_META_STATUS, value))
        {
            ok = false;
            if(m.val.getAsInt32(RsGeneralDataService::MSG_META_STATUS+GXS_MASK, mask))
            {
                GxsMsgReq req;
                std::set<RsGxsMessageId> msgIdV;
                msgIdV.insert(m.msgId.second);
                req.insert(std::make_pair(m.msgId.first, msgIdV));
                GxsMsgMetaResult result;
                mDataStore->retrieveGxsMsgMetaData(req, result);
                GxsMsgMetaResult::iterator mit = result.find(m.msgId.first);

                if(mit != result.end())
                {
                    std::vector<const RsGxsMsgMetaData*>& msgMetaV = mit->second;

                    if(!msgMetaV.empty())
                    {
                        const RsGxsMsgMetaData* meta = *(msgMetaV.begin());
                        value = (meta->mMsgStatus & ~mask) | (mask & value);
						changed = (static_cast<int64_t>(meta->mMsgStatus) != value);
                        m.val.put(RsGeneralDataService::MSG_META_STATUS, value);
                        //delete meta;
                        ok = true;
                    }
                }
                m.val.removeKeyValue(RsGeneralDataService::MSG_META_STATUS+GXS_MASK);
            }
        }

        ok &= mDataStore->updateMessageMetaData(m) == 1;
        uint32_t token = mit->first;

        if(ok)
        {
            mDataAccess->updatePublicRequestStatus(token, RsTokenService::COMPLETE);
            if (changed)
            {
                msgIds[m.msgId.first].insert(m.msgId.second);
            }
        }
        else
        {
            mDataAccess->updatePublicRequestStatus(token, RsTokenService::FAILED);
        }

        {
            RS_STACK_MUTEX(mGenMtx);
            mMsgNotify.insert(std::make_pair(token, m.msgId));
        }
    }

    if (!msgIds.empty())
    {
        RS_STACK_MUTEX(mGenMtx);

        for(auto it(msgIds.begin());it!=msgIds.end();++it)
            for(auto& msg_id:it->second)
				mNotifications.push_back(new RsGxsMsgChange(RsGxsNotify::TYPE_PROCESSED, it->first, msg_id, false));
    }
}

void RsGenExchange::processGrpMetaChanges()
{
    std::map<uint32_t, GrpLocMetaData > metaMap;

    {
        RS_STACK_MUTEX(mGenMtx);
        if (mGrpLocMetaMap.empty())
        {
            return;
        }
        metaMap = mGrpLocMetaMap;
        mGrpLocMetaMap.clear();
    }

    std::list<RsGxsGroupId> grpChanged;

    std::map<uint32_t, GrpLocMetaData>::iterator mit;
    for (mit = metaMap.begin(); mit != metaMap.end(); ++mit)
    {
        GrpLocMetaData& g = mit->second;
        uint32_t token = mit->first;

        // process mask
        bool ok = processGrpMask(g.grpId, g.val);

        ok = ok && (mDataStore->updateGroupMetaData(g) == 1);

        if(ok)
        {
            mDataAccess->updatePublicRequestStatus(token, RsTokenService::COMPLETE);
            grpChanged.push_back(g.grpId);
        }else
        {
            mDataAccess->updatePublicRequestStatus(token, RsTokenService::FAILED);
        }

        {
            RS_STACK_MUTEX(mGenMtx);
            mGrpNotify.insert(std::make_pair(token, g.grpId));
        }
    }

    for(auto& groupId:grpChanged)
    {
        RS_STACK_MUTEX(mGenMtx);

        mNotifications.push_back(new RsGxsGroupChange(RsGxsNotify::TYPE_PROCESSED,groupId, true));
    }
}

bool RsGenExchange::processGrpMask(const RsGxsGroupId& grpId, ContentValue &grpCv)
{
    // first find out which mask is involved
    int32_t value, mask, currValue;
    std::string key;
    const RsGxsGrpMetaData* grpMeta = NULL;
    bool ok = false;

	RsGxsGrpMetaTemporaryMap grpMetaMap;
    std::map<RsGxsGroupId, RsGxsGrpMetaData* >::iterator mit;
    grpMetaMap.insert(std::make_pair(grpId, (RsGxsGrpMetaData*)(NULL)));

    mDataStore->retrieveGxsGrpMetaData(grpMetaMap);

    if((mit = grpMetaMap.find(grpId)) != grpMetaMap.end())
    {
        grpMeta = mit->second;

        if (!grpMeta)
        {
#ifdef GEN_EXCH_DEBUG
            std::cerr << "RsGenExchange::processGrpMask() Ignore update for not existing grp id " << grpId.toStdString();
            std::cerr << std::endl;
#endif
            return false;
        }
        ok = true;
    }

    if(grpCv.getAsInt32(RsGeneralDataService::GRP_META_STATUS, value) && grpMeta)
    {
        key = RsGeneralDataService::GRP_META_STATUS;
        currValue = grpMeta->mGroupStatus;
    }
    else if(grpCv.getAsInt32(RsGeneralDataService::GRP_META_SUBSCRIBE_FLAG, value) && grpMeta)
    {
        key = RsGeneralDataService::GRP_META_SUBSCRIBE_FLAG;
        currValue = grpMeta->mSubscribeFlags;
    }
	else
        return !(grpCv.empty());

    ok &= grpCv.getAsInt32(key+GXS_MASK, mask);

    // remove mask entry so it doesn't affect actual entry
    grpCv.removeKeyValue(key+GXS_MASK);

    // apply mask to current value
    value = (currValue & ~mask) | (value & mask);

    grpCv.put(key, value);

    return ok;
}

void RsGenExchange::publishMsgs()
{

	RS_STACK_MUTEX(mGenMtx) ;

	rstime_t now = time(NULL);

	// stick back msgs pending signature
	typedef std::map<uint32_t, GxsPendingItem<RsGxsMsgItem*, uint32_t> > PendSignMap;

	PendSignMap::iterator sign_it = mMsgPendingSign.begin();

	for(; sign_it != mMsgPendingSign.end(); ++sign_it)
	{
		GxsPendingItem<RsGxsMsgItem*, uint32_t>& item = sign_it->second;
		mMsgsToPublish.insert(std::make_pair(sign_it->first, item.mItem));
	}

	std::map<RsGxsGroupId, std::list<RsGxsMsgItem*> > msgChangeMap;
	std::map<uint32_t, RsGxsMsgItem*>::iterator mit = mMsgsToPublish.begin();

	for(; mit != mMsgsToPublish.end(); ++mit)
	{
#ifdef GEN_EXCH_DEBUG
		std::cerr << "RsGenExchange::publishMsgs() Publishing a Message";
		std::cerr << std::endl;
#endif

		RsGxsMsgItem* msgItem = mit->second;
		uint32_t token = mit->first;

		uint32_t size = mSerialiser->size(msgItem);
		char* mData = new char[size];

		// for fatal sign creation
		bool createOk = false;

		// if sign requests to try later
		bool tryLater = false;

		bool serialOk = mSerialiser->serialise(msgItem, mData, &size);

		if(serialOk)
		{
			RsNxsMsg* msg = new RsNxsMsg(mServType);
			msg->grpId = msgItem->meta.mGroupId;

			msg->msg.setBinData(mData, size);

			// now create meta
			msg->metaData = new RsGxsMsgMetaData();
			*(msg->metaData) = msgItem->meta;

			// assign time stamp
			msg->metaData->mPublishTs = time(NULL);

			// now intialise msg (sign it)
			uint8_t createReturn = createMessage(msg);

			if(createReturn == CREATE_FAIL)
			{
				createOk = false;
			}
			else if(createReturn == CREATE_FAIL_TRY_LATER)
			{
				PendSignMap::iterator pit = mMsgPendingSign.find(token);
				tryLater = true;

				// add to queue of messages waiting for a successful
				// sign attempt
				if(pit == mMsgPendingSign.end())
				{
					GxsPendingItem<RsGxsMsgItem*, uint32_t> gsi(msgItem, token,time(NULL));
					mMsgPendingSign.insert(std::make_pair(token, gsi));
				}
				else
				{
					// remove from attempts queue if over sign
					// attempts limit
					if(pit->second.mFirstTryTS + SIGN_MAX_WAITING_TIME < now)
					{
						std::cerr << "Pending signature grp=" << pit->second.mItem->meta.mGroupId << ", msg=" << pit->second.mItem->meta.mMsgId << ", has exceeded validation time limit. The author's key can probably not be obtained. This is unexpected." << std::endl;

						mMsgPendingSign.erase(token);
						tryLater = false;
					}
				}

				createOk = false;
			}
			else if(createReturn == CREATE_SUCCESS)
			{
				createOk = true;

				// erase from queue if it exists
				mMsgPendingSign.erase(token);
			}
			else // unknown return, just fail
				createOk = false;



			RsGxsMessageId msgId;
			RsGxsGroupId grpId = msgItem->meta.mGroupId;

			bool validSize = false;

			// check message not over single msg storage limit
			if(createOk)
			{
				validSize = mDataStore->validSize(msg);
			}

			if(createOk && validSize)
			{
				// empty orig msg id means this is the original
				// msg.
                // (csoler) Why are we doing this???

				if(msg->metaData->mOrigMsgId.isNull())
				{
					msg->metaData->mOrigMsgId = msg->metaData->mMsgId;
				}

				// now serialise meta data
				size = msg->metaData->serial_size();

				char* metaDataBuff = new char[size];
				bool s = msg->metaData->serialise(metaDataBuff, &size);
				s &= msg->meta.setBinData(metaDataBuff, size);
				if (!s)
					std::cerr << "(WW) Can't serialise or set bin data" << std::endl;

				msg->metaData->mMsgStatus = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;
				msgId = msg->msgId;
				grpId = msg->grpId;
				msg->metaData->recvTS = time(NULL);
                
                // FIXTESTS global variable rsPeers not available in unittests!
                if(rsPeers)
                    mRoutingClues[msg->metaData->mAuthorId].insert(rsPeers->getOwnId()) ;
                
				computeHash(msg->msg, msg->metaData->mHash);
				mDataAccess->addMsgData(msg);

                mPublishedMsgs[token] = *msg->metaData;

                RsGxsMsgItem *msg_item = dynamic_cast<RsGxsMsgItem*>(mSerialiser->deserialise(msg->msg.bin_data,&msg->msg.bin_len)) ;
                msg_item->meta = *msg->metaData;

				delete msg ;

				msgChangeMap[grpId].push_back(msg_item);

				delete[] metaDataBuff;

                if(mNetService != NULL)
                    mNetService->stampMsgServerUpdateTS(grpId) ;

				// add to published to allow acknowledgement
				mMsgNotify.insert(std::make_pair(mit->first, std::make_pair(grpId, msgId)));
				mDataAccess->updatePublicRequestStatus(mit->first, RsTokenService::COMPLETE);

			}
			else
			{
				// delete msg if create msg not ok
				delete msg;

				if(!tryLater)
					mDataAccess->updatePublicRequestStatus(mit->first,
							RsTokenService::FAILED);

				std::cerr << "RsGenExchange::publishMsgs() failed to publish msg " << std::endl;
			}
		}
		else
		{
			std::cerr << "RsGenExchange::publishMsgs() failed to serialise msg " << std::endl;
		}

		delete[] mData;

		if(!tryLater)
			delete msgItem;
	}

	// clear msg item map as we're done publishing them and all
	// entries are invalid
	mMsgsToPublish.clear();

    for(auto it(msgChangeMap.begin());it!=msgChangeMap.end();++it)
        for(auto& msg_item: it->second)
		{
			RsGxsMsgChange* ch = new RsGxsMsgChange(RsGxsNotify::TYPE_PUBLISHED,msg_item->meta.mGroupId, msg_item->meta.mMsgId, false);
			ch->mNewMsgItem = msg_item;

			mNotifications.push_back(ch);
		}
}

RsGenExchange::ServiceCreate_Return RsGenExchange::service_CreateGroup(RsGxsGrpItem* /* grpItem */,
		RsTlvSecurityKeySet& /* keySet */)
{
#ifdef GEN_EXCH_DEBUG
	std::cerr << "RsGenExchange::service_CreateGroup(): Does nothing"
			  << std::endl;
#endif
	return SERVICE_CREATE_SUCCESS;
}


#define PENDING_SIGN_TIMEOUT 10 //  5 seconds


void RsGenExchange::processGroupUpdatePublish()
{
	RS_STACK_MUTEX(mGenMtx) ;

	// get keys for group update publish

	// first build meta request map for groups to be updated
	RsGxsGrpMetaTemporaryMap grpMeta;

	for(auto vit = mGroupUpdatePublish.begin(); vit != mGroupUpdatePublish.end(); ++vit)
	{
		GroupUpdatePublish& gup = *vit;
		const RsGxsGroupId& groupId = gup.grpItem->meta.mGroupId;
		grpMeta.insert(std::make_pair(groupId, (RsGxsGrpMetaData*)(NULL)));
	}

	if(grpMeta.empty())
		return;

	mDataStore->retrieveGxsGrpMetaData(grpMeta);

	// now
	for(auto vit = mGroupUpdatePublish.begin(); vit != mGroupUpdatePublish.end(); ++vit)
	{
		GroupUpdatePublish& gup = *vit;
		const RsGxsGroupId& groupId = gup.grpItem->meta.mGroupId;
		std::map<RsGxsGroupId, RsGxsGrpMetaData*>::iterator mit = grpMeta.find(groupId);

		const RsGxsGrpMetaData* meta = NULL;
		if(mit == grpMeta.end() || mit->second == NULL)
		{
			std::cerr << "Error! could not find meta of old group to update!" << std::endl;
			mDataAccess->updatePublicRequestStatus(gup.mToken, RsTokenService::FAILED);
			delete gup.grpItem;
			continue;
		}
		else
			meta = mit->second;

		//gup.grpItem->meta = *meta;
		GxsGrpPendingSign ggps(gup.grpItem, gup.mToken);

		if(checkKeys(meta->keys))
		{
			ggps.mKeys = meta->keys;

			GxsSecurity::createPublicKeysFromPrivateKeys(ggps.mKeys) ;

			ggps.mHaveKeys = true;
			ggps.mStartTS = time(NULL);
			ggps.mLastAttemptTS = 0;
			ggps.mIsUpdate = true;
			ggps.mToken = gup.mToken;
			mGrpsToPublish.push_back(ggps);
		}
		else
		{
			std::cerr << "(EE) publish group fails because RS cannot find the private publish and author keys" << std::endl;

			delete gup.grpItem;
			mDataAccess->updatePublicRequestStatus(gup.mToken, RsTokenService::FAILED);
		}
	}

	mGroupUpdatePublish.clear();
}


void RsGenExchange::processRoutingClues()
{
    RS_STACK_MUTEX(mGenMtx) ;

    for(std::map<RsGxsId,std::set<RsPeerId> >::const_iterator it = mRoutingClues.begin();it!=mRoutingClues.end();++it)
        for(std::set<RsPeerId>::const_iterator it2(it->second.begin());it2!=it->second.end();++it2)
            rsGRouter->addRoutingClue(GRouterKeyId(it->first),(*it2) ) ;

    mRoutingClues.clear() ;
}

void RsGenExchange::processGroupDelete()
{
	RS_STACK_MUTEX(mGenMtx);

    // get keys for group delete publish
	typedef std::pair<bool, RsGxsGroupId> GrpNote;
	std::map<uint32_t, GrpNote> toNotify;

	std::vector<GroupDeletePublish>::iterator vit = mGroupDeletePublish.begin();
	for(; vit != mGroupDeletePublish.end(); ++vit)
	{
		std::vector<RsGxsGroupId> gprIds;
		gprIds.push_back(vit->mGroupId);
		mDataStore->removeGroups(gprIds);
		toNotify.insert(std::make_pair( vit->mToken, GrpNote(true, vit->mGroupId)));
	}


	std::list<RsGxsGroupId> grpDeleted;
	std::map<uint32_t, GrpNote>::iterator mit = toNotify.begin();
	for(; mit != toNotify.end(); ++mit)
	{
		GrpNote& note = mit->second;
		RsTokenService::GxsRequestStatus status =
		        note.first ? RsTokenService::COMPLETE
		                   : RsTokenService::FAILED;

		mGrpNotify.insert(std::make_pair(mit->first, note.second));
		mDataAccess->updatePublicRequestStatus(mit->first, status);

		if(note.first)
			grpDeleted.push_back(note.second);
	}

    for(auto& groupId:grpDeleted)
	{
		RsGxsGroupChange* gc = new RsGxsGroupChange(RsGxsNotify::TYPE_GROUP_DELETED, groupId,false);
		mNotifications.push_back(gc);
	}

	mGroupDeletePublish.clear();
}

void RsGenExchange::processMessageDelete()
{
    RS_STACK_MUTEX(mGenMtx) ;
#ifdef TODO
	typedef std::pair<bool, RsGxsGroupId> GrpNote;
	std::map<uint32_t, GrpNote> toNotify;
#endif

	for( std::vector<MsgDeletePublish>::iterator vit = mMsgDeletePublish.begin(); vit != mMsgDeletePublish.end(); ++vit)
	{
#ifdef TODO 
		uint32_t token = (*vit).mToken;
		const RsGxsGroupId& groupId = gdp.grpItem->meta.mGroupId;
		toNotify.insert(std::make_pair( token, GrpNote(true, groupId)));
#endif
		mDataStore->removeMsgs( (*vit).mMsgs );
	}

//	std::list<RsGxsGroupId> grpDeleted;
//	std::map<uint32_t, GrpNote>::iterator mit = toNotify.begin();
//	for(; mit != toNotify.end(); ++mit)
//	{
//		GrpNote& note = mit->second;
//		uint8_t status = note.first ? RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE
//		                            : RsTokenService::GXS_REQUEST_V2_STATUS_FAILED;
//
//		mGrpNotify.insert(std::make_pair(mit->first, note.second));
//		mDataAccess->updatePublicRequestStatus(mit->first, status);
//
//		if(note.first)
//			grpDeleted.push_back(note.second);
//	}

    for(uint32_t i=0;i<mMsgDeletePublish.size();++i)
        for(auto it(mMsgDeletePublish[i].mMsgs.begin());it!=mMsgDeletePublish[i].mMsgs.end();++it)
			mNotifications.push_back(new RsGxsGroupChange(RsGxsNotify::TYPE_MESSAGE_DELETED,it->first, false));

	mMsgDeletePublish.clear();
}

bool RsGenExchange::checkKeys(const RsTlvSecurityKeySet& keySet)
{

	typedef std::map<RsGxsId, RsTlvPrivateRSAKey> keyMap;
	const keyMap& allKeys = keySet.private_keys;
	keyMap::const_iterator cit = allKeys.begin();
    
        bool adminFound = false, publishFound = false;
	for(; cit != allKeys.end(); ++cit)
	{
                const RsTlvPrivateRSAKey& key = cit->second;
                if(key.keyFlags & RSTLV_KEY_TYPE_FULL)		// this one is not useful. Just a security.
                {
                    if(key.keyFlags & RSTLV_KEY_DISTRIB_ADMIN)
                        adminFound = true;

                    if(key.keyFlags & RSTLV_KEY_DISTRIB_PUBLISH)
                        publishFound = true;

                }
                else if(key.keyFlags & RSTLV_KEY_TYPE_PUBLIC_ONLY)		// this one is not useful. Just a security.
                {
                    std::cerr << "(EE) found a public only key in the private key list" << std::endl;
                    return false ;
                }
	}

	// user must have both private and public parts of publish and admin keys
        return adminFound && publishFound;
}

void RsGenExchange::publishGrps()
{
    std::list<RsGxsGroupId> groups_to_subscribe ;
    
    {
	    RS_STACK_MUTEX(mGenMtx) ;
	    NxsGrpSignPendVect::iterator vit = mGrpsToPublish.begin();

        typedef struct _GrpNote {

            _GrpNote(bool success,bool is_update,const RsGxsGroupId& gid) : mSuccess(success),mIsUpdate(is_update),mGroupId(gid){}

            bool mSuccess;
            bool mIsUpdate;
            RsGxsGroupId mGroupId;

        } GrpNote;

        std::map<uint32_t, GrpNote> toNotify; // used to notify about token processing if success or fails because of timeout

	    while( vit != mGrpsToPublish.end() )
	    {
		    GxsGrpPendingSign& ggps = *vit;

		    /* do intial checks to see if this entry has expired */
		    rstime_t now = time(NULL) ;
		    uint32_t token = ggps.mToken;


		    if(now > (ggps.mStartTS + PENDING_SIGN_TIMEOUT) )
		    {
			    // timed out
                toNotify.insert(std::make_pair( token, GrpNote(false,ggps.mIsUpdate, RsGxsGroupId())));
			    delete ggps.mItem;
			    vit = mGrpsToPublish.erase(vit);

			    continue;
		    }

		    RsGxsGroupId grpId;
		    RsNxsGrp* grp = new RsNxsGrp(mServType);
		    RsGxsGrpItem* grpItem = ggps.mItem;

		    RsTlvSecurityKeySet fullKeySet;

		    if(!(ggps.mHaveKeys))
		    {
			    generateGroupKeys(fullKeySet, true);
			    ggps.mHaveKeys = true;
			    ggps.mKeys = fullKeySet;
		    }
		    else
			{
				// We should just merge the keys instead of overwriting them, because the update may not contain private parts.

			    fullKeySet = ggps.mKeys;
			}

		    // find private admin key
		    RsTlvPrivateRSAKey privAdminKey;
		    bool privKeyFound = false;
		    for(std::map<RsGxsId, RsTlvPrivateRSAKey>::iterator mit_keys = fullKeySet.private_keys.begin(); mit_keys != fullKeySet.private_keys.end(); ++mit_keys)
		    {
			    RsTlvPrivateRSAKey& key = mit_keys->second;

			    if(key.keyFlags == (RSTLV_KEY_DISTRIB_ADMIN | RSTLV_KEY_TYPE_FULL))
			    {
				    privAdminKey = key;
				    privKeyFound = true;
			    }
		    }

		    uint8_t create = CREATE_FAIL;

		    if(privKeyFound)
		    {
			    // get group id from private admin key id
			    grpItem->meta.mGroupId = grp->grpId = RsGxsGroupId(privAdminKey.keyId);

			    ServiceCreate_Return ret = service_CreateGroup(grpItem, fullKeySet);

			    bool serialOk = false, servCreateOk;

			    if(ret == SERVICE_CREATE_SUCCESS)
			    {
				    uint32_t size = mSerialiser->size(grpItem);
				    char *gData = new char[size];
				    serialOk = mSerialiser->serialise(grpItem, gData, &size);
				    grp->grp.setBinData(gData, size);
				    delete[] gData;
				    servCreateOk = true;

			    }else
			    {
				    servCreateOk = false;
			    }

			    if(serialOk && servCreateOk)
			    {
				    grp->metaData = new RsGxsGrpMetaData();
				    grpItem->meta.mPublishTs = time(NULL);
				    *(grp->metaData) = grpItem->meta;

				    // TODO: change when publish key optimisation added (public groups don't have publish key
				    grp->metaData->mSubscribeFlags = GXS_SERV::GROUP_SUBSCRIBE_ADMIN | GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED
				                    | GXS_SERV::GROUP_SUBSCRIBE_PUBLISH;

				    create = createGroup(grp, fullKeySet);

#ifdef GEN_EXCH_DEBUG
				    std::cerr << "RsGenExchange::publishGrps() ";
				    std::cerr << " GrpId: " << grp->grpId;
				    std::cerr << " CircleType: " << (uint32_t) grp->metaData->mCircleType;
				    std::cerr << " CircleId: " << grp->metaData->mCircleId.toStdString();
				    std::cerr << std::endl;
#endif

				    if(create == CREATE_SUCCESS)
				    {
					    // Here we need to make sure that no private keys are included. This is very important since private keys
					    // can be used to modify the group. Normally the private key set is whiped out by createGroup, but

					    grp->metaData->keys.private_keys.clear() ;

					    uint32_t mdSize = grp->metaData->serial_size(RS_GXS_GRP_META_DATA_CURRENT_API_VERSION);

					    {
						    RsTemporaryMemory metaData(mdSize);
						    serialOk = grp->metaData->serialise(metaData, mdSize,RS_GXS_GRP_META_DATA_CURRENT_API_VERSION);
#warning csoler: TODO: grp->meta should be renamed grp->public_meta !
						    grp->meta.setBinData(metaData, mdSize);
					    }
						mPublishedGrps[ggps.mToken] = *grp->metaData ;	// save the connexion between the token and the created metadata, without private keys

					    // Place back private keys for publisher and database storage
					    grp->metaData->keys.private_keys = fullKeySet.private_keys;

					    if(mDataStore->validSize(grp) && serialOk)
					    {
						    grpId = grp->grpId;
						    computeHash(grp->grp, grp->metaData->mHash);
						    grp->metaData->mRecvTS = time(NULL);

                            // Also push in notifications. We do it here when we still have pointers on the old and new groups

                            RsGxsGroupChange *c = new RsGxsGroupChange(ggps.mIsUpdate?RsGxsNotify::TYPE_UPDATED:RsGxsNotify::TYPE_PUBLISHED, grpId,false);

                            c->mNewGroupItem = dynamic_cast<RsGxsGrpItem*>(mSerialiser->deserialise(grp->grp.bin_data,&grp->grp.bin_len));
                            c->mNewGroupItem->meta = *grp->metaData;	// grp will be deleted because mDataStore will destroy it on update

                            if(ggps.mIsUpdate)
                            {
                                RsNxsGrpDataTemporaryMap oldGrpDatas;
                                oldGrpDatas.insert(std::make_pair(grpId, (RsNxsGrp*)NULL));

                                if(mDataStore->retrieveNxsGrps(oldGrpDatas,true,false) && oldGrpDatas.size() == 1)
                                {
                                    auto oldGrp = oldGrpDatas[grpId];
                                    c->mOldGroupItem = dynamic_cast<RsGxsGrpItem*>(mSerialiser->deserialise(oldGrp->grp.bin_data,&oldGrp->grp.bin_len));
                                    c->mOldGroupItem->meta = *oldGrp->metaData;
                                }
                            }

                            mNotifications.push_back(c);

                            // now store (and incidently delete grp)

                            if(ggps.mIsUpdate)
                                mDataAccess->updateGroupData(grp);
                            else
                                mDataAccess->addGroupData(grp);

                            delete grp ;
							groups_to_subscribe.push_back(grpId) ;
					    }
					    else
					    {
						    create = CREATE_FAIL;
					    }
				    }
			    }
			    else if(ret == SERVICE_CREATE_FAIL_TRY_LATER)
			    {
				    // if the service is not ready yet, reset the start timestamp to give the service more time
				    // the service should have it's own timeout mechanism
				    // services should return SERVICE_CREATE_FAIL if the action timed out
				    // at the moment this is only important for the idservice:
				    //   the idservice may ask the user for a password, and the user needs time
                    ggps.mStartTS = now;
				    create = CREATE_FAIL_TRY_LATER;
			    }
			    else if(ret == SERVICE_CREATE_FAIL)
				    create = CREATE_FAIL;
		    }
		    else
		    {
#ifdef GEN_EXCH_DEBUG
			    std::cerr << "RsGenExchange::publishGrps() Could not find private publish keys " << std::endl;
#endif
			    create = CREATE_FAIL;
		    }

		    if(create == CREATE_FAIL)
		    {
#ifdef GEN_EXCH_DEBUG
			    std::cerr << "RsGenExchange::publishGrps() failed to publish grp " << std::endl;
#endif
			    delete grp;
			    delete grpItem;
			    vit = mGrpsToPublish.erase(vit);
                toNotify.insert(std::make_pair(token, GrpNote(false, ggps.mIsUpdate,grpId)));

		    }
		    else if(create == CREATE_FAIL_TRY_LATER)
		    {
#ifdef GEN_EXCH_DEBUG
			    std::cerr << "RsGenExchange::publishGrps() failed grp, trying again " << std::endl;
#endif
			    delete grp;
			    ggps.mLastAttemptTS = time(NULL);
			    ++vit;
		    }
		    else if(create == CREATE_SUCCESS)
		    {
			    delete grpItem;
			    vit = mGrpsToPublish.erase(vit);

#ifdef GEN_EXCH_DEBUG
			    std::cerr << "RsGenExchange::publishGrps() ok -> pushing to notifies"
			              << std::endl;
#endif

			    // add to published to allow acknowledgement
                toNotify.insert(std::make_pair(token, GrpNote(true,ggps.mIsUpdate,grpId)));
		    }
	    }

        // toNotify contains the token and the group update/creation info. We parse it to
        //   - notify that group creation/update for a specific token was

        for(auto mit=toNotify.begin(); mit != toNotify.end(); ++mit)
	    {
		    GrpNote& note = mit->second;
            RsTokenService::GxsRequestStatus status = note.mSuccess ? RsTokenService::COMPLETE : RsTokenService::FAILED;

            mGrpNotify.insert(std::make_pair(mit->first, note.mGroupId));	// always notify
            mDataAccess->updatePublicRequestStatus(mit->first, status);		// update the token request with the given status
        }
    }

    // This is done off-mutex to avoid possible cross deadlocks with the net service.
    
    if(mNetService!=NULL)
		for(std::list<RsGxsGroupId>::const_iterator it(groups_to_subscribe.begin());it!=groups_to_subscribe.end();++it)
			mNetService->subscribeStatusChanged((*it),true) ;
}

uint32_t RsGenExchange::generatePublicToken()
{
    return mDataAccess->generatePublicToken();
}

bool RsGenExchange::updatePublicRequestStatus(
        uint32_t token, RsTokenService::GxsRequestStatus status )
{
    return mDataAccess->updatePublicRequestStatus(token, status);
}

bool RsGenExchange::disposeOfPublicToken(const uint32_t &token)
{
    return mDataAccess->disposeOfPublicToken(token);
}

RsGeneralDataService* RsGenExchange::getDataStore()
{
    return mDataStore;
}

bool RsGenExchange::getGroupKeys(const RsGxsGroupId &grpId, RsTlvSecurityKeySet &keySet)
{
	if(grpId.isNull())
		return false;

	RS_STACK_MUTEX(mGenMtx) ;

	RsGxsGrpMetaTemporaryMap grpMeta;
	grpMeta[grpId] = NULL;
	mDataStore->retrieveGxsGrpMetaData(grpMeta);

	if(grpMeta.empty())
		return false;

	const RsGxsGrpMetaData* meta = grpMeta[grpId];

	if(meta == NULL)
		return false;

	keySet = meta->keys;
	GxsSecurity::createPublicKeysFromPrivateKeys(keySet) ;

	return true;
}

void RsGenExchange::shareGroupPublishKey(const RsGxsGroupId& grpId,const std::set<RsPeerId>& peers)
{
    if(grpId.isNull())
        return ;

    mNetService->sharePublishKey(grpId,peers) ;
}

void RsGenExchange::processRecvdData()
{
    processRecvdGroups();

    processRecvdMessages();

    performUpdateValidation();

}


void RsGenExchange::computeHash(const RsTlvBinaryData& data, RsFileHash& hash)
{
	pqihash pHash;
	pHash.addData(data.bin_data, data.bin_len);
	pHash.Complete(hash);
}

void RsGenExchange::processRecvdMessages()
{
    std::list<RsGxsMessageId> messages_to_reject ;

    {
	    RS_STACK_MUTEX(mGenMtx) ;

        rstime_t now = time(NULL);

	    if(mMsgPendingValidate.empty())
			return ;
#ifdef GEN_EXCH_DEBUG
		else
		    std::cerr << "processing received messages" << std::endl;
#endif
		// 1 - First, make sure items metadata is deserialised, clean old failed items, and collect the groups Ids we have to check

		RsGxsGrpMetaTemporaryMap grpMetas;

	    for(NxsMsgPendingVect::iterator pend_it = mMsgPendingValidate.begin();pend_it != mMsgPendingValidate.end();)
	    {
		    GxsPendingItem<RsNxsMsg*, RsGxsGrpMsgIdPair>& gpsi = pend_it->second;
			RsNxsMsg *msg = gpsi.mItem ;

			if(msg->metaData == NULL)
			{
				RsGxsMsgMetaData* meta = new RsGxsMsgMetaData();

				if(msg->meta.bin_len != 0 && meta->deserialise(msg->meta.bin_data, &(msg->meta.bin_len)))
					msg->metaData = meta;
				else
					delete meta;
			}

			bool accept_new_msg = msg->metaData != NULL && acceptNewMessage(msg->metaData,msg->msg.bin_len);

			if(!accept_new_msg)
				messages_to_reject.push_back(msg->metaData->mMsgId); // This prevents reloading the message again at next sync.

		    if(!accept_new_msg || gpsi.mFirstTryTS + VALIDATE_MAX_WAITING_TIME < now)
		    {
#ifdef GEN_EXCH_DEBUG
				std::cerr << "Pending validation grp=" << gpsi.mId.first << ", msg=" << gpsi.mId.second << ", has exceeded validation time limit. The author's key can probably not be obtained. This is unexpected." << std::endl;
#endif

			    delete gpsi.mItem;
			    pend_it = mMsgPendingValidate.erase(pend_it);
		    }
		    else
		    {
				grpMetas.insert(std::make_pair(pend_it->second.mItem->grpId, (RsGxsGrpMetaData*)NULL));
			    ++pend_it;
		    }
	    }

		// 2 - Retrieve the metadata for the associated groups. The test is here to avoid the default behavior to
		//     retrieve all groups when the list is empty

		if(!grpMetas.empty())
			mDataStore->retrieveGxsGrpMetaData(grpMetas);

	    GxsMsgReq msgIds;
	    RsNxsMsgDataTemporaryList msgs_to_store;

#ifdef GEN_EXCH_DEBUG
	    std::cerr << "  updating received messages:" << std::endl;
#endif

		// 3 - Validate each message

	    for(NxsMsgPendingVect::iterator pend_it = mMsgPendingValidate.begin();pend_it != mMsgPendingValidate.end();)
	    {
		    RsNxsMsg* msg = pend_it->second.mItem;

            // (cyril) Normally we should discard posts that are older than the sync request. But that causes a problem because
            // 	RsGxsNetService requests posts to sync by chunks of 20. So if the 20 are discarded, they will be re-synced next time, and the sync process
            // 	will indefinitly loop on the same 20 posts. Since the posts are there already, keeping them is the least problematique way to fix this problem.
            //
			//      uint32_t max_sync_age = ( mNetService != NULL)?( mNetService->getSyncAge(msg->metaData->mGroupId)):RS_GXS_DEFAULT_MSG_REQ_PERIOD;
			//
			//		if(max_sync_age != 0 && msg->metaData->mPublishTs + max_sync_age < time(NULL))
			//      {
			//			std::cerr << "(WW) not validating message " << msg->metaData->mMsgId << " in group " << msg->metaData->mGroupId << " because it is older than synchronisation limit. This message was probably sent by a friend node that does not accept sync limits already." << std::endl;
			//          ok = false ;
			//      }

#ifdef GEN_EXCH_DEBUG
		    std::cerr << "    deserialised info: grp id=" << msg->grpId << ", msg id=" << msg->msgId ;
#endif
			std::map<RsGxsGroupId, RsGxsGrpMetaData*>::iterator mit = grpMetas.find(msg->grpId);

#ifdef GEN_EXCH_DEBUG
			    std::cerr << "    msg info         : grp id=" << msg->grpId << ", msg id=" << msg->msgId << std::endl;
#endif
			// validate msg

			if(mit == grpMetas.end())
			{
				std::cerr << "RsGenExchange::processRecvdMessages(): impossible situation: grp meta " << msg->grpId << " not available." << std::endl;
				++pend_it ;
				continue ;
			}

			const RsGxsGrpMetaData *grpMeta = mit->second;
			RsTlvSecurityKeySet keys = grpMeta->keys ;

			GxsSecurity::createPublicKeysFromPrivateKeys(keys);	// make sure we have the public keys that correspond to the private ones, as it happens. Most of the time this call does nothing.

			int validateReturn = validateMsg(msg, grpMeta->mGroupFlags, grpMeta->mSignFlags, keys);

#ifdef GEN_EXCH_DEBUG
			std::cerr << "    grpMeta.mSignFlags: " << std::hex << grpMeta->mSignFlags << std::dec << std::endl;
			std::cerr << "    grpMeta.mAuthFlags: " << std::hex << grpMeta->mAuthenFlags << std::dec << std::endl;
			std::cerr << "    message validation result: " << (int)validateReturn << std::endl;
#endif

			if(validateReturn == VALIDATE_SUCCESS)
			{
				msg->metaData->mMsgStatus = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED | GXS_SERV::GXS_MSG_STATUS_GUI_NEW | GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD;
				msgs_to_store.push_back(msg);

                msgIds[msg->grpId].insert(msg->msgId);
				// std::vector<RsGxsMessageId> &msgv = msgIds[msg->grpId];
				// if (std::find(msgv.begin(), msgv.end(), msg->msgId) == msgv.end())
				// 	msgv.push_back(msg->msgId);

				computeHash(msg->msg, msg->metaData->mHash);
				msg->metaData->recvTS = time(NULL);

#ifdef GEN_EXCH_DEBUG
				std::cerr << "    new status flags: " << msg->metaData->mMsgStatus << std::endl;
				std::cerr << "    computed hash: " << msg->metaData->mHash << std::endl;
				std::cerr << "Message received. Identity=" << msg->metaData->mAuthorId << ", from peer " << msg->PeerId() << std::endl;
#endif

				if(!msg->metaData->mAuthorId.isNull())
					mRoutingClues[msg->metaData->mAuthorId].insert(msg->PeerId()) ;
			}
			else if(validateReturn == VALIDATE_FAIL)
			{
				// In this case, we notify the network exchange service not to DL the message again, at least not yet.

#ifdef GEN_EXCH_DEBUG
			    std::cerr << "Validation failed for message id "
			              << "msg->grpId: " << msg->grpId << ", msgId: " << msg->msgId << std::endl;
#endif
				messages_to_reject.push_back(msg->msgId) ;
				delete msg ;
			}
			else if(validateReturn == VALIDATE_FAIL_TRY_LATER)
			{
				++pend_it ;
				continue;
			}

			// Remove the entry from mMsgPendingValidate, but do not delete msg since it's either pushed into msg_to_store or deleted in the FAIL case!

			NxsMsgPendingVect::iterator tmp = pend_it ;
			++tmp ;
			mMsgPendingValidate.erase(pend_it) ;
			pend_it = tmp ;
	    }

	    if(!msgIds.empty())
	    {
#ifdef GEN_EXCH_DEBUG
		    std::cerr << "  removing existing and old messages from incoming list." << std::endl;
#endif
		    removeDeleteExistingMessages(msgs_to_store, msgIds);

#ifdef GEN_EXCH_DEBUG
		    std::cerr << "  storing remaining messages" << std::endl;
#endif

            for(auto& nxs_msg: msgs_to_store)
            {
                RsGxsMsgItem *item = dynamic_cast<RsGxsMsgItem*>(mSerialiser->deserialise(nxs_msg->msg.bin_data,&nxs_msg->msg.bin_len));
                item->meta = *nxs_msg->metaData;

				RsGxsMsgChange* c = new RsGxsMsgChange(RsGxsNotify::TYPE_RECEIVED_NEW, item->meta.mGroupId, item->meta.mMsgId,false);
                c->mNewMsgItem = item;

				mNotifications.push_back(c);
			}

            mDataStore->storeMessage(msgs_to_store);	// All items will be destroyed later on, since msgs_to_store is a temporary map
	    }
    }

    // Done off-mutex to avoid cross deadlocks in the netservice that might call the RsGenExchange as an observer..

    if(mNetService != NULL)
	    for(std::list<RsGxsMessageId>::const_iterator it(messages_to_reject.begin());it!=messages_to_reject.end();++it)
		    mNetService->rejectMessage(*it) ;
}

bool RsGenExchange::acceptNewGroup(const RsGxsGrpMetaData* /*grpMeta*/ ) { return true; }
bool RsGenExchange::acceptNewMessage(const RsGxsMsgMetaData* /*grpMeta*/,uint32_t /*size*/ ) { return true; }

void RsGenExchange::processRecvdGroups()
{
    RS_STACK_MUTEX(mGenMtx) ;

	if(mGrpPendingValidate.empty())
		return;

#ifdef GEN_EXCH_DEBUG
    std::cerr << "RsGenExchange::Processing received groups" << std::endl;
#endif
	std::list<RsGxsGroupId> grpIds;
	RsNxsGrpDataTemporaryList grps_to_store;

	// 1 - retrieve the existing groups so as to check what's not new
	std::vector<RsGxsGroupId> existingGrpIds;
	mDataStore->retrieveGroupIds(existingGrpIds);

	// 2 - go through each and every new group data and validate the signatures.

	for(NxsGrpPendValidVect::iterator vit = mGrpPendingValidate.begin(); vit != mGrpPendingValidate.end();)
	{
		GxsPendingItem<RsNxsGrp*, RsGxsGroupId>& gpsi = vit->second;
		RsNxsGrp* grp = gpsi.mItem;

		if(grp->metaData == NULL)
		{
			RsGxsGrpMetaData* meta = new RsGxsGrpMetaData();

			if(grp->meta.bin_len != 0 && meta->deserialise(grp->meta.bin_data, grp->meta.bin_len))
				grp->metaData = meta ;
			else
				delete meta ;
		}
#ifdef GEN_EXCH_DEBUG
		std::cerr << "  processing validation for group " << grp->metaData->mGroupId << ", original attempt time: " << time(NULL) - gpsi.mFirstTryTS << " seconds ago" << std::endl;
#endif

		// early deletion of group from the pending list if it's malformed, not accepted, or has been tried unsuccessfully for too long

        if(grp->metaData == NULL || !acceptNewGroup(grp->metaData) || gpsi.mFirstTryTS + VALIDATE_MAX_WAITING_TIME < time(NULL))
		{
			NxsGrpPendValidVect::iterator tmp(vit) ;
			++tmp ;
			delete grp ;
			mGrpPendingValidate.erase(vit) ;
			vit = tmp ;
			continue;
		}

		// group signature validation

		uint8_t ret = validateGrp(grp);

		if(ret == VALIDATE_SUCCESS)
		{
			grp->metaData->mGroupStatus = GXS_SERV::GXS_GRP_STATUS_UNPROCESSED | GXS_SERV::GXS_GRP_STATUS_UNREAD;

			computeHash(grp->grp, grp->metaData->mHash);

			// group has been validated. Let's notify the global router for the clue

			if(!grp->metaData->mAuthorId.isNull())
			{
#ifdef GEN_EXCH_DEBUG
				std::cerr << "Group routage info: Identity=" << grp->metaData->mAuthorId << " from " << grp->PeerId() << std::endl;
#endif
				mRoutingClues[grp->metaData->mAuthorId].insert(grp->PeerId()) ;
			}

			// This has been moved here (as opposed to inside part for new groups below) because it is used to update the server TS when updates
			// of grp metadata arrive.

			grp->metaData->mRecvTS = time(NULL);

			// now check if group already exists

			if(std::find(existingGrpIds.begin(), existingGrpIds.end(), grp->grpId) == existingGrpIds.end())
			{
				grp->metaData->mOriginator = grp->PeerId();
				grp->metaData->mSubscribeFlags = GXS_SERV::GROUP_SUBSCRIBE_NOT_SUBSCRIBED;

				grps_to_store.push_back(grp);
				grpIds.push_back(grp->grpId);
			}
			else
			{
				GroupUpdate update;
				update.newGrp = grp;
				mGroupUpdates.push_back(update);
			}
		}
		else if(ret == VALIDATE_FAIL)
		{
#ifdef GEN_EXCH_DEBUG
			std::cerr << "  failed to validate incoming meta, grpId: " << grp->grpId << ": wrong signature" << std::endl;
#endif
			delete grp;
		}
		else  if(ret == VALIDATE_FAIL_TRY_LATER)
		{
#ifdef GEN_EXCH_DEBUG
			std::cerr << "  failed to validate incoming grp, trying again later. grpId: " << grp->grpId << std::endl;
#endif
			++vit ;
			continue;
		}

		// Erase entry from the list

		NxsGrpPendValidVect::iterator tmp(vit) ;
		++tmp ;
		mGrpPendingValidate.erase(vit) ;
		vit = tmp ;
	}

	if(!grps_to_store.empty())
	{
        for(auto Grp:grps_to_store)
		{
			RsGxsGroupChange* c = new RsGxsGroupChange(RsGxsNotify::TYPE_RECEIVED_NEW, Grp->grpId, false);

            c->mNewGroupItem = dynamic_cast<RsGxsGrpItem*>(mSerialiser->deserialise(Grp->grp.bin_data,&Grp->grp.bin_len));

			mNotifications.push_back(c);
		}

		mDataStore->storeGroup(grps_to_store);
#ifdef GEN_EXCH_DEBUG
        std::cerr << "  adding the following grp ids to notification: " << std::endl;
        for(std::list<RsGxsGroupId>::const_iterator it(grpIds.begin());it!=grpIds.end();++it)
            std::cerr << "    " << *it << std::endl;
#endif
	}
}

void RsGenExchange::performUpdateValidation()
{
	RS_STACK_MUTEX(mGenMtx) ;

	if(mGroupUpdates.empty())
		return;

#ifdef GEN_EXCH_DEBUG
	std::cerr << "RsGenExchange::performUpdateValidation() " << std::endl;
#endif

	RsNxsGrpDataTemporaryMap grpDatas;

	for(auto vit(mGroupUpdates.begin()); vit != mGroupUpdates.end(); ++vit)
		grpDatas.insert(std::make_pair(vit->newGrp->grpId, (RsNxsGrp*)NULL));

	if(grpDatas.empty() || !mDataStore->retrieveNxsGrps(grpDatas,true,false))
    {
        if(grpDatas.empty())
			RsErr() << __PRETTY_FUNCTION__ << " Validation of multiple group updates failed: no group in list!" << std::endl;
        else
            RsErr() << __PRETTY_FUNCTION__ << " Validation of multiple group updates failed: cannot retrieve froup data for these groups!" << std::endl;

		return;
    }

#ifdef GEN_EXCH_DEBUG
	std::cerr << "RsGenExchange::performUpdateValidation() " << std::endl;
#endif

	RsNxsGrpDataTemporaryList grps ;

	for(auto vit(mGroupUpdates.begin()); vit != mGroupUpdates.end(); ++vit)
	{
		GroupUpdate& gu = *vit;

		auto mit = grpDatas.find(gu.newGrp->grpId);

        if(mit == grpDatas.end())
        {
            RsErr() << __PRETTY_FUNCTION__ << " Validation of group update failed for group " << gu.newGrp->grpId << " because previous grp version cannot be found." << std::endl;
            continue;
        }
        RsGxsGrpMetaData *oldGrpMeta(mit->second->metaData);
        RsNxsGrp *oldGrp(mit->second);

		if(updateValid(*oldGrpMeta, *gu.newGrp))
		{
			if(gu.newGrp->metaData->mCircleType == GXS_CIRCLE_TYPE_YOUR_FRIENDS_ONLY)
				gu.newGrp->metaData->mOriginator = gu.newGrp->PeerId();

			// Keep subscriptionflag to what it was. This avoids clearing off the flag when updates to group meta information
			// is received.

			gu.newGrp->metaData->mSubscribeFlags = oldGrpMeta->mSubscribeFlags ;

			// Also keep private keys if present

			if(!gu.newGrp->metaData->keys.private_keys.empty())
				std::cerr << "(EE) performUpdateValidation() group " <<gu.newGrp->metaData->mGroupId << " has been received with private keys. This is very unexpected!" << std::endl;
			else
				gu.newGrp->metaData->keys.private_keys = oldGrpMeta->keys.private_keys ;

            // Now prepare notification of the client

			RsGxsGroupChange *c = new RsGxsGroupChange(RsGxsNotify::TYPE_UPDATED,gu.newGrp->metaData->mGroupId,false);

            c->mNewGroupItem = dynamic_cast<RsGxsGrpItem*>(mSerialiser->deserialise(gu.newGrp->grp.bin_data,&gu.newGrp->grp.bin_len));
            c->mNewGroupItem->meta = *gu.newGrp->metaData;		// gu.newGrp will be deleted because mDataStore will destroy it on update

            c->mOldGroupItem = dynamic_cast<RsGxsGrpItem*>(mSerialiser->deserialise(oldGrp->grp.bin_data,&oldGrp->grp.bin_len));
            c->mOldGroupItem->meta = *oldGrpMeta;				// no need to delete mit->second, as it will be deleted automatically in the temporary map

			mNotifications.push_back(c);

			// finally, add the group to the list to send to mDataStore

			grps.push_back(gu.newGrp);
		}
		else
		{
			delete gu.newGrp;    // delete here because mDataStore will not take care of this one.  no need to delete mit->second, as it will be deleted automatically in the temporary map
			gu.newGrp = NULL ;
		}
	}

    mDataStore->updateGroup(grps);

#ifdef GEN_EXCH_DEBUG
	std::cerr << "  adding the following grp ids to notification: " << std::endl;
#endif

	// cleanup

	mGroupUpdates.clear();
}

bool RsGenExchange::updateValid(const RsGxsGrpMetaData& oldGrpMeta, const RsNxsGrp& newGrp) const
{
	std::map<SignType, RsTlvKeySignature>& signSet = newGrp.metaData->signSet.keySignSet;
	std::map<SignType, RsTlvKeySignature>::iterator mit = signSet.find(INDEX_AUTHEN_ADMIN);

	if(mit == signSet.end())
	{
#ifdef GEN_EXCH_DEBUG
		std::cerr << "RsGenExchange::updateValid() new admin sign not found! " << std::endl;
		std::cerr << "RsGenExchange::updateValid() grpId: " << oldGrpMeta.mGroupId << std::endl;
#endif

		return false;
	}

	RsTlvKeySignature adminSign = mit->second;
	RsTlvSecurityKeySet old_keys = oldGrpMeta.keys ;

	GxsSecurity::createPublicKeysFromPrivateKeys(old_keys);	// make sure we have the public keys that correspond to the private ones, as it happens. Most of the time this call does nothing.

	std::map<RsGxsId, RsTlvPublicRSAKey>& keys = old_keys.public_keys;
	std::map<RsGxsId, RsTlvPublicRSAKey>::iterator keyMit = keys.find(RsGxsId(oldGrpMeta.mGroupId));

	if(keyMit == keys.end())
	{
#ifdef GEN_EXCH_DEBUG
		std::cerr << "RsGenExchange::updateValid() admin key not found! " << std::endl;
#endif
		return false;
	}

	// also check this is the latest published group
	bool latest = newGrp.metaData->mPublishTs > oldGrpMeta.mPublishTs;

    mGixs->timeStampKey(newGrp.metaData->mAuthorId, RsIdentityUsage(RsServiceType(mServType),RsIdentityUsage::GROUP_ADMIN_SIGNATURE_CREATION, oldGrpMeta.mGroupId)) ;

    return GxsSecurity::validateNxsGrp(newGrp, adminSign, keyMit->second) && latest;
}

void RsGenExchange::setGroupReputationCutOff(uint32_t& token, const RsGxsGroupId& grpId, int CutOff)
{
					RS_STACK_MUTEX(mGenMtx) ;
    token = mDataAccess->generatePublicToken();

    GrpLocMetaData g;
    g.grpId = grpId;
    g.val.put(RsGeneralDataService::GRP_META_CUTOFF_LEVEL, (int32_t)CutOff);
    mGrpLocMetaMap.insert(std::make_pair(token, g));
}

void RsGenExchange::removeDeleteExistingMessages( std::list<RsNxsMsg*>& msgs, GxsMsgReq& msgIdsNotify)
{
	// first get grp ids of messages to be stored

	RsGxsGroupId::std_set mGrpIdsUnique;

	for(std::list<RsNxsMsg*>::const_iterator cit = msgs.begin(); cit != msgs.end(); ++cit)
		mGrpIdsUnique.insert((*cit)->metaData->mGroupId);

	//RsGxsGroupId::std_list grpIds(mGrpIdsUnique.begin(), mGrpIdsUnique.end());
	//RsGxsGroupId::std_list::const_iterator it = grpIds.begin();
	typedef std::map<RsGxsGroupId, RsGxsMessageId::std_set> MsgIdReq;
	MsgIdReq msgIdReq;

	// now get a list of all msgs ids for each group
	for(RsGxsGroupId::std_set::const_iterator it(mGrpIdsUnique.begin()); it != mGrpIdsUnique.end(); ++it)
	{
		mDataStore->retrieveMsgIds(*it, msgIdReq[*it]);

#ifdef GEN_EXCH_DEBUG
		const std::set<RsGxsMessageId>& vec(msgIdReq[*it]) ;
		std::cerr << "  retrieved " << vec.size() << " message ids for group " << *it << std::endl;

		for(auto it2(vec.begin());it2!=vec.end();++it2)
			std::cerr << "    " << *it2   << std::endl;
#endif
	}

	// now for each msg to be stored that exist in the retrieved msg/grp "index" delete and erase from map
	for(std::list<RsNxsMsg*>::iterator cit2 = msgs.begin(); cit2 != msgs.end();)
	{
		const RsGxsMessageId::std_set& msgIds = msgIdReq[(*cit2)->metaData->mGroupId];

#ifdef GEN_EXCH_DEBUG
		std::cerr << "    grpid=" << (*cit2)->grpId << ", msgid=" << (*cit2)->msgId ;
#endif

		// Avoid storing messages that are already in the database, as well as messages that are too old (or generally do not pass the database storage test)
		//
        if(msgIds.find((*cit2)->metaData->mMsgId) != msgIds.end() || !messagePublicationTest( *(*cit2)->metaData))
		{
			// msg exist in retrieved index. We should use a std::set here instead of a vector.

			RsGxsMessageId::std_set& notifyIds = msgIdsNotify[ (*cit2)->metaData->mGroupId];
			RsGxsMessageId::std_set::iterator it2 = notifyIds.find((*cit2)->metaData->mMsgId);

			if(it2 != notifyIds.end())
			{
				notifyIds.erase(it2);
				if (notifyIds.empty())
				{
					msgIdsNotify.erase( (*cit2)->metaData->mGroupId);
				}
			}
#ifdef GEN_EXCH_DEBUG
			std::cerr << "    discarding " << (*cit2)->msgId << std::endl;
#endif

			delete *cit2;
			cit2 = msgs.erase(cit2);
		}
		else
			++cit2;
	}
}

DistantSearchGroupStatus RsGenExchange::getDistantSearchStatus(const RsGxsGroupId& group_id)
{
    return mNetService->getDistantSearchStatus(group_id) ;
}
void RsGenExchange::turtleGroupRequest(const RsGxsGroupId& group_id)
{
    mNetService->turtleGroupRequest(group_id) ;
}
void RsGenExchange::turtleSearchRequest(const std::string& match_string)
{
    mNetService->turtleSearchRequest(match_string) ;
}

bool RsGenExchange::localSearch( const std::string& matchString,
                  std::list<RsGxsGroupSummary>& results )
{
	return mNetService->search(matchString, results);
}

bool RsGenExchange::exportGroupBase64(
        std::string& radix, const RsGxsGroupId& groupId, std::string& errMsg )
{
	constexpr auto fname = __PRETTY_FUNCTION__;
	const auto failure = [&](const std::string& err)
	{
		errMsg = err;
		RsErr() << fname << " " << err << std::endl;
		return false;
	};

	if(groupId.isNull()) return failure("groupId cannot be null");

    // We have no blocking API here, so we need to make a blocking request manually.
	const std::list<RsGxsGroupId> groupIds({groupId});
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
	uint32_t token;
	mDataAccess->requestGroupInfo( token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds);

    // provide a sync response: actually wait for the token.
    std::chrono::milliseconds maxWait = std::chrono::milliseconds(10000);
    std::chrono::milliseconds checkEvery = std::chrono::milliseconds(100);

	auto timeout = std::chrono::steady_clock::now() + maxWait;	// wait for 10 secs at most
	auto st = mDataAccess->requestStatus(token);

	while( !(st == RsTokenService::FAILED || st >= RsTokenService::COMPLETE) && std::chrono::steady_clock::now() < timeout )
	{
		std::this_thread::sleep_for(checkEvery);
		st = mDataAccess->requestStatus(token);
	}
	if(st != RsTokenService::COMPLETE)
		return failure( "waitToken(...) failed with: " + std::to_string(st) );

	uint8_t* buf = nullptr;
	uint32_t size;
	RsGxsGroupId grpId;

	if(!getSerializedGroupData(token, grpId, buf, size))
		return failure("failed retrieving GXS data");

	Radix64::encode(buf, static_cast<int>(size), radix);
	free(buf);

	return true;
}

bool RsGenExchange::importGroupBase64(
        const std::string& radix, RsGxsGroupId& groupId,
        std::string& errMsg )
{
	constexpr auto fname = __PRETTY_FUNCTION__;
	const auto failure = [&](const std::string& err)
	{
		errMsg = err;
		RsErr() << fname << " " << err << std::endl;
		return false;
	};

	if(radix.empty()) return failure("radix is empty");

	std::vector<uint8_t> mem = Radix64::decode(radix);
	if(mem.empty()) return failure("radix seems corrupted");

	// On success this also import the group as pending validation
	if(!deserializeGroupData(
	            mem.data(), static_cast<uint32_t>(mem.size()),
	            reinterpret_cast<RsGxsGroupId*>(&groupId) ))
		return failure("failed deserializing group");

	return true;
}

RsGxsChanges::RsGxsChanges() :
    RsEvent(RsEventType::GXS_CHANGES), mServiceType(RsServiceType::NONE),
    mService(nullptr) {}

RsGxsIface::~RsGxsIface() = default;
RsGxsGroupSummary::~RsGxsGroupSummary() = default;
