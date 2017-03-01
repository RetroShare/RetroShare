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

#include "p3gxsmails.h"
#include "util/stacktrace.h"

typedef unsigned int uint;

p3GxsMails::~p3GxsMails()
{
	p3Config::saveConfiguration();

	{
		RS_STACK_MUTEX(ingoingMutex);
		for ( auto& kv : ingoingQueue ) delete kv.second;
	}
}

bool p3GxsMails::sendMail( RsGxsMailId& mailId,
                           GxsMailSubServices service,
                           const RsGxsId& own_gxsid, const RsGxsId& recipient,
                           const uint8_t* data, uint32_t size,
                           RsGxsMailEncryptionMode cm )
{
	std::cout << "p3GxsMails::sendEmail(...)" << std::endl;

	if(!idService.isOwnId(own_gxsid))
	{
		std::cerr << "p3GxsMails::sendEmail(...) isOwnId(own_gxsid) false!"
		          << std::endl;
		return false;
	}

	if(recipient.isNull())
	{
		std::cerr << "p3GxsMails::sendEmail(...) got invalid recipient"
		          << std::endl;
		print_stacktrace();
		return false;
	}

	OutgoingRecord pr( recipient, service, data, size );
	pr.mailItem.meta.mAuthorId = own_gxsid;
	pr.mailItem.cryptoType = cm;
	pr.mailItem.mailId = RSRandom::random_u64();

	{
		RS_STACK_MUTEX(outgoingMutex);
		outgoingQueue.insert(prMap::value_type(pr.mailItem.mailId, pr));
	}

	mailId = pr.mailItem.mailId;
	return true;
}

bool p3GxsMails::querySendMailStatus(RsGxsMailId mailId, GxsMailStatus& st)
{
	auto it = outgoingQueue.find(mailId);
	if( it != outgoingQueue.end() )
	{
		st = it->second.status;
		return true;
	}
	return false;
}

void p3GxsMails::registerGxsMailsClient(
        GxsMailSubServices serviceType, GxsMailsClient* service)
{
	RS_STACK_MUTEX(servClientsMutex);
	servClients[serviceType] = service;
}

void p3GxsMails::handleResponse(uint32_t token, uint32_t req_type)
{
	std::cout << "p3GxsMails::handleResponse(" << token << ", " << req_type
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
			        && meta.mGroupId != preferredGroupId;

			if(shoudlSubscribe)
				subscribeToGroup(token, meta.mGroupId, true);
			else if(shoudlUnSubscribe)
				subscribeToGroup(token, meta.mGroupId, false);

#ifdef GXS_MAIL_GRP_DEBUG
			char buff[30];
			struct tm* timeinfo;
			timeinfo = localtime(&meta.mLastPost);
			strftime(buff, sizeof(buff), "%Y %b %d %H:%M", timeinfo);

			std::cout << "p3GxsMails::handleResponse(...) GROUPS_LIST "
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

		if(preferredGroupId.isNull())
		{
			/* This is true only at first run when we haven't received mail
			 * distribuition groups from friends
			 * TODO: We should check if we have some connected friend too, to
			 * avoid to create yet another never used mail distribution group.
			 */

			std::cerr << "p3GxsMails::handleResponse(...) preferredGroupId.isNu"
			          << "ll() let's create a new group." << std::endl;
			uint32_t token;
			publishGroup(token, new RsGxsMailGroupItem());
			queueRequest(token, GROUP_CREATE);
		}

		break;
	}
	case GROUP_CREATE:
	{
		std::cerr << "p3GxsMails::handleResponse(...) GROUP_CREATE" << std::endl;
		RsGxsGroupId grpId;
		acknowledgeTokenGrp(token, grpId);
		supersedePreferredGroup(grpId);
		break;
	}
	case MAILS_UPDATE:
	{
		std::cout << "p3GxsMails::handleResponse(...) MAILS_UPDATE" << std::endl;
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
				switch(static_cast<GxsMailItemsSubtypes>(gIt->PacketSubType()))
				{
				case GxsMailItemsSubtypes::GXS_MAIL_SUBTYPE_MAIL:
				case GxsMailItemsSubtypes::GXS_MAIL_SUBTYPE_RECEIPT:
				{
					RsGxsMailBaseItem* mb =
					        dynamic_cast<RsGxsMailBaseItem*>(*mIt);
					if(mb)
					{
						RS_STACK_MUTEX(ingoingMutex);
						ingoingQueue.insert(inMap::value_type(mb->mailId, mb));
					}
					else
						std::cerr << "p3GxsMails::handleResponse(...) "
						          << "GXS_MAIL_SUBTYPE_MAIL cast error, "
						          << "something really wrong is happening"
						          << std::endl;
					break;
				}
				default:
					std::cerr << "p3GxsMails::handleResponse(...) MAILS_UPDATE "
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
		std::cerr << "p3GxsMails::handleResponse(...) Unknown req_type: "
		          << req_type << std::endl;
		break;
	}
}

void p3GxsMails::service_tick()
{
	GxsTokenQueue::checkRequests();

	{
		RS_STACK_MUTEX(outgoingMutex);
		for ( auto it = outgoingQueue.begin(); it != outgoingQueue.end(); )
		{
			OutgoingRecord& pr(it->second);
			GxsMailStatus oldStatus = pr.status;
			processOutgoingRecord(pr);
			if (oldStatus != pr.status) notifyClientService(pr);
			if( pr.status >= GxsMailStatus::RECEIPT_RECEIVED )
				it = outgoingQueue.erase(it);
			else ++it;
		}
	}


	{
		RS_STACK_MUTEX(ingoingMutex);
		for( auto it = ingoingQueue.begin(); it != ingoingQueue.end(); )
		{
			switch(static_cast<GxsMailItemsSubtypes>(
			           it->second->PacketSubType()))
			{
			case GxsMailItemsSubtypes::GXS_MAIL_SUBTYPE_MAIL:
			{
				RsGxsMailItem* msg = dynamic_cast<RsGxsMailItem*>(it->second);
				if(!msg)
				{
					std::cerr << "p3GxsMails::service_tick() (EE) "
					          << "GXS_MAIL_SUBTYPE_MAIL dynamic_cast failed, "
					          << "something really wrong is happening!"
					          << std::endl;
				}
				else
				{
					std::cout << "p3GxsMails::service_tick() "
					          << "GXS_MAIL_SUBTYPE_MAIL handling: "
					          << msg->meta.mMsgId
					          << " with cryptoType: "
					          << static_cast<uint32_t>(msg->cryptoType)
					          << " recipientHint: " << msg->recipientHint
					          << " mailId: "<< msg->mailId
					          << " payload.size(): " << msg->payload.size()
					          << std::endl;
					handleEcryptedMail(msg);
				}
				break;
			}
			case GxsMailItemsSubtypes::GXS_MAIL_SUBTYPE_RECEIPT:
			{
				RsGxsMailPresignedReceipt* rcpt =
				        dynamic_cast<RsGxsMailPresignedReceipt*>(it->second);
				if(!rcpt)
				{
					std::cerr << "p3GxsMails::service_tick() (EE) "
					          << "GXS_MAIL_SUBTYPE_RECEIPT dynamic_cast failed,"
					          << " something really wrong is happening!"
					          << std::endl;
				}
				else if(idService.isOwnId(rcpt->meta.mAuthorId))
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
				std::cerr << "p3GxsMails::service_tick() (EE) got something "
				          << "really unknown into ingoingQueue!!" << std::endl;
				break;
			}

			delete it->second; it = ingoingQueue.erase(it);
		}
	}
}

RsGenExchange::ServiceCreate_Return p3GxsMails::service_CreateGroup(
        RsGxsGrpItem* grpItem, RsTlvSecurityKeySet& /*keySet*/ )
{
	std::cout << "p3GxsMails::service_CreateGroup(...) "
	          << grpItem->meta.mGroupId << std::endl;
	return SERVICE_CREATE_SUCCESS;
}

void p3GxsMails::notifyChanges(std::vector<RsGxsNotify*>& changes)
{
	std::cout << "p3GxsMails::notifyChanges(...)" << std::endl;
	for( std::vector<RsGxsNotify*>::const_iterator it = changes.begin();
	     it != changes.end(); ++it )
	{
		RsGxsGroupChange* grpChange = dynamic_cast<RsGxsGroupChange *>(*it);
		RsGxsMsgChange* msgChange = dynamic_cast<RsGxsMsgChange *>(*it);

		if (grpChange)
		{
			std::cout << "p3GxsMails::notifyChanges(...) grpChange" << std::endl;
			requestGroupsData(&(grpChange->mGrpIdList));
		}
		else if(msgChange)
		{
			std::cout << "p3GxsMails::notifyChanges(...) msgChange" << std::endl;
			uint32_t token;
			RsTokReqOptions opts; opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
			getTokenService()->requestMsgInfo( token, 0xcaca,
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
					std::cout << "p3GxsMails::notifyChanges(...) got "
					          << "notification for message " << msgId
					          << " in group " << grpId << std::endl;
				}
			}
		}
	}
}

uint32_t p3GxsMails::AuthenPolicy()
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

bool p3GxsMails::requestGroupsData(const std::list<RsGxsGroupId>* groupIds)
{
	//	std::cout << "p3GxsMails::requestGroupsList()" << std::endl;
	uint32_t token;
	RsTokReqOptions opts; opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
	if(!groupIds) getTokenService()->requestGroupInfo(token, 0xcaca, opts);
	else getTokenService()->requestGroupInfo(token, 0xcaca, opts, *groupIds);
	GxsTokenQueue::queueRequest(token, GROUPS_LIST);
	return true;
}

bool p3GxsMails::handleEcryptedMail(const RsGxsMailItem* mail)
{
	std::cout << "p3GxsMails::handleEcryptedMail(...)" << std::endl;

	std::set<RsGxsId> decryptIds;
	std::list<RsGxsId> ownIds;
	idService.getOwnIds(ownIds);
	for(auto it = ownIds.begin(); it != ownIds.end(); ++it)
		if(mail->maybeRecipient(*it)) decryptIds.insert(*it);

	// Hint match none of our own ids
	if(decryptIds.empty())
	{
		std::cout << "p3GxsMails::handleEcryptedMail(...) hint doesn't match" << std::endl;
		return true;
	}

	switch (mail->cryptoType)
	{
	case RsGxsMailEncryptionMode::CLEAR_TEXT:
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
	case RsGxsMailEncryptionMode::RSA:
	{
		bool ok = true;
		for( std::set<RsGxsId>::const_iterator it = decryptIds.begin();
		     it != decryptIds.end(); ++it )
		{
			const RsGxsId& decryptId(*it);
			uint8_t* decrypted_data = NULL;
			uint32_t decrypted_data_size = 0;
			uint32_t decryption_error;
			if( idService.decryptData( &mail->payload[0],
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

bool p3GxsMails::dispatchDecryptedMail( const RsGxsId& authorId,
                                        const RsGxsId& decryptId,
                                        const uint8_t* decrypted_data,
                                        uint32_t decrypted_data_size )
{
	std::cout << "p3GxsMails::dispatchDecryptedMail(, , " << decrypted_data_size
	          << ")" << std::endl;

	uint16_t csri = 0;
	uint32_t offset = 0;
	if(!getRawUInt16( decrypted_data, decrypted_data_size, &offset, &csri))
	{
		std::cerr << "p3GxsMails::dispatchDecryptedMail(...) (EE) fatal error "
		          << "deserializing service type, something really wrong is "
		          << "happening!" << std::endl;
		return false;
	}
	GxsMailSubServices rsrvc = static_cast<GxsMailSubServices>(csri);

	RsNxsMailPresignedReceipt* receipt = new RsNxsMailPresignedReceipt();
	uint32_t rcptsize = decrypted_data_size;
	if(!receipt->deserialize(decrypted_data, rcptsize, offset))
	{
		std::cerr << "p3GxsMails::dispatchDecryptedMail(...) (EE) fatal error "
		          << "deserializing presigned return receipt , something really"
		          << " wrong is happening!" << std::endl;
		delete receipt;
		return false;
	}
	std::cout << "p3GxsMails::dispatchDecryptedMail(...) dispatching receipt "
	          << "with: msgId: " << receipt->msgId << std::endl;

	std::vector<RsNxsMsg*> rcct; rcct.push_back(receipt);
	RsGenExchange::notifyNewMessages(rcct);

	GxsMailsClient* recipientService = NULL;
	{
		RS_STACK_MUTEX(servClientsMutex);
		recipientService = servClients[rsrvc];
	}

	if(recipientService)
		return recipientService->receiveGxsMail( authorId, decryptId,
		                                         &decrypted_data[offset],
		                                         decrypted_data_size-offset );
	else
	{
		std::cerr << "p3GxsMails::dispatchReceivedMail(...) "
		          << "got message for unknown service: "
		          << csri << std::endl;
		return false;
	}
}

void p3GxsMails::processOutgoingRecord(OutgoingRecord& pr)
{
	//std::cout << "p3GxsMails::processRecord(...)" << std::endl;

	switch (pr.status)
	{
	case GxsMailStatus::PENDING_PROCESSING:
	{
		pr.mailItem.saltRecipientHint(pr.recipient);
		pr.mailItem.saltRecipientHint(RsGxsId::random());
	}
	case GxsMailStatus::PENDING_PREFERRED_GROUP:
	{
		if(preferredGroupId.isNull())
		{
			requestGroupsData();
			pr.status = GxsMailStatus::PENDING_PREFERRED_GROUP;
			break;
		}

		pr.mailItem.meta.mGroupId = preferredGroupId;
	}
	case GxsMailStatus::PENDING_RECEIPT_CREATE:
	{
		RsGxsMailPresignedReceipt grcpt;
		grcpt.meta = pr.mailItem.meta;
		grcpt.meta.mPublishTs = time(NULL);
		grcpt.mailId = pr.mailItem.mailId;
		uint32_t groff = 0, grsz = grcpt.size();
		std::vector<uint8_t> grsrz;
		grsrz.resize(grsz);
		grcpt.serialize(&grsrz[0], grsz, groff);

		pr.presignedReceipt.grpId = preferredGroupId;
		pr.presignedReceipt.metaData = new RsGxsMsgMetaData();
		*pr.presignedReceipt.metaData = grcpt.meta;
		pr.presignedReceipt.msg.setBinData(&grsrz[0], grsz);
	}
	case GxsMailStatus::PENDING_RECEIPT_SIGNATURE:
	{
		switch (RsGenExchange::createMessage(&pr.presignedReceipt))
		{
		case CREATE_SUCCESS: break;
		case CREATE_FAIL_TRY_LATER:
			pr.status = GxsMailStatus::PENDING_RECEIPT_CREATE;
			return;
		default:
			pr.status = GxsMailStatus::FAILED_RECEIPT_SIGNATURE;
			goto processingFailed;
		}

		uint32_t metaSize = pr.presignedReceipt.metaData->serial_size();
		std::vector<uint8_t> srx; srx.resize(metaSize);
		pr.presignedReceipt.metaData->serialise(&srx[0], &metaSize);
		pr.presignedReceipt.meta.setBinData(&srx[0], metaSize);
	}
	case GxsMailStatus::PENDING_PAYLOAD_CREATE:
	{
		uint16_t serv = static_cast<uint16_t>(pr.clientService);
		uint32_t rcptsize = pr.presignedReceipt.serial_size();
		uint32_t datasize = pr.mailData.size();
		pr.mailItem.payload.resize(2 + rcptsize + datasize);
		uint32_t offset = 0;
		setRawUInt16(&pr.mailItem.payload[0], 2, &offset, serv);
		pr.presignedReceipt.serialise( &pr.mailItem.payload[offset],
		                               rcptsize );
		offset += rcptsize;
		memcpy(&pr.mailItem.payload[offset], &pr.mailData[0], datasize);
	}
	case GxsMailStatus::PENDING_PAYLOAD_ENCRYPT:
	{
		switch (pr.mailItem.cryptoType)
		{
		case RsGxsMailEncryptionMode::CLEAR_TEXT:
		{
			std::cerr << "p3GxsMails::sendMail(...) you are sending a mail "
			          << "without encryption, everyone can read it!"
			          << std::endl;
			break;
		}
		case RsGxsMailEncryptionMode::RSA:
		{
			uint8_t* encryptedData = NULL;
			uint32_t encryptedSize = 0;
			uint32_t encryptError = 0;
			if( idService.encryptData( &pr.mailItem.payload[0],
			                           pr.mailItem.payload.size(),
			                           encryptedData, encryptedSize,
			                           pr.recipient, encryptError, true ) )
			{
				pr.mailItem.payload.resize(encryptedSize);
				memcpy( &pr.mailItem.payload[0], encryptedData,
				        encryptedSize );
				free(encryptedData);
				break;
			}
			else
			{
				std::cerr << "p3GxsMails::sendMail(...) RSA encryption failed! "
				          << "error_status: " << encryptError << std::endl;
				pr.status = GxsMailStatus::FAILED_ENCRYPTION;
				goto processingFailed;
			}
		}
		case RsGxsMailEncryptionMode::UNDEFINED_ENCRYPTION:
		default:
			std::cerr << "p3GxsMails::sendMail(...) attempt to send mail with "
			          << "wrong EncryptionMode: "
			          << static_cast<uint>(pr.mailItem.cryptoType)
			          << " dropping mail!" << std::endl;
			pr.status = GxsMailStatus::FAILED_ENCRYPTION;
			goto processingFailed;
		}
	}
	case GxsMailStatus::PENDING_PUBLISH:
	{
		std::cout << "p3GxsMails::sendEmail(...) sending mail to: "
		          << pr.recipient
		          << " with cryptoType: "
		          << static_cast<uint>(pr.mailItem.cryptoType)
		          << " recipientHint: " << pr.mailItem.recipientHint
		          << " receiptId: " << pr.mailItem.mailId
		          << " payload size: " << pr.mailItem.payload.size()
		          << std::endl;

		uint32_t token;
		publishMsg(token, new RsGxsMailItem(pr.mailItem));
		pr.status = GxsMailStatus::PENDING_RECEIPT_RECEIVE;
		break;
	}
	//case GxsMailStatus::PENDING_TRANSFER:
	case GxsMailStatus::PENDING_RECEIPT_RECEIVE:
	{
		RS_STACK_MUTEX(ingoingMutex);
		auto range = ingoingQueue.equal_range(pr.mailItem.mailId);
		for( auto it = range.first; it != range.second; ++it)
		{
			RsGxsMailPresignedReceipt* rt =
			        dynamic_cast<RsGxsMailPresignedReceipt*>(it->second);
			if(rt && idService.isOwnId(rt->meta.mAuthorId))
			{
				ingoingQueue.erase(it); delete rt;
				pr.status = GxsMailStatus::RECEIPT_RECEIVED;
				break;
			}
		}
		// TODO: Resend message if older then treshold
		break;
	}
	case GxsMailStatus::RECEIPT_RECEIVED:
		break;

processingFailed:
	case GxsMailStatus::FAILED_RECEIPT_SIGNATURE:
	case GxsMailStatus::FAILED_ENCRYPTION:
	default:
	{
		std::cout << "p3GxsMails::processRecord(" << pr.mailItem.mailId
		          << ") failed with: " << static_cast<uint>(pr.status)
		          << std::endl;
		break;
	}
	}
}

void p3GxsMails::notifyClientService(const OutgoingRecord& pr)
{
	RS_STACK_MUTEX(servClientsMutex);
	auto it = servClients.find(pr.clientService);
	if( it != servClients.end())
	{
		GxsMailsClient* serv(it->second);
		if(serv)
		{
			serv->notifySendMailStatus(pr.mailItem, pr.status);
			return;
		}
	}

	std::cerr << "p3GxsMails::processRecord(...) (EE) processed"
	          << " mail for unkown service: "
	          << static_cast<uint32_t>(pr.clientService)
	          << " fatally failed with: "
	          << static_cast<uint32_t>(pr.status) << std::endl;
	print_stacktrace();
}

RsSerialiser* p3GxsMails::setupSerialiser()
{
	RsSerialiser* rss = new RsSerialiser;
	rss->addSerialType(new RsGxsMailSerializer);
	return rss;
}

bool p3GxsMails::saveList(bool &cleanup, std::list<RsItem *>& saveList)
{
	std::cout << "p3GxsMails::saveList(...)" << saveList.size() << " "
	          << ingoingQueue.size() << " " << outgoingQueue.size()
	          << std::endl;

	outgoingMutex.lock();
	ingoingMutex.lock();

	for ( auto& kv : outgoingQueue ) saveList.push_back(&kv.second);
	for ( auto& kv : ingoingQueue ) saveList.push_back(kv.second);

	std::cout << "p3GxsMails::saveList(...)" << saveList.size() << " "
	          << ingoingQueue.size() << " " << outgoingQueue.size()
	          << std::endl;

	cleanup = false;
	return true;
}

void p3GxsMails::saveDone()
{
	outgoingMutex.unlock();
	ingoingMutex.unlock();
}

bool p3GxsMails::loadList(std::list<RsItem *>&loadList)
{
	std::cout << "p3GxsMails::loadList(...) " << loadList.size() << " "
	          << ingoingQueue.size() << " " << outgoingQueue.size()
	          << std::endl;

	for(auto& v : loadList)
		switch(static_cast<GxsMailItemsSubtypes>(v->PacketSubType()))
		{
		case GxsMailItemsSubtypes::GXS_MAIL_SUBTYPE_MAIL:
		case GxsMailItemsSubtypes::GXS_MAIL_SUBTYPE_RECEIPT:
		{
			RsGxsMailBaseItem* mi = dynamic_cast<RsGxsMailBaseItem*>(v);
			if(mi)
			{
				RS_STACK_MUTEX(ingoingMutex);
				ingoingQueue.insert(inMap::value_type(mi->mailId, mi));
			}
			break;
		}
		case GxsMailItemsSubtypes::OUTGOING_RECORD_ITEM:
		{
			OutgoingRecord* ot = dynamic_cast<OutgoingRecord*>(v);
			if(ot)
			{
				RS_STACK_MUTEX(outgoingMutex);
				outgoingQueue.insert(
				            prMap::value_type(ot->mailItem.mailId, *ot));
			}
			delete v;
			break;
		}
		case GxsMailItemsSubtypes::GXS_MAIL_SUBTYPE_GROUP:
		default:
			std::cerr << "p3GxsMails::loadList(...) (EE) got item with "
			          << "unhandled type: "
			          << static_cast<uint>(v->PacketSubType())
			          << std::endl;
			delete v;
			break;
		}

	std::cout << "p3GxsMails::loadList(...) " << loadList.size() << " "
	          << ingoingQueue.size() << " " << outgoingQueue.size()
	          << std::endl;

	return true;
}

