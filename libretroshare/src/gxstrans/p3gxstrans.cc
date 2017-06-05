/*
 * GXS Mailing Service
 * Copyright (C) 2016-2017  Gioacchino Mazzurco <gio@eigenlab.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "util/rsdir.h"
#include "gxstrans/p3gxstrans.h"
#include "util/stacktrace.h"

typedef unsigned int uint;

RsGxsTrans *rsGxsTrans = NULL ;

p3GxsTrans::~p3GxsTrans()
{
	p3Config::saveConfiguration();

	{
		RS_STACK_MUTEX(mIngoingMutex);
		for ( auto& kv : mIngoingQueue ) delete kv.second;
	}
}

bool p3GxsTrans::getStatistics(GxsTransStatistics& stats)
{
    stats.prefered_group_id = mPreferredGroupId;
    stats.outgoing_records.clear();

    {
		RS_STACK_MUTEX(mOutgoingMutex);

		for ( auto it = mOutgoingQueue.begin(); it != mOutgoingQueue.end(); ++it)
		{
			const OutgoingRecord& pr(it->second);

			RsGxsTransOutgoingRecord rec ;
            rec.status = pr.status ;
            rec.send_TS = pr.mailItem.meta.mPublishTs ;
            rec.group_id = pr.mailItem.meta.mGroupId ;
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
	std::cout << "p3GxsTrans::sendEmail(...)" << std::endl;

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
	pr.mailItem.meta.mAuthorId = own_gxsid;
	pr.mailItem.meta.mMsgId.clear();
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
	std::cout << "p3GxsTrans::handleResponse(" << token << ", " << req_type
	          << ")" << std::endl;
	switch (req_type)
	{
	case GROUPS_LIST:
	{
		std::vector<RsGxsGrpItem*> groups;
		getGroupData(token, groups);

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
			bool old = olderThen( meta.mLastPost,
			                      UNUSED_GROUP_UNSUBSCRIBE_INTERVAL );
			bool supersede = supersedePreferredGroup(meta.mGroupId);
			uint32_t token;

			bool shoudlSubscribe = !subscribed && ( !old || supersede );
			bool shoudlUnSubscribe = subscribed && old
			        && meta.mGroupId != mPreferredGroupId;

			if(shoudlSubscribe)
				RsGenExchange::subscribeToGroup(token, meta.mGroupId, true);
			else if(shoudlUnSubscribe)
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

		if(mPreferredGroupId.isNull())
		{
			/* This is true only at first run when we haven't received mail
			 * distribuition groups from friends
			 * TODO: We should check if we have some connected friend too, to
			 * avoid to create yet another never used mail distribution group.
			 */

			std::cerr << "p3GxsTrans::handleResponse(...) preferredGroupId.isNu"
			          << "ll() let's create a new group." << std::endl;
			uint32_t token;
			publishGroup(token, new RsGxsTransGroupItem());
			queueRequest(token, GROUP_CREATE);
		}

		break;
	}
	case GROUP_CREATE:
	{
		std::cerr << "p3GxsTrans::handleResponse(...) GROUP_CREATE" << std::endl;
		RsGxsGroupId grpId;
		acknowledgeTokenGrp(token, grpId);
		supersedePreferredGroup(grpId);
		break;
	}
	case MAILS_UPDATE:
	{
		std::cout << "p3GxsTrans::handleResponse(...) MAILS_UPDATE" << std::endl;
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
					RsGxsTransBaseItem* mb =
					        dynamic_cast<RsGxsTransBaseItem*>(*mIt);
					if(mb)
					{
						RS_STACK_MUTEX(mIngoingMutex);
						mIngoingQueue.insert(inMap::value_type(mb->mailId, mb));
						IndicateConfigChanged();
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
}

void p3GxsTrans::GxsTransIntegrityCleanupThread::getMessagesToDelete(GxsMsgReq& m)
{
	RS_STACK_MUTEX(mMtx) ;

	m = mMsgToDel ;
	mMsgToDel.clear();
}

void p3GxsTrans::GxsTransIntegrityCleanupThread::run()
{
    // first take out all the groups
    std::map<RsGxsGroupId, RsNxsGrp*> grp;
    mDs->retrieveNxsGrps(grp, true, true);

    std::cerr << "GxsTransIntegrityCleanupThread::run()" << std::endl;

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

    std::map<RsGxsTransId,std::pair<RsGxsGroupId,RsGxsMessageId> > stored_msgs ;
    std::list<RsGxsTransId> received_msgs ;

    GxsMsgResult msgs;
    mDs->retrieveNxsMsgs(grps, msgs, false, true);

    for(GxsMsgResult::iterator mit = msgs.begin();mit != msgs.end(); ++mit)
    {
	    std::vector<RsNxsMsg*>& msgV = mit->second;
	    std::vector<RsNxsMsg*>::iterator vit = msgV.begin();

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
                std::cerr << "  " << msg->metaData->mMsgId << ": Mail data with ID " << std::hex << std::setfill('0') << std::setw(16) << mitem->mailId << std::dec << " from " << msg->metaData->mAuthorId << " size: " << msg->msg.bin_len << std::endl;

                stored_msgs[mitem->mailId] = std::make_pair(msg->metaData->mGroupId,msg->metaData->mMsgId) ;
            }
            else if(NULL != (pitem = dynamic_cast<RsGxsTransPresignedReceipt*>(item)))
            {
                std::cerr << "  " << msg->metaData->mMsgId << ": Signed rcpt of ID " << std::hex << pitem->mailId << std::dec << " from " << msg->metaData->mAuthorId << " size: " << msg->msg.bin_len << std::endl;

                received_msgs.push_back(pitem->mailId) ;
            }
            else
                std::cerr << "  Unknown item type!" << std::endl;

		    delete msg;
	    }
    }

    GxsMsgReq msgsToDel ;

    std::cerr << "Msg removal report:" << std::endl;

    for(std::list<RsGxsTransId>::const_iterator it(received_msgs.begin());it!=received_msgs.end();++it)
    {
		std::map<RsGxsTransId,std::pair<RsGxsGroupId,RsGxsMessageId> >::const_iterator it2 = stored_msgs.find(*it) ;

        if(stored_msgs.end() != it2)
        {
            msgsToDel[it2->second.first].push_back(it2->second.second);

            std::cerr << "  scheduling msg " << std::hex << it2->second.first << "," << it2->second.second << " for deletion." << std::endl;
        }
    }

	RS_STACK_MUTEX(mMtx) ;
	mMsgToDel = msgsToDel ;
}

void p3GxsTrans::service_tick()
{
	GxsTokenQueue::checkRequests();

    time_t now = time(NULL);

    if(mLastMsgCleanup + MAX_DELAY_BETWEEN_CLEANUPS < now)
    {
        if(!mCleanupThread)
            mCleanupThread = new GxsTransIntegrityCleanupThread(getDataStore());

        if(mCleanupThread->isRunning())
            std::cerr << "Cleanup thread is already running. Not running it again!" << std::endl;
        else
        {
            std::cerr << "Starting GxsIntegrity cleanup thread." << std::endl;

			mCleanupThread->start() ;
            mLastMsgCleanup = now ;
        }
    }

	// now grab collected messages to delete

	if(mCleanupThread != NULL && !mCleanupThread->isRunning())
	{
		GxsMsgReq msgToDel ;

		mCleanupThread->getMessagesToDelete(msgToDel) ;

		if(!msgToDel.empty())
		{
			std::cerr << "p3GxsTrans::service_tick(): deleting messages." << std::endl;
			getDataStore()->removeMsgs(msgToDel);
		}
	}

	{
		RS_STACK_MUTEX(mOutgoingMutex);
		for ( auto it = mOutgoingQueue.begin(); it != mOutgoingQueue.end(); )
		{
			OutgoingRecord& pr(it->second);
			GxsTransSendStatus oldStatus = pr.status;

			locked_processOutgoingRecord(pr);
			bool changed = false ;

			if (oldStatus != pr.status) notifyClientService(pr);
			if( pr.status >= GxsTransSendStatus::RECEIPT_RECEIVED )
			{
				it = mOutgoingQueue.erase(it);
				changed = true ;
			}
			else ++it;

			if(changed)
				IndicateConfigChanged();
		}
	}


	{
		RS_STACK_MUTEX(mIngoingMutex);
		for( auto it = mIngoingQueue.begin(); it != mIngoingQueue.end(); )
		{
			switch(static_cast<GxsTransItemsSubtypes>(
			           it->second->PacketSubType()))
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
					std::cout << "p3GxsTrans::service_tick() "
					          << "GXS_MAIL_SUBTYPE_MAIL handling: "
					          << msg->meta.mMsgId
					          << " with cryptoType: "
					          << static_cast<uint32_t>(msg->cryptoType)
					          << " recipientHint: " << msg->recipientHint
					          << " mailId: "<< msg->mailId
					          << " payload.size(): " << msg->payload.size()
					          << std::endl;
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
					/* TODO: It is a receipt for a message sent by someone else
					 * we can delete original mail from our GXS DB without
					 * waiting for GXS_STORAGE_PERIOD */
				}
				break;
			}
			default:
				std::cerr << "p3GxsTrans::service_tick() (EE) got something "
				          << "really unknown into ingoingQueue!!" << std::endl;
				break;
			}

			delete it->second; it = mIngoingQueue.erase(it);

			IndicateConfigChanged();
		}
	}
}

RsGenExchange::ServiceCreate_Return p3GxsTrans::service_CreateGroup(
        RsGxsGrpItem* grpItem, RsTlvSecurityKeySet& /*keySet*/ )
{
	std::cout << "p3GxsTrans::service_CreateGroup(...) "
	          << grpItem->meta.mGroupId << std::endl;
	return SERVICE_CREATE_SUCCESS;
}

void p3GxsTrans::notifyChanges(std::vector<RsGxsNotify*>& changes)
{
	std::cout << "p3GxsTrans::notifyChanges(...)" << std::endl;
	for( std::vector<RsGxsNotify*>::const_iterator it = changes.begin();
	     it != changes.end(); ++it )
	{
		RsGxsGroupChange* grpChange = dynamic_cast<RsGxsGroupChange *>(*it);
		RsGxsMsgChange* msgChange = dynamic_cast<RsGxsMsgChange *>(*it);

		if (grpChange)
		{
			std::cout << "p3GxsTrans::notifyChanges(...) grpChange" << std::endl;
			requestGroupsData(&(grpChange->mGrpIdList));
		}
		else if(msgChange)
		{
			std::cout << "p3GxsTrans::notifyChanges(...) msgChange" << std::endl;
			uint32_t token;
			RsTokReqOptions opts; opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
			RsGenExchange::getTokenService()->requestMsgInfo( token, 0xcaca,
			                                   opts, msgChange->msgChangeMap );
			GxsTokenQueue::queueRequest(token, MAILS_UPDATE);

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
		}
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
	uint32_t token;
	RsTokReqOptions opts; opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
	if(!groupIds) RsGenExchange::getTokenService()->requestGroupInfo(token, 0xcaca, opts);
	else RsGenExchange::getTokenService()->requestGroupInfo(token, 0xcaca, opts, *groupIds);
	GxsTokenQueue::queueRequest(token, GROUPS_LIST);
	return true;
}

bool p3GxsTrans::handleEncryptedMail(const RsGxsTransMailItem* mail)
{
	std::cout << "p3GxsTrans::handleEcryptedMail(...)" << std::endl;

	std::set<RsGxsId> decryptIds;
	std::list<RsGxsId> ownIds;
	mIdService.getOwnIds(ownIds);
	for(auto it = ownIds.begin(); it != ownIds.end(); ++it)
		if(mail->maybeRecipient(*it)) decryptIds.insert(*it);

	// Hint match none of our own ids
	if(decryptIds.empty())
	{
		std::cout << "p3GxsTrans::handleEcryptedMail(...) hint doesn't match"
		          << std::endl;
		return true;
	}

	switch (mail->cryptoType)
	{
	case RsGxsTransEncryptionMode::CLEAR_TEXT:
	{
		uint16_t csri = 0;
		uint32_t off = 0;
		getRawUInt16(&mail->payload[0], mail->payload.size(), &off, &csri);
		std::cerr << "service: " << csri << " got CLEAR_TEXT mail!"
		          << std::endl;
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
	std::cout << "p3GxsTrans::dispatchDecryptedMail(, , " << decrypted_data_size
	          << ")" << std::endl;

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
	std::cout << "p3GxsTrans::dispatchDecryptedMail(...) dispatching receipt "
	          << "with: msgId: " << receipt->msgId << std::endl;

	std::vector<RsNxsMsg*> rcct; rcct.push_back(receipt);
	RsGenExchange::notifyNewMessages(rcct);

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
		pr.mailItem.meta.mPublishTs = time(NULL);
	}
	case GxsTransSendStatus::PENDING_PREFERRED_GROUP:
	{
		if(mPreferredGroupId.isNull())
		{
			requestGroupsData();
			pr.status = GxsTransSendStatus::PENDING_PREFERRED_GROUP;
			break;
		}

		pr.mailItem.meta.mGroupId = mPreferredGroupId;
	}
	case GxsTransSendStatus::PENDING_RECEIPT_CREATE:
	{
		RsGxsTransPresignedReceipt grcpt;
		grcpt.meta = pr.mailItem.meta;
		grcpt.meta.mPublishTs = time(NULL);
		grcpt.mailId = pr.mailItem.mailId;
		uint32_t grsz = RsGxsTransSerializer().size(&grcpt);
		std::vector<uint8_t> grsrz;
		grsrz.resize(grsz);
		RsGxsTransSerializer().serialise(&grcpt, &grsrz[0], &grsz);

		pr.presignedReceipt.grpId = mPreferredGroupId;
		pr.presignedReceipt.metaData = new RsGxsMsgMetaData();
		*pr.presignedReceipt.metaData = grcpt.meta;
		pr.presignedReceipt.msg.setBinData(&grsrz[0], grsz);
	}
	case GxsTransSendStatus::PENDING_RECEIPT_SIGNATURE:
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
	case GxsTransSendStatus::PENDING_PAYLOAD_ENCRYPT:
	{
		switch (pr.mailItem.cryptoType)
		{
		case RsGxsTransEncryptionMode::CLEAR_TEXT:
		{
			std::cerr << "p3GxsTrans::sendMail(...) you are sending a mail "
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
				std::cerr << "p3GxsTrans::sendMail(...) RSA encryption failed! "
				          << "error_status: " << encryptError << std::endl;
				pr.status = GxsTransSendStatus::FAILED_ENCRYPTION;
				goto processingFailed;
			}
		}
		case RsGxsTransEncryptionMode::UNDEFINED_ENCRYPTION:
		default:
			std::cerr << "p3GxsTrans::sendMail(...) attempt to send mail with "
			          << "wrong EncryptionMode: "
			          << static_cast<uint>(pr.mailItem.cryptoType)
			          << " dropping mail!" << std::endl;
			pr.status = GxsTransSendStatus::FAILED_ENCRYPTION;
			goto processingFailed;
		}
	}
	case GxsTransSendStatus::PENDING_PUBLISH:
	{
		std::cout << "p3GxsTrans::sendEmail(...) sending mail to: "
		          << pr.recipient
		          << " with cryptoType: "
		          << static_cast<uint>(pr.mailItem.cryptoType)
		          << " recipientHint: " << pr.mailItem.recipientHint
		          << " receiptId: " << pr.mailItem.mailId
		          << " payload size: " << pr.mailItem.payload.size()
		          << std::endl;

		uint32_t token;
		publishMsg(token, new RsGxsTransMailItem(pr.mailItem));
		pr.status = GxsTransSendStatus::PENDING_RECEIPT_RECEIVE;

		IndicateConfigChanged();	// This causes the saving of the message after all data has been filled in.
		break;
	}
	//case GxsTransSendStatus::PENDING_TRANSFER:
	case GxsTransSendStatus::PENDING_RECEIPT_RECEIVE:
	{
		RS_STACK_MUTEX(mIngoingMutex);
		auto range = mIngoingQueue.equal_range(pr.mailItem.mailId);
		bool changed = false ;

		for( auto it = range.first; it != range.second; ++it)
		{
			RsGxsTransPresignedReceipt* rt = dynamic_cast<RsGxsTransPresignedReceipt*>(it->second);

			if(rt && mIdService.isOwnId(rt->meta.mAuthorId))
			{
				mIngoingQueue.erase(it); delete rt;
				pr.status = GxsTransSendStatus::RECEIPT_RECEIVED;

				changed = true ;
				break;
			}
		}
		if(changed)
			IndicateConfigChanged();

		// TODO: Resend message if older then treshold
		break;
	}
	case GxsTransSendStatus::RECEIPT_RECEIVED:
		break;

processingFailed:
	case GxsTransSendStatus::FAILED_RECEIPT_SIGNATURE:
	case GxsTransSendStatus::FAILED_ENCRYPTION:
	default:
	{
		std::cout << "p3GxsTrans::processRecord(" << pr.mailItem.mailId
		          << ") failed with: " << static_cast<uint>(pr.status)
		          << std::endl;
		break;
	}
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
	std::cout << "p3GxsTrans::saveList(...)" << saveList.size() << " " << mIngoingQueue.size() << " " << mOutgoingQueue.size() << std::endl;

	mOutgoingMutex.lock();
	mIngoingMutex.lock();

	for ( auto& kv : mOutgoingQueue )
	{
		std::cerr << "Saving outgoing item, ID " << std::hex << std::setfill('0') << std::setw(16) << kv.first << std::dec << "Group id: " << kv.second.mailItem.meta.mGroupId << ", TS=" << kv.second.mailItem.meta.mPublishTs << std::endl;
		saveList.push_back(&kv.second);
	}
	for ( auto& kv : mIngoingQueue )
	{
		std::cerr << "Saving incoming item, ID " << std::hex << std::setfill('0') << std::setw(16) << kv.first << std::endl;
		saveList.push_back(kv.second);
	}

	std::cout << "p3GxsTrans::saveList(...)" << saveList.size() << " " << mIngoingQueue.size() << " " << mOutgoingQueue.size() << std::endl;

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
	std::cout << "p3GxsTrans::loadList(...) " << loadList.size() << " "
	          << mIngoingQueue.size() << " " << mOutgoingQueue.size()
	          << std::endl;

	for(auto& v : loadList)
		switch(static_cast<GxsTransItemsSubtypes>(v->PacketSubType()))
		{
		case GxsTransItemsSubtypes::GXS_TRANS_SUBTYPE_MAIL:
		case GxsTransItemsSubtypes::GXS_TRANS_SUBTYPE_RECEIPT:
		{
			RsGxsTransBaseItem* mi = dynamic_cast<RsGxsTransBaseItem*>(v);
			if(mi)
			{
				RS_STACK_MUTEX(mIngoingMutex);
				mIngoingQueue.insert(inMap::value_type(mi->mailId, mi));
			}
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

				std::cerr << "Loading outgoing item, ID " << std::hex << std::setfill('0') << std::setw(16) << ot->mailItem.mailId<< std::dec << "Group id: " << ot->mailItem.meta.mGroupId << ", TS=" << ot->mailItem.meta.mPublishTs << std::endl;
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

	std::cout << "p3GxsTrans::loadList(...) " << loadList.size() << " "
	          << mIngoingQueue.size() << " " << mOutgoingQueue.size()
	          << std::endl;

	return true;
}
