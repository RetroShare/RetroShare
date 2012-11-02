
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
#include <openssl/rsa.h>

#include "pqi/pqihash.h"
#include "rsgenexchange.h"
#include "gxssecurity.h"
#include "util/contentvalue.h"
#include "rsgxsflags.h"

RsGenExchange::RsGenExchange(RsGeneralDataService *gds,
                             RsNetworkExchangeService *ns, RsSerialType *serviceSerialiser, uint16_t servType, RsGixs* gixs)
: mGenMtx("GenExchange"), mDataStore(gds), mNetService(ns), mSerialiser(serviceSerialiser), mServType(servType), mGixs(gixs)
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

bool RsGenExchange::createGroup(RsNxsGrp *grp)
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

    /* add public keys to grp */

    meta->keys.keys[adminKey.keyId] = adminKey;
    meta->keys.keys[pubKey.keyId] = pubKey;

    // group is self signing
    // for the creation of group signature
    // only public admin and publish keys are present
    // key set
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

    /* now add private keys to grp */
    meta->keys.keys[privAdminKey.keyId] = privAdminKey;
    meta->keys.keys[privPubKey.keyId] = privPubKey;

    // add admin sign to grpMeta
    meta->signSet.keySignSet[GXS_SERV::FLAG_AUTHEN_ADMIN] = adminSign;

    pqihash hash;

    // get hash of msg data to create msg id
    hash.addData(allGrpData, allGrpDataLen);
    hash.Complete(meta->mGroupId);
    grp->grpId = meta->mGroupId;

    adminKey.TlvClear();
    privAdminKey.TlvClear();
    privPubKey.TlvClear();
    pubKey.TlvClear();

    // clean up
    RSA_free(rsa_admin);
    RSA_free(rsa_admin_pub);

    RSA_free(rsa_publish);
    RSA_free(rsa_publish_pub);

    delete[] allGrpData;
    delete[] metaData;

    return ok;
}

bool RsGenExchange::createMessage(RsNxsMsg* msg)
{
	const RsGxsGroupId& id = msg->grpId;

	std::map<RsGxsGroupId, RsGxsGrpMetaData*> metaMap;

	metaMap.insert(std::make_pair(id, (RsGxsGrpMetaData*)(NULL)));
	mDataStore->retrieveGxsGrpMetaData(metaMap);
	bool ok = true;
	RSA* rsa_pub = NULL;

	if(!metaMap[id])
	{
		return false;
	}
	else
	{
		// get publish key
		RsGxsGrpMetaData* grpMeta = metaMap[id];

		// public and shared is publish key
                RsTlvSecurityKeySet& keys = grpMeta->keys;
		RsTlvSecurityKey* pubKey;

		std::map<std::string, RsTlvSecurityKey>::iterator mit =
				keys.keys.begin(), mit_end = keys.keys.end();
		bool pub_key_found = false;
		for(; mit != mit_end; mit++)
		{

			pub_key_found = mit->second.keyFlags & (RSTLV_KEY_DISTRIB_PRIVATE | RSTLV_KEY_TYPE_FULL);
			if(pub_key_found)
				break;
		}

		if(pub_key_found)
		{
			RsGxsMsgMetaData &meta = *(msg->metaData);

			uint32_t metaDataLen = meta.serial_size();
			uint32_t allMsgDataLen = metaDataLen + msg->msg.bin_len;
			char* metaData = new char[metaDataLen];
			char* allMsgData = new char[allMsgDataLen]; // msgData + metaData

			meta.serialise(metaData, &metaDataLen);

			// copy msg data and meta in allmsgData buffer
			memcpy(allMsgData, msg->msg.bin_data, msg->msg.bin_len);
			memcpy(allMsgData+(msg->msg.bin_len), metaData, metaDataLen);

			// private publish key
			pubKey = &(mit->second);

			RsTlvKeySignatureSet& signSet = meta.signSet;
			RsTlvKeySignature pubSign = signSet.keySignSet[GXS_SERV::FLAG_AUTHEN_PUBLISH];

			GxsSecurity::getSignature(allMsgData, allMsgDataLen, pubKey, pubSign);

			// get hash of msg data to create msg id
			pqihash hash;
			hash.addData(allMsgData, allMsgDataLen);
			hash.Complete(msg->msgId);

			// assign msg id to msg meta
			msg->metaData->mMsgId = msg->msgId;

			//place signature in msg meta
			signSet.keySignSet[GXS_SERV::FLAG_AUTHEN_PUBLISH] = pubSign;

			// clean up

			delete[] metaData;
			delete[] allMsgData;
		}
		else
		{
			ok = false;
		}

		delete grpMeta;
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

                        if(item != NULL)
                        {
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


RsTokenService* RsGenExchange::getTokenService()
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


void RsGenExchange::publishGroup(uint32_t& token, RsGxsGrpItem *grpItem)
{

    RsStackMutex stack(mGenMtx);

    token = mDataAccess->generatePublicToken();
    mGrpsToPublish.insert(std::make_pair(token, grpItem));
}

void RsGenExchange::publishMsg(uint32_t& token, RsGxsMsgItem *msgItem)
{
    RsStackMutex stack(mGenMtx);

    token = mDataAccess->generatePublicToken();
    mMsgsToPublish.insert(std::make_pair(token, msgItem));
}

void RsGenExchange::setGroupSubscribeFlags(uint32_t& token, const RsGxsGroupId& grpId, const uint32_t& flag, const uint32_t& mask)
{
	/* TODO APPLY MASK TO FLAGS */
    RsStackMutex stack(mGenMtx);
    token = mDataAccess->generatePublicToken();

    GrpLocMetaData g;
    g.grpId = grpId;
    g.val.put(RsGeneralDataService::GRP_META_SUBSCRIBE_FLAG, (int32_t)flag);
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
        bool ok = mDataStore->updateMessageMetaData(m) == 1;
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
        bool ok = mDataStore->updateGroupMetaData(g) == 1;

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

            delete[] mData;
            delete msgItem;
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
                    grpItem->meta.mPublishTs = time(NULL);
                    *(grp->metaData) = grpItem->meta;
                    grp->metaData->mSubscribeFlags = GXS_SERV::GROUP_SUBSCRIBE_ADMIN;
                    ok &= createGroup(grp);
                    size = grp->metaData->serial_size();
                    char mData[size];
                    grp->metaData->mGroupId = grp->grpId;
                    ok &= grp->metaData->serialise(mData, size);
                    grp->meta.setBinData(mData, size);
                    RsGxsGroupId grpId = grp->grpId;
                    mDataAccess->addGroupData(grp);

                    // add to published to allow acknowledgement
                    mGrpNotify.insert(std::make_pair(mit->first, grpId));
                    mDataAccess->updatePublicRequestStatus(mit->first, RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE);
		}

		if(!ok)
		{

#ifdef GEN_EXCH_DEBUG
			std::cerr << "RsGenExchange::publishGrps() failed to publish grp " << std::endl;
#endif
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
	mGrpsToPublish.clear();
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

    if(ok)
    {
        grp->metaData = new RsGxsGrpMetaData();
        grpItem->meta.mPublishTs = time(NULL);
        *(grp->metaData) = grpItem->meta;
        grp->metaData->mSubscribeFlags = ~GXS_SERV::GROUP_SUBSCRIBE_MASK;
        createGroup(grp);
        size = grp->metaData->serial_size();
        char mData[size];
        grp->metaData->mGroupId = grp->grpId;
        ok = grp->metaData->serialise(mData, size);
        grp->meta.setBinData(mData, size);

        mDataAccess->addGroupData(grp);
    }

    if(!ok)
    {

#ifdef GEN_EXCH_DEBUG
            std::cerr << "RsGenExchange::createDummyGroup(); failed to publish grp " << std::endl;
#endif
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

    for(; vit != mReceivedMsgs.end(); vit++)
    {
        RsNxsMsg* msg = *vit;
        RsGxsMsgMetaData* meta = new RsGxsMsgMetaData();
        bool ok = meta->deserialise(msg->meta.bin_data, &(msg->meta.bin_len));

        if(ok)
        {
            msgs.insert(std::make_pair(msg, meta));
            msgIds[msg->grpId].push_back(msg->msgId);
        }
        else
        {
#ifdef GXS_GENX_DEBUG
            std::cerr << "failed to deserialise incoming meta, grpId: "
                    << msg->grpId << ", msgId: " << msg->msgId << std::endl;
#endif
            delete msg;
            delete meta;
        }
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

