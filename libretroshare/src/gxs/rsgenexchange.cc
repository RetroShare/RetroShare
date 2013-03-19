
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

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>

#include "pqi/pqihash.h"
#include "rsgenexchange.h"
#include "gxssecurity.h"
#include "util/contentvalue.h"
#include "retroshare/rsgxsflags.h"
#include "rsgixs.h"
#include "rsgxsutil.h"


#define PUB_GRP_MASK 0x000f
#define RESTR_GRP_MASK 0x00f0
#define PRIV_GRP_MASK 0x0f00
#define GRP_OPTIONS_MASK 0xf000

#define PUB_GRP_OFFSET 0
#define RESTR_GRP_OFFSET 8
#define PRIV_GRP_OFFSET 16
#define GRP_OPTIONS_OFFSET 24

#define GXS_MASK "GXS_MASK_HACK"

#define GEN_EXCH_DEBUG	1

RsGenExchange::RsGenExchange(RsGeneralDataService *gds, RsNetworkExchangeService *ns,
                             RsSerialType *serviceSerialiser, uint16_t servType, RsGixs* gixs, uint32_t authenPolicy)
: mGenMtx("GenExchange"), mDataStore(gds), mNetService(ns), mSerialiser(serviceSerialiser),
    mServType(servType), mGixs(gixs), mAuthenPolicy(authenPolicy),
    CREATE_FAIL(0), CREATE_SUCCESS(1), CREATE_FAIL_TRY_LATER(2), SIGN_MAX_ATTEMPTS(5),
    SIGN_FAIL(0), SIGN_SUCCESS(1), SIGN_FAIL_TRY_LATER(2),
    VALIDATE_FAIL(0), VALIDATE_SUCCESS(1), VALIDATE_FAIL_TRY_LATER(2), VALIDATE_MAX_ATTEMPTS(5)
{

    mDataAccess = new RsGxsDataAccess(gds);

}

RsGenExchange::~RsGenExchange()
{
    // need to destruct in a certain order (bad thing, TODO: put down instance ownership rules!)
    delete mNetService;

    delete mDataAccess;
    mDataAccess = NULL;

    delete mDataStore;
    mDataStore = NULL;

}

void RsGenExchange::run()
{

    double timeDelta = 0.1; // slow tick

    while(isRunning())
    {
        tick();

#ifndef WINDOWS_SYS
        usleep((int) (timeDelta * 1000000));
#else
        Sleep((int) (timeDelta * 1000));
#endif
    }
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

	processRecvdData();

	if(!mNotifications.empty())
	{
		notifyChanges(mNotifications);
		mNotifications.clear();
	}

	// implemented service tick function
	service_tick();
}

bool RsGenExchange::acknowledgeTokenMsg(const uint32_t& token,
                RsGxsGrpMsgIdPair& msgId)
{
	RsStackMutex stack(mGenMtx);

        std::map<uint32_t, RsGxsGrpMsgIdPair >::iterator mit =
                        mMsgNotify.find(token);

        if(mit == mMsgNotify.end())
        {
            return false;
        }


        msgId = mit->second;

        // no dump token as client has ackowledged its completion
        mDataAccess->disposeOfPublicToken(token);

	return true;
}



bool RsGenExchange::acknowledgeTokenGrp(const uint32_t& token,
		RsGxsGroupId& grpId)
{
	RsStackMutex stack(mGenMtx);

	std::map<uint32_t, RsGxsGroupId >::iterator mit =
                        mGrpNotify.find(token);

        if(mit == mGrpNotify.end())
		return false;

	grpId = mit->second;

	// no dump token as client has ackowledged its completion
	mDataAccess->disposeOfPublicToken(token);

	return true;
}

void RsGenExchange::generateGroupKeys(RsTlvSecurityKeySet& privatekeySet,
		RsTlvSecurityKeySet& publickeySet, bool genPublishKeys)
{
    /* create Keys */

    // admin keys
    RSA *rsa_admin = RSA_generate_key(2048, 65537, NULL, NULL);
    RSA *rsa_admin_pub = RSAPublicKey_dup(rsa_admin);

    /* set admin keys */
    RsTlvSecurityKey adminKey, privAdminKey;

    GxsSecurity::setRSAPublicKey(adminKey, rsa_admin_pub);
    GxsSecurity::setRSAPrivateKey(privAdminKey, rsa_admin);

    adminKey.startTS = time(NULL);
    adminKey.endTS = adminKey.startTS + 60 * 60 * 24 * 365 * 5; /* approx 5 years */

    privAdminKey.startTS = adminKey.startTS;
    privAdminKey.endTS = 0; /* no end */

    // for now all public
    adminKey.keyFlags = RSTLV_KEY_DISTRIB_ADMIN | RSTLV_KEY_TYPE_PUBLIC_ONLY;
    privAdminKey.keyFlags = RSTLV_KEY_DISTRIB_ADMIN | RSTLV_KEY_TYPE_FULL;

    publickeySet.keys[adminKey.keyId] = adminKey;
    privatekeySet.keys[privAdminKey.keyId] = privAdminKey;

    // clean up
    RSA_free(rsa_admin);
    RSA_free(rsa_admin_pub);

    if(genPublishKeys)
    {
        // publish keys
        RSA *rsa_publish = RSA_generate_key(2048, 65537, NULL, NULL);
        RSA *rsa_publish_pub = RSAPublicKey_dup(rsa_publish);

        /* set publish keys */
        RsTlvSecurityKey pubKey, privPubKey;

        GxsSecurity::setRSAPublicKey(pubKey, rsa_publish_pub);
        GxsSecurity::setRSAPrivateKey(privPubKey, rsa_publish);

        pubKey.startTS = adminKey.startTS;
        pubKey.endTS = pubKey.startTS + 60 * 60 * 24 * 365 * 5; /* approx 5 years */

        privPubKey.startTS = adminKey.startTS;
        privPubKey.endTS = 0; /* no end */

        // for now all public
        pubKey.keyFlags = RSTLV_KEY_DISTRIB_PUBLIC | RSTLV_KEY_TYPE_PUBLIC_ONLY;
        privPubKey.keyFlags = RSTLV_KEY_DISTRIB_PRIVATE | RSTLV_KEY_TYPE_FULL;

        publickeySet.keys[pubKey.keyId] = pubKey;
        privatekeySet.keys[privPubKey.keyId] = privPubKey;

        RSA_free(rsa_publish);
        RSA_free(rsa_publish_pub);
    }
}

uint8_t RsGenExchange::createGroup(RsNxsGrp *grp, RsTlvSecurityKeySet& privateKeySet, RsTlvSecurityKeySet& publicKeySet)
{
    std::cerr << "RsGenExchange::createGroup()";
    std::cerr << std::endl;

    RsGxsGrpMetaData* meta = grp->metaData;

    /* add public admin and publish keys to grp */

    // find private admin key
    RsTlvSecurityKey privAdminKey;
    std::map<std::string, RsTlvSecurityKey>::iterator mit = privateKeySet.keys.begin();

    bool privKeyFound = false; // private admin key
    for(; mit != privateKeySet.keys.end(); mit++)
    {
        RsTlvSecurityKey& key = mit->second;

        if((key.keyFlags & RSTLV_KEY_DISTRIB_ADMIN) && (key.keyFlags & RSTLV_KEY_TYPE_FULL))
        {
            privAdminKey = key;
            privKeyFound = true;
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
    uint32_t metaDataLen = meta->serial_size();
    uint32_t allGrpDataLen = metaDataLen + grp->grp.bin_len;
    char* metaData = new char[metaDataLen];
    char* allGrpData = new char[allGrpDataLen]; // msgData + metaData

    meta->serialise(metaData, metaDataLen);

    // copy msg data and meta in allMsgData buffer
    memcpy(allGrpData, grp->grp.bin_data, grp->grp.bin_len);
    memcpy(allGrpData+(grp->grp.bin_len), metaData, metaDataLen);

    RsTlvKeySignature adminSign;
    bool ok = GxsSecurity::getSignature(allGrpData, allGrpDataLen, &privAdminKey, adminSign);

    // add admin sign to grpMeta
    meta->signSet.keySignSet[GXS_SERV::FLAG_AUTHEN_ADMIN] = adminSign;

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
    if ((!grpMeta.mAuthorId.empty()) || checkAuthenFlag(pos, author_flag))
    {
        needIdentitySign = true;
        std::cerr << "Needs Identity sign! (Service Flags)";
        std::cerr << std::endl;
    }

    if (needIdentitySign)
    {
        if(mGixs)
        {
            bool haveKey = mGixs->havePrivateKey(grpMeta.mAuthorId);

            if(haveKey)
            {
                RsTlvSecurityKey authorKey;
                mGixs->getPrivateKey(grpMeta.mAuthorId, authorKey);
                RsTlvKeySignature sign;

                if(GxsSecurity::getSignature((char*)grpData.bin_data, grpData.bin_len,
                                                &authorKey, sign))
                {
                	id_ret = SIGN_SUCCESS;
                }
                else
                {
                	id_ret = SIGN_FAIL;
                }

                signSet.keySignSet[GXS_SERV::FLAG_AUTHEN_IDENTITY] = sign;
            }
            else
            {
            	mGixs->requestPrivateKey(grpMeta.mAuthorId);

                std::cerr << "RsGenExchange::createGroupSignatures(): ";
                std::cerr << " ERROR AUTHOR KEY: " <<  grpMeta.mAuthorId
                		  << " is not Cached / available for Message Signing\n";
                std::cerr << "RsGenExchange::createGroupSignatures():  Requestiong AUTHOR KEY";
                std::cerr << std::endl;

                id_ret = SIGN_FAIL_TRY_LATER;
            }
        }
        else
        {
            std::cerr << "RsGenExchange::createGroupSignatures()";
            std::cerr << "Gixs not enabled while request identity signature validation!" << std::endl;
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

    std::cerr << "RsGenExchange::createMsgSignatures() for Msg.mMsgName: " << msgMeta.mMsgName;
    std::cerr << std::endl;


    // publish signature is determined by whether group is public or not
    // for private group signature is not needed as it needs decrypting with
    // the private publish key anyways

    // restricted is a special case which heeds whether publish sign needs to be checked or not
    // one may or may not want

    uint8_t author_flag = GXS_SERV::MSG_AUTHEN_ROOT_AUTHOR_SIGN;
    uint8_t publish_flag = GXS_SERV::MSG_AUTHEN_ROOT_PUBLISH_SIGN;

    if(!msgMeta.mParentId.empty())
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
        std::cerr << "Needs Publish sign! (Service Flags)";
        std::cerr << std::endl;
    }

    // Check required permissions, and allow them to sign it - if they want too - as well!
    if (checkAuthenFlag(pos, author_flag))
    {
        needIdentitySign = true;
        std::cerr << "Needs Identity sign! (Service Flags)";
        std::cerr << std::endl;
    }

    if (!msgMeta.mAuthorId.empty())
    {
        needIdentitySign = true;
        std::cerr << "Needs Identity sign! (AuthorId Exists)";
        std::cerr << std::endl;
    }

    if(needPublishSign)
    {
        // public and shared is publish key
        RsTlvSecurityKeySet& keys = grpMeta.keys;
        RsTlvSecurityKey* pubKey;

        std::map<std::string, RsTlvSecurityKey>::iterator mit =
                        keys.keys.begin(), mit_end = keys.keys.end();
        bool pub_key_found = false;
        for(; mit != mit_end; mit++)
        {

                pub_key_found = mit->second.keyFlags == (RSTLV_KEY_DISTRIB_PRIVATE | RSTLV_KEY_TYPE_FULL);
                if(pub_key_found)
                        break;
        }

        if (pub_key_found)
        {
            // private publish key
            pubKey = &(mit->second);

            RsTlvKeySignature pubSign = signSet.keySignSet[GXS_SERV::FLAG_AUTHEN_PUBLISH];

            publishSignSuccess = GxsSecurity::getSignature((char*)msgData.bin_data, msgData.bin_len, pubKey, pubSign);

            //place signature in msg meta
            signSet.keySignSet[GXS_SERV::FLAG_AUTHEN_PUBLISH] = pubSign;
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

                if(GxsSecurity::getSignature((char*)msgData.bin_data, msgData.bin_len,
                                                &authorKey, sign))
                {
                	id_ret = SIGN_SUCCESS;
                }
                else
                {
                	id_ret = SIGN_FAIL;
                }

                signSet.keySignSet[GXS_SERV::FLAG_AUTHEN_IDENTITY] = sign;
            }
            else
            {
            	mGixs->requestPrivateKey(msgMeta.mAuthorId);

                std::cerr << "RsGenExchange::createMsgSignatures(): ";
                std::cerr << " ERROR AUTHOR KEY: " <<  msgMeta.mAuthorId
                		  << " is not Cached / available for Message Signing\n";
                std::cerr << "RsGenExchange::createMsgSignatures():  Requestiong AUTHOR KEY";
                std::cerr << std::endl;

                id_ret = SIGN_FAIL_TRY_LATER;
            }
        }
        else
        {
            std::cerr << "RsGenExchange::createMsgSignatures()";
            std::cerr << "Gixs not enabled while request identity signature validation!" << std::endl;
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
		hash.Complete(msg->msgId);

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

int RsGenExchange::validateMsg(RsNxsMsg *msg, const uint32_t& grpFlag, RsTlvSecurityKeySet& grpKeySet)
{
    bool needIdentitySign = false;
    bool needPublishSign = false;
    bool publishValidate = true, idValidate = true;

    uint8_t author_flag = GXS_SERV::MSG_AUTHEN_ROOT_AUTHOR_SIGN;
    uint8_t publish_flag = GXS_SERV::MSG_AUTHEN_ROOT_PUBLISH_SIGN;

    if(!msg->metaData->mParentId.empty())
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
    if ((checkAuthenFlag(pos, author_flag)) || (!msg->metaData->mAuthorId.empty()))
        needIdentitySign = true;


    RsGxsMsgMetaData& metaData = *(msg->metaData);

    if(needPublishSign)
    {
        RsTlvKeySignature sign = metaData.signSet.keySignSet[GXS_SERV::FLAG_AUTHEN_PUBLISH];

        std::map<std::string, RsTlvSecurityKey>& keys = grpKeySet.keys;
        std::map<std::string, RsTlvSecurityKey>::iterator mit = keys.begin();

        std::string keyId;
        for(; mit != keys.end() ; mit++)
        {
            RsTlvSecurityKey& key = mit->second;

            if((key.keyFlags & RSTLV_KEY_DISTRIB_PUBLIC) &&
               (key.keyFlags & RSTLV_KEY_TYPE_PUBLIC_ONLY))
            {
                keyId = key.keyId;
            }
        }

        if(!keyId.empty())
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
                bool auth_key_fetched = mGixs->getKey(metaData.mAuthorId, authorKey) == 1;

				if (auth_key_fetched)
				{

	                RsTlvKeySignature sign = metaData.signSet.keySignSet[GXS_SERV::FLAG_AUTHEN_IDENTITY];
	                idValidate &= GxsSecurity::validateNxsMsg(*msg, sign, authorKey);
				}
				else
				{
                     std::cerr << "RsGenExchange::validateMsg()";
                     std::cerr << " ERROR Cannot Retrieve AUTHOR KEY for Message Validation";
                     std::cerr << std::endl;
                     idValidate = false;
                }

            }else
            {
                std::list<std::string> peers;
                mGixs->requestKey(metaData.mAuthorId, peers);
                return VALIDATE_FAIL_TRY_LATER;
            }
        }
        else
        {
#ifdef GEN_EXHANGE_DEBUG
            std::cerr << "Gixs not enabled while request identity signature validation!" << std::endl;
#endif
            idValidate = false;
        }
    }
    else
    {
    	idValidate = true;
    }

    if(publishValidate && idValidate)
    	return VALIDATE_SUCCESS;
    else
    	return VALIDATE_FAIL;

}

int RsGenExchange::validateGrp(RsNxsGrp* grp, RsTlvSecurityKeySet& grpKeySet)
{

	bool needIdentitySign = false, idValidate = false;
	RsGxsGrpMetaData& metaData = *(grp->metaData);

    int id_ret;

    uint8_t author_flag = GXS_SERV::GRP_OPTION_AUTHEN_AUTHOR_SIGN;

    PrivacyBitPos pos = GRP_OPTION_BITS;

    // Check required permissions, and allow them to sign it - if they want too - as well!
    if (!(metaData.mAuthorId.empty()) || checkAuthenFlag(pos, author_flag))
    {
        needIdentitySign = true;
        std::cerr << "Needs Identity sign! (Service Flags)";
        std::cerr << std::endl;
    }

	if(needIdentitySign)
	{
		if(mGixs)
		{
			bool haveKey = mGixs->haveKey(metaData.mAuthorId);

			if(haveKey)
			{

				RsTlvSecurityKey authorKey;
				bool auth_key_fetched = mGixs->getKey(metaData.mAuthorId, authorKey) == 1;

				if (auth_key_fetched)
				{

					RsTlvKeySignature sign = metaData.signSet.keySignSet[GXS_SERV::FLAG_AUTHEN_IDENTITY];
					idValidate = GxsSecurity::validateNxsGrp(*grp, sign, authorKey);
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
				std::list<std::string> peers;
				mGixs->requestKey(metaData.mAuthorId, peers);
				return VALIDATE_FAIL_TRY_LATER;
			}
		}
		else
		{
#ifdef GEN_EXHANGE_DEBUG
			std::cerr << "Gixs not enabled while request identity signature validation!" << std::endl;
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
    std::cerr << "RsGenExchange::checkMsgAuthenFlag(pos: " << pos << " flag: ";
    std::cerr << (int) flag << " mAuthenPolicy: " << mAuthenPolicy << ")";
    std::cerr << std::endl;

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

void RsGenExchange::receiveChanges(std::vector<RsGxsNotify*>& changes)
{
	RsStackMutex stack(mGenMtx);

	std::vector<RsGxsNotify*>::iterator vit = changes.begin();

	for(; vit != changes.end(); vit++)
	{
		RsGxsNotify* n = *vit;
		RsGxsGroupChange* gc;
		RsGxsMsgChange* mc;
		if((mc = dynamic_cast<RsGxsMsgChange*>(n)) != NULL)
		{
				mMsgChange.push_back(mc);
		}
		else if((gc = dynamic_cast<RsGxsGroupChange*>(n)) != NULL)
		{
				mGroupChange.push_back(gc);
		}
		else
		{
			delete n;
		}
	}

}

void RsGenExchange::msgsChanged(std::map<RsGxsGroupId,
                             std::vector<RsGxsMessageId> >& msgs)
{
	if(mGenMtx.trylock())
	{
		while(!mMsgChange.empty())
		{
			RsGxsMsgChange* mc = mMsgChange.back();
			msgs = mc->msgChangeMap;
			mMsgChange.pop_back();
			delete mc;
		}
            mGenMtx.unlock();
	}
}

void RsGenExchange::groupsChanged(std::list<RsGxsGroupId>& grpIds)
{

	if(mGenMtx.trylock())
	{
		while(!mGroupChange.empty())
		{
			RsGxsGroupChange* gc = mGroupChange.back();
			std::list<RsGxsGroupId>& gList = gc->mGrpIdList;
			std::list<RsGxsGroupId>::iterator lit = gList.begin();
			for(; lit != gList.end(); lit++)
					grpIds.push_back(*lit);

			mGroupChange.pop_back();
			delete gc;
		}
            mGenMtx.unlock();
	}
}

bool RsGenExchange::subscribeToGroup(uint32_t& token, const RsGxsGroupId& grpId, bool subscribe)
{
    if(subscribe)
        setGroupSubscribeFlags(token, grpId, GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED,
        		(GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED | GXS_SERV::GROUP_SUBSCRIBE_NOT_SUBSCRIBED));
    else
        setGroupSubscribeFlags(token, grpId, GXS_SERV::GROUP_SUBSCRIBE_NOT_SUBSCRIBED,
        		(GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED | GXS_SERV::GROUP_SUBSCRIBE_NOT_SUBSCRIBED));

    return true;
}
bool RsGenExchange::updated(bool willCallGrpChanged, bool willCallMsgChanged)
{
	bool changed = false;

	if(mGenMtx.trylock())
	{
		changed =  (!mGroupChange.empty() || !mMsgChange.empty());

		if(!willCallGrpChanged)
		{
			while(!mGroupChange.empty())
			{
				RsGxsGroupChange* gc = mGroupChange.back();
				mGroupChange.pop_back();
				delete gc;
			}
		}

		if(!willCallMsgChanged)
		{
			while(!mMsgChange.empty())
			{
				RsGxsMsgChange* mc = mMsgChange.back();
				mMsgChange.pop_back();
				delete mc;
			}
		}

		mGenMtx.unlock();
	}

	return changed;
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
	for(; lit != metaL.end(); lit++)
	{
		RsGxsGrpMetaData& gMeta = *(*lit);
		m = gMeta;
		groupInfo.push_back(m);
		delete (*lit);
	}

	return ok;
}


bool RsGenExchange::getMsgMeta(const uint32_t &token,
                               GxsMsgMetaMap &msgInfo)
{
	std::list<RsGxsMsgMetaData*> metaL;
	GxsMsgMetaResult result;
	bool ok = mDataAccess->getMsgSummary(token, result);

	GxsMsgMetaResult::iterator mit = result.begin();

	for(; mit != result.end(); mit++)
	{
		std::vector<RsGxsMsgMetaData*>& metaV = mit->second;

		msgInfo[mit->first] = std::vector<RsMsgMetaData>();
		std::vector<RsMsgMetaData>& msgInfoV = msgInfo[mit->first];

		std::vector<RsGxsMsgMetaData*>::iterator vit = metaV.begin();
		RsMsgMetaData meta;
		for(; vit != metaV.end(); vit++)
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

        for(; mit != result.end(); mit++)
        {
                std::vector<RsGxsMsgMetaData*>& metaV = mit->second;

                msgMeta[mit->first] = std::vector<RsMsgMetaData>();
                std::vector<RsMsgMetaData>& msgInfoV = msgMeta[mit->first];

                std::vector<RsGxsMsgMetaData*>::iterator vit = metaV.begin();
                RsMsgMetaData meta;
                for(; vit != metaV.end(); vit++)
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

	std::cerr << "RsGenExchange::getGroupData() RsNxsGrp::len: " << nxsGrps.size();
	std::cerr << std::endl;

        if(ok)
	{
		for(; lit != nxsGrps.end(); lit++)
		{
			RsTlvBinaryData& data = (*lit)->grp;
			RsItem* item = mSerialiser->deserialise(data.bin_data, &data.bin_len);

                        if(item)
                        {
				RsGxsGrpItem* gItem = dynamic_cast<RsGxsGrpItem*>(item);
				if (gItem)
				{
					gItem->meta = *((*lit)->metaData);
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

bool RsGenExchange::getMsgData(const uint32_t &token,
                               GxsMsgDataMap &msgItems)
{

    RsStackMutex stack(mGenMtx);
    NxsMsgDataResult msgResult;
    bool ok = mDataAccess->getMsgData(token, msgResult);
    NxsMsgDataResult::iterator mit = msgResult.begin();

    if(ok)
    {
        for(; mit != msgResult.end(); mit++)
        {
            std::vector<RsGxsMsgItem*> gxsMsgItems;
            const RsGxsGroupId& grpId = mit->first;
            std::vector<RsNxsMsg*>& nxsMsgsV = mit->second;
            std::vector<RsNxsMsg*>::iterator vit
            = nxsMsgsV.begin();
            for(; vit != nxsMsgsV.end(); vit++)
            {
                RsNxsMsg*& msg = *vit;

                RsItem* item = mSerialiser->deserialise(msg->msg.bin_data,
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
            msgItems[grpId] = gxsMsgItems;
        }
    }
    return ok;
}

bool RsGenExchange::getMsgRelatedData(const uint32_t &token, GxsMsgRelatedDataMap &msgItems)
{

    RsStackMutex stack(mGenMtx);
    NxsMsgRelatedDataResult msgResult;
    bool ok = mDataAccess->getMsgRelatedData(token, msgResult);
    NxsMsgRelatedDataResult::iterator mit = msgResult.begin();

    if(ok)
    {
        for(; mit != msgResult.end(); mit++)
        {
            std::vector<RsGxsMsgItem*> gxsMsgItems;
            const RsGxsGrpMsgIdPair& msgId = mit->first;
            std::vector<RsNxsMsg*>& nxsMsgsV = mit->second;
            std::vector<RsNxsMsg*>::iterator vit
            = nxsMsgsV.begin();
            for(; vit != nxsMsgsV.end(); vit++)
            {
                RsNxsMsg*& msg = *vit;

                RsItem* item = mSerialiser->deserialise(msg->msg.bin_data,
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
            msgItems[msgId] = gxsMsgItems;
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
    RsStackMutex stack(mGenMtx);

    std::vector<RsNxsGrp*>::iterator vit = groups.begin();

    // store these for tick() to pick them up
    for(; vit != groups.end(); vit++)
    {
    	RsNxsGrp* grp = *vit;
    	NxsGrpPendValidVect::iterator received = std::find(mReceivedGrps.begin(),
    			mReceivedGrps.end(), grp->grpId);

    	// drop group if you already have them
    	// TODO: move this to nxs layer to save bandwidth
    	if(received == mReceivedGrps.end())
    	{
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
    RsStackMutex stack(mGenMtx);

    std::vector<RsNxsMsg*>::iterator vit = messages.begin();

    // store these for tick() to pick them up
    for(; vit != messages.end(); vit++)
    {
    	RsNxsMsg* msg = *vit;

    	NxsMsgPendingVect::iterator it =
    			std::find(mMsgPendingValidate.begin(), mMsgPendingValidate.end(), getMsgIdPair(*msg));

    	// if we have msg already just delete it
    	if(it == mMsgPendingValidate.end())
    		mReceivedMsgs.push_back(msg);
    	else
    		delete msg;
    }

}


void RsGenExchange::publishGroup(uint32_t& token, RsGxsGrpItem *grpItem)
{

    RsStackMutex stack(mGenMtx);
    token = mDataAccess->generatePublicToken();
    GxsGrpPendingSign ggps(grpItem, token);
    mGrpsToPublish.push_back(ggps);

#ifdef GEN_EXCH_DEBUG	
    std::cerr << "RsGenExchange::publishGroup() token: " << token;
    std::cerr << std::endl;
#endif

}

void RsGenExchange::publishMsg(uint32_t& token, RsGxsMsgItem *msgItem)
{
    RsStackMutex stack(mGenMtx);
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
    RsStackMutex stack(mGenMtx);
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
    RsStackMutex stack(mGenMtx);
    token = mDataAccess->generatePublicToken();

    GrpLocMetaData g;
    g.grpId = grpId;
    g.val.put(RsGeneralDataService::GRP_META_STATUS, (int32_t)status);
    g.val.put(RsGeneralDataService::GRP_META_STATUS+GXS_MASK, (int32_t)mask); // HACK, need to perform mask operation in a non-blocking location
    mGrpLocMetaMap.insert(std::make_pair(token, g));
}


void RsGenExchange::setGroupServiceString(uint32_t& token, const RsGxsGroupId& grpId, const std::string& servString)
{
    RsStackMutex stack(mGenMtx);
    token = mDataAccess->generatePublicToken();

    GrpLocMetaData g;
    g.grpId = grpId;
    g.val.put(RsGeneralDataService::GRP_META_SERV_STRING, servString);
    mGrpLocMetaMap.insert(std::make_pair(token, g));
}

void RsGenExchange::setMsgStatusFlags(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, const uint32_t& status, const uint32_t& mask)
{
	/* TODO APPLY MASK TO FLAGS */
    RsStackMutex stack(mGenMtx);
    token = mDataAccess->generatePublicToken();

    MsgLocMetaData m;
    m.val.put(RsGeneralDataService::MSG_META_STATUS, (int32_t)status);
    m.val.put(RsGeneralDataService::MSG_META_STATUS+GXS_MASK, (int32_t)mask); // HACK, need to perform mask operation in a non-blocking location
    m.msgId = msgId;
    mMsgLocMetaMap.insert(std::make_pair(token, m));
}

void RsGenExchange::setMsgServiceString(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, const std::string& servString )
{
    RsStackMutex stack(mGenMtx);
    token = mDataAccess->generatePublicToken();

    MsgLocMetaData m;
    m.val.put(RsGeneralDataService::MSG_META_SERV_STRING, servString);
    m.msgId = msgId;
    mMsgLocMetaMap.insert(std::make_pair(token, m));
}

void RsGenExchange::processMsgMetaChanges()
{

    RsStackMutex stack(mGenMtx);

    std::map<uint32_t, MsgLocMetaData>::iterator mit = mMsgLocMetaMap.begin(),
    mit_end = mMsgLocMetaMap.end();

    for(; mit != mit_end; mit++)
    {
        MsgLocMetaData& m = mit->second;

        int32_t value, mask;
        bool ok = true;

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
        }else
        {
            mDataAccess->updatePublicRequestStatus(token, RsTokenService::GXS_REQUEST_V2_STATUS_FAILED);
        }
        mMsgNotify.insert(std::make_pair(token, m.msgId));
    }

    mMsgLocMetaMap.clear();
}

void RsGenExchange::processGrpMetaChanges()
{
    RsStackMutex stack(mGenMtx);

    std::map<uint32_t, GrpLocMetaData>::iterator mit = mGrpLocMetaMap.begin(),
    mit_end = mGrpLocMetaMap.end();

    for(; mit != mit_end; mit++)
    {
        GrpLocMetaData& g = mit->second;
        uint32_t token = mit->first;

        // process mask
        bool ok = processGrpMask(g.grpId, g.val);

        ok &= mDataStore->updateGroupMetaData(g) == 1;

        if(ok)
        {
            mDataAccess->updatePublicRequestStatus(token, RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE);
        }else
        {
            mDataAccess->updatePublicRequestStatus(token, RsTokenService::GXS_REQUEST_V2_STATUS_FAILED);
        }
        mGrpNotify.insert(std::make_pair(token, g.grpId));
    }

    mGrpLocMetaMap.clear();
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
        ok = true;
    }

    if(grpCv.getAsInt32(RsGeneralDataService::GRP_META_STATUS, value))
    {
        key = RsGeneralDataService::GRP_META_STATUS;
        currValue = grpMeta->mGroupStatus;
    }
    else if(grpCv.getAsInt32(RsGeneralDataService::GRP_META_SUBSCRIBE_FLAG, value))
    {
        key = RsGeneralDataService::GRP_META_SUBSCRIBE_FLAG;
        currValue = grpMeta->mSubscribeFlags;
    }else
    {
        if(grpMeta)
            delete grpMeta;
        return true;
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

	RsStackMutex stack(mGenMtx);

	// stick back msgs pending signature
	typedef std::map<uint32_t, GxsPendingItem<RsGxsMsgItem*, uint32_t> > PendSignMap;

	PendSignMap::iterator sign_it = mMsgPendingSign.begin();

	for(; sign_it != mMsgPendingSign.end(); sign_it++)
	{
		GxsPendingItem<RsGxsMsgItem*, uint32_t>& item = sign_it->second;
		mMsgsToPublish.insert(std::make_pair(sign_it->first, item.mItem));
	}

	std::map<uint32_t, RsGxsMsgItem*>::iterator mit = mMsgsToPublish.begin();

	for(; mit != mMsgsToPublish.end(); mit++)
	{
		std::cerr << "RsGenExchange::publishMsgs() Publishing a Message";
		std::cerr << std::endl;

		RsNxsMsg* msg = new RsNxsMsg(mServType);
		RsGxsMsgItem* msgItem = mit->second;
		const uint32_t& token = mit->first;

		msg->grpId = msgItem->meta.mGroupId;

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
						pit->second.mAttempts++;
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
				if(msg->metaData->mOrigMsgId.empty())
				{
					msg->metaData->mOrigMsgId = msg->metaData->mMsgId;
				}

				// now serialise meta data
				size = msg->metaData->serial_size();

				char* metaDataBuff = new char[size];
				bool s = msg->metaData->serialise(metaDataBuff, &size);
				s &= msg->meta.setBinData(metaDataBuff, size);

				msg->metaData->mMsgStatus = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED | GXS_SERV::GXS_MSG_STATUS_UNREAD;
				msgId = msg->msgId;
				grpId = msg->grpId;
				mDataAccess->addMsgData(msg);

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

#ifdef GEN_EXCH_DEBUG
				std::cerr << "RsGenExchange::publishMsgs() failed to publish msg " << std::endl;
#endif
			}
		}
		else
		{
#ifdef GEN_EXCH_DEBUG
			std::cerr << "RsGenExchange::publishMsgs() failed to serialise msg " << std::endl;
#endif
		}

		delete[] mData;

		if(!tryLater)
			delete msgItem;
	}

	// clear msg item map as we're done publishing them and all
	// entries are invalid
	mMsgsToPublish.clear();
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

void RsGenExchange::publishGrps()
{
    RsStackMutex stack(mGenMtx);
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
    				token, GrpNote(false, "")));
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
        std::map<std::string, RsTlvSecurityKey>::iterator mit_keys = privatekeySet.keys.begin();

        bool privKeyFound = false;
        for(; mit_keys != privatekeySet.keys.end(); mit_keys++)
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
			grpItem->meta.mGroupId = grp->grpId = privAdminKey.keyId;

			// what!? this will remove the private keys!
			privatekeySet.keys.insert(publicKeySet.keys.begin(),
					publicKeySet.keys.end());

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

				if(create == CREATE_SUCCESS)
				{

					uint32_t mdSize = grp->metaData->serial_size();
					char* metaData = new char[mdSize];
					serialOk = grp->metaData->serialise(metaData, mdSize);
					grp->meta.setBinData(metaData, mdSize);
					delete[] metaData;

					// place back private keys for publisher
					grp->metaData->keys = privatekeySet;

					if(mDataStore->validSize(grp) && serialOk)
					{
						grpId = grp->grpId;
						mDataAccess->addGroupData(grp);
					}
					else
					{
						create = CREATE_FAIL;
					}
				}
			}
			else if(ret == SERVICE_CREATE_FAIL_TRY_LATER)
			{
				create = CREATE_FAIL_TRY_LATER;
			}
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
        	ggps.mLastAttemptTS = time(NULL);
        	vit++;
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

    for(; mit != toNotify.end(); mit++)
    {
    	GrpNote& note = mit->second;
    	uint8_t status = note.first ? RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE
    			: RsTokenService::GXS_REQUEST_V2_STATUS_FAILED;

    	mGrpNotify.insert(std::make_pair(mit->first, note.second));
		mDataAccess->updatePublicRequestStatus(mit->first, status);
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
    if(grpId.empty())
        return false;

    RsStackMutex stack(mGenMtx);

    std::map<RsGxsGroupId, RsGxsGrpMetaData*> grpMeta;
    grpMeta[grpId] = NULL;
    mDataStore->retrieveGxsGrpMetaData(grpMeta);

    if(grpMeta.empty())
        return false;

    RsGxsGrpMetaData* meta = grpMeta[grpId];

    if(meta == NULL)
        return false;

    keySet = meta->keys;
    return true;
}

void RsGenExchange::createDummyGroup(RsGxsGrpItem *grpItem)
{
    RsStackMutex stack(mGenMtx);

    RsNxsGrp* grp = new RsNxsGrp(mServType);
    uint32_t size = mSerialiser->size(grpItem);
    char gData[size];
    bool ok = mSerialiser->serialise(grpItem, gData, &size);
    grp->grp.setBinData(gData, size);

    RsTlvSecurityKeySet privateKeySet, publicKeySet;
    generateGroupKeys(privateKeySet, publicKeySet,
                      !(grpItem->meta.mGroupFlags & GXS_SERV::FLAG_PRIVACY_PUBLIC));

    // find private admin key
    RsTlvSecurityKey privAdminKey;
    std::map<std::string, RsTlvSecurityKey>::iterator mit_keys = privateKeySet.keys.begin();

    bool privKeyFound = false;
    for(; mit_keys != privateKeySet.keys.end(); mit_keys++)
    {
        RsTlvSecurityKey& key = mit_keys->second;

        if(key.keyFlags == (RSTLV_KEY_DISTRIB_ADMIN | RSTLV_KEY_TYPE_FULL))
        {
            privAdminKey = key;
            privKeyFound = true;
        }
    }


    if(privKeyFound)
    {
        // get group id from private admin key id
        grpItem->meta.mGroupId = grp->grpId = privAdminKey.keyId;
    }
    else
    {
        ok = false;
    }

    service_CreateGroup(grpItem, privateKeySet);

    if(ok)
    {
        grp->metaData = new RsGxsGrpMetaData();
        grpItem->meta.mPublishTs = time(NULL);
        *(grp->metaData) = grpItem->meta;
        grp->metaData->mSubscribeFlags = GXS_SERV::GROUP_SUBSCRIBE_ADMIN;
        createGroup(grp, privateKeySet, publicKeySet);

        mDataAccess->addGroupData(grp);
    }

    if(!ok)
    {

#ifdef GEN_EXCH_DEBUG
#endif
            std::cerr << "RsGenExchange::createDummyGroup(); failed to publish grp " << std::endl;
        delete grp;
    }

    delete grpItem;
}


void RsGenExchange::processRecvdData()
{
    processRecvdGroups();

    processRecvdMessages();
}



void RsGenExchange::processRecvdMessages()
{
    RsStackMutex stack(mGenMtx);


    NxsMsgPendingVect::iterator pend_it = mMsgPendingValidate.begin();

    for(; pend_it != mMsgPendingValidate.end();)
    {
    	GxsPendingItem<RsNxsMsg*, RsGxsGrpMsgIdPair>& gpsi = *pend_it;

    	if(gpsi.mAttempts == VALIDATE_MAX_ATTEMPTS)
    	{
    		delete gpsi.mItem;
    		pend_it = mMsgPendingValidate.erase(pend_it);
    	}
    	else
    	{
    		mReceivedMsgs.push_back(gpsi.mItem);
    		pend_it++;
    	}
    }

    std::vector<RsNxsMsg*>::iterator vit = mReceivedMsgs.begin();
    GxsMsgReq msgIds;
    std::map<RsNxsMsg*, RsGxsMsgMetaData*> msgs;

    std::map<RsGxsGroupId, RsGxsGrpMetaData*> grpMetas;

    // coalesce group meta retrieval for performance
    for(; vit != mReceivedMsgs.end(); vit++)
    {
        RsNxsMsg* msg = *vit;
        grpMetas.insert(std::make_pair(msg->grpId, (RsGxsGrpMetaData*)NULL));
    }

    mDataStore->retrieveGxsGrpMetaData(grpMetas);

    for(vit = mReceivedMsgs.begin(); vit != mReceivedMsgs.end(); vit++)
    {
        RsNxsMsg* msg = *vit;
        RsGxsMsgMetaData* meta = new RsGxsMsgMetaData();
        bool ok = meta->deserialise(msg->meta.bin_data, &(msg->meta.bin_len));
        msg->metaData = meta;

        uint8_t validateReturn = VALIDATE_FAIL;

        if(ok)
        {
            std::map<RsGxsGroupId, RsGxsGrpMetaData*>::iterator mit = grpMetas.find(msg->grpId);

            // validate msg
            if(mit != grpMetas.end())
            {
                RsGxsGrpMetaData* grpMeta = mit->second;
                validateReturn = validateMsg(msg, grpMeta->mGroupFlags, grpMeta->keys);
            }

            if(validateReturn == VALIDATE_SUCCESS)
            {
                meta->mMsgStatus = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED | GXS_SERV::GXS_MSG_STATUS_UNREAD;
                msgs.insert(std::make_pair(msg, meta));
                msgIds[msg->grpId].push_back(msg->msgId);

                NxsMsgPendingVect::iterator validated_entry = std::find(mMsgPendingValidate.begin(), mMsgPendingValidate.end(),
                		getMsgIdPair(*msg));

                if(validated_entry != mMsgPendingValidate.end()) mMsgPendingValidate.erase(validated_entry);
            }
        }
        else
        {
        	validateReturn = VALIDATE_FAIL;
        }

        if(validateReturn == VALIDATE_FAIL)
        {

#ifdef GXS_GENX_DEBUG
            std::cerr << "failed to deserialise incoming meta, grpId: "
                    << msg->grpId << ", msgId: " << msg->msgId << std::endl;
#endif

            NxsMsgPendingVect::iterator failed_entry = std::find(mMsgPendingValidate.begin(), mMsgPendingValidate.end(),
            		getMsgIdPair(*msg));

            if(failed_entry != mMsgPendingValidate.end()) mMsgPendingValidate.erase(failed_entry);
			delete msg;


        }
        else if(validateReturn == VALIDATE_FAIL_TRY_LATER)
        {

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
        mDataStore->storeMessage(msgs);
        RsGxsMsgChange* c = new RsGxsMsgChange();
        c->msgChangeMap = msgIds;
        mNotifications.push_back(c);
    }

    mReceivedMsgs.clear();
}

void RsGenExchange::processRecvdGroups()
{
    RsStackMutex stack(mGenMtx);

    NxsGrpPendValidVect::iterator vit = mReceivedGrps.begin();
    std::map<RsNxsGrp*, RsGxsGrpMetaData*> grps;

    std::list<RsGxsGroupId> grpIds;

    while( vit != mReceivedGrps.end())
    {
    	GxsPendingItem<RsNxsGrp*, RsGxsGroupId>& gpsi = *vit;
        RsNxsGrp* grp = gpsi.mItem;
        RsGxsGrpMetaData* meta = new RsGxsGrpMetaData();
        bool deserialOk = meta->deserialise(grp->meta.bin_data, grp->meta.bin_len);
        bool erase = true;

        if(deserialOk)
        {
        	grp->metaData = meta;
        	uint8_t ret = validateGrp(grp, meta->keys);


        	if(ret == VALIDATE_SUCCESS)
        	{
				meta->mGroupStatus = GXS_SERV::GXS_GRP_STATUS_UNPROCESSED | GXS_SERV::GXS_GRP_STATUS_UNREAD;
				meta->mSubscribeFlags = GXS_SERV::GROUP_SUBSCRIBE_NOT_SUBSCRIBED;
				grps.insert(std::make_pair(grp, meta));
				grpIds.push_back(grp->grpId);

				erase = true;
        	}
        	else if(ret == VALIDATE_FAIL)
        	{
#ifdef GXS_GENX_DEBUG
				std::cerr << "failed to deserialise incoming meta, grpId: "
						<< grp->grpId << std::endl;
#endif
				delete grp;
				erase = true;
        	}
        	else  if(ret == VALIDATE_FAIL_TRY_LATER)
        	{
        		if(gpsi.mAttempts == VALIDATE_MAX_ATTEMPTS)
        		{
        			delete grp;
        			erase = true;
        		}
        		else
        		{
        			erase = false;
        		}
        	}
        }
        else
        {
        	delete grp;
			delete meta;
			erase = true;
        }

        if(erase)
        	vit = mReceivedGrps.erase(vit);
        else
        	vit++;
    }

    if(!grpIds.empty())
    {
        RsGxsGroupChange* c = new RsGxsGroupChange();
        c->mGrpIdList = grpIds;
        mNotifications.push_back(c);
        mDataStore->storeGroup(grps);
    }
}
