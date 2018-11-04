/*******************************************************************************
 * libretroshare/src/gxs: rsgxsutil.cc                                         *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2013-2013 by Christopher Evi-Parker                               *
 * Copyright (C) 2018  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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

#ifdef RS_DEEP_SEARCH
#	include "deep_search/deep_search.h"
#	include "services/p3gxschannels.h"
#	include "rsitems/rsgxschannelitems.h"
#endif

static const uint32_t MAX_GXS_IDS_REQUESTS_NET   =  10 ; // max number of requests from cache/net (avoids killing the system!)

//#define DEBUG_GXSUTIL 1

#ifdef DEBUG_GXSUTIL
#define GXSUTIL_DEBUG() std::cerr << "[" << time(NULL)  << "] : GXS_UTIL : " << __FUNCTION__ << " : "
#endif

RsGxsMessageCleanUp::RsGxsMessageCleanUp(RsGeneralDataService* const dataService, RsGenExchange *genex, uint32_t chunkSize)
: mDs(dataService), mGenExchangeClient(genex), CHUNK_SIZE(chunkSize)
{
	RsGxsGrpMetaTemporaryMap grpMeta;
	mDs->retrieveGxsGrpMetaData(grpMeta);

	for(auto cit=grpMeta.begin();cit != grpMeta.end(); ++cit)
		mGrpMeta.push_back(cit->second);
}

bool RsGxsMessageCleanUp::clean()
{
	uint32_t i = 1;

	rstime_t now = time(NULL);

#ifdef DEBUG_GXSUTIL
	uint16_t service_type = mGenExchangeClient->serviceType() ;
	GXSUTIL_DEBUG() << "  Cleaning up groups in service " << std::hex << service_type << std::dec << std::endl;
#endif
	while(!mGrpMeta.empty())
	{
		const RsGxsGrpMetaData* grpMeta = mGrpMeta.back();
		const RsGxsGroupId& grpId = grpMeta->mGroupId;
		mGrpMeta.pop_back();
		GxsMsgReq req;
		GxsMsgMetaResult result;

		req[grpId] = std::set<RsGxsMessageId>();
		mDs->retrieveGxsMsgMetaData(req, result);

		GxsMsgMetaResult::iterator mit = result.begin();

#ifdef DEBUG_GXSUTIL
		GXSUTIL_DEBUG() << "  Cleaning up group message for group ID " << grpId << std::endl;
#endif
		req.clear();

        uint32_t store_period = mGenExchangeClient->getStoragePeriod(grpId) ;

		for(; mit != result.end(); ++mit)
		{
			std::vector<RsGxsMsgMetaData*>& metaV = mit->second;

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
				RsGxsMsgMetaData* meta = metaV[i];

				bool have_kids = (messages_with_kids.find(meta->mMsgId)!=messages_with_kids.end());

				// check if expired
				bool remove = store_period > 0 && ((meta->mPublishTs + store_period) < now) && !have_kids;

				// check client does not want the message kept regardless of age
				remove &= !(meta->mMsgStatus & GXS_SERV::GXS_MSG_STATUS_KEEP);

				// if not subscribed remove messages (can optimise this really)
				remove = remove ||  (grpMeta->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_NOT_SUBSCRIBED);
				remove = remove || !(grpMeta->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED);

#ifdef DEBUG_GXSUTIL
				GXSUTIL_DEBUG() << "    msg id " << meta->mMsgId << " in grp " << grpId << ": keep_flag=" << bool(meta->mMsgStatus & GXS_SERV::GXS_MSG_STATUS_KEEP)
				                << " subscribed: " << bool(grpMeta->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED) << " store_period: " << store_period
				                << " kids: " << have_kids << " now - meta->mPublishTs: " << now - meta->mPublishTs ;
#endif

				if( remove )
				{
					req[grpId].insert(meta->mMsgId);
                    
#ifdef DEBUG_GXSUTIL
					std::cerr << "    Scheduling for removal." << std::endl;
#endif
				}
#ifdef DEBUG_GXSUTIL
				else
					std::cerr << std::endl;
#endif

				delete meta;
			}
		}

		mDs->removeMsgs(req);

		i++;
		if(i > CHUNK_SIZE) break;
	}

	return mGrpMeta.empty();
}

RsGxsIntegrityCheck::RsGxsIntegrityCheck(
        RsGeneralDataService* const dataService, RsGenExchange* genex,
        RsSerialType& serializer, RsGixs* gixs ) :
    mDs(dataService), mGenExchangeClient(genex), mSerializer(serializer),
    mDone(false), mIntegrityMutex("integrity"), mGixs(gixs) {}

void RsGxsIntegrityCheck::run()
{
	check();

	RS_STACK_MUTEX(mIntegrityMutex);
	mDone = true;
}

bool RsGxsIntegrityCheck::check()
{
#ifdef RS_DEEP_SEARCH
	bool isGxsChannels = mGenExchangeClient->serviceType() == RS_SERVICE_GXS_TYPE_CHANNELS;
	std::set<RsGxsGroupId> indexedGroups;
#endif

    // first take out all the groups
    std::map<RsGxsGroupId, RsNxsGrp*> grp;
    mDs->retrieveNxsGrps(grp, true, true);
    std::vector<RsGxsGroupId> grpsToDel;
    GxsMsgReq msgIds;
    GxsMsgReq grps;

    std::map<RsGxsId,RsIdentityUsage> used_gxs_ids ;
    std::set<RsGxsGroupId> subscribed_groups ;

    // compute hash and compare to stored value, if it fails then simply add it
	// to list
	for( std::map<RsGxsGroupId, RsNxsGrp*>::iterator git = grp.begin();
	     git != grp.end(); ++git )
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
					subscribed_groups.insert(git->first);

					if(!grp->metaData->mAuthorId.isNull())
					{
#ifdef DEBUG_GXSUTIL
						GXSUTIL_DEBUG() << "TimeStamping group authors' key ID " << grp->metaData->mAuthorId << " in group ID " << grp->grpId << std::endl;
#endif
						if( rsReputations && rsReputations->overallReputationLevel(grp->metaData->mAuthorId ) > RsReputations::REPUTATION_LOCALLY_NEGATIVE )
							used_gxs_ids.insert(std::make_pair(grp->metaData->mAuthorId, RsIdentityUsage(mGenExchangeClient->serviceType(), RsIdentityUsage::GROUP_AUTHOR_KEEP_ALIVE,grp->grpId)));
					}
				}
			}
			else msgIds.erase(msgIds.find(grp->grpId));

#ifdef RS_DEEP_SEARCH
			if( isGxsChannels
			        && grp->metaData->mCircleType == GXS_CIRCLE_TYPE_PUBLIC
			        && grp->metaData->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED )
			{
				RsGxsGrpMetaData meta;
				meta.deserialise(grp->meta.bin_data, grp->meta.bin_len);

				uint32_t blz = grp->grp.bin_len;
				RsItem* rIt = mSerializer.deserialise(grp->grp.bin_data,
				                                      &blz);

				if( RsGxsChannelGroupItem* cgIt =
				        dynamic_cast<RsGxsChannelGroupItem*>(rIt) )
				{
					RsGxsChannelGroup cg;
					cgIt->toChannelGroup(cg, false);
					cg.mMeta = meta;

					indexedGroups.insert(grp->grpId);
					DeepSearch::indexChannelGroup(cg);
				}
				else
				{
					std::cerr << __PRETTY_FUNCTION__ << " Group: "
					          << meta.mGroupId.toStdString() << " "
					          << meta.mGroupName
					          << " doesn't seems a channel, please "
					          << "report to developers"
					          << std::endl;
					print_stacktrace();
				}

				delete rIt;
			}
#endif
		}
		else
		{
			grpsToDel.push_back(grp->grpId);
#ifdef RS_DEEP_SEARCH
			if(isGxsChannels) DeepSearch::removeChannelFromIndex(grp->grpId);
#endif
		}

		if( !(grp->metaData->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED) &&
		        !(grp->metaData->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN) &&
		        !(grp->metaData->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_PUBLISH) )
		{
			RsGroupNetworkStats stats;
			mGenExchangeClient->getGroupNetworkStats(grp->grpId,stats);

			if( stats.mSuppliers == 0 && stats.mMaxVisibleCount == 0
			        && stats.mGrpAutoSync )
			{
#ifdef DEBUG_GXSUTIL
				GXSUTIL_DEBUG() << "Scheduling group \"" << grp->metaData->mGroupName << "\" ID=" << grp->grpId << " in service " << std::hex << mGenExchangeClient->serviceType() << std::dec << " for deletion because it has no suppliers not any visible data at friends." << std::endl;
#endif
				grpsToDel.push_back(grp->grpId);
			}
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
	    std::set<RsGxsMessageId> &msgIdV = msgIdsIt->second;

	    std::set<RsGxsMessageId>::iterator msgIdIt;
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
				msgsToDel[grpId].insert(msgId);
#ifdef RS_DEEP_SEARCH
				if(isGxsChannels)
					DeepSearch::removeChannelPostFromIndex(grpId, msgId);
#endif
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
				std::cerr << __PRETTY_FUNCTION__ <<" (EE) deleting message data"
				          << " with wrong hash or null meta data. meta="
				          << (void*)msg->metaData << std::endl;
				msgsToDel[msg->grpId].insert(msg->msgId);
#ifdef RS_DEEP_SEARCH
				if(isGxsChannels)
					DeepSearch::removeChannelPostFromIndex(msg->grpId, msg->msgId);
#endif
			}
			else if (subscribed_groups.count(msg->metaData->mGroupId))
			{
#ifdef RS_DEEP_SEARCH
				if( isGxsChannels
				        && indexedGroups.count(msg->metaData->mGroupId) )
				{
					RsGxsMsgMetaData meta;
					meta.deserialise(msg->meta.bin_data, &msg->meta.bin_len);

					uint32_t blz = msg->msg.bin_len;
					RsItem* rIt = mSerializer.deserialise(msg->msg.bin_data,
					                                      &blz);

					if( RsGxsChannelPostItem* cgIt =
					        dynamic_cast<RsGxsChannelPostItem*>(rIt) )
					{
						RsGxsChannelPost cg;
						cgIt->toChannelPost(cg, false);
						cg.mMeta = meta;

						DeepSearch::indexChannelPost(cg);
					}
					else if(dynamic_cast<RsGxsCommentItem*>(rIt)) {}
					else if(dynamic_cast<RsGxsVoteItem*>(rIt)) {}
					else
					{
						std::cerr << __PRETTY_FUNCTION__ << " Message: "
						          << meta.mMsgId.toStdString()
						          << " in group: "
						          << meta.mGroupId.toStdString() << " "
						          << " doesn't seems a channel post, please "
						          << "report to developers"
						          << std::endl;
						print_stacktrace();
					}

					delete rIt;
				}
#endif

				if(!msg->metaData->mAuthorId.isNull())
				{
#ifdef DEBUG_GXSUTIL
					GXSUTIL_DEBUG() << "TimeStamping message authors' key ID " << msg->metaData->mAuthorId << " in message " << msg->msgId << ", group ID " << msg->grpId<< std::endl;
#endif
					if(rsReputations!=NULL && rsReputations->overallReputationLevel(msg->metaData->mAuthorId) > RsReputations::REPUTATION_LOCALLY_NEGATIVE)
						used_gxs_ids.insert(std::make_pair(msg->metaData->mAuthorId,RsIdentityUsage(mGenExchangeClient->serviceType(),RsIdentityUsage::MESSAGE_AUTHOR_KEEP_ALIVE,msg->metaData->mGroupId,msg->metaData->mMsgId))) ;
				}
			}

		    delete msg;
	    }
    }

    mDs->removeMsgs(msgsToDel);

	{
		RS_STACK_MUTEX(mIntegrityMutex);

		std::vector<RsGxsGroupId>::iterator grpIt;
		for(grpIt = grpsToDel.begin(); grpIt != grpsToDel.end(); ++grpIt)
		{
			mDeletedGrps.push_back(*grpIt);
		}
		mDeletedMsgs = msgsToDel;

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

			if(!mGixs->haveKey(gxs_ids[n].first))	// checks if we have it already in the cache (conservative way to ensure that we atually have it)
			{
				mGixs->requestKey(gxs_ids[n].first,connected_friends,gxs_ids[n].second);

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
			mGixs->timeStampKey(gxs_ids[n].first,gxs_ids[n].second);

			gxs_ids[n] = gxs_ids[gxs_ids.size()-1] ;
			gxs_ids.pop_back() ;
		}
#ifdef DEBUG_GXSUTIL
		GXSUTIL_DEBUG() << "  total actual cache requests: "<< nb_requested_not_in_cache << std::endl;
#endif
	}

    return true;
}

bool RsGxsIntegrityCheck::isDone()
{
	RS_STACK_MUTEX(mIntegrityMutex);
	return mDone;
}

void RsGxsIntegrityCheck::getDeletedIds(std::list<RsGxsGroupId>& grpIds, std::map<RsGxsGroupId, std::set<RsGxsMessageId> >& msgIds)
{
	RS_STACK_MUTEX(mIntegrityMutex);
	grpIds = mDeletedGrps;
	msgIds = mDeletedMsgs;
}

