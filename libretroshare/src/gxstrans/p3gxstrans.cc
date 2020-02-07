/*******************************************************************************
 * libretroshare/src/gxstrans: p3gxstrans.cc                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2016-2019  Gioacchino Mazzurco <gio@eigenlab.org>             *
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
#include "util/rsdir.h"
#include "gxstrans/p3gxstrans.h"
#include "util/stacktrace.h"
#include "util/rsdebug.h"

//#define DEBUG_GXSTRANS 1

typedef unsigned int uint;

/*extern*/ RsGxsTrans* rsGxsTrans = nullptr;

const uint32_t p3GxsTrans::MAX_DELAY_BETWEEN_CLEANUPS = 900; // every 15 mins. Could be less.

p3GxsTrans::~p3GxsTrans()
{
    // (cyril) this cannot be called here! There's chances the thread that saves configs will be dead already!
	//p3Config::saveConfiguration();

	{
		RS_STACK_MUTEX(mIngoingMutex);
		for ( auto& kv : mIncomingQueue) delete kv.second;
	}
}

bool p3GxsTrans::getStatistics(GxsTransStatistics& stats)
{
	{
		RS_STACK_MUTEX(mDataMutex);
		stats.prefered_group_id = mPreferredGroupId;
	}
    stats.outgoing_records.clear();

    {
		RS_STACK_MUTEX(mOutgoingMutex);

		for ( auto it = mOutgoingQueue.begin(); it != mOutgoingQueue.end(); ++it)
		{
			const OutgoingRecord& pr(it->second);

			RsGxsTransOutgoingRecord rec ;
            rec.status = pr.status ;
            rec.send_TS = pr.sent_ts ;
            rec.group_id = pr.group_id ;
            rec.trans_id = pr.mailItem.mailId ;
            rec.recipient = pr.recipient ;
            rec.data_size = pr.mailData.size();
            rec.data_hash = RsDirUtil::sha1sum(pr.mailData.data(),pr.mailData.size());
            rec.client_service = pr.clientService ;

            stats.outgoing_records.push_back(rec) ;
        }
    }

    return true;
}

bool p3GxsTrans::sendData( RsGxsTransId& mailId,
                           GxsTransSubServices service,
                           const RsGxsId& own_gxsid, const RsGxsId& recipient,
                           const uint8_t* data, uint32_t size,
                           RsGxsTransEncryptionMode cm )
{
#ifdef DEBUG_GXSTRANS
	std::cout << "p3GxsTrans::sendEmail(...)" << std::endl;
#endif

	if(!mIdService.isOwnId(own_gxsid))
	{
		std::cerr << "p3GxsTrans::sendEmail(...) isOwnId(own_gxsid) false!"
		          << std::endl;
		return false;
	}

	if(recipient.isNull())
	{
		std::cerr << "p3GxsTrans::sendEmail(...) got invalid recipient"
		          << std::endl;
		print_stacktrace();
		return false;
	}

	OutgoingRecord pr( recipient, service, data, size );

    pr.mailItem.clear();
	pr.author = own_gxsid; //pr.mailItem.meta.mAuthorId = own_gxsid;
	pr.mailItem.cryptoType = cm;
	pr.mailItem.mailId = RSRandom::random_u64();

	{
		RS_STACK_MUTEX(mOutgoingMutex);
		mOutgoingQueue.insert(prMap::value_type(pr.mailItem.mailId, pr));
	}

	mailId = pr.mailItem.mailId;

	IndicateConfigChanged();	// This causes the saving of the message after all data has been filled in.
	return true;
}

bool p3GxsTrans::querySendStatus(RsGxsTransId mailId, GxsTransSendStatus& st)
{
	RS_STACK_MUTEX(mOutgoingMutex);
	auto it = mOutgoingQueue.find(mailId);
	if( it != mOutgoingQueue.end() )
	{
		st = it->second.status;
		return true;
	}
	return false;
}

void p3GxsTrans::registerGxsTransClient(
        GxsTransSubServices serviceType, GxsTransClient* service)
{
	RS_STACK_MUTEX(mServClientsMutex);
	mServClients[serviceType] = service;
}

void p3GxsTrans::handleResponse(uint32_t token, uint32_t req_type)
{
#ifdef DEBUG_GXSTRANS
	std::cout << "p3GxsTrans::handleResponse(" << token << ", " << req_type << ")" << std::endl;
#endif
	bool changed = false ;

	switch (req_type)
	{
	case GROUPS_LIST:
	{
#ifdef DEBUG_GXSTRANS
		std::cerr << "  Reviewing available groups. " << std::endl;
#endif
		std::vector<RsGxsGrpItem*> groups;
		getGroupData(token, groups);

		// First recompute the prefered group Id.

		{
			RS_STACK_MUTEX(mDataMutex);

			for( auto grp : groups )
			{
				locked_supersedePreferredGroup(grp->meta.mGroupId);

				if(RsGenExchange::getStoragePeriod(grp->meta.mGroupId) != GXS_STORAGE_PERIOD)
				{
					std::cerr << "(WW) forcing storage period in GxsTrans group " << grp->meta.mGroupId << " to " << GXS_STORAGE_PERIOD << " seconds. Value was " <<  RsGenExchange::getStoragePeriod(grp->meta.mGroupId) << std::endl;

					RsGenExchange::setStoragePeriod(grp->meta.mGroupId,GXS_STORAGE_PERIOD) ;
				}
			}
		}

#ifdef DEBUG_GXSTRANS
		std::cerr << "  computed preferred group id: " << mPreferredGroupId << std::endl;
#endif
		for( auto grp : groups )
		{
			/* For each group check if it is better candidate then
			 * preferredGroupId, if it is supplant it and subscribe if it is not
			 * subscribed yet.
			 * Otherwise if it has recent messages subscribe.
			 * If the group was already subscribed has no recent messages
			 * unsubscribe.
			 */

			const RsGroupMetaData& meta = grp->meta;
			bool subscribed = IS_GROUP_SUBSCRIBED(meta.mSubscribeFlags);

			// if mLastPost is 0, then the group is not subscribed, so it only has impact on shouldSubscribe.  In any case, a group
			// with no information shouldn't be subscribed, so the olderThen() test is still valid in the case mLastPost=0.

			bool old = olderThen( meta.mLastPost, UNUSED_GROUP_UNSUBSCRIBE_INTERVAL );
			uint32_t token;

			bool shouldSubscribe   = false ;
			bool shouldUnSubscribe = false ;
			{
				RS_STACK_MUTEX(mDataMutex);

				shouldSubscribe   = (!subscribed) && ((!old)|| meta.mGroupId == mPreferredGroupId );
				shouldUnSubscribe = ( subscribed) &&    old && meta.mGroupId != mPreferredGroupId;
			}

#ifdef DEBUG_GXSTRANS
			std::cout << "  group " << grp->meta.mGroupId << ", subscribed: " << subscribed << " last post: " << meta.mLastPost << " should subscribe: "<< shouldSubscribe
			           << ", should unsubscribe: " << shouldUnSubscribe << std::endl;
#endif
			if(shouldSubscribe)
				RsGenExchange::subscribeToGroup(token, meta.mGroupId, true);
			else if(shouldUnSubscribe)
				RsGenExchange::subscribeToGroup(token, meta.mGroupId, false);

#ifdef GXS_MAIL_GRP_DEBUG
			char buff[30];
			struct tm* timeinfo;
			timeinfo = localtime(&meta.mLastPost);
			strftime(buff, sizeof(buff), "%Y %b %d %H:%M", timeinfo);

			std::cout << "p3GxsTrans::handleResponse(...) GROUPS_LIST "
			          << "meta.mGroupId: " << meta.mGroupId
			          << " meta.mLastPost: " << buff
			          << " subscribed: " << subscribed
			          << " old: " << old
			          << " shoudlSubscribe: " << shoudlSubscribe
			          << " shoudlUnSubscribe: " << shoudlUnSubscribe
			          << std::endl;
#endif // GXS_MAIL_GRP_DEBUG

			delete grp;
		}

		bool have_preferred_group = false ;

		{
			RS_STACK_MUTEX(mDataMutex);
			have_preferred_group = !mPreferredGroupId.isNull();
		}

		if(!have_preferred_group)
		{
			/* This is true only at first run when we haven't received mail
			 * distribuition groups from friends */

#ifdef DEBUG_GXSTRANS
			std::cerr << "p3GxsTrans::handleResponse(...) preferredGroupId.isNu"
			          << "ll() let's create a new group." << std::endl;
#endif
			uint32_t token;
			publishGroup(token, new RsGxsTransGroupItem());
			queueRequest(token, GROUP_CREATE);
		}

		break;
	}
	case GROUP_CREATE:
	{
#ifdef DEBUG_GXSTRANS
		std::cerr << "p3GxsTrans::handleResponse(...) GROUP_CREATE" << std::endl;
#endif
		RsGxsGroupId grpId;
		acknowledgeTokenGrp(token, grpId);

		RS_STACK_MUTEX(mDataMutex);
		locked_supersedePreferredGroup(grpId);
		break;
	}
	case MAILS_UPDATE:
	{
#ifdef DEBUG_GXSTRANS
		std::cout << "p3GxsTrans::handleResponse(...) MAILS_UPDATE" << std::endl;
#endif
		typedef std::map<RsGxsGroupId, std::vector<RsGxsMsgItem*> > GxsMsgDataMap;
		GxsMsgDataMap gpMsgMap;
		getMsgData(token, gpMsgMap);
		for ( GxsMsgDataMap::iterator gIt = gpMsgMap.begin();
		      gIt != gpMsgMap.end(); ++gIt )
		{
			typedef std::vector<RsGxsMsgItem*> vT;
			vT& mv(gIt->second);
			for( vT::const_iterator mIt = mv.begin(); mIt != mv.end(); ++mIt )
			{
				RsGxsMsgItem* gIt = *mIt;
				switch(static_cast<GxsTransItemsSubtypes>(gIt->PacketSubType()))
				{
				case GxsTransItemsSubtypes::GXS_TRANS_SUBTYPE_MAIL:
				case GxsTransItemsSubtypes::GXS_TRANS_SUBTYPE_RECEIPT:
				{
					RsGxsTransBaseMsgItem* mb = dynamic_cast<RsGxsTransBaseMsgItem*>(*mIt);

					if(mb)
					{
						RS_STACK_MUTEX(mIngoingMutex);
						mIncomingQueue.insert(inMap::value_type(mb->mailId,mb));

						changed = true ;
					}
					else
						std::cerr << "p3GxsTrans::handleResponse(...) "
						          << "GXS_MAIL_SUBTYPE_MAIL cast error, "
						          << "something really wrong is happening"
						          << std::endl;
					break;
				}
				default:
					std::cerr << "p3GxsTrans::handleResponse(...) MAILS_UPDATE "
					          << "Unknown mail subtype : "
					          << static_cast<uint>(gIt->PacketSubType())
					          << std::endl;
					delete gIt;
					break;
				}
			}
		}
		break;
	}
	default:
		std::cerr << "p3GxsTrans::handleResponse(...) Unknown req_type: "
		          << req_type << std::endl;
		break;
	}

	if(changed)
		IndicateConfigChanged();
}
void p3GxsTrans::GxsTransIntegrityCleanupThread::getPerUserStatistics(std::map<RsGxsId,MsgSizeCount>& m)
{
	RS_STACK_MUTEX(mMtx) ;

	m = total_message_size_and_count ;
}

void p3GxsTrans::GxsTransIntegrityCleanupThread::getMessagesToDelete(GxsMsgReq& m)
{
	RS_STACK_MUTEX(mMtx) ;

	m = mMsgToDel ;
	mMsgToDel.clear();
}

// This method does two things:
//  1 - cleaning up old messages and messages for which an ACK has been received.
//  2 - building per user statistics across groups. This is important because it allows to mitigate the excess of
//      messages, which might be due to spam.
//
// Note: the anti-spam system is disabled the level of GXS, because we want to allow to send anonymous messages
//      between identities that might not have a reputation yet. Still, messages from identities with a bad reputation
//      are still deleted by GXS.
//
// The group limits are enforced according to the following rules:
//   * a temporal sliding window is computed for each identity and the number of messages signed by this identity is counted
//	 *
//
//
// Deleted messages are notified to the RsGxsNetService part which keeps a list of delete messages so as not to request them again
// during the same session. This allows to safely delete messages while avoiding re-synchronisation from friend nodes.

void p3GxsTrans::GxsTransIntegrityCleanupThread::run()
{
    // first take out all the groups

    std::map<RsGxsGroupId, RsNxsGrp*> grp;
    mDs->retrieveNxsGrps(grp, true, true);

#ifdef DEBUG_GXSTRANS
    std::cerr << "GxsTransIntegrityCleanupThread::run()" << std::endl;
#endif

    // compute hash and compare to stored value, if it fails then simply add it
    // to list

    GxsMsgReq grps;
    for(std::map<RsGxsGroupId, RsNxsGrp*>::iterator git = grp.begin(); git != grp.end(); ++git)
    {
	    RsNxsGrp* grp = git->second;

		// store the group for retrieveNxsMsgs
		grps[grp->grpId];

	    delete grp;
    }

    // now messages

	std::map<RsGxsId,MsgSizeCount> totalMessageSizeAndCount;

    std::map<RsGxsTransId,std::pair<RsGxsGroupId,RsGxsMessageId> > stored_msgs ;
    std::list<RsGxsTransId> received_msgs ;

    GxsMsgResult msgs;
    mDs->retrieveNxsMsgs(grps, msgs, false, true);

    for(GxsMsgResult::iterator mit = msgs.begin();mit != msgs.end(); ++mit)
    {
	    std::vector<RsNxsMsg*>& msgV = mit->second;
	    std::vector<RsNxsMsg*>::iterator vit = msgV.begin();
#ifdef DEBUG_GXSTRANS
		std::cerr << "Group " << mit->first << ": " << std::endl;
#endif

	    for(; vit != msgV.end(); ++vit)
	    {
		    RsNxsMsg* msg = *vit;

            RsGxsTransSerializer s ;
            uint32_t size = msg->msg.bin_len;
            RsItem *item = s.deserialise(msg->msg.bin_data,&size);
            RsGxsTransMailItem *mitem ;
            RsGxsTransPresignedReceipt *pitem ;

            if(item == NULL)
                std::cerr << "  Unrecocognised item type!" << std::endl;
            else if(NULL != (mitem = dynamic_cast<RsGxsTransMailItem*>(item)))
            {
#ifdef DEBUG_GXSTRANS
                std::cerr << "  " << msg->metaData->mMsgId << ": Mail data with ID " << std::hex << std::setfill('0') << std::setw(16) << mitem->mailId << std::dec << " from " << msg->metaData->mAuthorId << " size: " << msg->msg.bin_len << std::endl;
#endif

                stored_msgs[mitem->mailId] = std::make_pair(msg->metaData->mGroupId,msg->metaData->mMsgId) ;
            }
            else if(NULL != (pitem = dynamic_cast<RsGxsTransPresignedReceipt*>(item)))
            {
#ifdef DEBUG_GXSTRANS
                std::cerr << "  " << msg->metaData->mMsgId << ": Signed rcpt of ID " << std::hex << pitem->mailId << std::dec << " from " << msg->metaData->mAuthorId << " size: " << msg->msg.bin_len << std::endl;
#endif

                received_msgs.push_back(pitem->mailId) ;
            }
            else
                std::cerr << "  Unknown item type!" << std::endl;

			totalMessageSizeAndCount[msg->metaData->mAuthorId].size += msg->msg.bin_len ;
			totalMessageSizeAndCount[msg->metaData->mAuthorId].count++;
		    delete msg;

			delete item;
	    }
    }

	// From the collected information, build a list of group messages to delete.

    GxsMsgReq msgsToDel ;

#ifdef DEBUG_GXSTRANS
    std::cerr << "Msg removal report:" << std::endl;

	std::cerr << "  Per user size and count: " << std::endl;
	for(std::map<RsGxsId,MsgSizeCount>::const_iterator it(totalMessageSizeAndCount.begin());it!=totalMessageSizeAndCount.end();++it)
		std::cerr << "     " << it->first << ": " << it->second.count << " messages, for a total size of " << it->second.size << " bytes." << std::endl;
#endif

    for(std::list<RsGxsTransId>::const_iterator it(received_msgs.begin());it!=received_msgs.end();++it)
    {
		std::map<RsGxsTransId,std::pair<RsGxsGroupId,RsGxsMessageId> >::const_iterator it2 = stored_msgs.find(*it) ;

        if(stored_msgs.end() != it2)
        {
            msgsToDel[it2->second.first].insert(it2->second.second);

#ifdef DEBUG_GXSTRANS
            std::cerr << "  scheduling msg " << std::hex << it2->second.first << "," << it2->second.second << " for deletion." << std::endl;
#endif
        }
    }

	RS_STACK_MUTEX(mMtx) ;
	mMsgToDel = msgsToDel ;
	total_message_size_and_count = totalMessageSizeAndCount;
    mDone = true;
}

bool p3GxsTrans::GxsTransIntegrityCleanupThread::isDone()
{
    RS_STACK_MUTEX(mMtx) ;
    return mDone ;
}
void p3GxsTrans::service_tick()
{
	GxsTokenQueue::checkRequests();

    rstime_t now = time(NULL);
	bool changed = false ;

    if(mLastMsgCleanup + MAX_DELAY_BETWEEN_CLEANUPS < now)
    {
		RS_STACK_MUTEX(mPerUserStatsMutex);
        if(!mCleanupThread)
            mCleanupThread = new GxsTransIntegrityCleanupThread(getDataStore());

        if(mCleanupThread->isRunning())
            std::cerr << "Cleanup thread is already running. Not running it again!" << std::endl;
        else
        {
#ifdef DEBUG_GXSTRANS
            std::cerr << "Starting GxsIntegrity cleanup thread." << std::endl;
#endif

			mCleanupThread->start() ;
            mLastMsgCleanup = now ;
        }

		// This forces to review all groups, and decide to subscribe or not to each of them.

		requestGroupsData();
    }

	// now grab collected messages to delete

    if(mCleanupThread != NULL && mCleanupThread->isDone())
	{
		RS_STACK_MUTEX(mPerUserStatsMutex);
		GxsMsgReq msgToDel ;

		mCleanupThread->getMessagesToDelete(msgToDel) ;

		if(!msgToDel.empty())
		{
#ifdef DEBUG_GXSTRANS
			std::cerr << "p3GxsTrans::service_tick(): deleting messages." << std::endl;
#endif
            uint32_t token ;
            deleteMsgs(token,msgToDel);
		}

		mCleanupThread->getPerUserStatistics(per_user_statistics) ;

#ifdef DEBUG_GXSTRANS
		std::cerr << "p3GxsTrans: Got new set of per user statistics:"<< std::endl;
		for(std::map<RsGxsId,MsgSizeCount>::const_iterator it(per_user_statistics.begin());it!=per_user_statistics.end();++it)
			std::cerr << "  " << it->first << ": " << it->second.count << " " << it->second.size << std::endl;
#endif
        // Waiting here is very important because the thread may still be updating its semaphores after setting isDone() to true
        // If we delete it during this operation it will corrupt the stack and cause unpredictable errors.

        while(mCleanupThread->isRunning())
        {
            std::cerr << "Waiting for mCleanupThread to terminate..." << std::endl;
            rstime::rs_usleep(500*1000);
        }

		delete mCleanupThread;
		mCleanupThread=NULL ;
	}

	{
		RS_STACK_MUTEX(mOutgoingMutex);
		for ( auto it = mOutgoingQueue.begin(); it != mOutgoingQueue.end(); )
		{
			OutgoingRecord& pr(it->second);
			GxsTransSendStatus oldStatus = pr.status;

			locked_processOutgoingRecord(pr);

			if (oldStatus != pr.status) notifyClientService(pr);
			if( pr.status >= GxsTransSendStatus::RECEIPT_RECEIVED )
			{
				it = mOutgoingQueue.erase(it);
				changed = true ;
			}
			else ++it;
		}
	}


	{
		RS_STACK_MUTEX(mIngoingMutex);
		for( auto it = mIncomingQueue.begin(); it != mIncomingQueue.end(); )
		{
			switch(static_cast<GxsTransItemsSubtypes>( it->second->PacketSubType()))
			{
			case GxsTransItemsSubtypes::GXS_TRANS_SUBTYPE_MAIL:
			{
				RsGxsTransMailItem* msg = dynamic_cast<RsGxsTransMailItem*>(it->second);

				if(!msg)
				{
					std::cerr << "p3GxsTrans::service_tick() (EE) "
					          << "GXS_MAIL_SUBTYPE_MAIL dynamic_cast failed, "
					          << "something really wrong is happening!"
					          << std::endl;
				}
				else
				{
#ifdef DEBUG_GXSTRANS
					std::cout << "p3GxsTrans::service_tick() "
					          << "GXS_MAIL_SUBTYPE_MAIL handling: "
					          << msg->meta.mMsgId
					          << " with cryptoType: "
					          << static_cast<uint32_t>(msg->cryptoType)
					          << " recipientHint: " << msg->recipientHint
					          << " mailId: "<< msg->mailId
					          << " payload.size(): " << msg->payload.size()
					          << std::endl;
#endif
					handleEncryptedMail(msg);
				}
				break;
			}
			case GxsTransItemsSubtypes::GXS_TRANS_SUBTYPE_RECEIPT:
			{
				RsGxsTransPresignedReceipt* rcpt = dynamic_cast<RsGxsTransPresignedReceipt*>(it->second);

				if(!rcpt)
				{
					std::cerr << "p3GxsTrans::service_tick() (EE) "
					          << "GXS_MAIL_SUBTYPE_RECEIPT dynamic_cast failed,"
					          << " something really wrong is happening!"
					          << std::endl;
				}
				else if(mIdService.isOwnId(rcpt->meta.mAuthorId))
				{
					/* It is a receipt for a mail sent by this node live it in
					 * ingoingQueue so processOutgoingRecord(...) will take care
					 * of it at next tick */
					++it;
					continue;
				}
				else
				{
					/* It is a receipt for a message sent by someone else
					 * we can delete original mail from our GXS DB without
					 * waiting for GXS_STORAGE_PERIOD, this has been implemented
					 * already by Cyril into GxsTransIntegrityCleanupThread */
				}
				break;
			}
			default:
				std::cerr << "p3GxsTrans::service_tick() (EE) got something "
				          << "really unknown into ingoingQueue!!" << std::endl;
				break;
			}

			delete it->second ;
			it = mIncomingQueue.erase(it);
			changed = true ;
		}
	}

	if(changed)
		IndicateConfigChanged();
}

RsGenExchange::ServiceCreate_Return p3GxsTrans::service_CreateGroup(
        RsGxsGrpItem* /*grpItem*/, RsTlvSecurityKeySet& /*keySet*/ )
{
#ifdef DEBUG_GXSTRANS
	std::cout << "p3GxsTrans::service_CreateGroup(...) "
	          << grpItem->meta.mGroupId << std::endl;
#endif
	return SERVICE_CREATE_SUCCESS;
}

void p3GxsTrans::notifyChanges(std::vector<RsGxsNotify*>& changes)
{
#ifdef DEBUG_GXSTRANS
	std::cout << "p3GxsTrans::notifyChanges(...)" << std::endl;
#endif
	for( auto it = changes.begin(); it != changes.end(); ++it )
	{
		RsGxsGroupChange* grpChange = dynamic_cast<RsGxsGroupChange *>(*it);
		RsGxsMsgChange* msgChange = dynamic_cast<RsGxsMsgChange *>(*it);

		if (grpChange)
		{
#ifdef DEBUG_GXSTRANS
			std::cout << "p3GxsTrans::notifyChanges(...) grpChange" << std::endl;
#endif
			requestGroupsData(&(grpChange->mGrpIdList));
		}
		else if(msgChange)
		{
#ifdef DEBUG_GXSTRANS
			std::cout << "p3GxsTrans::notifyChanges(...) msgChange" << std::endl;
#endif
			uint32_t token;
			RsTokReqOptions opts; opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
			RsGenExchange::getTokenService()->requestMsgInfo( token, 0xcaca,
			                                   opts, msgChange->msgChangeMap );
			GxsTokenQueue::queueRequest(token, MAILS_UPDATE);

#ifdef DEBUG_GXSTRANS
			for( GxsMsgReq::const_iterator it = msgChange->msgChangeMap.begin();
			     it != msgChange->msgChangeMap.end(); ++it )
			{
				const RsGxsGroupId& grpId = it->first;
				const std::vector<RsGxsMessageId>& msgsIds = it->second;
				typedef std::vector<RsGxsMessageId>::const_iterator itT;
				for(itT vit = msgsIds.begin(); vit != msgsIds.end(); ++vit)
				{
					const RsGxsMessageId& msgId = *vit;
					std::cout << "p3GxsTrans::notifyChanges(...) got "
					          << "notification for message " << msgId
					          << " in group " << grpId << std::endl;
				}
			}
#endif
		}
        delete *it;
	}
}

uint32_t p3GxsTrans::AuthenPolicy()
{
	uint32_t policy = 0;
	uint32_t flag = 0;

	// This ensure propagated message have valid author signature
	flag = GXS_SERV::MSG_AUTHEN_ROOT_AUTHOR_SIGN |
	        GXS_SERV::MSG_AUTHEN_CHILD_AUTHOR_SIGN;
	RsGenExchange::setAuthenPolicyFlag( flag, policy,
	                                    RsGenExchange::PUBLIC_GRP_BITS );

	/* This ensure that in for restricted and private groups only authorized
	 * authors get the messages. Really not used ATM but don't hurts. */
	flag |= GXS_SERV::MSG_AUTHEN_ROOT_PUBLISH_SIGN |
	        GXS_SERV::MSG_AUTHEN_CHILD_PUBLISH_SIGN;
	RsGenExchange::setAuthenPolicyFlag( flag, policy,
	                                    RsGenExchange::RESTRICTED_GRP_BITS );
	RsGenExchange::setAuthenPolicyFlag( flag, policy,
	                                    RsGenExchange::PRIVATE_GRP_BITS );

	/* This seems never used RetroShare wide but we should investigate it
	 * more before considering this conclusive */
	flag = 0;
	RsGenExchange::setAuthenPolicyFlag( flag, policy,
	                                    RsGenExchange::GRP_OPTION_BITS );

	return policy;
}

bool p3GxsTrans::requestGroupsData(const std::list<RsGxsGroupId>* groupIds)
{
	//	std::cout << "p3GxsTrans::requestGroupsList()" << std::endl;
	uint32_t token=0;
	RsTokReqOptions opts; opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
	if(!groupIds)
		RsGenExchange::getTokenService()->requestGroupInfo(token, 0xcaca, opts);
	else
		RsGenExchange::getTokenService()->requestGroupInfo(token, 0xcaca, opts, *groupIds);

	GxsTokenQueue::queueRequest(token, GROUPS_LIST);
	return true;
}

bool p3GxsTrans::handleEncryptedMail(const RsGxsTransMailItem* mail)
{
#ifdef DEBUG_GXSTRANS
	std::cout << "p3GxsTrans::handleEcryptedMail(...)" << std::endl;
#endif

	std::set<RsGxsId> decryptIds;
	std::list<RsGxsId> ownIds;
	mIdService.getOwnIds(ownIds);
	for(auto it = ownIds.begin(); it != ownIds.end(); ++it)
		if(mail->maybeRecipient(*it)) decryptIds.insert(*it);

	// Hint match none of our own ids
	if(decryptIds.empty())
	{
#ifdef DEBUG_GXSTRANS
		std::cout << "p3GxsTrans::handleEcryptedMail(...) hint doesn't match" << std::endl;
#endif
		return true;
	}

	switch (mail->cryptoType)
	{
	case RsGxsTransEncryptionMode::CLEAR_TEXT:
	{
		uint16_t csri = 0;
		uint32_t off = 0;
		getRawUInt16(&mail->payload[0], mail->payload.size(), &off, &csri);

#ifdef DEBUG_GXSTRANS
		std::cerr << "service: " << csri << " got CLEAR_TEXT mail!"
		          << std::endl;
#endif
		/* As we cannot verify recipient without encryption, just pass the hint
		 * as recipient */
		return dispatchDecryptedMail( mail->meta.mAuthorId, mail->recipientHint,
		                              &mail->payload[0], mail->payload.size() );
	}
	case RsGxsTransEncryptionMode::RSA:
	{
		bool ok = true;
		for( std::set<RsGxsId>::const_iterator it = decryptIds.begin();
		     it != decryptIds.end(); ++it )
		{
			const RsGxsId& decryptId(*it);
			uint8_t* decrypted_data = NULL;
			uint32_t decrypted_data_size = 0;
			uint32_t decryption_error;
			if( mIdService.decryptData( &mail->payload[0],
			                            mail->payload.size(), decrypted_data,
			                            decrypted_data_size, decryptId,
			                            decryption_error ) )
				ok = ok && dispatchDecryptedMail( mail->meta.mAuthorId,
				                                  decryptId, decrypted_data,
				                                  decrypted_data_size );
			free(decrypted_data);
		}
		return ok;
	}
	default:
		std::cout << "Unknown encryption type:"
		          << static_cast<uint32_t>(mail->cryptoType) << std::endl;
		return false;
	}
}

bool p3GxsTrans::dispatchDecryptedMail( const RsGxsId& authorId,
                                        const RsGxsId& decryptId,
                                        const uint8_t* decrypted_data,
                                        uint32_t decrypted_data_size )
{
#ifdef DEBUG_GXSTRANS
	std::cout << "p3GxsTrans::dispatchDecryptedMail(, , " << decrypted_data_size
	          << ")" << std::endl;
#endif

	uint16_t csri = 0;
	uint32_t offset = 0;
	if(!getRawUInt16( decrypted_data, decrypted_data_size, &offset, &csri))
	{
		std::cerr << "p3GxsTrans::dispatchDecryptedMail(...) (EE) fatal error "
		          << "deserializing service type, something really wrong is "
		          << "happening!" << std::endl;
		return false;
	}
	GxsTransSubServices rsrvc = static_cast<GxsTransSubServices>(csri);

	uint32_t rcptsize = decrypted_data_size - offset;
	RsNxsTransPresignedReceipt* receipt =
	        static_cast<RsNxsTransPresignedReceipt*>(
	            RsNxsSerialiser(RS_SERVICE_TYPE_GXS_TRANS).deserialise(
	                const_cast<uint8_t*>(&decrypted_data[offset]), &rcptsize ));
	offset += rcptsize;
	if(!receipt)
	{
		std::cerr << "p3GxsTrans::dispatchDecryptedMail(...) (EE) fatal error "
		          << "deserializing presigned return receipt , something really"
		          << " wrong is happening!" << std::endl;
		return false;
	}
#ifdef DEBUG_GXSTRANS
	std::cout << "p3GxsTrans::dispatchDecryptedMail(...) dispatching receipt "
	          << "with: msgId: " << receipt->msgId << std::endl;
#endif

	std::vector<RsNxsMsg*> rcct; rcct.push_back(receipt);
	RsGenExchange::receiveNewMessages(rcct);

	GxsTransClient* recipientService = NULL;
	{
		RS_STACK_MUTEX(mServClientsMutex);
		recipientService = mServClients[rsrvc];
	}

	if(recipientService)
		return recipientService->receiveGxsTransMail(
		            authorId, decryptId, &decrypted_data[offset],
		            decrypted_data_size-offset );
	else
	{
		std::cerr << "p3GxsTrans::dispatchReceivedMail(...) "
		          << "got message for unknown service: "
		          << csri << std::endl;
		return false;
	}
}

void p3GxsTrans::locked_processOutgoingRecord(OutgoingRecord& pr)
{
	//std::cout << "p3GxsTrans::processRecord(...)" << std::endl;

	switch (pr.status)
	{
	case GxsTransSendStatus::PENDING_PROCESSING:
	{
		pr.mailItem.saltRecipientHint(pr.recipient);
		pr.mailItem.saltRecipientHint(RsGxsId::random());
		pr.sent_ts = time(NULL) ; //pr.mailItem.meta.mPublishTs = time(NULL);
	}
	/* fallthrough */
	case GxsTransSendStatus::PENDING_PREFERRED_GROUP:
	{
		RS_STACK_MUTEX(mDataMutex);

		if(mPreferredGroupId.isNull())
		{
			requestGroupsData();
			pr.status = GxsTransSendStatus::PENDING_PREFERRED_GROUP;
			break;
		}

		pr.group_id = mPreferredGroupId ; //pr.mailItem.meta.mGroupId = mPreferredGroupId;
	}
	/* fallthrough */
	case GxsTransSendStatus::PENDING_RECEIPT_CREATE:
	{
		RsGxsTransPresignedReceipt grcpt;
		grcpt.meta.mAuthorId = pr.author ;   //grcpt.meta = pr.mailItem.meta;
		grcpt.meta.mGroupId  = pr.group_id ; //grcpt.meta = pr.mailItem.meta;
		grcpt.meta.mMsgId.clear() ;
		grcpt.meta.mParentId.clear() ;
		grcpt.meta.mOrigMsgId.clear() ;
		grcpt.meta.mPublishTs = time(NULL);
		grcpt.mailId = pr.mailItem.mailId;
		uint32_t grsz = RsGxsTransSerializer().size(&grcpt);
		std::vector<uint8_t> grsrz;
		grsrz.resize(grsz);
		RsGxsTransSerializer().serialise(&grcpt, &grsrz[0], &grsz);

		{
			RS_STACK_MUTEX(mDataMutex);
			pr.presignedReceipt.grpId = mPreferredGroupId;
		}
		pr.presignedReceipt.metaData = new RsGxsMsgMetaData();
		*pr.presignedReceipt.metaData = grcpt.meta;
		pr.presignedReceipt.msg.setBinData(&grsrz[0], grsz);
	}
	/* fallthrough */
	case GxsTransSendStatus::PENDING_RECEIPT_SIGNATURE:					// (cyril) This step is never actually used.
	{
		switch (RsGenExchange::createMessage(&pr.presignedReceipt))
		{
		case CREATE_SUCCESS: break;
		case CREATE_FAIL_TRY_LATER:
			pr.status = GxsTransSendStatus::PENDING_RECEIPT_CREATE;
			return;
		default:
			pr.status = GxsTransSendStatus::FAILED_RECEIPT_SIGNATURE;
			goto processingFailed;
		}

		uint32_t metaSize = pr.presignedReceipt.metaData->serial_size();
		std::vector<uint8_t> srx; srx.resize(metaSize);
		pr.presignedReceipt.metaData->serialise(&srx[0], &metaSize);
		pr.presignedReceipt.meta.setBinData(&srx[0], metaSize);
	}
	/* fallthrough */
	case GxsTransSendStatus::PENDING_PAYLOAD_CREATE:
	{
		uint16_t serv = static_cast<uint16_t>(pr.clientService);
		uint32_t rcptsize = RsGxsTransSerializer().size(&pr.presignedReceipt);
		uint32_t datasize = pr.mailData.size();
		pr.mailItem.payload.resize(2 + rcptsize + datasize);
		uint32_t offset = 0;
		setRawUInt16(&pr.mailItem.payload[0], 2, &offset, serv);
		RsGxsTransSerializer().serialise(&pr.presignedReceipt,
		                                 &pr.mailItem.payload[offset],
		                                 &rcptsize);
		offset += rcptsize;
		memcpy(&pr.mailItem.payload[offset], &pr.mailData[0], datasize);
	}
	/* fallthrough */
	case GxsTransSendStatus::PENDING_PAYLOAD_ENCRYPT:
	{
		switch (pr.mailItem.cryptoType)
		{
		case RsGxsTransEncryptionMode::CLEAR_TEXT:
		{
			RsWarn() << __PRETTY_FUNCTION__ << " you are sending a mail "
			         << "without encryption, everyone can read it!"
			         << std::endl;
			break;
		}
		case RsGxsTransEncryptionMode::RSA:
		{
			uint8_t* encryptedData = NULL;
			uint32_t encryptedSize = 0;
			uint32_t encryptError = 0;
			if( mIdService.encryptData( &pr.mailItem.payload[0],
			                           pr.mailItem.payload.size(),
			                           encryptedData, encryptedSize,
			                           pr.recipient, encryptError, true ) )
			{
				pr.mailItem.payload.resize(encryptedSize);
				memcpy( &pr.mailItem.payload[0], encryptedData, encryptedSize );
				free(encryptedData);
				break;
			}
			else
			{
				RsErr() << __PRETTY_FUNCTION__ << " RSA encryption failed! "
				        << "error_status: " << encryptError << std::endl;
				pr.status = GxsTransSendStatus::FAILED_ENCRYPTION;
				goto processingFailed;
			}
		}
		case RsGxsTransEncryptionMode::UNDEFINED_ENCRYPTION:
		default:
			RsErr() << __PRETTY_FUNCTION__ << " attempt to send mail with "
			          << "wrong EncryptionMode: "
			          << static_cast<uint>(pr.mailItem.cryptoType)
			          << " dropping mail!" << std::endl;
			pr.status = GxsTransSendStatus::FAILED_ENCRYPTION;
			goto processingFailed;
		}
	}
	/* fallthrough */
	case GxsTransSendStatus::PENDING_PUBLISH:
	{
#ifdef DEBUG_GXSTRANS
		std::cout << "p3GxsTrans::sendEmail(...) sending mail to: "
		          << pr.recipient
		          << " with cryptoType: "
		          << static_cast<uint>(pr.mailItem.cryptoType)
		          << " recipientHint: " << pr.mailItem.recipientHint
		          << " receiptId: " << pr.mailItem.mailId
		          << " payload size: " << pr.mailItem.payload.size()
		          << std::endl;
#endif

		RsGxsTransMailItem *mail_item = new RsGxsTransMailItem(pr.mailItem);

		// pr.mailItem.meta is *not* serialised. So it is important to not rely on what's in it!

		mail_item->meta.mGroupId = pr.group_id ;
		mail_item->meta.mAuthorId = pr.author ;

		mail_item->meta.mMsgId.clear();
		mail_item->meta.mParentId.clear();
		mail_item->meta.mOrigMsgId.clear();

		uint32_t token;
		publishMsg(token, mail_item) ;

		pr.status = GxsTransSendStatus::PENDING_RECEIPT_RECEIVE;

		IndicateConfigChanged();	// This causes the saving of the message after pr.status has changed.
		break;
	}
	//case GxsTransSendStatus::PENDING_TRANSFER:
	case GxsTransSendStatus::PENDING_RECEIPT_RECEIVE:
	{
		RS_STACK_MUTEX(mIngoingMutex);
		auto range = mIncomingQueue.equal_range(pr.mailItem.mailId);
		bool changed = false;
		bool received = false;

		for( auto it = range.first; it != range.second; ++it)
		{
			RsGxsTransPresignedReceipt* rt = dynamic_cast<RsGxsTransPresignedReceipt*>(it->second);

			if(rt && mIdService.isOwnId(rt->meta.mAuthorId))
			{
				mIncomingQueue.erase(it); delete rt;
				pr.status = GxsTransSendStatus::RECEIPT_RECEIVED;

				changed = true;
				received = true;
				break;
			}
		}

		if(!received && time(nullptr) - pr.sent_ts > GXS_STORAGE_PERIOD)
		{
			changed = true;
			pr.status = GxsTransSendStatus::FAILED_TIMED_OUT;
		}

		if(changed)
			IndicateConfigChanged();

		break;
	}
	case GxsTransSendStatus::RECEIPT_RECEIVED:
		break;

processingFailed:
	case GxsTransSendStatus::FAILED_RECEIPT_SIGNATURE:
	case GxsTransSendStatus::FAILED_ENCRYPTION:
	case GxsTransSendStatus::FAILED_TIMED_OUT:
	default:
		RsErr() << __PRETTY_FUNCTION__ << " processing:" << pr.mailItem.mailId
		          << " failed with: " << static_cast<uint>(pr.status)
		          << std::endl;
		break;
	}
}

void p3GxsTrans::notifyClientService(const OutgoingRecord& pr)
{
	RS_STACK_MUTEX(mServClientsMutex);
	auto it = mServClients.find(pr.clientService);
	if( it != mServClients.end())
	{
		GxsTransClient* serv(it->second);
		if(serv)
		{
			serv->notifyGxsTransSendStatus(pr.mailItem.mailId, pr.status);
			return;
		}
	}

	std::cerr << "p3GxsTrans::processRecord(...) (EE) processed"
	          << " mail for unkown service: "
	          << static_cast<uint32_t>(pr.clientService)
	          << " fatally failed with: "
	          << static_cast<uint32_t>(pr.status) << std::endl;
	print_stacktrace();
}

RsSerialiser* p3GxsTrans::setupSerialiser()
{
	RsSerialiser* rss = new RsSerialiser;
	rss->addSerialType(new RsGxsTransSerializer);
	return rss;
}

bool p3GxsTrans::saveList(bool &cleanup, std::list<RsItem *>& saveList)
{
#ifdef DEBUG_GXSTRANS
	std::cout << "p3GxsTrans::saveList(...)" << saveList.size() << " " << mIncomingQueue.size() << " " << mOutgoingQueue.size() << std::endl;
#endif

	mOutgoingMutex.lock();
	mIngoingMutex.lock();

	for ( auto& kv : mOutgoingQueue )
	{
#ifdef DEBUG_GXSTRANS
		std::cerr << "Saving outgoing item, ID " << std::hex << std::setfill('0') << std::setw(16) << kv.first << std::dec << "Group id: " << kv.second.group_id << ", TS=" << kv.second.sent_ts << std::endl;
#endif
		saveList.push_back(&kv.second);
	}
	for ( auto& kv : mIncomingQueue )
	{
#ifdef DEBUG_GXSTRANS
		std::cerr << "Saving incoming item, ID " << std::hex << std::setfill('0') << std::setw(16) << kv.first << std::endl;
#endif
		saveList.push_back(kv.second);
	}

#ifdef DEBUG_GXSTRANS
	std::cout << "p3GxsTrans::saveList(...)" << saveList.size() << " " << mIncomingQueue.size() << " " << mOutgoingQueue.size() << std::endl;
#endif

	cleanup = false;
	return true;
}

void p3GxsTrans::saveDone()
{
	mOutgoingMutex.unlock();
	mIngoingMutex.unlock();
}

bool p3GxsTrans::loadList(std::list<RsItem *>&loadList)
{
#ifdef DEBUG_GXSTRANS
	std::cout << "p3GxsTrans::loadList(...) " << loadList.size() << " "
	          << mIncomingQueue.size() << " " << mOutgoingQueue.size()
	          << std::endl;
#endif

	for(auto& v : loadList)
		switch(static_cast<GxsTransItemsSubtypes>(v->PacketSubType()))
		{
		case GxsTransItemsSubtypes::GXS_TRANS_SUBTYPE_MAIL:
		case GxsTransItemsSubtypes::GXS_TRANS_SUBTYPE_RECEIPT:
		{
			RsGxsTransBaseMsgItem* mi = dynamic_cast<RsGxsTransBaseMsgItem*>(v);
			if(mi)
			{
				RS_STACK_MUTEX(mIngoingMutex);
				mIncomingQueue.insert(inMap::value_type(mi->mailId, mi));
			}
			break;
		}
		case GxsTransItemsSubtypes::OUTGOING_RECORD_ITEM_deprecated:
		{
			OutgoingRecord_deprecated* dot = dynamic_cast<OutgoingRecord_deprecated*>(v);

			if(dot)
			{
				std::cerr << "(EE) Read a deprecated GxsTrans outgoing item. Converting to new format..." << std::endl;

				OutgoingRecord ot(dot->recipient,dot->clientService,&dot->mailData[0],dot->mailData.size()) ;

				ot.status = dot->status ;

				ot.author.clear();		// These 3 fields cannot be stored in mailItem.meta, which is not serialised, so they are lost.
				ot.group_id.clear() ;
				ot.sent_ts = 0;

				ot.mailItem = dot->mailItem ;
				ot.presignedReceipt = dot->presignedReceipt;

				RS_STACK_MUTEX(mOutgoingMutex);
				mOutgoingQueue.insert(prMap::value_type(ot.mailItem.mailId, ot));

#ifdef DEBUG_GXSTRANS
				std::cerr << "Loaded outgoing item (converted), ID " << std::hex << std::setfill('0') << std::setw(16) << ot.mailItem.mailId<< std::dec << ", Group id: " << ot.group_id << ", TS=" << ot.sent_ts << std::endl;
#endif
			}
			delete v;
			break;
		}

		case GxsTransItemsSubtypes::OUTGOING_RECORD_ITEM:
		{
			OutgoingRecord* ot = dynamic_cast<OutgoingRecord*>(v);
			if(ot)
			{
				RS_STACK_MUTEX(mOutgoingMutex);
				mOutgoingQueue.insert(
				            prMap::value_type(ot->mailItem.mailId, *ot));

#ifdef DEBUG_GXSTRANS
				std::cerr << "Loading outgoing item, ID " << std::hex << std::setfill('0') << std::setw(16) << ot->mailItem.mailId<< std::dec << "Group id: " << ot->group_id << ", TS=" << ot->sent_ts << std::endl;
#endif
			}
			delete v;
			break;
		}
		case GxsTransItemsSubtypes::GXS_TRANS_SUBTYPE_GROUP:
		default:
			std::cerr << "p3GxsTrans::loadList(...) (EE) got item with "
			          << "unhandled type: "
			          << static_cast<uint>(v->PacketSubType())
			          << std::endl;
			delete v;
			break;
		}

#ifdef DEBUG_GXSTRANS
	std::cout << "p3GxsTrans::loadList(...) " << loadList.size() << " "
	          << mIncomingQueue.size() << " " << mOutgoingQueue.size()
	          << std::endl;
#endif

	return true;
}

bool p3GxsTrans::acceptNewMessage(const RsGxsMsgMetaData *msgMeta,uint32_t msg_size)
{
	// 1 - check the total size of the msgs for the author of this particular msg.

	// 2 - Reject depending on embedded limits.

	// Depending on reputation, the messages will be rejected:
	//
	//     Reputation  |   Maximum msg count  |  Maximum msg size
	//	   ------------+----------------------+------------------
	//     Negative    |          0           |         0          // This is already handled by the anti-spam
	//     R-Negative  |         10           |        10k
	//     Neutral     |        100           |        20k
	//     R-Positive  |        400           |         1M
	//     Positive    |       1000           |         2M

	// Ideally these values should be left as user-defined parameters, with the
	// default values below used as backup.

	static const uint32_t GXSTRANS_MAX_COUNT_REMOTELY_NEGATIVE_DEFAULT =   10 ;
	static const uint32_t GXSTRANS_MAX_COUNT_NEUTRAL_DEFAULT           =   40 ;
	static const uint32_t GXSTRANS_MAX_COUNT_REMOTELY_POSITIVE_DEFAULT =  400 ;
	static const uint32_t GXSTRANS_MAX_COUNT_LOCALLY_POSITIVE_DEFAULT  = 1000 ;

	static const uint32_t GXSTRANS_MAX_SIZE_REMOTELY_NEGATIVE_DEFAULT =       10 * 1024 ;
	static const uint32_t GXSTRANS_MAX_SIZE_NEUTRAL_DEFAULT           =      200 * 1024 ;
	static const uint32_t GXSTRANS_MAX_SIZE_REMOTELY_POSITIVE_DEFAULT =     1024 * 1024 ;
	static const uint32_t GXSTRANS_MAX_SIZE_LOCALLY_POSITIVE_DEFAULT  = 2 * 1024 * 1024 ;

	uint32_t max_count = 0 ;
	uint32_t max_size  = 0 ;
	uint32_t identity_flags = 0 ;

	RsReputationLevel rep_lev =
	        rsReputations->overallReputationLevel(
	            msgMeta->mAuthorId, &identity_flags );

	switch(rep_lev)
	{
	case RsReputationLevel::REMOTELY_NEGATIVE:
		max_count = GXSTRANS_MAX_COUNT_REMOTELY_NEGATIVE_DEFAULT;
		max_size  = GXSTRANS_MAX_SIZE_REMOTELY_NEGATIVE_DEFAULT;
		break ;
	case RsReputationLevel::NEUTRAL:
		max_count = GXSTRANS_MAX_COUNT_NEUTRAL_DEFAULT;
		max_size  = GXSTRANS_MAX_SIZE_NEUTRAL_DEFAULT;
		break;
	case RsReputationLevel::REMOTELY_POSITIVE:
		max_count = GXSTRANS_MAX_COUNT_REMOTELY_POSITIVE_DEFAULT;
		max_size  = GXSTRANS_MAX_SIZE_REMOTELY_POSITIVE_DEFAULT;
		break;
	case RsReputationLevel::LOCALLY_POSITIVE:
		max_count = GXSTRANS_MAX_COUNT_LOCALLY_POSITIVE_DEFAULT;
		max_size  = GXSTRANS_MAX_SIZE_LOCALLY_POSITIVE_DEFAULT;
		break;
	case RsReputationLevel::LOCALLY_NEGATIVE: // fallthrough
	default:
		max_count = 0;
		max_size = 0;
		break;
	}

	bool pgp_linked = identity_flags & RS_IDENTITY_FLAGS_PGP_LINKED;

	if(rep_lev <= RsReputationLevel::NEUTRAL && !pgp_linked)
	{
		max_count /= 10 ;
		max_size  /= 10 ;
	}

	RS_STACK_MUTEX(mPerUserStatsMutex);

	MsgSizeCount& s(per_user_statistics[msgMeta->mAuthorId]);

#ifdef DEBUG_GXSTRANS
	std::cerr << "GxsTrans::acceptMessage(): size=" << msg_size << ", grp=" << msgMeta->mGroupId << ", gxs_id=" << msgMeta->mAuthorId << ", pgp_linked=" << pgp_linked << ", current (size,cnt)=("
	          << s.size << "," << s.count << ") reputation=" << rep_lev << ", limits=(" << max_size << "," << max_count << ") " ;
#endif

	if(s.size + msg_size > max_size || 1+s.count > max_count)
	{
#ifdef DEBUG_GXSTRANS
		std::cerr << "=> rejected." << std::endl;
#endif
		return false ;
	}
	else
	{
#ifdef DEBUG_GXSTRANS
		std::cerr << "=> accepted." << std::endl;
#endif

		s.count++ ;
		s.size += msg_size ;	// update the statistics, so that it's not possible to pass a bunch of msgs at once below the limits.

		return true ;
	}
}



