
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

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include "rsgenexchange.h"
#include "gxssecurity.h"

RsGenExchange::RsGenExchange(RsGeneralDataService *gds,
                             RsNetworkExchangeService *ns, RsSerialType *serviceSerialiser, uint16_t servType)
: mGenMtx("GenExchange"), mDataStore(gds), mNetService(ns), mSerialiser(serviceSerialiser), mServType(servType)
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


void RsGenExchange::tick()
{
	mDataAccess->processRequests();

	publishGrps();

	publishMsgs();

	notifyChanges(mNotifications);
	mNotifications.clear();

}

bool RsGenExchange::acknowledgeTokenMsg(const uint32_t& token,
		std::pair<RsGxsGroupId, RsGxsMessageId>& msgId)
{
	RsStackMutex stack(mGenMtx);

	std::map<uint32_t, std::pair<RsGxsGroupId, RsGxsMessageId> >::iterator mit =
			mMsgPublished.find(token);

	if(mit == mMsgPublished.end())
		return false;

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
			mGrpPublished.find(token);

	if(mit == mGrpPublished.end())
		return false;

	grpId = mit->second;

	// no dump token as client has ackowledged its completion
	mDataAccess->disposeOfPublicToken(token);

	return true;
}

void RsGenExchange::createGroup(RsNxsGrp *grp)
{
    /* create Keys */

	// admin keys
    RSA *rsa_admin = RSA_generate_key(2048, 65537, NULL, NULL);
    RSA *rsa_admin_pub = RSAPublicKey_dup(rsa_admin);

    // publish keys
    RSA *rsa_publish = RSA_generate_key(2048, 65537, NULL, NULL);
    RSA *rsa_publish_pub = RSAPublicKey_dup(rsa_admin);

    /* set keys */
    RsTlvSecurityKey adminKey, privAdminKey;

    /* set publish keys */
    RsTlvSecurityKey pubKey, privPubKey;

    GxsSecurity::setRSAPublicKey(adminKey, rsa_admin_pub);
    GxsSecurity::setRSAPrivateKey(privAdminKey, rsa_admin);

    GxsSecurity::setRSAPublicKey(pubKey, rsa_publish_pub);
    GxsSecurity::setRSAPrivateKey(privPubKey, rsa_publish);


    // for now all public
    adminKey.keyFlags = RSTLV_KEY_DISTRIB_ADMIN | RSTLV_KEY_TYPE_PUBLIC_ONLY;
    privAdminKey.keyFlags = RSTLV_KEY_DISTRIB_ADMIN | RSTLV_KEY_TYPE_FULL;

    // for now all public
    pubKey.keyFlags = RSTLV_KEY_DISTRIB_PUBLIC | RSTLV_KEY_TYPE_PUBLIC_ONLY;
    privPubKey.keyFlags = RSTLV_KEY_DISTRIB_PRIVATE | RSTLV_KEY_TYPE_FULL;

    adminKey.startTS = time(NULL);
    adminKey.endTS = 0; /* no end */
    RsGxsGrpMetaData* meta = grp->metaData;

    /* add keys to grp */

    meta->keys.keys[adminKey.keyId] = adminKey;
    meta->keys.keys[privAdminKey.keyId] = privAdminKey;
    meta->keys.keys[pubKey.keyId] = pubKey;
    meta->keys.keys[privPubKey.keyId] = privPubKey;

    meta->mGroupId = adminKey.keyId;
    grp->grpId = meta->mGroupId;

    adminKey.TlvClear();
    privAdminKey.TlvClear();
    privPubKey.TlvClear();
    pubKey.TlvClear();

    // free the private key for now, as it is not in use
    RSA_free(rsa_admin);
    RSA_free(rsa_admin_pub);

    RSA_free(rsa_publish);
    RSA_free(rsa_publish_pub);
}

bool RsGenExchange::createMessage(RsNxsMsg* msg)
{
	const RsGxsGroupId& id = msg->grpId;

	std::map<RsGxsGroupId, RsGxsGrpMetaData*> metaMap;

	metaMap.insert(std::make_pair(id, (RsGxsGrpMetaData*)(NULL)));
	mDataStore->retrieveGxsGrpMetaData(metaMap);
	bool ok = true;

	if(!metaMap[id])
	{
		return false;
	}
	else
	{

		// get publish key
		RsGxsGrpMetaData* meta = metaMap[id];

		// public and shared is publish key
		RsTlvSecurityKeySet& keys = meta->keys;
		RsTlvSecurityKey* pubKey;

		std::map<std::string, RsTlvSecurityKey>::iterator mit =
				keys.keys.begin(), mit_end = keys.keys.end();
		bool pub_key_found = false;
		for(; mit != mit_end; mit++)
		{

			pub_key_found = mit->second.keyFlags & (RSTLV_KEY_TYPE_FULL | RSTLV_KEY_DISTRIB_PUBLIC);

			if(pub_key_found)
				break;
		}

		if(pub_key_found)
		{
			pubKey = &(mit->second);
			RSA* rsa_pub = GxsSecurity::extractPrivateKey(*pubKey);
			EVP_PKEY *key_pub = EVP_PKEY_new();
			EVP_PKEY_assign_RSA(key_pub, rsa_pub);

			/* calc and check signature */
			EVP_MD_CTX *mdctx = EVP_MD_CTX_create();

			ok = EVP_SignInit(mdctx, EVP_sha1()) == 1;
			ok = EVP_SignUpdate(mdctx, msg->msg.bin_data, msg->msg.bin_len) == 1;

			unsigned int siglen = EVP_PKEY_size(key_pub);
			unsigned char sigbuf[siglen];
			ok = EVP_SignFinal(mdctx, sigbuf, &siglen, key_pub) == 1;

			RsGxsMsgMetaData &meta = *(msg->metaData);
			meta.pubSign.signData.setBinData(sigbuf, siglen);
			meta.pubSign.keyId = pubKey->keyId;

			msg->metaData->mMsgId = msg->msgId = GxsSecurity::getBinDataSign(sigbuf, siglen);

			// clean up
			EVP_MD_CTX_destroy(mdctx);
			EVP_PKEY_free(key_pub);
			RSA_free(rsa_pub);
		}
		else
		{
			ok = false;
		}

		delete meta;
	}

	return ok;
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

bool RsGenExchange::getGroupData(const uint32_t &token, std::vector<RsGxsGrpItem *>& grpItem)
{

	std::list<RsNxsGrp*> nxsGrps;
	bool ok = mDataAccess->getGroupData(token, nxsGrps);

	std::list<RsNxsGrp*>::iterator lit = nxsGrps.begin();

	if(ok)
	{
		for(; lit != nxsGrps.end(); lit++)
		{
			RsTlvBinaryData& data = (*lit)->grp;
			RsItem* item = mSerialiser->deserialise(data.bin_data, &data.bin_len);

			if(item != NULL){
				RsGxsGrpItem* gItem = dynamic_cast<RsGxsGrpItem*>(item);
				gItem->meta = *((*lit)->metaData);
				grpItem.push_back(gItem);
				delete *lit;
			}
		}
	}
    return ok;
}

bool RsGenExchange::getMsgData(const uint32_t &token,
                               GxsMsgDataMap &msgItems)
{
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


RsTokenServiceV2* RsGenExchange::getTokenService()
{
    return mDataAccess;
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


bool RsGenExchange::publishGroup(uint32_t& token, RsGxsGrpItem *grpItem)
{

    RsStackMutex stack(mGenMtx);

    token = mDataAccess->generatePublicToken();
    mGrpsToPublish.insert(std::make_pair(token, grpItem));

    return true;
}

bool RsGenExchange::publishMsg(uint32_t& token, RsGxsMsgItem *msgItem)
{

	RsStackMutex stack(mGenMtx);

	token = mDataAccess->generatePublicToken();
	mMsgsToPublish.insert(std::make_pair(token, msgItem));

    return true;
}


void RsGenExchange::publishMsgs()
{
	RsStackMutex stack(mGenMtx);

	std::map<uint32_t, RsGxsMsgItem*>::iterator mit = mMsgsToPublish.begin();

	for(; mit != mMsgsToPublish.end(); )
	{

		RsNxsMsg* msg = new RsNxsMsg(mServType);
		RsGxsMsgItem* msgItem = mit->second;
		uint32_t size = mSerialiser->size(msgItem);
		char mData[size];
		bool ok = mSerialiser->serialise(msgItem, mData, &size);

		if(ok)
		{
			msg->metaData = new RsGxsMsgMetaData();
			*(msg->metaData) = msgItem->meta;
			ok = createMessage(msg);

			if(ok)
				ok = mDataAccess->addMsgData(msg);

			mMsgPublished.insert(std::make_pair(mit->first, std::make_pair(msg->grpId, msg->msgId)));
		}

		// if addition failed then delete nxs message
		if(!ok)
		{
#ifdef GEN_EXCH_DEBUG
			std::cerr << "RsGenExchange::publishMsgs() failed to publish msg " << std::endl;
#endif
			mMsgPublished.insert(std::make_pair(mit->first, std::make_pair(RsGxsGroupId(""), RsGxsMessageId(""))));
			delete msg;
			continue;

		}

		delete msgItem; // delete msg item as we're done with it
	}

	// clear msg list as we're done publishing them and entries
	// are invalid
	mMsgsToPublish.clear();
}

void RsGenExchange::publishGrps()
{

	RsStackMutex stack(mGenMtx);

	std::map<uint32_t, RsGxsGrpItem*>::iterator mit = mGrpsToPublish.begin();

        for(; mit != mGrpsToPublish.end(); mit++)
	{

		RsNxsGrp* grp = new RsNxsGrp(mServType);
		RsGxsGrpItem* grpItem = mit->second;
		uint32_t size = mSerialiser->size(grpItem);

		char gData[size];
		bool ok = mSerialiser->serialise(grpItem, gData, &size);
                grp->grp.setBinData(gData, size);

		if(ok)
		{
			grp->metaData = new RsGxsGrpMetaData();
			*(grp->metaData) = grpItem->meta;
            createGroup(grp);
            size = grp->metaData->serial_size();
            char mData[size];

            ok = grp->metaData->serialise(mData, size);
            grp->meta.setBinData(mData, size);

			ok = mDataAccess->addGroupData(grp);

			// add to published to allow acknowledgement
			mGrpPublished.insert(std::make_pair(mit->first, grp->grpId));
			mDataAccess->updatePublicRequestStatus(mit->first, RsTokenServiceV2::GXS_REQUEST_STATUS_COMPLETE);
		}

		if(!ok)
		{

#ifdef GEN_EXCH_DEBUG
			std::cerr << "RsGenExchange::publishGrps() failed to publish grp " << std::endl;
#endif
			delete grp;

			// add to published to allow acknowledgement, grpid is empty as grp creation failed
			mGrpPublished.insert(std::make_pair(mit->first, RsGxsGroupId("")));
			mDataAccess->updatePublicRequestStatus(mit->first, RsTokenServiceV2::GXS_REQUEST_STATUS_FAILED);
			continue;
		}

		delete grpItem;
	}

	// clear grp list as we're done publishing them and entries
	// are invalid
	mGrpsToPublish.clear();
}
void RsGenExchange::processRecvdData()
{
}


void RsGenExchange::processRecvdMessages()
{
}


void RsGenExchange::processRecvdGroups()
{
}

