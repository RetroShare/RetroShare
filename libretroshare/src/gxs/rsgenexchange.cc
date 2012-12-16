
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
#include "rsgxsflags.h"
#include "rsgixs.h"


#define PUB_GRP_MASK 0x000f
#define RESTR_GRP_MASK 0x00f0
#define PRIV_GRP_MASK 0x0f00
#define GRP_OPTIONS_MASK 0xf000

#define PUB_GRP_OFFSET 0
#define RESTR_GRP_OFFSET 8
#define PRIV_GRP_OFFSET 9
#define GRP_OPTIONS_OFFSET 7

#define GXS_MASK "GXS_MASK_HACK"

#define GEN_EXCH_DEBUG	1

RsGenExchange::RsGenExchange(RsGeneralDataService *gds, RsNetworkExchangeService *ns,
                             RsSerialType *serviceSerialiser, uint16_t servType, RsGixs* gixs, uint32_t authenPolicy)
: mGenMtx("GenExchange"), mDataStore(gds), mNetService(ns), mSerialiser(serviceSerialiser),
    mServType(servType), mGixs(gixs), mAuthenPolicy(authenPolicy)
{

    mDataAccess = new RsGxsDataAccess(gds);

}

RsGenExchange::~RsGenExchange()
{
    // need to destruct in a certain order (prob a bad thing!)
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
	mDataAccess->processRequests();

	publishGrps();

	publishMsgs();

        processGrpMetaChanges();

        processMsgMetaChanges();

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

void RsGenExchange::generateGroupKeys(RsTlvSecurityKeySet& keySet, bool genPublishKeys)
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
    adminKey.endTS = 0; /* no end */

    privAdminKey.startTS = adminKey.startTS;
    privAdminKey.endTS = 0; /* no end */

    // for now all public
    adminKey.keyFlags = RSTLV_KEY_DISTRIB_ADMIN | RSTLV_KEY_TYPE_PUBLIC_ONLY;
    privAdminKey.keyFlags = RSTLV_KEY_DISTRIB_ADMIN | RSTLV_KEY_TYPE_FULL;

    keySet.keys[adminKey.keyId] = adminKey;
    keySet.keys[privAdminKey.keyId] = privAdminKey;

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

        keySet.keys[pubKey.keyId] = pubKey;
        keySet.keys[privPubKey.keyId] = privPubKey;

        RSA_free(rsa_publish);
        RSA_free(rsa_publish_pub);
    }

}

bool RsGenExchange::createGroup(RsNxsGrp *grp, RsTlvSecurityKeySet& keySet)
{
    std::cerr << "RsGenExchange::createGroup()";
    std::cerr << std::endl;

    RsGxsGrpMetaData* meta = grp->metaData;

    /* add public admin and publish keys to grp */

    // find private admin key
    RsTlvSecurityKey privAdminKey;
    std::map<std::string, RsTlvSecurityKey>::iterator mit = keySet.keys.begin();

    bool privKeyFound = false;
    for(; mit != keySet.keys.end(); mit++)
    {
        RsTlvSecurityKey& key = mit->second;

        // add public admin key
        if(key.keyFlags & (RSTLV_KEY_DISTRIB_ADMIN | RSTLV_KEY_TYPE_PUBLIC_ONLY))
            meta->keys.keys.insert(std::make_pair(key.keyId, key));

        // add public publish key
        if(key.keyFlags & (RSTLV_KEY_DISTRIB_PUBLIC | RSTLV_KEY_TYPE_PUBLIC_ONLY))
            meta->keys.keys.insert(std::make_pair(key.keyId, key));

        if(key.keyFlags & (RSTLV_KEY_DISTRIB_ADMIN | RSTLV_KEY_TYPE_FULL))
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

    // set meta to be transported as meta without private
    // key components
    grp->meta.setBinData(metaData, metaDataLen);

    // but meta that is stored locally
    // has all keys
    // nxs net transports only bin data
    meta->keys = keySet;

    // clean up
    delete[] allGrpData;
    delete[] metaData;

    if (!ok)
    {
	std::cerr << "RsGenExchange::createGroup() ERROR !okay (getSignature error)";
	std::cerr << std::endl;
    }

    return ok;
}

bool RsGenExchange::createMsgSignatures(RsTlvKeySignatureSet& signSet, RsTlvBinaryData& msgData,
                                        const RsGxsMsgMetaData& msgMeta, RsGxsGrpMetaData& grpMeta)
{
    bool isParent = false;
    bool needPublishSign = false, needIdentitySign = false;
    bool ok = true;
    uint32_t grpFlag = grpMeta.mGroupFlags;

    // publish signature is determined by whether group is public or not
    // for private group signature is not needed as it needs decrypting with
    // the private publish key anyways

    // restricted is a special case which heeds whether publish sign needs to be checked or not
    // one may or may not want

    if(msgMeta.mParentId.empty())
    {
        isParent = true;
    }
    else
    {
        isParent = false;
    }


    if(isParent)
    {
        needIdentitySign = false;
        needPublishSign = false;

        if(grpFlag & GXS_SERV::FLAG_PRIVACY_PUBLIC)
        {
            needPublishSign = false;

            if(checkMsgAuthenFlag(PUBLIC_GRP_BITS, GXS_SERV::MSG_AUTHEN_ROOT_AUTHOR_SIGN))
                needIdentitySign = true;

        }
        else if(grpFlag & GXS_SERV::FLAG_PRIVACY_RESTRICTED)
        {
            needPublishSign = true;

            if(checkMsgAuthenFlag(RESTRICTED_GRP_BITS, GXS_SERV::MSG_AUTHEN_ROOT_AUTHOR_SIGN))
                needIdentitySign = true;
        }
        else if(grpFlag & GXS_SERV::FLAG_PRIVACY_PRIVATE)
        {
            needPublishSign = false;

            if(checkMsgAuthenFlag(PRIVATE_GRP_BITS, GXS_SERV::MSG_AUTHEN_ROOT_AUTHOR_SIGN))
                needIdentitySign = true;
        }

    }else
    {
        if(grpFlag & GXS_SERV::FLAG_PRIVACY_PUBLIC)
        {
            needPublishSign = false;

            if(checkMsgAuthenFlag(PUBLIC_GRP_BITS, GXS_SERV::MSG_AUTHEN_CHILD_AUTHOR_SIGN))
                needIdentitySign = true;

        }
        else if(grpFlag & GXS_SERV::FLAG_PRIVACY_RESTRICTED)
        {
            if(checkMsgAuthenFlag(RESTRICTED_GRP_BITS, GXS_SERV::MSG_AUTHEN_CHILD_PUBLISH_SIGN))
                needPublishSign = true;

            if(checkMsgAuthenFlag(RESTRICTED_GRP_BITS, GXS_SERV::MSG_AUTHEN_CHILD_AUTHOR_SIGN))
                needIdentitySign = true;
        }
        else if(grpFlag & GXS_SERV::FLAG_PRIVACY_PRIVATE)
        {
            needPublishSign = false;

            if(checkMsgAuthenFlag(PRIVATE_GRP_BITS, GXS_SERV::MSG_AUTHEN_CHILD_AUTHOR_SIGN))
                needIdentitySign = true;
        }
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

        // private publish key
        pubKey = &(mit->second);

        RsTlvKeySignature pubSign = signSet.keySignSet[GXS_SERV::FLAG_AUTHEN_PUBLISH];

        ok &= GxsSecurity::getSignature((char*)msgData.bin_data, msgData.bin_len, pubKey, pubSign);

        //place signature in msg meta
        signSet.keySignSet[GXS_SERV::FLAG_AUTHEN_PUBLISH] = pubSign;
    }


    if(needIdentitySign)
    {
        if(mGixs)
        {
            bool haveKey = mGixs->havePrivateKey(msgMeta.mAuthorId);

            if(haveKey)
            {
                mGixs->requestPrivateKey(msgMeta.mAuthorId);

                RsTlvSecurityKey authorKey;

                double timeDelta = 0.002; // fast polling
                time_t now = time(NULL);

                // poll immediately but, don't spend more than a second polling
                while( (mGixs->getPrivateKey(msgMeta.mAuthorId, authorKey) == -1) &&
                       ((now + 1) >> time(NULL))
                    )
                {
    #ifndef WINDOWS_SYS
            usleep((int) (timeDelta * 1000000));
    #else
            Sleep((int) (timeDelta * 1000));
    #endif
                }


                RsTlvKeySignature sign;
                ok &= GxsSecurity::getSignature((char*)msgData.bin_data, msgData.bin_len,
                                                &authorKey, sign);
                signSet.keySignSet[GXS_SERV::FLAG_AUTHEN_IDENTITY] = sign;

            }else
            {
                ok = false;
            }
        }
        else
        {
#ifdef GEN_EXHANGE_DEBUG
            std::cerr << "Gixs not enabled while request identity signature validation!" << std::endl;
#endif
            ok = false;
        }
    }

    return ok;


}

bool RsGenExchange::createMessage(RsNxsMsg* msg)
{
	const RsGxsGroupId& id = msg->grpId;

	std::map<RsGxsGroupId, RsGxsGrpMetaData*> metaMap;

	metaMap.insert(std::make_pair(id, (RsGxsGrpMetaData*)(NULL)));
	mDataStore->retrieveGxsGrpMetaData(metaMap);
        bool ok = true;
        RsGxsMsgMetaData &meta = *(msg->metaData);

	if(!metaMap[id])
	{
            return false;
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
            ok &= createMsgSignatures(meta.signSet, msgData, meta, *grpMeta);

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

	return ok;
}

bool RsGenExchange::validateMsg(RsNxsMsg *msg, const uint32_t& grpFlag, RsTlvSecurityKeySet& grpKeySet)
{
    bool isParent = false;
    bool checkPublishSign, checkIdentitySign;
    bool valid = true;

    // publish signature is determined by whether group is public or not
    // for private group signature is not needed as it needs decrypting with
    // the private publish key anyways

    // restricted is a special case which heeds whether publish sign needs to be checked or not
    // one may or may not want

    if(msg->metaData->mParentId.empty())
    {
        isParent = true;
    }
    else
    {
        isParent = false;
    }


    if(isParent)
    {
        checkIdentitySign = false;
        checkPublishSign = false;

        if(grpFlag & GXS_SERV::FLAG_PRIVACY_PUBLIC)
        {
            checkPublishSign = false;

            if(checkMsgAuthenFlag(PUBLIC_GRP_BITS, GXS_SERV::MSG_AUTHEN_ROOT_AUTHOR_SIGN))
                checkIdentitySign = true;

        }
        else if(grpFlag & GXS_SERV::FLAG_PRIVACY_RESTRICTED)
        {
            checkPublishSign = true;

            if(checkMsgAuthenFlag(RESTRICTED_GRP_BITS, GXS_SERV::MSG_AUTHEN_ROOT_AUTHOR_SIGN))
                checkIdentitySign = true;
        }
        else if(grpFlag & GXS_SERV::FLAG_PRIVACY_PRIVATE)
        {
            checkPublishSign = false;

            if(checkMsgAuthenFlag(PRIVATE_GRP_BITS, GXS_SERV::MSG_AUTHEN_ROOT_AUTHOR_SIGN))
                checkIdentitySign = true;
        }

    }else
    {
        if(grpFlag & GXS_SERV::FLAG_PRIVACY_PUBLIC)
        {
            checkPublishSign = false;

            if(checkMsgAuthenFlag(PUBLIC_GRP_BITS, GXS_SERV::MSG_AUTHEN_CHILD_AUTHOR_SIGN))
                checkIdentitySign = true;

        }
        else if(grpFlag & GXS_SERV::FLAG_PRIVACY_RESTRICTED)
        {
            if(checkMsgAuthenFlag(RESTRICTED_GRP_BITS, GXS_SERV::MSG_AUTHEN_CHILD_PUBLISH_SIGN))
                checkPublishSign = true;

            if(checkMsgAuthenFlag(RESTRICTED_GRP_BITS, GXS_SERV::MSG_AUTHEN_CHILD_AUTHOR_SIGN))
                checkIdentitySign = true;
        }
        else if(grpFlag & GXS_SERV::FLAG_PRIVACY_PRIVATE)
        {
            checkPublishSign = false;

            if(checkMsgAuthenFlag(PRIVATE_GRP_BITS, GXS_SERV::MSG_AUTHEN_CHILD_AUTHOR_SIGN))
                checkIdentitySign = true;
        }
    }

    RsGxsMsgMetaData& metaData = *(msg->metaData);

    if(checkPublishSign)
    {
        RsTlvKeySignature sign = metaData.signSet.keySignSet[GXS_SERV::FLAG_AUTHEN_PUBLISH];

        if(grpKeySet.keys.find(sign.keyId) != grpKeySet.keys.end())
        {
            RsTlvSecurityKey publishKey = grpKeySet.keys[sign.keyId];
            valid &= GxsSecurity::validateNxsMsg(*msg, sign, publishKey);
        }
        else
        {
            valid = false;
        }

    }



    if(checkIdentitySign)
    {
        if(mGixs)
        {
            bool haveKey = mGixs->haveKey(metaData.mAuthorId);

            if(haveKey)
            {
                std::list<std::string> peers;
                mGixs->requestKey(metaData.mAuthorId, peers);

                RsTlvSecurityKey authorKey;

                double timeDelta = 0.002; // fast polling
                time_t now = time(NULL);
                // poll immediately but, don't spend more than a second polling
                while( (mGixs->getKey(metaData.mAuthorId, authorKey) == -1) &&
                       ((now + 1) >> time(NULL))
                    )
                {
    #ifndef WINDOWS_SYS
            usleep((int) (timeDelta * 1000000));
    #else
            Sleep((int) (timeDelta * 1000));
    #endif
                }

                RsTlvKeySignature sign = metaData.signSet.keySignSet[GXS_SERV::FLAG_AUTHEN_PUBLISH];
                valid &= GxsSecurity::validateNxsMsg(*msg, sign, authorKey);
            }else
            {
                valid = false;
            }
        }
        else
        {
#ifdef GEN_EXHANGE_DEBUG
            std::cerr << "Gixs not enabled while request identity signature validation!" << std::endl;
#endif
            valid = false;
        }

    }

    return valid;
}

bool RsGenExchange::checkMsgAuthenFlag(const PrivacyBitPos& pos, const uint8_t& flag) const
{
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

                        if(item != NULL)
                        {
				RsGxsGrpItem* gItem = dynamic_cast<RsGxsGrpItem*>(item);
				gItem->meta = *((*lit)->metaData);
				grpItem.push_back(gItem);
				delete *lit;
			}
			else
			{
				std::cerr << "RsGenExchange::getGroupData() ERROR deserialising item";
				std::cerr << std::endl;
			}
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
                RsGxsMsgItem* mItem = dynamic_cast<RsGxsMsgItem*>(item);
                mItem->meta = *((*vit)->metaData); // get meta info from nxs msg
                gxsMsgItems.push_back(mItem);
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
                RsGxsMsgItem* mItem = dynamic_cast<RsGxsMsgItem*>(item);

                if(mItem != NULL)
                {
                    mItem->meta = *((*vit)->metaData); // get meta info from nxs msg
                    gxsMsgItems.push_back(mItem);
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
    std::vector<RsNxsGrp*>::iterator vit = groups.begin();

    // store these for tick() to pick them up
    for(; vit != groups.end(); vit++)
       mReceivedGrps.push_back(*vit);

}

void RsGenExchange::notifyNewMessages(std::vector<RsNxsMsg *>& messages)
{
    std::vector<RsNxsMsg*>::iterator vit = messages.begin();

    // store these for tick() to pick them up
    for(; vit != messages.end(); vit++)
        mReceivedMsgs.push_back(*vit);

}


void RsGenExchange::publishGroup(uint32_t& token, RsGxsGrpItem *grpItem)
{

    RsStackMutex stack(mGenMtx);
    token = mDataAccess->generatePublicToken();
    mGrpsToPublish.insert(std::make_pair(token, grpItem));

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

    // remove mask entry so it doesn't affect
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

	std::map<uint32_t, RsGxsMsgItem*>::iterator mit = mMsgsToPublish.begin();

        for(; mit != mMsgsToPublish.end(); mit++)
	{

            RsNxsMsg* msg = new RsNxsMsg(mServType);
            RsGxsMsgItem* msgItem = mit->second;

            msg->grpId = msgItem->meta.mGroupId;

            uint32_t size = mSerialiser->size(msgItem);
            char* mData = new char[size];
            bool serialOk = false;
            bool createOk = false;
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
                createOk = createMessage(msg);
                RsGxsMessageId msgId;
                RsGxsGroupId grpId = msgItem->meta.mGroupId;

                bool msgDoesnExist = false;

                if(createOk)
                {

                    GxsMsgReq req;
                    std::vector<RsGxsMessageId> msgV;
                    msgV.push_back(msg->msgId);
                    GxsMsgMetaResult res;
                    req.insert(std::make_pair(msg->grpId, msgV));
                    mDataStore->retrieveGxsMsgMetaData(req, res);
                    msgDoesnExist = res[grpId].empty();

#ifdef GEN_EXCH_DEBUG
                    if (!msgDoesnExist)
                    {
                        std::cerr << "RsGenExchange::publishMsgs() msg exists already :( " << std::endl;
                    }
#endif
                }

                if(createOk && msgDoesnExist)
                {
                    // empty orig msg id means this is the original
                    // msg
                    // TODO: a non empty msgid means one should at least
                    // have the msg on disk, after which this msg is signed
                    // based on the security settings
                    // public grp (sign by grp public pub key, private/id: signed by
                    // id
                    if(msg->metaData->mOrigMsgId.empty())
                    {
                        msg->metaData->mOrigMsgId = msg->metaData->mMsgId;
                    }

                    // now serialise meta data
                    size = msg->metaData->serial_size();
                    char metaDataBuff[size];
                    msg->metaData->serialise(metaDataBuff, &size);
                    msg->meta.setBinData(metaDataBuff, size);

                    msg->metaData->mMsgStatus = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED | GXS_SERV::GXS_MSG_STATUS_UNREAD;
                    msgId = msg->msgId;
                    grpId = msg->grpId;
                    mDataAccess->addMsgData(msg);


                    // add to published to allow acknowledgement
                    mMsgNotify.insert(std::make_pair(mit->first, std::make_pair(grpId, msgId)));
                    mDataAccess->updatePublicRequestStatus(mit->first, RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE);
                }
                else
                {
                    // delete msg if created wasn't ok
                    delete msg;
                    mDataAccess->updatePublicRequestStatus(mit->first, RsTokenService::GXS_REQUEST_V2_STATUS_FAILED);

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
            delete msgItem;
	}

	// clear msg list as we're done publishing them and entries
	// are invalid
	mMsgsToPublish.clear();
}

void RsGenExchange::service_CreateGroup(RsGxsGrpItem* grpItem, RsTlvSecurityKeySet& keySet)
{
#ifdef GEN_EXCH_DEBUG
	std::cerr << "RsGenExchange::service_CreateGroup(): Does nothing"
			  << std::endl;
#endif
	return;
}


#define GEN_EXCH_GRP_CHUNK 3

void RsGenExchange::publishGrps()
{

    RsStackMutex stack(mGenMtx);

    std::map<uint32_t, RsGxsGrpItem*>::iterator mit = mGrpsToPublish.begin();
    std::vector<uint32_t> toRemove;
    int i = 0;
    for(; mit != mGrpsToPublish.end(); mit++)
    {

        if(i > GEN_EXCH_GRP_CHUNK-1) break;

        toRemove.push_back(mit->first);
        i++;

        RsNxsGrp* grp = new RsNxsGrp(mServType);
        RsGxsGrpItem* grpItem = mit->second;

        RsTlvSecurityKeySet keySet;
        generateGroupKeys(keySet,
                          !(grpItem->meta.mGroupFlags & GXS_SERV::FLAG_PRIVACY_PUBLIC));

        // find private admin key
        RsTlvSecurityKey privAdminKey;
        std::map<std::string, RsTlvSecurityKey>::iterator mit_keys = keySet.keys.begin();

        bool privKeyFound = false;
        for(; mit_keys != keySet.keys.end(); mit_keys++)
        {
            RsTlvSecurityKey& key = mit_keys->second;

            if(key.keyFlags == (RSTLV_KEY_DISTRIB_ADMIN | RSTLV_KEY_TYPE_FULL))
            {
                privAdminKey = key;
                privKeyFound = true;
            }
        }

        bool ok = true;

        if(privKeyFound)
        {
            // get group id from private admin key id
            grpItem->meta.mGroupId = grp->grpId = privAdminKey.keyId;
        }
        else
        {
            ok = false;
        }


        service_CreateGroup(grpItem, keySet);

        uint32_t size = mSerialiser->size(grpItem);
        char gData[size];
        ok = mSerialiser->serialise(grpItem, gData, &size);

        if (!ok)
        {
            std::cerr << "RsGenExchange::publishGrps() !ok ERROR After First Serialise" << std::endl;
        }

        grp->grp.setBinData(gData, size);

        if(ok)
        {
            grp->metaData = new RsGxsGrpMetaData();
            grpItem->meta.mPublishTs = time(NULL);
            *(grp->metaData) = grpItem->meta;
            grp->metaData->mSubscribeFlags = GXS_SERV::GROUP_SUBSCRIBE_ADMIN;

            ok &= createGroup(grp, keySet);

            if (!ok)
            {
                    std::cerr << "RsGenExchange::publishGrps() !ok ERROR After createGroup" << std::endl;
            }

            RsGxsGroupId grpId = grp->grpId;
            mDataAccess->addGroupData(grp);

            std::cerr << "RsGenExchange::publishGrps() ok -> pushing to notifies" << std::endl;

            // add to published to allow acknowledgement
            mGrpNotify.insert(std::make_pair(mit->first, grpId));
            mDataAccess->updatePublicRequestStatus(mit->first, RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE);
        }

        if(!ok)
        {

#ifdef GEN_EXCH_DEBUG
#endif
            std::cerr << "RsGenExchange::publishGrps() failed to publish grp " << std::endl;
            delete grp;

            // add to published to allow acknowledgement, grpid is empty as grp creation failed
            mGrpNotify.insert(std::make_pair(mit->first, RsGxsGroupId("")));
            mDataAccess->updatePublicRequestStatus(mit->first, RsTokenService::GXS_REQUEST_V2_STATUS_FAILED);
            continue;
        }

        delete grpItem;

    }

    // clear grp list as we're done publishing them and entries
    // are invalid


    for(int i = 0; i < toRemove.size(); i++)
       mGrpsToPublish.erase(toRemove[i]);
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

    RsTlvSecurityKeySet keySet;
    generateGroupKeys(keySet,
                      !(grpItem->meta.mGroupFlags & GXS_SERV::FLAG_PRIVACY_PUBLIC));

    // find private admin key
    RsTlvSecurityKey privAdminKey;
    std::map<std::string, RsTlvSecurityKey>::iterator mit_keys = keySet.keys.begin();

    bool privKeyFound = false;
    for(; mit_keys != keySet.keys.end(); mit_keys++)
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

    service_CreateGroup(grpItem, keySet);

    if(ok)
    {
        grp->metaData = new RsGxsGrpMetaData();
        grpItem->meta.mPublishTs = time(NULL);
        *(grp->metaData) = grpItem->meta;
        grp->metaData->mSubscribeFlags = GXS_SERV::GROUP_SUBSCRIBE_ADMIN;
        createGroup(grp, keySet);

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


        if(ok)
        {
            std::map<RsGxsGroupId, RsGxsGrpMetaData*>::iterator mit = grpMetas.find(msg->grpId);

            // validate msg
            if(mit != grpMetas.end()){
                RsGxsGrpMetaData* grpMeta = mit->second;
                ok = true;
//                msg->metaData = meta;
  //             ok &= validateMsg(msg, grpMeta->mGroupFlags, grpMeta->keys);
    //           msg->metaData = NULL;
            }
            else
                ok = false;

            if(ok)
            {
                meta->mMsgStatus = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED | GXS_SERV::GXS_MSG_STATUS_UNREAD;
                msgs.insert(std::make_pair(msg, meta));
                msgIds[msg->grpId].push_back(msg->msgId);
            }
        }

        if(!ok)
        {
#ifdef GXS_GENX_DEBUG
            std::cerr << "failed to deserialise incoming meta, grpId: "
                    << msg->grpId << ", msgId: " << msg->msgId << std::endl;
#endif
            delete msg;
            delete meta;
        }
    }

    std::map<RsGxsGroupId, RsGxsGrpMetaData*>::iterator mit = grpMetas.begin();

    // clean up resources
    for(; mit != grpMetas.end(); mit++)
    {
        delete mit->second;
    }

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

    std::vector<RsNxsGrp*>::iterator vit = mReceivedGrps.begin();
    std::map<RsNxsGrp*, RsGxsGrpMetaData*> grps;

    std::list<RsGxsGroupId> grpIds;

    for(; vit != mReceivedGrps.end(); vit++)
    {
        RsNxsGrp* grp = *vit;
        RsGxsGrpMetaData* meta = new RsGxsGrpMetaData();
        bool ok = meta->deserialise(grp->meta.bin_data, grp->meta.bin_len);

        if(ok)
        {
            meta->mGroupStatus = GXS_SERV::GXS_GRP_STATUS_UNPROCESSED | GXS_SERV::GXS_GRP_STATUS_UNREAD;
            grps.insert(std::make_pair(grp, meta));
            grpIds.push_back(grp->grpId);
        }
        else
        {
#ifdef GXS_GENX_DEBUG
            std::cerr << "failed to deserialise incoming meta, grpId: "
                    << grp->grpId << std::endl;
#endif
            delete grp;
            delete meta;
        }
    }

    if(!grpIds.empty())
    {
        RsGxsGroupChange* c = new RsGxsGroupChange();
        c->grpIdList = grpIds;
        mNotifications.push_back(c);
        mDataStore->storeGroup(grps);
    }

    mReceivedGrps.clear();

}

