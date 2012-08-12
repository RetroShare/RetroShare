
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

void RsGenExchange::createGroup(RsNxsGrp *grp)
{
    /* create Keys */
    RSA *rsa_admin = RSA_generate_key(2048, 65537, NULL, NULL);
    RSA *rsa_admin_pub = RSAPublicKey_dup(rsa_admin);


    /* set keys */
    RsTlvSecurityKey adminKey;
    GxsSecurity::setRSAPublicKey(adminKey, rsa_admin_pub);

    adminKey.startTS = time(NULL);
    adminKey.endTS = 0; /* no end */
    RsGxsGrpMetaData* meta = grp->metaData;
    meta->keys.keys[adminKey.keyId] = adminKey;
    meta->mGroupId = adminKey.keyId;
    grp->grpId = meta->mGroupId;

    adminKey.TlvClear();

    // free the private key for now, as it is not in use
    RSA_free(rsa_admin);
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


bool RsGenExchange::publishGroup(RsGxsGrpItem *grpItem)
{

	RsStackMutex stack(mGenMtx);
	mGrpsToPublish.push_back(grpItem);

    return true;
}

bool RsGenExchange::publishMsg(RsGxsMsgItem *msgItem)
{

	RsStackMutex stack(mGenMtx);

	mMsgsToPublish.push_back(msgItem);
    return true;
}


void RsGenExchange::publishMsgs()
{
	RsStackMutex stack(mGenMtx);

	std::vector<RsGxsMsgItem*>::iterator vit = mMsgsToPublish.begin();

	for(; vit != mMsgsToPublish.end(); )
	{

		RsNxsMsg* msg = new RsNxsMsg(mServType);
		RsGxsMsgItem* msgItem = *vit;
		uint32_t size = mSerialiser->size(msgItem);
		char mData[size];
		bool ok = mSerialiser->serialise(msgItem, mData, &size);

		if(ok)
		{
			msg->metaData = new RsGxsMsgMetaData();
			*(msg->metaData) = msgItem->meta;
			ok = mDataAccess->addMsgData(msg);

			if(ok)
			{
				RsGxsMsgChange* mc = new RsGxsMsgChange();
				mNotifications.push_back(mc);
			}
		}

		// if addition failed then delete nxs message
		if(!ok)
		{
#ifdef GEN_EXCH_DEBUG
			std::cerr << "RsGenExchange::publishMsgs() failed to serialise msg " << std::endl;
#endif
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

	std::vector<RsGxsGrpItem*>::iterator vit = mGrpsToPublish.begin();

	for(; vit != mGrpsToPublish.end();)
	{

		RsNxsGrp* grp = new RsNxsGrp(mServType);
		RsGxsGrpItem* grpItem = *vit;
		uint32_t size = mSerialiser->size(grpItem);

		char gData[size];
		bool ok = mSerialiser->serialise(grpItem, gData, &size);
                grp->grp.setBinData(gData, size);

		if(ok)
		{
			grp->metaData = new RsGxsGrpMetaData();
			*(grp->metaData) = grpItem->meta;
                        createGroup(grp);
			ok = mDataAccess->addGroupData(grp);
			RsGxsGroupChange* gc = new RsGxsGroupChange();
                        gc->grpIdList.push_back(grp->grpId);
			mNotifications.push_back(gc);
		}

		if(!ok)
		{

#ifdef GEN_EXCH_DEBUG
			std::cerr << "RsGenExchange::publishGrps() failed to publish grp " << std::endl;
#endif
			delete grp;
			continue;
		}

		delete grpItem;

		vit = mGrpsToPublish.erase(vit);
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

