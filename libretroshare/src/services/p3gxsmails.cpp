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


bool p3GxsMails::sendMail( GxsMailsClient::GxsMailSubServices service,
                           const RsGxsId& own_gxsid, const RsGxsId& recipient,
                           const uint8_t* data, uint32_t size,
                           RsGxsMailBaseItem::EncryptionMode cm)
{
	std::cout << "p3GxsMails::sendEmail(...)" << std::endl;

	if(preferredGroupId.isNull())
	{
		requestGroupsData();
		std::cerr << "p3GxsMails::sendEmail(...) preferredGroupId.isNull()!"
		          << std::endl;
		return false;
	}

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

	RsGxsMailItem* item = new RsGxsMailItem();

	// Public metadata
	item->meta.mAuthorId = own_gxsid;
	item->meta.mGroupId = preferredGroupId;
	item->cryptoType = cm;
	item->saltRecipientHint(recipient);
	item->saltRecipientHint(RsGxsId::random());
	item->receiptId = RSRandom::random_u64();

	RsNxsMailPresignedReceipt nrcpt;
	preparePresignedReceipt(*item, nrcpt);

	uint16_t serv = static_cast<uint16_t>(service);
	uint32_t rcptsize = nrcpt.serial_size();
	item->payload.resize(2 + rcptsize + size);
	uint32_t offset = 0;
	setRawUInt16(&item->payload[0], 2, &offset, serv);
	nrcpt.serialise(&item->payload[offset], rcptsize); offset += rcptsize;
	memcpy(&item->payload[offset], data, size); //offset += size;

	std::cout << "p3GxsMails::sendMail(...) receipt size: " << rcptsize << std::endl;

	switch (cm)
	{
	case RsGxsMailBaseItem::CLEAR_TEXT:
	{
		std::cerr << "p3GxsMails::sendMail(...) you are sending a mail without"
		          << " encryption, everyone can read it!" << std::endl;
		print_stacktrace();
		break;
	}
	case RsGxsMailBaseItem::RSA:
	{
		uint8_t* encryptedData = NULL;
		uint32_t encryptedSize = 0;
		uint32_t encryptError = 0;
		if( idService.encryptData( &item->payload[0], item->payload.size(),
		                           encryptedData, encryptedSize,
		                           recipient, encryptError, true ) )
		{
			item->payload.resize(encryptedSize);
			memcpy(&item->payload[0], encryptedData, encryptedSize);
			free(encryptedData);
			break;
		}
		else
		{
			std::cerr << "p3GxsMails::sendMail(...) RSA encryption failed! "
			          << "error_status: " << encryptError << std::endl;
			print_stacktrace();
			return false;
		}
	}
	case RsGxsMailBaseItem::UNDEFINED_ENCRYPTION:
	default:
		std::cerr << "p3GxsMails::sendMail(...) attempt to send mail with wrong"
		          << " EncryptionMode " << cm << " dropping mail!" << std::endl;
		print_stacktrace();
		return false;
	}

	uint32_t token;
	std::cout << "p3GxsMails::sendEmail(...) sending mail to: "<< recipient
	          << " with cryptoType: " << item->cryptoType
	          << " recipientHint: " << item->recipientsHint
	          << " receiptId: " << item->receiptId
	          << " payload size: " << item->payload.size() << std::endl;
	publishMsg(token, item);
	return true;
}

void p3GxsMails::registerGxsMailsClient(
        GxsMailsClient::GxsMailSubServices serviceType, GxsMailsClient* service)
{
	RS_STACK_MUTEX(servClientsMutex);
	servClients[serviceType] = service;
}


void p3GxsMails::handleResponse(uint32_t token, uint32_t req_type)
{
	//std::cout << "p3GxsMails::handleResponse(" << token << ", " << req_type << ")" << std::endl;
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
				RsGxsMsgItem* gItem = *mIt;
				switch(gItem->PacketSubType())
				{
				case GXS_MAIL_SUBTYPE_MAIL:
				{
					RsGxsMailItem* msg = dynamic_cast<RsGxsMailItem*>(gItem);
					if(!msg)
					{
						std::cerr << "p3GxsMails::handleResponse(...) "
						          << "GXS_MAIL_SUBTYPE_MAIL cast error, "
						          << "something really wrong is happening"
						          << std::endl;
						break;
					}

					std::cout << "p3GxsMails::handleResponse(...) MAILS_UPDATE "
					          << "GXS_MAIL_SUBTYPE_MAIL handling: "
					          << msg->meta.mMsgId
					          << " with cryptoType: "<< msg->cryptoType
					          << " recipientHint: " << msg->recipientsHint
					          << " receiptId: "<< msg->receiptId
					          << " payload.size(): " << msg->payload.size()
					          << std::endl;

					handleEcryptedMail(msg);
					break;
				}
				case GXS_MAIL_SUBTYPE_RECEIPT:
				{
					RsGxsMailPresignedReceipt* msg =
					        dynamic_cast<RsGxsMailPresignedReceipt*>(gItem);
					if(!msg)
					{
						std::cerr << "p3GxsMails::handleResponse(...) "
						          << "GXS_MAIL_SUBTYPE_RECEIPT cast error, "
						          << "something really wrong is happening"
						          << std::endl;
						break;
					}

					std::cout << "p3GxsMails::handleResponse(...) MAILS_UPDATE "
					          << "GXS_MAIL_SUBTYPE_RECEIPT handling: "
					          << msg->meta.mMsgId
					          << "with receiptId: "<< msg->receiptId
					          << std::endl;

					/* TODO: Notify client services if the original mail was
					 *   sent from this node and mark for deletion, otherwise
					 *   just mark original mail for deletion. */

					break;
				}
				default:
					std::cerr << "p3GxsMails::handleResponse(...) MAILS_UPDATE "
					          << "Unknown mail subtype : "
					          << static_cast<uint32_t>(gItem->PacketSubType())
					          << std::endl;
					break;
				}
				delete gItem;
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
	static int tc = 0;
	++tc;

	if(((tc % 1000) == 0) || (tc == 50)) requestGroupsData();

	if(tc == 500)
	{
		RsGxsId gxsidA("d0df7474bdde0464679e6ef787890287");
		RsGxsId gxsidB("d060bea09dfa14883b5e6e517eb580cd");
		if(idService.isOwnId(gxsidA))
		{
			std::string ciao("CiAone!");
			sendMail( GxsMailsClient::TEST_SERVICE, gxsidA, gxsidB,
			          reinterpret_cast<const uint8_t*>(ciao.data()),
			          ciao.size(), RsGxsMailBaseItem::RSA );
		}
//		else if(idService.isOwnId(gxsidB))
//		{
//			std::string ciao("CiBuono!");
//			sendMail( GxsMailsClient::TEST_SERVICE, gxsidB, gxsidA,
//			          reinterpret_cast<const uint8_t*>(ciao.data()),
//			          ciao.size(), RsGxsMailBaseItem::RSA );
//		}
	}

	GxsTokenQueue::checkRequests();
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
	case RsGxsMailBaseItem::CLEAR_TEXT:
	{
		uint16_t csri = 0;
		uint32_t off = 0;
		getRawUInt16(&mail->payload[0], mail->payload.size(), &off, &csri);
		std::cerr << "service: " << csri << " got CLEAR_TEXT mail!"
		          << std::endl;
		return dispatchDecryptedMail( mail, &mail->payload[0],
		                                     mail->payload.size() );
	}
	case RsGxsMailBaseItem::RSA:
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
		          << mail->cryptoType << std::endl;
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
		return reecipientService->receiveGxsMail( received_msg,
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

bool p3GxsMails::preparePresignedReceipt(const RsGxsMailItem& mail, RsNxsMailPresignedReceipt& receipt)
{
	RsGxsMailPresignedReceipt grcpt;
	grcpt.meta = mail.meta;
	grcpt.meta.mPublishTs = time(NULL);
	grcpt.receiptId = mail.receiptId;
	uint32_t groff = 0, grsz = grcpt.size();
	std::vector<uint8_t> grsrz;
	grsrz.resize(grsz);
	grcpt.serialize(&grsrz[0], grsz, groff);
	receipt.msg.setBinData(&grsrz[0], grsz);

	receipt.grpId = preferredGroupId;
	receipt.metaData = new RsGxsMsgMetaData();
	*receipt.metaData = grcpt.meta;

	if(createMessage(&receipt) != CREATE_SUCCESS)
	{
		std::cout << "p3GxsMails::preparePresignedReceipt(...) receipt creation"
		          << " failed!" << std::endl;
		return false;
	}

	uint32_t metaSize = receipt.metaData->serial_size();
	std::vector<uint8_t> srx; srx.resize(metaSize);
	receipt.metaData->serialise(&srx[0], &metaSize);
	receipt.meta.setBinData(&srx[0], metaSize);

	std::cout << "p3GxsMails::preparePresignedReceipt(...) prepared receipt"
	          << "with: grcpt.meta.mMsgId: " << grcpt.meta.mMsgId
	          << " msgId: " << receipt.msgId
	          << " metaData.mMsgId: " << receipt.metaData->mMsgId
	          << std::endl;
	return true;
}

