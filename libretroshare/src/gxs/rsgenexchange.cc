
/*
 * libretroshare/src/gxs: rsgenexchange.cc
 *
 * RetroShare Gxs exchange interface.
 *
 * Copyright 2012-2012 by Christopher Evi-Parker, Robert Fernie
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

#include <unistd.h>

#include "pqi/pqihash.h"
#include "rsgenexchange.h"
#include "gxssecurity.h"
#include "util/contentvalue.h"
#include "retroshare/rsgxsflags.h"
#include "retroshare/rsgxscircles.h"
#include "retroshare/rsgrouter.h"
#include "retroshare/rsidentity.h"
#include "retroshare/rspeers.h"
#include "rsgixs.h"
#include "rsgxsutil.h"
#include "rsserver/p3face.h"

#include <algorithm>

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

#define GEN_EXCH_DEBUG	1

#define MSG_CLEANUP_PERIOD     60*5  //  5 minutes
#define INTEGRITY_CHECK_PERIOD 60*30 // 30 minutes

RsGenExchange::RsGenExchange(RsGeneralDataService *gds, RsNetworkExchangeService *ns,
                             RsSerialType *serviceSerialiser, uint16_t servType, RsGixs* gixs,
                             uint32_t authenPolicy, uint32_t messageStorePeriod)
  : mGenMtx("GenExchange"),
    mDataStore(gds),
    mNetService(ns),
    mSerialiser(serviceSerialiser),
  mServType(servType),
  mGixs(gixs),
  mAuthenPolicy(authenPolicy),
  MESSAGE_STORE_PERIOD(messageStorePeriod),
  mCleaning(false),
  mLastClean((int)time(NULL) - (int)(RSRandom::random_u32() % MSG_CLEANUP_PERIOD)),	// this helps unsynchronising the checks for the different services
  mMsgCleanUp(NULL),
  mChecking(false),
  mLastCheck((int)time(NULL) - (int)(RSRandom::random_u32() % INTEGRITY_CHECK_PERIOD)),	// this helps unsynchronising the checks for the different services
  mIntegrityCheck(NULL),
  CREATE_FAIL(0),
  CREATE_SUCCESS(1),
  CREATE_FAIL_TRY_LATER(2),
  SIGN_MAX_ATTEMPTS(5),
  SIGN_FAIL(0),
  SIGN_SUCCESS(1),
  SIGN_FAIL_TRY_LATER(2),
  VALIDATE_FAIL(0),
  VALIDATE_SUCCESS(1),
  VALIDATE_FAIL_TRY_LATER(2),
  VALIDATE_MAX_ATTEMPTS(5)
{

    mDataAccess = new RsGxsDataAccess(gds);

}

void RsGenExchange::setNetworkExchangeService(RsNetworkExchangeService *ns)
{
    if(mNetService != NULL)
        std::cerr << "(EE) Cannot override existing network exchange service. Make sure it has been deleted otherwise." << std::endl;
    else
        mNetService = ns ;
}

#ifdef TO_BE_DELETED_IF_NOT_USEFUL
// This class has been tested so as to see where the database gets modified.
class RsDataBaseTester
{
public:
    RsDataBaseTester(RsGeneralDataService *store,const RsGxsGroupId& grpId,const std::string& info)
            :_grpId(grpId),_store(store),_info(info)
    {
        //std::cerr << "RsDataBaseTester: (" << _info << ") retrieving messages for group " << grpId << std::endl;
        _store->retrieveMsgIds(_grpId, _msgIds1) ;
    }

    ~RsDataBaseTester()
    {
        //std::cerr << "RsDataBaseTester: (" << _info << ") testing messages for group " << _grpId << std::endl;
        _store->retrieveMsgIds(_grpId, _msgIds2) ;

        bool all_idendical = true ;
    std::cerr << std::dec ;

        if(_msgIds1.size() != _msgIds2.size())
            std::cerr << "  " << _info << " (EE) The two arrays are different (size1=" << _msgIds1.size() << ", size2=" << _msgIds2.size() << ") !!" << std::endl;
        else
            for(uint32_t i=0;i<_msgIds1.size();++i)
                if(_msgIds1[i] != _msgIds2[i])
                    std::cerr << "  " << _info << " (EE) The two arrays are different for i=" << i << " !!" << std::endl;
    }
    RsGxsGroupId _grpId ;
    RsGeneralDataService *_store ;
    std::vector<RsGxsMessageId> _msgIds1 ;
    std::vector<RsGxsMessageId> _msgIds2 ;
    std::string _info ;
};
#endif

RsGenExchange::~RsGenExchange()
{
    // need to destruct in a certain order (bad thing, TODO: put down instance ownership rules!)
    delete mNetService;

    delete mDataAccess;
    mDataAccess = NULL;

    delete mDataStore;
    mDataStore = NULL;

}

bool RsGenExchange::getGroupServerUpdateTS(const RsGxsGroupId& gid, time_t& grp_server_update_TS, time_t& msg_server_update_TS) 
{
    return mNetService->getGroupServerUpdateTS(gid,grp_server_update_TS,msg_server_update_TS) ;
}

void RsGenExchange::data_tick()
{

    static const double timeDelta = 0.1; // slow tick in sec

        tick();
    usleep((int) (timeDelta * 1000 *1000)); // timeDelta sec
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

	processRecvdData();

	processRoutingClues() ;

	if(!mNotifications.empty())
	{
		notifyChanges(mNotifications);
		mNotifications.clear();
	}

	// implemented service tick function
	service_tick();

	time_t now = time(NULL);
    
	if((mLastClean + MSG_CLEANUP_PERIOD < now) || mCleaning)
	{
		if(mMsgCleanUp)
		{
			if(mMsgCleanUp->clean())
			{
				mCleaning = false;
				delete mMsgCleanUp;
				mMsgCleanUp = NULL;
				mLastClean = time(NULL);
			}

		}else
		{
			mMsgCleanUp = new RsGxsMessageCleanUp(mDataStore, MESSAGE_STORE_PERIOD, 1);
			mCleaning = true;
		}
	}

	now = time(NULL);
	if(mChecking || (mLastCheck + INTEGRITY_CHECK_PERIOD < now))
	{
		if(mIntegrityCheck)
		{
			if(mIntegrityCheck->isDone())
			{
				std::list<RsGxsGroupId> grpIds;
				std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > msgIds;
				mIntegrityCheck->getDeletedIds(grpIds, msgIds);

				if (!grpIds.empty())
				{
					RS_STACK_MUTEX(mGenMtx) ;

					RsGxsGroupChange* gc = new RsGxsGroupChange(RsGxsNotify::TYPE_PROCESSED, false);
					gc->mGrpIdList = grpIds;
#ifdef GEN_EXCH_DEBUG
                    			std::cerr << "  adding the following grp ids to notification: " << std::endl;
                                	for(std::list<RsGxsGroupId>::const_iterator it(grpIds.begin());it!=grpIds.end();++it)
                                        	std::cerr << "    " << *it << std::endl;
#endif
					mNotifications.push_back(gc);
				}

				if (!msgIds.empty()) {
					RS_STACK_MUTEX(mGenMtx) ;

					RsGxsMsgChange* c = new RsGxsMsgChange(RsGxsNotify::TYPE_PROCESSED, false);
					c->msgChangeMap = msgIds;
					mNotifications.push_back(c);
				}

				delete mIntegrityCheck;
				mIntegrityCheck = NULL;
				mLastCheck = time(NULL);
				mChecking = false;
			}
		}
		else
		{
			mIntegrityCheck = new RsGxsIntegrityCheck(mDataStore,mGixs);
			mIntegrityCheck->start();
			mChecking = true;
		}
	}
}

bool RsGenExchange::messagePublicationTest(const RsGxsMsgMetaData& meta)
{
	time_t now = time(NULL) ;

	return meta.mMsgStatus & GXS_SERV::GXS_MSG_STATUS_KEEP || meta.mPublishTs + MESSAGE_STORE_PERIOD >= now ;
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

void RsGenExchange::generateGroupKeys(RsTlvSecurityKeySet& privatekeySet, RsTlvSecurityKeySet& publickeySet, bool genPublishKeys)
{
    /* create Keys */
    RsTlvSecurityKey adminKey, privAdminKey;
	 GxsSecurity::generateKeyPair(adminKey,privAdminKey) ;

    // for now all public
    adminKey.keyFlags = RSTLV_KEY_DISTRIB_ADMIN | RSTLV_KEY_TYPE_PUBLIC_ONLY;
    privAdminKey.keyFlags = RSTLV_KEY_DISTRIB_ADMIN | RSTLV_KEY_TYPE_FULL;

    publickeySet.keys[adminKey.keyId] = adminKey;
    privatekeySet.keys[privAdminKey.keyId] = privAdminKey;

    if(genPublishKeys)
    {
        /* set publish keys */
        RsTlvSecurityKey publishKey, privPublishKey;
    GxsSecurity::generateKeyPair(publishKey,privPublishKey) ;

        // for now all public
        publishKey.keyFlags = RSTLV_KEY_DISTRIB_PUBLISH | RSTLV_KEY_TYPE_PUBLIC_ONLY;
        privPublishKey.keyFlags = RSTLV_KEY_DISTRIB_PUBLISH | RSTLV_KEY_TYPE_FULL;

        publickeySet.keys[publishKey.keyId] = publishKey;
        privatekeySet.keys[privPublishKey.keyId] = privPublishKey;
    }
}

void RsGenExchange::generatePublicFromPrivateKeys(const RsTlvSecurityKeySet &privatekeySet, RsTlvSecurityKeySet &publickeySet)
{
	// actually just copy settings of one key except mark its key flags public

	publickeySet = RsTlvSecurityKeySet() ;
	RsTlvSecurityKey pubkey ;

	for(std::map<RsGxsId, RsTlvSecurityKey>::const_iterator cit=privatekeySet.keys.begin(); cit != privatekeySet.keys.end(); ++cit)
		if(GxsSecurity::extractPublicKey(cit->second,pubkey))
			publickeySet.keys.insert(std::make_pair(pubkey.keyId, pubkey));
}

uint8_t RsGenExchange::createGroup(RsNxsGrp *grp, RsTlvSecurityKeySet& privateKeySet, RsTlvSecurityKeySet& publicKeySet)
{
#ifdef GEN_EXCH_DEBUG
    std::cerr << "RsGenExchange::createGroup()";
    std::cerr << std::endl;
#endif

    RsGxsGrpMetaData* meta = grp->metaData;

    /* add public admin and publish keys to grp */

    // find private admin key
    RsTlvSecurityKey privAdminKey;
    std::map<RsGxsId, RsTlvSecurityKey>::iterator mit = privateKeySet.keys.begin();

    bool privKeyFound = false; // private admin key
    for(; mit != privateKeySet.keys.end(); ++mit)
    {
        RsTlvSecurityKey& key = mit->second;

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

    meta->keys = publicKeySet; // only public keys are included to be transported

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
                RsTlvSecurityKey authorKey;
                mGixs->getPrivateKey(grpMeta.mAuthorId, authorKey);
                RsTlvKeySignature sign;

                if(GxsSecurity::getSignature((char*)grpData.bin_data, grpData.bin_len,
                                                authorKey, sign))
                {
                	id_ret = SIGN_SUCCESS;
                    	mGixs->timeStampKey(grpMeta.mAuthorId) ;
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
                                        const RsGxsMsgMetaData& msgMeta, RsGxsGrpMetaData& grpMeta)
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
        RsTlvSecurityKeySet& keys = grpMeta.keys;
        RsTlvSecurityKey* publishKey;

        std::map<RsGxsId, RsTlvSecurityKey>::iterator mit =
                        keys.keys.begin(), mit_end = keys.keys.end();
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
		    RsTlvSecurityKey authorKey;
		    mGixs->getPrivateKey(msgMeta.mAuthorId, authorKey);
		    RsTlvKeySignature sign;

		    if(GxsSecurity::getSignature((char*)msgData.bin_data, msgData.bin_len, authorKey, sign))
		    {
			    id_ret = SIGN_SUCCESS;
			    mGixs->timeStampKey(msgMeta.mAuthorId) ;
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
	std::map<RsGxsGroupId, RsGxsGrpMetaData*> metaMap;

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
		RsGxsGrpMetaData* grpMeta = metaMap[id];

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
		msg->msgId = hashId;

		// assign msg id to msg meta
		msg->metaData->mMsgId = msg->msgId;

		delete[] metaData;
		delete[] allMsgData;

		delete grpMeta;
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

int RsGenExchange::validateMsg(RsNxsMsg *msg, const uint32_t& grpFlag, const uint32_t& signFlag, RsTlvSecurityKeySet& grpKeySet)
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

        std::map<RsGxsId, RsTlvSecurityKey>& keys = grpKeySet.keys;
        std::map<RsGxsId, RsTlvSecurityKey>::iterator mit = keys.begin();

        RsGxsId keyId;
        for(; mit != keys.end() ; ++mit)
    {
        RsTlvSecurityKey& key = mit->second;

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
            RsTlvSecurityKey& key = keys[keyId];
            publishValidate &= GxsSecurity::validateNxsMsg(*msg, sign, key);
        }
        else
        {
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

		    RsTlvSecurityKey authorKey;
		    bool auth_key_fetched = mGixs->getKey(metaData.mAuthorId, authorKey) ;

		    if (auth_key_fetched)
		    {
			    RsTlvKeySignature sign = metaData.signSet.keySignSet[INDEX_AUTHEN_IDENTITY];
			    idValidate &= GxsSecurity::validateNxsMsg(*msg, sign, authorKey);
			    mGixs->timeStampKey(metaData.mAuthorId) ;
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

				// now check reputation of the message author
				float reputation_threshold =  ( (signFlag & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG) && !(details.mFlags & RS_IDENTITY_FLAGS_PGP_LINKED)) ? (RsReputations::REPUTATION_THRESHOLD_ANTI_SPAM): (RsReputations::REPUTATION_THRESHOLD_DEFAULT) ;

				if(details.mReputation.mOverallReputationScore < reputation_threshold)
				{
#ifdef GEN_EXCH_DEBUG	
					std::cerr << "RsGenExchange::validateMsg(): message from " << metaData.mAuthorId << ", rejected because reputation score (" << details.mReputation.mOverallReputationScore <<") is below the accepted threshold (" << reputation_threshold << ")" << std::endl;
#endif
					idValidate = false ;
				}
#ifdef GEN_EXCH_DEBUG	
				else
					std::cerr << "RsGenExchange::validateMsg(): message from " << metaData.mAuthorId << ", accepted. Reputation score (" << details.mReputation.mOverallReputationScore <<") is above accepted threshold (" << reputation_threshold << ")" << std::endl;
#endif
			}

		}
	    }
            else
            {
                std::list<RsPeerId> peers;
                peers.push_back(msg->PeerId());
                mGixs->requestKey(metaData.mAuthorId, peers);
                
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

			    RsTlvSecurityKey authorKey;
			    bool auth_key_fetched = mGixs->getKey(metaData.mAuthorId, authorKey) ;

			    if (auth_key_fetched)
			    {

				    RsTlvKeySignature sign = metaData.signSet.keySignSet[INDEX_AUTHEN_IDENTITY];
				    idValidate = GxsSecurity::validateNxsGrp(*grp, sign, authorKey);

#ifdef GEN_EXCH_DEBUG
				    std::cerr << "  key ID validation result: " << idValidate << std::endl;
#endif
				    mGixs->timeStampKey(metaData.mAuthorId) ;
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
			    mGixs->requestKey(metaData.mAuthorId, peers);
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

static void addMessageChanged(std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &msgs, const std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &msgChanged)
{
    if (msgs.empty()) {
        msgs = msgChanged;
    } else {
        std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >::const_iterator mapIt;
        for (mapIt = msgChanged.begin(); mapIt != msgChanged.end(); ++mapIt) {
            const RsGxsGroupId &grpId = mapIt->first;
            const std::vector<RsGxsMessageId> &srcMsgIds = mapIt->second;
            std::vector<RsGxsMessageId> &destMsgIds = msgs[grpId];

            std::vector<RsGxsMessageId>::const_iterator msgIt;
            for (msgIt = srcMsgIds.begin(); msgIt != srcMsgIds.end(); ++msgIt) {
                if (std::find(destMsgIds.begin(), destMsgIds.end(), *msgIt) == destMsgIds.end()) {
                    destMsgIds.push_back(*msgIt);
                }
            }
        }
    }
}

void RsGenExchange::receiveChanges(std::vector<RsGxsNotify*>& changes)
{
#ifdef GEN_EXCH_DEBUG
    std::cerr << "RsGenExchange::receiveChanges()" << std::endl;
#endif
    RsGxsChanges out;
    out.mService = getTokenService();

    // collect all changes in one GxsChanges object
    std::vector<RsGxsNotify*>::iterator vit = changes.begin();
    for(; vit != changes.end(); ++vit)
    {
        RsGxsNotify* n = *vit;
        RsGxsGroupChange* gc;
        RsGxsMsgChange* mc;
        if((mc = dynamic_cast<RsGxsMsgChange*>(n)) != NULL)
        {
            if (mc->metaChange())
            {
                addMessageChanged(out.mMsgsMeta, mc->msgChangeMap);
            }
            else
            {
                addMessageChanged(out.mMsgs, mc->msgChangeMap);
            }
        }
        else if((gc = dynamic_cast<RsGxsGroupChange*>(n)) != NULL)
        {
            if(gc->metaChange())
            {
                out.mGrpsMeta.splice(out.mGrpsMeta.end(), gc->mGrpIdList);
            }
            else
            {
                out.mGrps.splice(out.mGrps.end(), gc->mGrpIdList);
            }
        }
        else
            std::cerr << "(EE) Unknown changes type!!" << std::endl;
        
        delete n;
    }
    changes.clear() ;
    
    RsServer::notify()->notifyGxsChange(out);
}

bool RsGenExchange::subscribeToGroup(uint32_t& token, const RsGxsGroupId& grpId, bool subscribe)
{
    if(subscribe)
        setGroupSubscribeFlags(token, grpId, GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED,
                               (GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED | GXS_SERV::GROUP_SUBSCRIBE_NOT_SUBSCRIBED));
    else
        setGroupSubscribeFlags(token, grpId, GXS_SERV::GROUP_SUBSCRIBE_NOT_SUBSCRIBED,
                               (GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED | GXS_SERV::GROUP_SUBSCRIBE_NOT_SUBSCRIBED));

    if(mNetService != NULL)
        mNetService->subscribeStatusChanged(grpId,subscribe) ;
    else
        std::cerr << "(EE) No mNetService in RsGenExchange for service 0x" << std::hex << mServType << std::dec << std::endl;

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
	return mDataAccess->getMsgList(token, msgIds);
}

bool RsGenExchange::getMsgRelatedList(const uint32_t &token, MsgRelatedIdResult &msgIds)
{
    return mDataAccess->getMsgRelatedList(token, msgIds);
}

bool RsGenExchange::getGroupMeta(const uint32_t &token, std::list<RsGroupMetaData> &groupInfo)
{
	std::list<RsGxsGrpMetaData*> metaL;
	bool ok = mDataAccess->getGroupSummary(token, metaL);

	std::list<RsGxsGrpMetaData*>::iterator lit = metaL.begin();
	RsGroupMetaData m;
	for(; lit != metaL.end(); ++lit)
	{
		RsGxsGrpMetaData& gMeta = *(*lit);
        m = gMeta;
        RsGroupNetworkStats sts ;

    if(mNetService != NULL && mNetService->getGroupNetworkStats((*lit)->mGroupId,sts))
    {
        m.mPop = sts.mSuppliers ;
        m.mVisibleMsgCount = sts.mMaxVisibleCount ;
    }
    else
    {
        m.mPop= 0 ;
        m.mVisibleMsgCount = 0 ;
        }

        groupInfo.push_back(m);
		delete (*lit);
	}

	return ok;
}

bool RsGenExchange::getMsgMeta(const uint32_t &token,
                               GxsMsgMetaMap &msgInfo)
{
#ifdef GEN_EXCH_DEBUG
	std::cerr << "RsGenExchange::getMsgMeta(): retrieving meta data for token " << token << std::endl;
#endif
	std::list<RsGxsMsgMetaData*> metaL;
	GxsMsgMetaResult result;
	bool ok = mDataAccess->getMsgSummary(token, result);

	GxsMsgMetaResult::iterator mit = result.begin();

	for(; mit != result.end(); ++mit)
	{
		std::vector<RsGxsMsgMetaData*>& metaV = mit->second;

		std::vector<RsMsgMetaData>& msgInfoV = msgInfo[mit->first];

		std::vector<RsGxsMsgMetaData*>::iterator vit = metaV.begin();
		RsMsgMetaData meta;
		for(; vit != metaV.end(); ++vit)
		{
			RsGxsMsgMetaData& m = *(*vit);
			meta = m;
			msgInfoV.push_back(meta);
			delete *vit;
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
                std::vector<RsGxsMsgMetaData*>& metaV = mit->second;

                std::vector<RsMsgMetaData>& msgInfoV = msgMeta[mit->first];

                std::vector<RsGxsMsgMetaData*>::iterator vit = metaV.begin();
                RsMsgMetaData meta;
                for(; vit != metaV.end(); ++vit)
                {
                        RsGxsMsgMetaData& m = *(*vit);
                        meta = m;
                        msgInfoV.push_back(meta);
                        delete *vit;
                }
                metaV.clear();
        }

        return ok;
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

            if(mNetService != NULL && mNetService->getGroupNetworkStats(gItem->meta.mGroupId,sts))
        {
                gItem->meta.mPop = sts.mSuppliers ;
        gItem->meta.mVisibleMsgCount  = sts.mMaxVisibleCount;
        }
            else
        {
                gItem->meta.mPop = 0 ;
        gItem->meta.mVisibleMsgCount = 0 ;
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
			else
			{
				std::cerr << "RsGenExchange::getGroupData() ERROR deserialising item";
				std::cerr << std::endl;
			}
			delete *lit;
		}
	}
	return ok;
}

bool RsGenExchange::getMsgData(const uint32_t &token, GxsMsgDataMap &msgItems)
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

bool RsGenExchange::getMsgRelatedData(const uint32_t &token, GxsMsgRelatedDataMap &msgItems)
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

void RsGenExchange::notifyNewGroups(std::vector<RsNxsGrp *> &groups)
{
	RS_STACK_MUTEX(mGenMtx) ;

    std::vector<RsNxsGrp*>::iterator vit = groups.begin();

    // store these for tick() to pick them up
    for(; vit != groups.end(); ++vit)
    {
    	RsNxsGrp* grp = *vit;
    	NxsGrpPendValidVect::iterator received = std::find(mReceivedGrps.begin(),
    			mReceivedGrps.end(), grp->grpId);

    	// drop group if you already have them
    	// TODO: move this to nxs layer to save bandwidth
    	if(received == mReceivedGrps.end())
    	{
#ifdef GEN_EXCH_DEBUG
		std::cerr << "RsGenExchange::notifyNewGroups() Received GrpId: " << grp->grpId;
		std::cerr << std::endl;
#endif

    		GxsPendingItem<RsNxsGrp*, RsGxsGroupId> gpsi(grp, grp->grpId);
    		mReceivedGrps.push_back(gpsi);
    	}
    	else
    	{
    		delete grp;
    	}
    }

}


void RsGenExchange::notifyNewMessages(std::vector<RsNxsMsg *>& messages)
{
	RS_STACK_MUTEX(mGenMtx) ;

    std::vector<RsNxsMsg*>::iterator vit = messages.begin();

    // store these for tick() to pick them up
    for(; vit != messages.end(); ++vit)
    {
    	RsNxsMsg* msg = *vit;

    	NxsMsgPendingVect::iterator it =
    			std::find(mMsgPendingValidate.begin(), mMsgPendingValidate.end(), getMsgIdPair(*msg));

    	// if we have msg already just delete it
    	if(it == mMsgPendingValidate.end())
	{
#ifdef GEN_EXCH_DEBUG
		std::cerr << "RsGenExchange::notifyNewMessages() Received Msg: ";
		std::cerr << " GrpId: " << msg->grpId;
		std::cerr << " MsgId: " << msg->msgId;
		std::cerr << std::endl;
#endif

    		mReceivedMsgs.push_back(msg);
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

void RsGenExchange::notifyReceivePublishKey(const RsGxsGroupId &grpId)
{
	RS_STACK_MUTEX(mGenMtx);

	RsGxsGroupChange* gc = new RsGxsGroupChange(RsGxsNotify::TYPE_PUBLISHKEY, false);
	gc->mGrpIdList.push_back(grpId);
	mNotifications.push_back(gc);
}

void RsGenExchange::notifyChangedGroupStats(const RsGxsGroupId &grpId)
{
	RS_STACK_MUTEX(mGenMtx);

	RsGxsGroupChange* gc = new RsGxsGroupChange(RsGxsNotify::TYPE_PROCESSED, false);
	gc->mGrpIdList.push_back(grpId);
	mNotifications.push_back(gc);
}

void RsGenExchange::publishGroup(uint32_t& token, RsGxsGrpItem *grpItem)
{

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
					RS_STACK_MUTEX(mGenMtx) ;
	token = mDataAccess->generatePublicToken();
        mGroupUpdatePublish.push_back(GroupUpdatePublish(grpItem, token));

#ifdef GEN_EXCH_DEBUG
    std::cerr << "RsGenExchange::updateGroup() token: " << token;
    std::cerr << std::endl;
#endif
}

void RsGenExchange::deleteGroup(uint32_t& token, RsGxsGrpItem* grpItem)
{
					RS_STACK_MUTEX(mGenMtx) ;
	token = mDataAccess->generatePublicToken();
	mGroupDeletePublish.push_back(GroupDeletePublish(grpItem, token));

#ifdef GEN_EXCH_DEBUG
    std::cerr << "RsGenExchange::deleteGroup() token: " << token;
	std::cerr << std::endl;
#endif
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
					RS_STACK_MUTEX(mGenMtx) ;
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
                std::vector<RsGxsMessageId> msgIdV;
                msgIdV.push_back(m.msgId.second);
                req.insert(std::make_pair(m.msgId.first, msgIdV));
                GxsMsgMetaResult result;
                mDataStore->retrieveGxsMsgMetaData(req, result);
                GxsMsgMetaResult::iterator mit = result.find(m.msgId.first);

                if(mit != result.end())
                {
                    std::vector<RsGxsMsgMetaData*>& msgMetaV = mit->second;

                    if(!msgMetaV.empty())
                    {
                        RsGxsMsgMetaData* meta = *(msgMetaV.begin());
                        value = (meta->mMsgStatus & ~mask) | (mask & value);
                        changed = (meta->mMsgStatus != value);
                        m.val.put(RsGeneralDataService::MSG_META_STATUS, value);
                        delete meta;
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
            mDataAccess->updatePublicRequestStatus(token, RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE);
            if (changed)
            {
                msgIds[m.msgId.first].push_back(m.msgId.second);
            }
        }
        else
        {
            mDataAccess->updatePublicRequestStatus(token, RsTokenService::GXS_REQUEST_V2_STATUS_FAILED);
        }

        {
            RS_STACK_MUTEX(mGenMtx);
            mMsgNotify.insert(std::make_pair(token, m.msgId));
        }
    }

    if (!msgIds.empty()) {
        RS_STACK_MUTEX(mGenMtx);
        RsGxsMsgChange* c = new RsGxsMsgChange(RsGxsNotify::TYPE_PROCESSED, true);
        c->msgChangeMap = msgIds;
        mNotifications.push_back(c);
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
            mDataAccess->updatePublicRequestStatus(token, RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE);
            grpChanged.push_back(g.grpId);
        }else
        {
            mDataAccess->updatePublicRequestStatus(token, RsTokenService::GXS_REQUEST_V2_STATUS_FAILED);
        }

        {
            RS_STACK_MUTEX(mGenMtx);
            mGrpNotify.insert(std::make_pair(token, g.grpId));
        }
    }

    if(!grpChanged.empty())
    {
        RS_STACK_MUTEX(mGenMtx);
        RsGxsGroupChange* gc = new RsGxsGroupChange(RsGxsNotify::TYPE_PROCESSED, true);
        gc->mGrpIdList = grpChanged;
        mNotifications.push_back(gc);
#ifdef GEN_EXCH_DEBUG
                    			std::cerr << "  adding the following grp ids to notification: " << std::endl;
                                	for(std::list<RsGxsGroupId>::const_iterator it(grpChanged.begin());it!=grpChanged.end();++it)
                                        	std::cerr << "    " << *it << std::endl;
#endif
    }
}

bool RsGenExchange::processGrpMask(const RsGxsGroupId& grpId, ContentValue &grpCv)
{
    // first find out which mask is involved
    int32_t value, mask, currValue;
    std::string key;
    RsGxsGrpMetaData* grpMeta = NULL;
    bool ok = false;

    std::map<RsGxsGroupId, RsGxsGrpMetaData* > grpMetaMap;
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
    }else
    {
        if(grpMeta)
            delete grpMeta;
        return !(grpCv.empty());
    }

    ok &= grpCv.getAsInt32(key+GXS_MASK, mask);

    // remove mask entry so it doesn't affect actual entry
    grpCv.removeKeyValue(key+GXS_MASK);

    // apply mask to current value
    value = (currValue & ~mask) | (value & mask);

    grpCv.put(key, value);

    if(grpMeta)
        delete grpMeta;

    return ok;
}

void RsGenExchange::publishMsgs()
{

					RS_STACK_MUTEX(mGenMtx) ;

	// stick back msgs pending signature
	typedef std::map<uint32_t, GxsPendingItem<RsGxsMsgItem*, uint32_t> > PendSignMap;

	PendSignMap::iterator sign_it = mMsgPendingSign.begin();

	for(; sign_it != mMsgPendingSign.end(); ++sign_it)
	{
		GxsPendingItem<RsGxsMsgItem*, uint32_t>& item = sign_it->second;
		mMsgsToPublish.insert(std::make_pair(sign_it->first, item.mItem));
	}

	std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > msgChangeMap;
	std::map<uint32_t, RsGxsMsgItem*>::iterator mit = mMsgsToPublish.begin();

	for(; mit != mMsgsToPublish.end(); ++mit)
	{
#ifdef GEN_EXCH_DEBUG
		std::cerr << "RsGenExchange::publishMsgs() Publishing a Message";
		std::cerr << std::endl;
#endif

		RsGxsMsgItem* msgItem = mit->second;
		const uint32_t& token = mit->first;

		uint32_t size = mSerialiser->size(msgItem);
		char* mData = new char[size];

		bool serialOk = false;

		// for fatal sign creation
		bool createOk = false;

		// if sign requests to try later
		bool tryLater = false;

		serialOk = mSerialiser->serialise(msgItem, mData, &size);

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
					GxsPendingItem<RsGxsMsgItem*, uint32_t> gsi(msgItem, token);
					mMsgPendingSign.insert(std::make_pair(token, gsi));
				}
				else
				{
					// remove from attempts queue if over sign
					// attempts limit
					if(pit->second.mAttempts == SIGN_MAX_ATTEMPTS)
					{
						mMsgPendingSign.erase(token);
						tryLater = false;
					}
					else
					{
						++pit->second.mAttempts;
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
				// msg
				if(msg->metaData->mOrigMsgId.isNull())
				{
					msg->metaData->mOrigMsgId = msg->metaData->mMsgId;
				}

				// now serialise meta data
				size = msg->metaData->serial_size();

				char* metaDataBuff = new char[size];
				bool s = msg->metaData->serialise(metaDataBuff, &size);
				s &= msg->meta.setBinData(metaDataBuff, size);

				msg->metaData->mMsgStatus = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;
				msgId = msg->msgId;
				grpId = msg->grpId;
				msg->metaData->recvTS = time(NULL);
                
                // FIXTESTS global variable rsPeers not available in unittests!
                if(rsPeers)
                {
                    mRoutingClues[msg->metaData->mAuthorId].insert(rsPeers->getOwnId()) ;
                    mTrackingClues.push_back(std::make_pair(msg->msgId,rsPeers->getOwnId())) ;
                }
                
				computeHash(msg->msg, msg->metaData->mHash);
				mDataAccess->addMsgData(msg);
				msgChangeMap[grpId].push_back(msgId);

				delete[] metaDataBuff;

				// add to published to allow acknowledgement
				mMsgNotify.insert(std::make_pair(mit->first, std::make_pair(grpId, msgId)));
				mDataAccess->updatePublicRequestStatus(mit->first, RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE);

			}
			else
			{
				// delete msg if create msg not ok
				delete msg;

				if(!tryLater)
					mDataAccess->updatePublicRequestStatus(mit->first,
							RsTokenService::GXS_REQUEST_V2_STATUS_FAILED);

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

	if(!msgChangeMap.empty())
	{
		RsGxsMsgChange* ch = new RsGxsMsgChange(RsGxsNotify::TYPE_PUBLISH, false);
		ch->msgChangeMap = msgChangeMap;
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
	std::map<RsGxsGroupId, RsGxsGrpMetaData*> grpMeta;
	std::vector<GroupUpdatePublish>::iterator vit = mGroupUpdatePublish.begin();

	for(; vit != mGroupUpdatePublish.end(); ++vit)
	{
		GroupUpdatePublish& gup = *vit;
		const RsGxsGroupId& groupId = gup.grpItem->meta.mGroupId;
		grpMeta.insert(std::make_pair(groupId, (RsGxsGrpMetaData*)(NULL)));
	}

        if(grpMeta.empty())
            return;

	mDataStore->retrieveGxsGrpMetaData(grpMeta);

	// now
	vit = mGroupUpdatePublish.begin();
	for(; vit != mGroupUpdatePublish.end(); ++vit)
	{
		GroupUpdatePublish& gup = *vit;
		const RsGxsGroupId& groupId = gup.grpItem->meta.mGroupId;
		std::map<RsGxsGroupId, RsGxsGrpMetaData*>::iterator mit = grpMeta.find(groupId);

		RsGxsGrpMetaData* meta = NULL;
		if(mit == grpMeta.end() || mit->second == NULL)
		{
			std::cerr << "Error! could not find meta of old group to update!" << std::endl;
			mDataAccess->updatePublicRequestStatus(gup.mToken, RsTokenService::GXS_REQUEST_V2_STATUS_FAILED);
			delete gup.grpItem;
			continue;
		}else
		{
			meta = mit->second;
		}


		//gup.grpItem->meta = *meta;
        GxsGrpPendingSign ggps(gup.grpItem, gup.mToken);

		bool publishAndAdminPrivatePresent = checkKeys(meta->keys);

		if(publishAndAdminPrivatePresent)
		{
			ggps.mPrivateKeys = meta->keys;
			generatePublicFromPrivateKeys(ggps.mPrivateKeys, ggps.mPublicKeys);
			ggps.mHaveKeys = true;
			ggps.mStartTS = time(NULL);
			ggps.mLastAttemptTS = 0;
			ggps.mIsUpdate = true;
			ggps.mToken = gup.mToken;
			mGrpsToPublish.push_back(ggps);
		}else
		{
			delete gup.grpItem;
			mDataAccess->updatePublicRequestStatus(gup.mToken, RsTokenService::GXS_REQUEST_V2_STATUS_FAILED);
		}
		delete meta;
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
    
    for(std::list<std::pair<RsGxsMessageId,RsPeerId> >::const_iterator it = mTrackingClues.begin();it!=mTrackingClues.end();++it)
	    rsGRouter->addTrackingInfo((*it).first,(*it).second) ;
    
    mTrackingClues.clear() ;
}
void RsGenExchange::processGroupDelete()
{
					RS_STACK_MUTEX(mGenMtx) ;

    // get keys for group delete publish
	typedef std::pair<bool, RsGxsGroupId> GrpNote;
	std::map<uint32_t, GrpNote> toNotify;

	std::vector<GroupDeletePublish>::iterator vit = mGroupDeletePublish.begin();
	for(; vit != mGroupDeletePublish.end(); ++vit)
	{
		GroupDeletePublish& gdp = *vit;
		uint32_t token = gdp.mToken;
		const RsGxsGroupId& groupId = gdp.grpItem->meta.mGroupId;
		std::vector<RsGxsGroupId> gprIds;
		gprIds.push_back(groupId);
		mDataStore->removeGroups(gprIds);
		toNotify.insert(std::make_pair(
		                  token, GrpNote(true, groupId)));
	}


	std::list<RsGxsGroupId> grpDeleted;
	std::map<uint32_t, GrpNote>::iterator mit = toNotify.begin();
	for(; mit != toNotify.end(); ++mit)
	{
		GrpNote& note = mit->second;
		uint8_t status = note.first ? RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE
		                            : RsTokenService::GXS_REQUEST_V2_STATUS_FAILED;

		mGrpNotify.insert(std::make_pair(mit->first, note.second));
		mDataAccess->updatePublicRequestStatus(mit->first, status);

		if(note.first)
			grpDeleted.push_back(note.second);
	}

	if(!grpDeleted.empty())
	{
		RsGxsGroupChange* gc = new RsGxsGroupChange(RsGxsNotify::TYPE_PUBLISH, false);
		gc->mGrpIdList = grpDeleted;
		mNotifications.push_back(gc);
	}

	mGroupDeletePublish.clear();
}

bool RsGenExchange::checkKeys(const RsTlvSecurityKeySet& keySet)
{

	typedef std::map<RsGxsId, RsTlvSecurityKey> keyMap;
	const keyMap& allKeys = keySet.keys;
	keyMap::const_iterator cit = allKeys.begin();
        bool adminFound = false, publishFound = false;
	for(; cit != allKeys.end(); ++cit)
	{
                const RsTlvSecurityKey& key = cit->second;
                if(key.keyFlags & RSTLV_KEY_TYPE_FULL)
                {
                    if(key.keyFlags & RSTLV_KEY_DISTRIB_ADMIN)
                        adminFound = true;

                    if(key.keyFlags & RSTLV_KEY_DISTRIB_PUBLISH)
                        publishFound = true;

                }
	}

	// user must have both private and public parts of publish and admin keys
        return adminFound && publishFound;
}

void RsGenExchange::publishGrps()
{
					RS_STACK_MUTEX(mGenMtx) ;
    NxsGrpSignPendVect::iterator vit = mGrpsToPublish.begin();

    typedef std::pair<bool, RsGxsGroupId> GrpNote;
    std::map<uint32_t, GrpNote> toNotify;

    while( vit != mGrpsToPublish.end() )
    {
    	GxsGrpPendingSign& ggps = *vit;

    	/* do intial checks to see if this entry has expired */
    	time_t now = time(NULL) ;
    	uint32_t token = ggps.mToken;


    	if(now > (ggps.mStartTS + PENDING_SIGN_TIMEOUT) )
    	{
    		// timed out
    		toNotify.insert(std::make_pair(
    				token, GrpNote(false, RsGxsGroupId())));
    		delete ggps.mItem;
    		vit = mGrpsToPublish.erase(vit);

    		continue;
    	}

    	RsGxsGroupId grpId;
        RsNxsGrp* grp = new RsNxsGrp(mServType);
        RsGxsGrpItem* grpItem = ggps.mItem;

        RsTlvSecurityKeySet privatekeySet, publicKeySet;

        if(!(ggps.mHaveKeys))
        {
        	generateGroupKeys(privatekeySet, publicKeySet, true);
        	ggps.mHaveKeys = true;
        	ggps.mPrivateKeys = privatekeySet;
        	ggps.mPublicKeys = publicKeySet;
        }
        else
        {
        	privatekeySet = ggps.mPrivateKeys;
        	publicKeySet = ggps.mPublicKeys;
        }

		// find private admin key
        RsTlvSecurityKey privAdminKey;
        std::map<RsGxsId, RsTlvSecurityKey>::iterator mit_keys = privatekeySet.keys.begin();

        bool privKeyFound = false;
        for(; mit_keys != privatekeySet.keys.end(); ++mit_keys)
        {
            RsTlvSecurityKey& key = mit_keys->second;

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

			ServiceCreate_Return ret = service_CreateGroup(grpItem, privatekeySet);


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

				create = createGroup(grp, privatekeySet, publicKeySet);

#ifdef GEN_EXCH_DEBUG
				std::cerr << "RsGenExchange::publishGrps() ";
				std::cerr << " GrpId: " << grp->grpId;
				std::cerr << " CircleType: " << (uint32_t) grp->metaData->mCircleType;
				std::cerr << " CircleId: " << grp->metaData->mCircleId.toStdString();
				std::cerr << std::endl;
#endif

				if(create == CREATE_SUCCESS)
				{

					uint32_t mdSize = grp->metaData->serial_size(RS_GXS_GRP_META_DATA_CURRENT_API_VERSION);
					char* metaData = new char[mdSize];
					serialOk = grp->metaData->serialise(metaData, mdSize,RS_GXS_GRP_META_DATA_CURRENT_API_VERSION);
					grp->meta.setBinData(metaData, mdSize);
					delete[] metaData;

					// place back private keys for publisher
					grp->metaData->keys = privatekeySet;

					if(mDataStore->validSize(grp) && serialOk)
					{
						grpId = grp->grpId;
						computeHash(grp->grp, grp->metaData->mHash);
						grp->metaData->mRecvTS = time(NULL);

						if(ggps.mIsUpdate)
							mDataAccess->updateGroupData(grp);
						else
                            mDataAccess->addGroupData(grp);

                        if(mNetService!=NULL)
                mNetService->subscribeStatusChanged(grpId,true) ;
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
            toNotify.insert(std::make_pair(
            		token, GrpNote(false, grpId)));

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
			toNotify.insert(std::make_pair(token,
					GrpNote(true,grpId)));
        }
    }

    std::map<uint32_t, GrpNote>::iterator mit = toNotify.begin();

    std::list<RsGxsGroupId> grpChanged;
    for(; mit != toNotify.end(); ++mit)
    {
    	GrpNote& note = mit->second;
    	uint8_t status = note.first ? RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE
    			: RsTokenService::GXS_REQUEST_V2_STATUS_FAILED;

    	mGrpNotify.insert(std::make_pair(mit->first, note.second));
		mDataAccess->updatePublicRequestStatus(mit->first, status);

		if(note.first)
			grpChanged.push_back(note.second);
    }

    if(!grpChanged.empty())
    {
    	RsGxsGroupChange* gc = new RsGxsGroupChange(RsGxsNotify::TYPE_PUBLISH, false);
    	gc->mGrpIdList = grpChanged;
    	mNotifications.push_back(gc);
#ifdef GEN_EXCH_DEBUG
                    			std::cerr << "  adding the following grp ids to notification: " << std::endl;
                                	for(std::list<RsGxsGroupId>::const_iterator it(grpChanged.begin());it!=grpChanged.end();++it)
                                        	std::cerr << "    " << *it << std::endl;
#endif
    }

}



uint32_t RsGenExchange::generatePublicToken()
{
    return mDataAccess->generatePublicToken();
}

bool RsGenExchange::updatePublicRequestStatus(const uint32_t &token, const uint32_t &status)
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

    std::map<RsGxsGroupId, RsGxsGrpMetaData*> grpMeta;
    grpMeta[grpId] = NULL;
    mDataStore->retrieveGxsGrpMetaData(grpMeta);

    if(grpMeta.empty())
        return false;

    RsGxsGrpMetaData* meta = grpMeta[grpId];

    if(meta == NULL)
        return false;

    keySet = meta->keys;

    for(std::map<RsGxsGroupId, RsGxsGrpMetaData*>::iterator it=grpMeta.begin();it!=grpMeta.end();++it)
        delete it->second ;

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
	RS_STACK_MUTEX(mGenMtx) ;

#ifdef GEN_EXCH_DEBUG
	if(!mMsgPendingValidate.empty())
		std::cerr << "processing received messages" << std::endl;
#endif
	NxsMsgPendingVect::iterator pend_it = mMsgPendingValidate.begin();

#ifdef GEN_EXCH_DEBUG
	if(!mMsgPendingValidate.empty())
		std::cerr << "  pending validation" << std::endl;
#endif
	for(; pend_it != mMsgPendingValidate.end();)
	{
		GxsPendingItem<RsNxsMsg*, RsGxsGrpMsgIdPair>& gpsi = *pend_it;

#ifdef GEN_EXCH_DEBUG
		std::cerr << "    grp=" << gpsi.mId.first << ", msg=" << gpsi.mId.second << ", attempts=" << gpsi.mAttempts ;
#endif
		if(gpsi.mAttempts == VALIDATE_MAX_ATTEMPTS)
		{
#ifdef GEN_EXCH_DEBUG
			std::cerr << " = max! deleting." << std::endl;
#endif
			delete gpsi.mItem;
			pend_it = mMsgPendingValidate.erase(pend_it);
		}
		else
		{
#ifdef GEN_EXCH_DEBUG
			std::cerr << " movign to recvd." << std::endl;
#endif
			mReceivedMsgs.push_back(gpsi.mItem);
			++pend_it;
		}
	}

	if(mReceivedMsgs.empty())
		return;

	std::vector<RsNxsMsg*>::iterator vit = mReceivedMsgs.begin();
	GxsMsgReq msgIds;
	std::map<RsNxsMsg*, RsGxsMsgMetaData*> msgs;

	std::map<RsGxsGroupId, RsGxsGrpMetaData*> grpMetas;

	// coalesce group meta retrieval for performance
	for(; vit != mReceivedMsgs.end(); ++vit)
	{
		RsNxsMsg* msg = *vit;
		grpMetas.insert(std::make_pair(msg->grpId, (RsGxsGrpMetaData*)NULL));
	}

	mDataStore->retrieveGxsGrpMetaData(grpMetas);

#ifdef GEN_EXCH_DEBUG
	std::cerr << "  updating received messages:" << std::endl;
#endif
	for(vit = mReceivedMsgs.begin(); vit != mReceivedMsgs.end(); ++vit)
	{
		RsNxsMsg* msg = *vit;
		RsGxsMsgMetaData* meta = new RsGxsMsgMetaData();

		bool ok = false;

		if(msg->meta.bin_len != 0)
			ok = meta->deserialise(msg->meta.bin_data, &(msg->meta.bin_len));

		msg->metaData = meta;

#ifdef GEN_EXCH_DEBUG
		std::cerr << "    deserialised info: grp id=" << meta->mGroupId << ", msg id=" << meta->mMsgId ;
#endif
		uint8_t validateReturn = VALIDATE_FAIL;

		if(ok)
		{
			std::map<RsGxsGroupId, RsGxsGrpMetaData*>::iterator mit = grpMetas.find(msg->grpId);

#ifdef GEN_EXCH_DEBUG
			std::cerr << "    msg info         : grp id=" << msg->grpId << ", msg id=" << msg->msgId << std::endl;
#endif
			RsGxsGrpMetaData* grpMeta = NULL ;

			// validate msg
			if(mit != grpMetas.end())
			{
				grpMeta = mit->second;
				validateReturn = validateMsg(msg, grpMeta->mGroupFlags, grpMeta->mSignFlags, grpMeta->keys);

#ifdef GEN_EXCH_DEBUG
				std::cerr << "    grpMeta.mSignFlags: " << std::hex << grpMeta->mSignFlags << std::dec << std::endl;
				std::cerr << "    grpMeta.mAuthFlags: " << std::hex << grpMeta->mAuthenFlags << std::dec << std::endl;
				std::cerr << "    message validation result: " << (int)validateReturn << std::endl;
#endif
			}

			if(validateReturn == VALIDATE_SUCCESS)
			{
				meta->mMsgStatus = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED | GXS_SERV::GXS_MSG_STATUS_GUI_NEW | GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD;
				msgs.insert(std::make_pair(msg, meta));

				std::vector<RsGxsMessageId> &msgv = msgIds[msg->grpId];
				if (std::find(msgv.begin(), msgv.end(), msg->msgId) == msgv.end())
				{
					msgv.push_back(msg->msgId);
				}

				NxsMsgPendingVect::iterator validated_entry = std::find(mMsgPendingValidate.begin(), mMsgPendingValidate.end(),
				                                                        getMsgIdPair(*msg));

				if(validated_entry != mMsgPendingValidate.end()) mMsgPendingValidate.erase(validated_entry);

				computeHash(msg->msg, meta->mHash);
				meta->recvTS = time(NULL);
#ifdef GEN_EXCH_DEBUG
				std::cerr << "    new status flags: " << meta->mMsgStatus << std::endl;
				std::cerr << "    computed hash: " << meta->mHash << std::endl;
				std::cerr << "Message received. Identity=" << msg->metaData->mAuthorId << ", from peer " << msg->PeerId() << std::endl;
#endif

				if(!msg->metaData->mAuthorId.isNull())
					mRoutingClues[msg->metaData->mAuthorId].insert(msg->PeerId()) ;

				if(grpMeta->mSignFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_TRACK_MESSAGES)
					mTrackingClues.push_back(std::make_pair(msg->msgId,msg->PeerId())) ;
			}
            
            		if(validateReturn == VALIDATE_FAIL)
		    	{
                        	// In this case, we notify the network exchange service not to DL the message again, at least not yet. 
                        
#ifdef GEN_EXCH_DEBUG
                        	std::cerr << "Notifying the network service to not download this message again." << std::endl;
#endif
                        	mNetService->rejectMessage(msg->msgId) ;
		    	}
		}
		else
		{
#ifdef GEN_EXCH_DEBUG
			std::cerr << " deserialisation failed!" <<std::endl;
#endif
			validateReturn = VALIDATE_FAIL;
		}

		if(validateReturn == VALIDATE_FAIL)
		{

#ifdef GEN_EXCH_DEBUG
			std::cerr << "Validation failed for message id "
			          << "msg->grpId: " << msg->grpId << ", msgId: " << msg->msgId << std::endl;
#endif

			NxsMsgPendingVect::iterator failed_entry = std::find(mMsgPendingValidate.begin(), mMsgPendingValidate.end(),
			                                                     getMsgIdPair(*msg));

			if(failed_entry != mMsgPendingValidate.end()) mMsgPendingValidate.erase(failed_entry);
			delete msg;


		}
		else if(validateReturn == VALIDATE_FAIL_TRY_LATER)
		{

#ifdef GEN_EXCH_DEBUG
			std::cerr << "failed to validate msg, trying again: "
			          << "msg->grpId: " << msg->grpId << ", msgId: " << msg->msgId << std::endl;
#endif

			RsGxsGrpMsgIdPair id;
			id.first = msg->grpId;
			id.second = msg->msgId;

			// first check you haven't made too many attempts

			NxsMsgPendingVect::iterator vit = std::find(
			                        mMsgPendingValidate.begin(), mMsgPendingValidate.end(), id);

			if(vit == mMsgPendingValidate.end())
			{
				GxsPendingItem<RsNxsMsg*, RsGxsGrpMsgIdPair> item(msg, id);
				mMsgPendingValidate.push_back(item);
			}else
			{
				vit->mAttempts++;
			}
		}
	}

	// clean up resources from group meta retrieval
	freeAndClearContainerResource<std::map<RsGxsGroupId, RsGxsGrpMetaData*>,
	                RsGxsGrpMetaData*>(grpMetas);

	if(!msgIds.empty())
	{
#ifdef GEN_EXCH_DEBUG
		std::cerr << "  removing existing and old messages from incoming list." << std::endl;
#endif
		removeDeleteExistingMessages(msgs, msgIds);

#ifdef GEN_EXCH_DEBUG
		std::cerr << "  storing remaining messages" << std::endl;
#endif
		mDataStore->storeMessage(msgs);

		RsGxsMsgChange* c = new RsGxsMsgChange(RsGxsNotify::TYPE_RECEIVE, false);
		c->msgChangeMap = msgIds;
		mNotifications.push_back(c);
	}

	mReceivedMsgs.clear();
}

void RsGenExchange::processRecvdGroups()
{
	RS_STACK_MUTEX(mGenMtx) ;

	if(mReceivedGrps.empty())
		return;

	NxsGrpPendValidVect::iterator vit = mReceivedGrps.begin();
	std::vector<RsGxsGroupId> existingGrpIds;
	std::list<RsGxsGroupId> grpIds;

	std::map<RsNxsGrp*, RsGxsGrpMetaData*> grps;

	mDataStore->retrieveGroupIds(existingGrpIds);

	while( vit != mReceivedGrps.end())
	{
		GxsPendingItem<RsNxsGrp*, RsGxsGroupId>& gpsi = *vit;
		RsNxsGrp* grp = gpsi.mItem;
		RsGxsGrpMetaData* meta = new RsGxsGrpMetaData();
		bool deserialOk = false;

		if(grp->meta.bin_len != 0)
			deserialOk = meta->deserialise(grp->meta.bin_data, grp->meta.bin_len);

		bool erase = true;

        if(deserialOk)
        {
#ifdef GEN_EXCH_DEBUG
            	std::cerr << "  processing validation for group " << meta->mGroupId << ", attempts number " << gpsi.mAttempts << std::endl;
#endif
        	grp->metaData = meta;
        	uint8_t ret = validateGrp(grp);

			if(ret == VALIDATE_SUCCESS)
			{
				meta->mGroupStatus = GXS_SERV::GXS_GRP_STATUS_UNPROCESSED | GXS_SERV::GXS_GRP_STATUS_UNREAD;

				computeHash(grp->grp, meta->mHash);

				// group has been validated. Let's notify the global router for the clue

		    		if(!meta->mAuthorId.isNull())
		    		{
#ifdef GEN_EXCH_DEBUG
				    std::cerr << "Group routage info: Identity=" << meta->mAuthorId << " from " << grp->PeerId() << std::endl;
#endif

			    		mRoutingClues[meta->mAuthorId].insert(grp->PeerId()) ;
		    		}
                                
                                // This has been moved here (as opposed to inside part for new groups below) because it is used to update the server TS when updates
                                // of grp metadata arrive.
                                
				meta->mRecvTS = time(NULL);

				// now check if group already existss
				if(std::find(existingGrpIds.begin(), existingGrpIds.end(), grp->grpId) == existingGrpIds.end())
				{
					if(meta->mCircleType == GXS_CIRCLE_TYPE_YOUREYESONLY)
						meta->mOriginator = grp->PeerId();

					meta->mSubscribeFlags = GXS_SERV::GROUP_SUBSCRIBE_NOT_SUBSCRIBED;
                    
					grps.insert(std::make_pair(grp, meta));
					grpIds.push_back(grp->grpId);
				}
				else
				{
					GroupUpdate update;
					update.newGrp = grp;
					mGroupUpdates.push_back(update);
				}
				erase = true;
			}
			else if(ret == VALIDATE_FAIL)
			{
#ifdef GEN_EXCH_DEBUG
				std::cerr << "  failed to validate incoming meta, grpId: " << grp->grpId << ": wrong signature" << std::endl;
#endif
				delete grp;
				erase = true;
			}
			else  if(ret == VALIDATE_FAIL_TRY_LATER)
			{

#ifdef GEN_EXCH_DEBUG
				std::cerr << "  failed to validate incoming grp, trying again. grpId: " << grp->grpId << std::endl;
#endif

        		if(gpsi.mAttempts == VALIDATE_MAX_ATTEMPTS)
        		{
        			delete grp;
        			erase = true;
#ifdef GEN_EXCH_DEBUG
				std::cerr << "  max attempts " << VALIDATE_MAX_ATTEMPTS << " reached. Will delete group " << grp->grpId << std::endl;
#endif
        		}
        		else
        		{
        			erase = false;
        		}
        	}
        }
        else
        {
            std::cerr << "(EE) deserialise error in group meta data" << std::endl;
        	delete grp;
			delete meta;
			erase = true;
		}

		if(erase)
			vit = mReceivedGrps.erase(vit);
		else
			++vit;
	}

	if(!grpIds.empty())
	{
		RsGxsGroupChange* c = new RsGxsGroupChange(RsGxsNotify::TYPE_RECEIVE, false);
		c->mGrpIdList = grpIds;
		mNotifications.push_back(c);
		mDataStore->storeGroup(grps);
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

	std::map<RsGxsGroupId, RsGxsGrpMetaData*> grpMetas;

	std::vector<GroupUpdate>::iterator vit = mGroupUpdates.begin();
	for(; vit != mGroupUpdates.end(); ++vit)
		grpMetas.insert(std::make_pair(vit->newGrp->grpId, (RsGxsGrpMetaData*)NULL));

	if(!grpMetas.empty())
		mDataStore->retrieveGxsGrpMetaData(grpMetas);
	else
		return;

	vit = mGroupUpdates.begin();
	for(; vit != mGroupUpdates.end(); ++vit)
	{
		GroupUpdate& gu = *vit;
		std::map<RsGxsGroupId, RsGxsGrpMetaData*>::iterator mit =
				grpMetas.find(gu.newGrp->grpId);
		gu.oldGrpMeta = mit->second;
		gu.validUpdate = updateValid(*(gu.oldGrpMeta), *(gu.newGrp));
	}

#ifdef GEN_EXCH_DEBUG
	std::cerr << "RsGenExchange::performUpdateValidation() " << std::endl;
#endif

	vit = mGroupUpdates.begin();
	std::map<RsNxsGrp*, RsGxsGrpMetaData*> grps;
	for(; vit != mGroupUpdates.end(); ++vit)
	{
		GroupUpdate& gu = *vit;

		if(gu.validUpdate)
		{
			if(gu.newGrp->metaData->mCircleType == GXS_CIRCLE_TYPE_YOUREYESONLY)
				gu.newGrp->metaData->mOriginator = gu.newGrp->PeerId();

            		// Keep subscriptionflag to what it was. This avoids clearing off the flag when updates to group meta information
            		// is received.
            
            		gu.newGrp->metaData->mSubscribeFlags = gu.oldGrpMeta->mSubscribeFlags ;
            
			grps.insert(std::make_pair(gu.newGrp, gu.newGrp->metaData));
		}
		else
		{
			delete gu.newGrp;
            		gu.newGrp = NULL ;
		}

		delete gu.oldGrpMeta;
        	gu.oldGrpMeta = NULL ;
	}
    	// notify the client
    
        RsGxsGroupChange* c = new RsGxsGroupChange(RsGxsNotify::TYPE_RECEIVE, true);
        
        for(uint32_t i=0;i<mGroupUpdates.size();++i)
            if(mGroupUpdates[i].newGrp != NULL)
	    {
		    c->mGrpIdList.push_back(mGroupUpdates[i].newGrp->grpId) ;
#ifdef GEN_EXCH_DEBUG
		    std::cerr << "    " << mGroupUpdates[i].newGrp->grpId << std::endl;
#endif
	    }
        
        mNotifications.push_back(c);
 
        // Warning: updateGroup will destroy the objects in grps. Dont use it afterwards!
        
	mDataStore->updateGroup(grps);
    
#ifdef GEN_EXCH_DEBUG
                    			std::cerr << "  adding the following grp ids to notification: " << std::endl;
#endif
       
        // cleanup
        
	mGroupUpdates.clear();
}

bool RsGenExchange::updateValid(RsGxsGrpMetaData& oldGrpMeta, RsNxsGrp& newGrp) const
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

	std::map<RsGxsId, RsTlvSecurityKey>& keys = oldGrpMeta.keys.keys;
	std::map<RsGxsId, RsTlvSecurityKey>::iterator keyMit = keys.find(RsGxsId(oldGrpMeta.mGroupId));

	if(keyMit == keys.end())
	{
#ifdef GEN_EXCH_DEBUG
		std::cerr << "RsGenExchange::updateValid() admin key not found! " << std::endl;
#endif
		return false;
	}

	// also check this is the latest published group
	bool latest = newGrp.metaData->mPublishTs > oldGrpMeta.mPublishTs;

    mGixs->timeStampKey(newGrp.metaData->mAuthorId) ;
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

void RsGenExchange::removeDeleteExistingMessages( RsGeneralDataService::MsgStoreMap& msgs, GxsMsgReq& msgIdsNotify) 
{
	// first get grp ids of messages to be stored

	RsGxsGroupId::std_set mGrpIdsUnique;

	for(RsGeneralDataService::MsgStoreMap::const_iterator cit = msgs.begin(); cit != msgs.end(); ++cit)
		mGrpIdsUnique.insert(cit->second->mGroupId);

	//RsGxsGroupId::std_list grpIds(mGrpIdsUnique.begin(), mGrpIdsUnique.end());
	//RsGxsGroupId::std_list::const_iterator it = grpIds.begin();
	typedef std::map<RsGxsGroupId, RsGxsMessageId::std_vector> MsgIdReq;
	MsgIdReq msgIdReq;

	// now get a list of all msgs ids for each group
	for(RsGxsGroupId::std_set::const_iterator it(mGrpIdsUnique.begin()); it != mGrpIdsUnique.end(); ++it)
	{
		mDataStore->retrieveMsgIds(*it, msgIdReq[*it]);

#ifdef GEN_EXCH_DEBUG
		const std::vector<RsGxsMessageId>& vec(msgIdReq[*it]) ;
		std::cerr << "  retrieved " << vec.size() << " message ids for group " << *it << std::endl;

		for(uint32_t i=0;i<vec.size();++i)
			std::cerr << "    " << vec[i] << std::endl;
#endif
	}

	//RsGeneralDataService::MsgStoreMap::iterator cit2 = msgs.begin();
	RsGeneralDataService::MsgStoreMap filtered;

	// now for each msg to be stored that exist in the retrieved msg/grp "index" delete and erase from map
	for(RsGeneralDataService::MsgStoreMap::iterator cit2 = msgs.begin(); cit2 != msgs.end(); ++cit2)
	{
		const RsGxsMessageId::std_vector& msgIds = msgIdReq[cit2->second->mGroupId];

#ifdef GEN_EXCH_DEBUG
		std::cerr << "    grpid=" << cit2->second->mGroupId << ", msgid=" << cit2->second->mMsgId ;
#endif

		// Avoid storing messages that are already in the database, as well as messages that are too old (or generally do not pass the database storage test)
		//
		if(std::find(msgIds.begin(), msgIds.end(), cit2->second->mMsgId) == msgIds.end() && messagePublicationTest(*cit2->second))
		{
			// passes tests, so add to filtered list
			//
			filtered.insert(*cit2);
#ifdef GEN_EXCH_DEBUG
			std::cerr << "    keeping " << cit2->second->mMsgId << std::endl;
#endif
		}
		else	// remove message from list
		{
			// msg exist in retrieved index
			RsGxsMessageId::std_vector& notifyIds = msgIdsNotify[cit2->second->mGroupId];
			RsGxsMessageId::std_vector::iterator it2 = std::find(notifyIds.begin(),
					notifyIds.end(), cit2->second->mMsgId);
			if(it2 != notifyIds.end())
			{
				notifyIds.erase(it2);
				if (notifyIds.empty())
				{
					msgIdsNotify.erase(cit2->second->mGroupId);
				}
			}
#ifdef GEN_EXCH_DEBUG
			std::cerr << "    discarding " << cit2->second->mMsgId << std::endl;
#endif

			delete cit2->first;
			// cit2->second will be deleted too in the destructor of cit2->first (RsNxsMsg)
		}
	}

	msgs = filtered;
}

