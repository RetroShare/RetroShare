/*******************************************************************************
 * libretroshare/src/gxs: rsgxsutil.cc                                         *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2013  Christopher Evi-Parker                                  *
 * Copyright (C) 2018-2021  Gioacchino Mazzurco <gio@eigenlab.org>             *
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

#include "util/rstime.h"

#include "rsgxsutil.h"
#include "retroshare/rsgxsflags.h"
#include "retroshare/rspeers.h"
#include "pqi/pqihash.h"
#include "gxs/rsgixs.h"

// The goals of this set of methods is to check GXS messages and groups for consistency, mostly
// re-ferifying signatures and hashes, to make sure that the data hasn't been tempered. This shouldn't
// happen anyway, but we still conduct these test as an extra safety measure.

static const uint32_t MAX_GXS_IDS_REQUESTS_NET   =  10 ; // max number of requests from cache/net (avoids killing the system!)

// #define DEBUG_GXSUTIL 1

#ifdef DEBUG_GXSUTIL
#define GXSUTIL_DEBUG() std::cerr << "[" << time(NULL)  << "] : GXS_UTIL : " << __FUNCTION__ << " : "
#endif

RsGxsCleanUp::RsGxsCleanUp(RsGeneralDataService* const dataService, RsGenExchange *genex, uint32_t chunkSize)
: mDs(dataService), mGenExchangeClient(genex), CHUNK_SIZE(chunkSize)
{
}

bool RsGxsCleanUp::clean(RsGxsGroupId& next_group_to_check,std::vector<RsGxsGroupId>& grps_to_delete,GxsMsgReq& messages_to_delete)
{
    RsGxsGrpMetaTemporaryMap grpMetaMap;
    mDs->retrieveGxsGrpMetaData(grpMetaMap);

    rstime_t now = time(NULL);

#ifdef DEBUG_GXSUTIL
    uint16_t service_type = mGenExchangeClient->serviceType() ;
    GXSUTIL_DEBUG() << "  Cleaning up groups in service " << std::hex << service_type << std::dec << " starting at group " << next_group_to_check << std::endl;
#endif
    // This method stores/takes the next group to check. This allows to limit group checking to a small part of the total groups
    // in the situation where it takes too much time. So when arriving here, we must start again from where we left last time.

    if(grpMetaMap.empty())		// nothing to do.
    {
        next_group_to_check.clear();
        return true;
    }

    auto it = next_group_to_check.isNull()?grpMetaMap.begin() : grpMetaMap.find(next_group_to_check);

    if(it == grpMetaMap.end())		// group wasn't found
        it = grpMetaMap.begin();

    bool full_round = false;			// did we have the time to test all groups?
    next_group_to_check = it->first;	// covers the case where next_group_to_check is null or not found

    while(true)	// check all groups, starting from the one indicated as parameter
    {
        const RsGxsGrpMetaData& grpMeta = *(it->second);

        // first check if we keep the group or not

        if(!mGenExchangeClient->service_checkIfGroupIsStillUsed(grpMeta))
        {
#ifdef DEBUG_GXSUTIL
            std::cerr << "  Scheduling group " << grpMeta.mGroupId << " for removal." << std::endl;
#endif
            grps_to_delete.push_back(grpMeta.mGroupId);
        }
        else
        {
            const RsGxsGroupId& grpId = grpMeta.mGroupId;
            GxsMsgReq req;
            GxsMsgMetaResult result;

            req[grpId] = std::set<RsGxsMessageId>();
            mDs->retrieveGxsMsgMetaData(req, result);

            GxsMsgMetaResult::iterator mit = result.begin();

#ifdef DEBUG_GXSUTIL
            GXSUTIL_DEBUG() << "  Cleaning up group message for group ID " << grpId << std::endl;
#endif
            uint32_t store_period = mGenExchangeClient->getStoragePeriod(grpId) ;

            for(; mit != result.end(); ++mit)
            {
                auto& metaV = mit->second;

                // First, make a map of which message have a child message. This allows to only delete messages that dont have child messages.
                // A more accurate way to go would be to compute the time of the oldest message and possibly delete all the branch, but in the
                // end the message tree will be deleted slice after slice, which should still be reasonnably fast.
                //
                std::set<RsGxsMessageId> messages_with_kids ;

                for( uint32_t i=0;i<metaV.size();++i)
                    if(!metaV[i]->mParentId.isNull())
                        messages_with_kids.insert(metaV[i]->mParentId) ;

                for( uint32_t i=0;i<metaV.size();++i)
                {
                    const auto& meta = metaV[i];

                    bool have_kids = (messages_with_kids.find(meta->mMsgId)!=messages_with_kids.end());

                    // check if expired
                    bool remove = store_period > 0 && ((meta->mPublishTs + store_period) < now) && !have_kids;

                    // check client does not want the message kept regardless of age
                    remove &= !(meta->mMsgStatus & GXS_SERV::GXS_MSG_STATUS_KEEP_FOREVER);

                    // if not subscribed remove messages (can optimise this really)
                    remove = remove ||  (grpMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_NOT_SUBSCRIBED);
                    remove = remove || !(grpMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED);

#ifdef DEBUG_GXSUTIL
                    GXSUTIL_DEBUG() << "    msg id " << meta->mMsgId << " in grp " << grpId << ": keep_flag=" << bool(meta->mMsgStatus & GXS_SERV::GXS_MSG_STATUS_KEEP_FOREVER)
                                    << " subscribed: " << bool(grpMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED) << " store_period: " << store_period
                                    << " kids: " << have_kids << " now - meta->mPublishTs: " << now - meta->mPublishTs ;
#endif

                    if( remove )
                    {
                        messages_to_delete[grpId].insert(meta->mMsgId);
#ifdef DEBUG_GXSUTIL
                        std::cerr << "    Scheduling for removal." << std::endl;
#endif
                    }
#ifdef DEBUG_GXSUTIL
                    else
                        std::cerr << std::endl;
#endif
                    //delete meta;
                }
            }
        }

        ++it;

        if(it == grpMetaMap.end())
            it = grpMetaMap.begin();

        // check if we looped already

        if(it->first == next_group_to_check)
        {
#ifdef DEBUG_GXSUTIL
            GXSUTIL_DEBUG() << "Had the time to test all groups. Will start again at " << it->first << std::endl;
#endif
            full_round = true;
            break;
        }

        // now check if we spent too much time on this already

        rstime_t tm = time(nullptr);

        //if(tm > now + 1) // we spent more than 1 sec on the job already
        if(tm > now) // we spent more than 1 sec on the job already
        {
#ifdef DEBUG_GXSUTIL
            GXSUTIL_DEBUG() << "Aborting cleanup because it took too much time already. Next group left to be " << it->first << std::endl;
#endif
            next_group_to_check = it->first;
            full_round = false;
            break;
        }
    }

    return full_round;
}

RsGxsIntegrityCheck::RsGxsIntegrityCheck(
        RsGeneralDataService* const dataService, RsGenExchange* genex,
        RsSerialType&, RsGixs* gixs )
  : mDs(dataService), mGenExchangeClient(genex),
    mDone(false), mIntegrityMutex("integrity"), mGixs(gixs) {}

void RsGxsIntegrityCheck::run()
{
	std::vector<RsGxsGroupId> grps_to_delete;
	GxsMsgReq msgs_to_delete;

    check(mGenExchangeClient->serviceType(), mGixs, mDs);

	RS_STACK_MUTEX(mIntegrityMutex);
	mDone = true;
}

bool RsGxsIntegrityCheck::check(uint16_t service_type, RsGixs *mgixs, RsGeneralDataService *mds)
{
#ifdef DEBUG_GXSUTIL
    GXSUTIL_DEBUG() << "Parsing all groups and messages MetaData in service " << std::hex << mds->serviceType() << std::endl;
#endif
    // first take out all the groups
    std::map<RsGxsGroupId, std::shared_ptr<RsGxsGrpMetaData> > grp;

    mds->retrieveGxsGrpMetaData(grp);

    GxsMsgReq msgIds;

    std::map<RsGxsId,RsIdentityUsage> used_gxs_ids ;
    std::set<RsGxsGroupId> subscribed_groups ;

    // Check that message ids...

    for( auto git = grp.begin(); git != grp.end(); ++git )
    {
            const auto& grpMeta = git->second;

            if (mds->retrieveMsgIds(grpMeta->mGroupId, msgIds[grpMeta->mGroupId]) == 1)
            {
                    if(grpMeta->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED)
                    {
                            subscribed_groups.insert(git->first);

                            if(!grpMeta->mAuthorId.isNull())
                            {
#ifdef DEBUG_GXSUTIL
                                    GXSUTIL_DEBUG() << "TimeStamping group authors' key ID " << grpMeta->mAuthorId << " in group ID " << grpMeta->mGroupId << std::endl;
#endif
                                    if( rsReputations && rsReputations->overallReputationLevel( grpMeta->mAuthorId ) > RsReputationLevel::LOCALLY_NEGATIVE )
                                            used_gxs_ids.insert(std::make_pair(grpMeta->mAuthorId, RsIdentityUsage(RsServiceType(service_type), RsIdentityUsage::GROUP_AUTHOR_KEEP_ALIVE,grpMeta->mGroupId)));
                            }
                    }
            }
            else
                    msgIds.erase(msgIds.find(grpMeta->mGroupId));	// could not get them, so group is removed from list.
    }

    // now messages
    GxsMsgMetaResult msgMetas;

    mds->retrieveGxsMsgMetaData(msgIds, msgMetas);

    for(auto mit=msgMetas.begin(); mit != msgMetas.end(); ++mit)
	{
        const auto& msgM = mit->second;

        for(auto vit=msgM.begin(); vit != msgM.end(); ++vit)
		{
            const auto& meta = *vit;

            if (subscribed_groups.count(meta->mGroupId))
                if(!meta->mAuthorId.isNull())
				{
#ifdef DEBUG_GXSUTIL
                    GXSUTIL_DEBUG() << "TimeStamping message authors' key ID " << meta->mAuthorId << " in message " << meta->mMsgId << ", group ID " << meta->mGroupId<< std::endl;
#endif
                    if( rsReputations && rsReputations->overallReputationLevel( meta->mAuthorId ) > RsReputationLevel::LOCALLY_NEGATIVE )
                        used_gxs_ids.insert(std::make_pair(meta->mAuthorId,RsIdentityUsage(RsServiceType(service_type),
                                                                                                    RsIdentityUsage::MESSAGE_AUTHOR_KEEP_ALIVE,
                                                                                                    meta->mGroupId,
                                                                                                    meta->mMsgId,
                                                                                                    meta->mParentId,
                                                                                                    meta->mThreadId))) ;
				}
	    }
    }

	{
#ifdef DEBUG_GXSUTIL
		GXSUTIL_DEBUG() << "At end of pass, this is the list used GXS ids: " << std::endl;
		GXSUTIL_DEBUG() << "  requesting them to GXS identity service to enforce loading." << std::endl;
#endif

		std::list<RsPeerId> connected_friends ;
		rsPeers->getOnlineList(connected_friends) ;

		std::vector<std::pair<RsGxsId,RsIdentityUsage> > gxs_ids ;

		for(std::map<RsGxsId,RsIdentityUsage>::const_iterator it(used_gxs_ids.begin());it!=used_gxs_ids.end();++it)
		{
			gxs_ids.push_back(*it) ;
#ifdef DEBUG_GXSUTIL
			GXSUTIL_DEBUG() << "    " << it->first <<  std::endl;
#endif
		}
		uint32_t nb_requested_not_in_cache = 0;

#ifdef DEBUG_GXSUTIL
		GXSUTIL_DEBUG() << "  issuing random get on friends for non existing IDs" << std::endl;
#endif

		// now request a cache update for them, which triggers downloading from friends, if missing.

		for(;nb_requested_not_in_cache<MAX_GXS_IDS_REQUESTS_NET && !gxs_ids.empty();)
		{
			uint32_t n = RSRandom::random_u32() % gxs_ids.size() ;
#ifdef DEBUG_GXSUTIL
			GXSUTIL_DEBUG() << "    requesting ID " << gxs_ids[n].first ;
#endif

            if(!mgixs->haveKey(gxs_ids[n].first))	// checks if we have it already in the cache (conservative way to ensure that we atually have it)
			{
                mgixs->requestKey(gxs_ids[n].first,connected_friends,gxs_ids[n].second);

				++nb_requested_not_in_cache ;
#ifdef DEBUG_GXSUTIL
				GXSUTIL_DEBUG() << "  ... from cache/net" << std::endl;
#endif
			}
			else
			{
#ifdef DEBUG_GXSUTIL
				GXSUTIL_DEBUG() << "  ... already in cache" << std::endl;
#endif
			}
            mgixs->timeStampKey(gxs_ids[n].first,gxs_ids[n].second);

			gxs_ids[n] = gxs_ids[gxs_ids.size()-1] ;
			gxs_ids.pop_back() ;
		}
#ifdef DEBUG_GXSUTIL
		GXSUTIL_DEBUG() << "  total actual cache requests: "<< nb_requested_not_in_cache << std::endl;
#endif
	}

    return true;
}

bool RsGxsSinglePassIntegrityCheck::check(
        uint16_t /*service_type*/, RsGixs* /*mgixs*/, RsGeneralDataService* mds,
        std::vector<RsGxsGroupId>& grpsToDel, GxsMsgReq& msgsToDel )
{
#ifdef DEBUG_GXSUTIL
    GXSUTIL_DEBUG() << "Parsing all groups and messages data in service " << std::hex << mds->serviceType() << " for integrity check. Could take a while..." << std::endl;
#endif

    // first take out all the groups
    std::map<RsGxsGroupId, RsNxsGrp*> grp;
    mds->retrieveNxsGrps(grp, true);
    GxsMsgReq msgIds;
    GxsMsgReq grps;

    std::map<RsGxsId,RsIdentityUsage> used_gxs_ids ;
    std::set<RsGxsGroupId> subscribed_groups ;

    // compute hash and compare to stored value, if it fails then simply add it
    // to list
    for( std::map<RsGxsGroupId, RsNxsGrp*>::iterator git = grp.begin(); git != grp.end(); ++git )
    {
        RsNxsGrp* grp = git->second;
        RsFileHash currHash;
        pqihash pHash;
        pHash.addData(grp->grp.bin_data, grp->grp.bin_len);
        pHash.Complete(currHash);

        if(currHash == grp->metaData->mHash)
        {
            // Get all message ids of group, store them in msgIds, creating the grp entry at the same time.

            if (mds->retrieveMsgIds(grp->grpId, msgIds[grp->grpId]) == 1)
            {
                // store the group for retrieveNxsMsgs
                grps[grp->grpId];

                if(grp->metaData->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED)
                    subscribed_groups.insert(git->first);
            }
            else
                msgIds.erase(msgIds.find(grp->grpId));	// could not get them, so group is removed from list.
        }
		else
		{
			RS_WARN( "deleting group ", grp->grpId,
			         " with wrong hash or null/corrupted meta data. meta=",
			         grp->metaData );
			grpsToDel.push_back(grp->grpId);
		}

        delete grp;
    }

    // now messages
    GxsMsgResult msgs;

    mds->retrieveNxsMsgs(grps, msgs, true);

    // Check msg ids and messages. Go through all message IDs referred to by the db call
    // and verify that the message belongs to the nxs msg data that was just retrieved.

    for(auto& msgIdsIt:msgIds)
    {
        const RsGxsGroupId&       grpId  = msgIdsIt.first;
        std::set<RsGxsMessageId>& msgIdV = msgIdsIt.second;

        std::set<RsGxsMessageId> nxsMsgS;
        std::vector<RsNxsMsg*>& nxsMsgV = msgs[grpId];

        // To make the search efficient, we first build a set of msgIds to search in.
        // Set build and search are both O(n log(n)).

        for(auto& nxsMsg:nxsMsgV)
            if(nxsMsg)
                nxsMsgS.insert(nxsMsg->msgId);

		for (auto& msgId:msgIdV)
			if(nxsMsgS.find(msgId) == nxsMsgS.end())
				msgsToDel[grpId].insert(msgId);
    }

    for(auto mit = msgs.begin(); mit != msgs.end(); ++mit)
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
				RS_WARN( "deleting message ", msg->msgId, " in group ",
				         msg->grpId,
				         " with wrong hash or null/corrupted meta data. meta=",
				         static_cast<void*>(msg->metaData) );
                msgsToDel[msg->grpId].insert(msg->msgId);
            }

            delete msg;
        }
    }

    return true;
}
bool RsGxsIntegrityCheck::isDone()
{
	RS_STACK_MUTEX(mIntegrityMutex);
	return mDone;
}

void RsGxsIntegrityCheck::getDeletedIds(std::vector<RsGxsGroupId>& grpIds, GxsMsgReq& msgIds)
{
	RS_STACK_MUTEX(mIntegrityMutex);
	grpIds = mDeletedGrps;
	msgIds = mDeletedMsgs;
}

