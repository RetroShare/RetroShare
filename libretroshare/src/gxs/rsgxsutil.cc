/*
 * libretroshare/src/gxs: rsgxsutil.cc
 *
 * RetroShare C++ Interface. Generic routines that are useful in GXS
 *
 * Copyright 2013-2013 by Christopher Evi-Parker
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

#include <time.h>

#include "rsgxsutil.h"
#include "retroshare/rsgxsflags.h"
#include "retroshare/rspeers.h"
#include "pqi/pqihash.h"
#include "gxs/rsgixs.h"

static const uint32_t MAX_GXS_IDS_REQUESTS_NET   =  10 ; // max number of requests from cache/net (avoids killing the system!)

//#define GXSUTIL_DEBUG 1

RsGxsMessageCleanUp::RsGxsMessageCleanUp(RsGeneralDataService* const dataService, uint32_t messageStorePeriod, uint32_t chunkSize)
: mDs(dataService), MESSAGE_STORE_PERIOD(messageStorePeriod), CHUNK_SIZE(chunkSize)
{

	std::map<RsGxsGroupId, RsGxsGrpMetaData*> grpMeta;
	mDs->retrieveGxsGrpMetaData(grpMeta);

	std::map<RsGxsGroupId, RsGxsGrpMetaData*>::iterator cit = grpMeta.begin();

	for(;cit != grpMeta.end(); ++cit)
	{
		mGrpMeta.push_back(cit->second);
	}
}


bool RsGxsMessageCleanUp::clean()
{
	int i = 1;

	time_t now = time(NULL);

	while(!mGrpMeta.empty())
	{
		RsGxsGrpMetaData* grpMeta = mGrpMeta.back();
		const RsGxsGroupId& grpId = grpMeta->mGroupId;
		mGrpMeta.pop_back();
		GxsMsgReq req;
		GxsMsgMetaResult result;

		req[grpId] = std::vector<RsGxsMessageId>();
		mDs->retrieveGxsMsgMetaData(req, result);

		GxsMsgMetaResult::iterator mit = result.begin();

		req.clear();

		for(; mit != result.end(); ++mit)
		{
			std::vector<RsGxsMsgMetaData*>& metaV = mit->second;
			std::vector<RsGxsMsgMetaData*>::iterator vit = metaV.begin();

			for(; vit != metaV.end(); )
			{
				RsGxsMsgMetaData* meta = *vit;

				// check if expired
				bool remove = (meta->mPublishTs + MESSAGE_STORE_PERIOD) < now;

				// check client does not want the message kept regardless of age
				remove &= !(meta->mMsgStatus & GXS_SERV::GXS_MSG_STATUS_KEEP);

				// if not subscribed remove messages (can optimise this really)
				remove = remove || (grpMeta->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_NOT_SUBSCRIBED);

				if( remove )
				{
					req[grpId].push_back(meta->mMsgId);
				}

				delete meta;
				vit = metaV.erase(vit);
			}
		}

		mDs->removeMsgs(req);

		delete grpMeta;

		i++;
		if(i > CHUNK_SIZE) break;
	}

	return mGrpMeta.empty();
}

RsGxsIntegrityCheck::RsGxsIntegrityCheck(RsGeneralDataService* const dataService, RsGixs *gixs) :
		mDs(dataService), mDone(false), mIntegrityMutex("integrity"),mGixs(gixs)
{ }

void RsGxsIntegrityCheck::run()
{
	check();
}

bool RsGxsIntegrityCheck::check()
{
    // first take out all the groups
    std::map<RsGxsGroupId, RsNxsGrp*> grp;
    mDs->retrieveNxsGrps(grp, true, true);
    std::vector<RsGxsGroupId> grpsToDel;
    GxsMsgReq msgIds;
    GxsMsgReq grps;

    std::set<RsGxsId> used_gxs_ids ;
    std::set<RsGxsGroupId> subscribed_groups ;

    // compute hash and compare to stored value, if it fails then simply add it
    // to list
    std::map<RsGxsGroupId, RsNxsGrp*>::iterator git = grp.begin();
    for(; git != grp.end(); ++git)
    {
	    RsNxsGrp* grp = git->second;
	    RsFileHash currHash;
	    pqihash pHash;
	    pHash.addData(grp->grp.bin_data, grp->grp.bin_len);
	    pHash.Complete(currHash);

	    if(currHash == grp->metaData->mHash)
	    {
		    // get all message ids of group
		    if (mDs->retrieveMsgIds(grp->grpId, msgIds[grp->grpId]) == 1)
		    {
			    // store the group for retrieveNxsMsgs
			    grps[grp->grpId];

			    if(grp->metaData->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED)
			    {
				    subscribed_groups.insert(git->first) ;

				    if(!grp->metaData->mAuthorId.isNull())
				    {
#ifdef GXSUTIL_DEBUG
					    std::cerr << "TimeStamping group authors' key ID " << grp->metaData->mAuthorId << " in group ID " << grp->grpId << std::endl;
#endif

					    used_gxs_ids.insert(grp->metaData->mAuthorId) ;
				    }
			    }
		    }
		    else
		    {
			    msgIds.erase(msgIds.find(grp->grpId));
			    //				grpsToDel.push_back(grp->grpId);
		    }

	    }
	    else
	    {
		    grpsToDel.push_back(grp->grpId);
	    }
	    delete grp;
    }

    mDs->removeGroups(grpsToDel);

    // now messages
    GxsMsgReq msgsToDel;
    GxsMsgResult msgs;

    mDs->retrieveNxsMsgs(grps, msgs, false, true);

    // check msg ids and messages
    GxsMsgReq::iterator msgIdsIt;
    for (msgIdsIt = msgIds.begin(); msgIdsIt != msgIds.end(); ++msgIdsIt)
    {
	    const RsGxsGroupId& grpId = msgIdsIt->first;
	    std::vector<RsGxsMessageId> &msgIdV = msgIdsIt->second;

	    std::vector<RsGxsMessageId>::iterator msgIdIt;
	    for (msgIdIt = msgIdV.begin(); msgIdIt != msgIdV.end(); ++msgIdIt)
	    {
		    const RsGxsMessageId& msgId = *msgIdIt;
		    std::vector<RsNxsMsg*> &nxsMsgV = msgs[grpId];

		    std::vector<RsNxsMsg*>::iterator nxsMsgIt;
		    for (nxsMsgIt = nxsMsgV.begin(); nxsMsgIt != nxsMsgV.end(); ++nxsMsgIt)
		    {
			    RsNxsMsg *nxsMsg = *nxsMsgIt;
			    if (nxsMsg && msgId == nxsMsg->msgId)
			    {
				    break;
			    }
		    }

		    if (nxsMsgIt == nxsMsgV.end())
		    {
			    msgsToDel[grpId].push_back(msgId);
		    }
	    }
    }

    GxsMsgResult::iterator mit = msgs.begin();

    for(; mit != msgs.end(); ++mit)
    {
	    std::vector<RsNxsMsg*>& msgV = mit->second;
	    std::vector<RsNxsMsg*>::iterator vit = msgV.begin();

	    for(; vit != msgV.end(); ++vit)
	    {
		    RsNxsMsg* msg = *vit;
		    RsFileHash currHash;
		    pqihash pHash;
		    pHash.addData(msg->msg.bin_data, msg->msg.bin_len);
		    pHash.Complete(currHash);

		    if(msg->metaData == NULL || currHash != msg->metaData->mHash)
		    {
			    std::cerr << "(EE) deleting message data with wrong hash or null meta data. meta=" << (void*)msg->metaData << std::endl;
			    msgsToDel[msg->grpId].push_back(msg->msgId);
		    }
		    else if(!msg->metaData->mAuthorId.isNull() && subscribed_groups.find(msg->metaData->mGroupId)!=subscribed_groups.end())
		    {
#ifdef GXSUTIL_DEBUG
			    std::cerr << "TimeStamping message authors' key ID " << msg->metaData->mAuthorId << " in message " << msg->msgId << ", group ID " << msg->grpId<< std::endl;
#endif
			    used_gxs_ids.insert(msg->metaData->mAuthorId) ;
		    }

		    delete msg;
	    }
    }

    mDs->removeMsgs(msgsToDel);

    RsStackMutex stack(mIntegrityMutex);
    mDone = true;

    std::vector<RsGxsGroupId>::iterator grpIt;
    for(grpIt = grpsToDel.begin(); grpIt != grpsToDel.end(); ++grpIt)
    {
	    mDeletedGrps.push_back(*grpIt);
    }
    mDeletedMsgs = msgsToDel;

#ifdef GXSUTIL_DEBUG
    std::cerr << "At end of pass, this is the list used GXS ids: " << std::endl;
    std::cerr << "  requesting them to GXS identity service to enforce loading." << std::endl;
#endif

    std::list<RsPeerId> connected_friends ;
    rsPeers->getOnlineList(connected_friends) ;

    std::vector<RsGxsId> gxs_ids ;

    for(std::set<RsGxsId>::const_iterator it(used_gxs_ids.begin());it!=used_gxs_ids.end();++it)
    {
	    gxs_ids.push_back(*it) ;
#ifdef GXSUTIL_DEBUG
	    std::cerr << "    " << *it <<  std::endl;
#endif
    }
    int nb_requested_not_in_cache = 0;

#ifdef GXSUTIL_DEBUG
    std::cerr << "  issuing random get on friends for non existing IDs" << std::endl;
#endif

    for(;nb_requested_not_in_cache<MAX_GXS_IDS_REQUESTS_NET && gxs_ids.size()>0;)
    {
	    uint32_t n = RSRandom::random_u32() % gxs_ids.size() ;
#ifdef GXSUTIL_DEBUG
	    std::cerr << "    requesting ID " << gxs_ids[n] ;
#endif

	    if(!mGixs->haveKey(gxs_ids[n]))	// checks if we have it already in the cache (conservative way to ensure that we atually have it)
	    { 
		    mGixs->requestKey(gxs_ids[n],connected_friends);

		    ++nb_requested_not_in_cache ;
#ifdef GXSUTIL_DEBUG
		    std::cerr << "  ... from cache/net" << std::endl;
#endif
	    }
        else
		    std::cerr << "  ... already in cache" << std::endl;

	    gxs_ids[n] = gxs_ids[gxs_ids.size()-1] ;
	    gxs_ids.pop_back() ;
    }
#ifdef GXSUTIL_DEBUG
        std::cerr << "  total actual cache requests: "<< nb_requested_not_in_cache << std::endl;
#endif
        
	return true;
}

bool RsGxsIntegrityCheck::isDone()
{
	RsStackMutex stack(mIntegrityMutex);
	return mDone;
}

void RsGxsIntegrityCheck::getDeletedIds(std::list<RsGxsGroupId>& grpIds, std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >& msgIds)
{
	RsStackMutex stack(mIntegrityMutex);

	grpIds = mDeletedGrps;
	msgIds = mDeletedMsgs;
}

