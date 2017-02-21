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


bool p3GxsMails::sendMail( RsGxsMailId& mailId,
                           GxsMailsClient::GxsMailSubServices service,
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
        GxsMailsClient::GxsMailSubServices serviceType, GxsMailsClient* service)
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

		for( std::vector<RsGxsGrpItem *>::iterator it = groups.begin();
		     it != groups.end(); ++it )
		{
			/* For each group check if it is better candidate then
			 * preferredGroupId, if it is supplant it and subscribe if it is not
			 * subscribed yet.
			 * Otherwise if it has recent messages subscribe.
			 * If the group was already subscribed has no recent messages
			 * unsubscribe.
			 */

			const RsGroupMetaData& meta = (*it)->meta;
			bool subscribed = IS_GROUP_SUBSCRIBED(meta.mSubscribeFlags);
			bool old = olderThen( meta.mLastPost,
			                      UNUSED_GROUP_UNSUBSCRIBE_INTERVAL );
			bool supersede = supersedePreferredGroup(meta.mGroupId);
			uint32_t token;

			if( !subscribed && ( !old || supersede ))
				subscribeToGroup(token, meta.mGroupId, true);
			else if( subscribed && old )
				subscribeToGroup(token, meta.mGroupId, false);
		}

		if(preferredGroupId.isNull())
		{
			/* This is true only at first run when we haven't received mail
			 * distribuition groups from friends
			 * TODO: We should check if we have some connected firend too, to
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
				switch(gIt->PacketSubType())
				{
				case GXS_MAIL_SUBTYPE_MAIL:
				case GXS_MAIL_SUBTYPE_RECEIPT:
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
			if( it->second->PacketSubType() != GXS_MAIL_SUBTYPE_MAIL )
			{ ++it; continue; }

			RsGxsMailItem* msg = dynamic_cast<RsGxsMailItem*>(it->second);
			if(!msg)
			{
				std::cout << "p3GxsMails::service_tick() GXS_MAIL_SUBTYPE_MAIL"
				          << "dynamic_cast failed, something really wrong is "
				          << "happening!" << std::endl;
				++it; continue;
			}

			std::cout << "p3GxsMails::service_tick() GXS_MAIL_SUBTYPE_MAIL "
			          << "handling: " << msg->meta.mMsgId
			          << " with cryptoType: "
			          << static_cast<uint32_t>(msg->cryptoType)
			          << " recipientHint: " << msg->recipientsHint
			          << " mailId: "<< msg->mailId
			          << " payload.size(): " << msg->payload.size()
			          << std::endl;

			handleEcryptedMail(msg);
			it = ingoingQueue.erase(it); delete msg;
		}
	}
}

RsGenExchange::ServiceCreate_Return p3GxsMails::service_CreateGroup(RsGxsGrpItem* grpItem, RsTlvSecurityKeySet& /*keySet*/)
{
	std::cout << "p3GxsMails::service_CreateGroup(...) " << grpItem->meta.mGroupId << std::endl;
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
		return dispatchDecryptedMail( mail, &mail->payload[0],
		                                     mail->payload.size() );
	}
	case RsGxsMailEncryptionMode::RSA:
	{
		bool ok = true;
		for( std::set<RsGxsId>::const_iterator it = decryptIds.begin();
		     it != decryptIds.end(); ++it )
		{
			uint8_t* decrypted_data = NULL;
			uint32_t decrypted_data_size = 0;
			uint32_t decryption_error;
			if( idService.decryptData( &mail->payload[0],
			                            mail->payload.size(), decrypted_data,
			                            decrypted_data_size, *it,
			                            decryption_error ) )
				ok = ok && dispatchDecryptedMail( mail, decrypted_data,
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

bool p3GxsMails::dispatchDecryptedMail( const RsGxsMailItem* received_msg,
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
	GxsMailsClient::GxsMailSubServices rsrvc;
	rsrvc = static_cast<GxsMailsClient::GxsMailSubServices>(csri);

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

	GxsMailsClient* reecipientService = NULL;
	{
		RS_STACK_MUTEX(servClientsMutex);
		reecipientService = servClients[rsrvc];
	}

	if(reecipientService)
		return reecipientService->receiveGxsMail( *received_msg,
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
		          << " recipientHint: " << pr.mailItem.recipientsHint
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
		auto it = ingoingQueue.find(pr.mailItem.mailId);
		if (it == ingoingQueue.end()) break;
		RsGxsMailPresignedReceipt* rt =
		        dynamic_cast<RsGxsMailPresignedReceipt*>(it->second);
		if( !rt || !idService.isOwnId(rt->meta.mAuthorId) ) break;

		ingoingQueue.erase(it); delete rt;
		pr.status = GxsMailStatus::RECEIPT_RECEIVED;
		// TODO: Malicious adversary could forge messages with same mailId and
		//   could end up overriding the legit receipt in ingoingQueue, and
		//   causing also a memleak(using unordered_multimap for ingoingQueue
		//   may fix this?)
		// TODO: Resend message if older then treshold
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

