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
	std::vector<const RsGxsId*> recipients;
	recipients.push_back(&recipient);
	return sendMail(service, own_gxsid, recipients, data, size, cm);
}

bool p3GxsMails::sendMail( GxsMailsClient::GxsMailSubServices service,
                           const RsGxsId& own_gxsid,
                           const std::vector<const RsGxsId*>& recipients,
                           const uint8_t* data, uint32_t size,
                           RsGxsMailBaseItem::EncryptionMode cm )
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

	std::set<RsGxsId> rcps;
	typedef std::vector<const RsGxsId*>::const_iterator itT;
	for(itT it = recipients.begin(); it != recipients.end(); it++)
	{
		const RsGxsId* gId = *it;

		if(!gId || gId->isNull())
		{
			std::cerr << "p3GxsMails::sendEmail(...) got invalid recipient"
			          << std::endl;
			print_stacktrace();
			return false;
		}

		rcps.insert(*gId);
	}
	if(rcps.empty())
	{
		std::cerr << "p3GxsMails::sendEmail(...) got no recipients"
		          << std::endl;
		print_stacktrace();
		return false;
	}

	RsGxsMailItem* item = new RsGxsMailItem();

	// Public metadata
	item->meta.mAuthorId = own_gxsid;
	item->meta.mGroupId = preferredGroupId;
	item->cryptoType = cm;

	typedef std::set<RsGxsId>::const_iterator siT;
	for(siT it = rcps.begin(); it != rcps.end(); ++it)
		item->saltRecipientHint(*it);

	// If there is jut one recipient salt with a random id to avoid leaking it
	if(rcps.size() == 1) item->saltRecipientHint(RsGxsId::random());

	/* At this point we do a lot of memory copying, it doesn't look pretty but
	 * ATM haven't thinked of an elegant way to have the GxsMailSubServices
	 * travelling encrypted withuot copying memory around or responsabilize the
	 * client service to embed it in data array that is awful */

	uint16_t serv = static_cast<uint16_t>(service);
	uint32_t clearTextPldSize = size+2;
	item->payload.resize(clearTextPldSize);
	uint32_t _discard = 0;
	setRawUInt16(&item->payload[0], 2, &_discard, serv);
	memcpy(&item->payload[2], data, size);

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
		if( idService.encryptData( &item->payload[0], clearTextPldSize,
		                           encryptedData, encryptedSize,
		                           rcps, encryptError, true ) )
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
	std::cout << "p3GxsMails::sendEmail(...) sending mail to:"<< *recipients[0]
	          << " payload size: : " << item->payload.size() << std::endl;
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
				RsGxsMsgItem* it = *mIt;
				std::cout << "p3GxsMails::handleResponse(...) MAILS_UPDATE "
				          << (uint32_t)it->PacketSubType() << std::endl;
				switch(it->PacketSubType())
				{
				case GXS_MAIL_SUBTYPE_MAIL:
				{
					RsGxsMailItem* msg = dynamic_cast<RsGxsMailItem*>(it);
					if(!msg)
					{
						std::cerr << "p3GxsMails::handleResponse(...) "
						          << "GXS_MAIL_SUBTYPE_MAIL cast error, "
						          << "something really wrong is happening"
						          << std::endl;
						break;
					}

					handleEcryptedMail(msg);
					break;
				}
				default:
					std::cerr << "p3GxsMails::handleResponse(...) MAILS_UPDATE "
					          << "Unknown mail subtype : "
					          << it->PacketSubType() << std::endl;
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
			sendMail( GxsMailsClient::MSG_SERVICE, gxsidA, gxsidB,
			          reinterpret_cast<const uint8_t*>(ciao.data()),
			          ciao.size(), RsGxsMailBaseItem::RSA );
		}
		else if(idService.isOwnId(gxsidB))
		{
			std::string ciao("CiBuono!");
			sendMail( GxsMailsClient::MSG_SERVICE, gxsidB, gxsidA,
			          reinterpret_cast<const uint8_t*>(ciao.data()),
			          ciao.size(), RsGxsMailBaseItem::RSA );
		}
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

bool p3GxsMails::handleEcryptedMail(RsGxsMailItem* mail)
{
	std::set<RsGxsId> decryptIds;
	std::list<RsGxsId> ownIds;
	idService.getOwnIds(ownIds);
	for(auto it = ownIds.begin(); it != ownIds.end(); ++it)
		if(mail->maybeRecipient(*it)) decryptIds.insert(*it);

	// Hint match none of our own ids
	if(decryptIds.empty()) goto hFail;

	for(auto it=decryptIds.begin(); it != decryptIds.end(); ++it)
		std::cout << *it;
	std::cout << std::endl;

	switch (mail->cryptoType)
	{
	case RsGxsMailBaseItem::CLEAR_TEXT:
	{
		uint16_t csri;
		uint32_t off = 0;
		getRawUInt16(&mail->payload[0], 2, &off, &csri);
		std::cerr << "service: " << csri << " got CLEAR_TEXT mail!"
		          << std::endl;
		if( !dispatchDecryptedMail( mail, &mail->payload[2],
		                            mail->payload.size()-2 ) )
			goto hFail;
		break;
	}
	case RsGxsMailBaseItem::RSA:
	{
		uint8_t* decrypted_data = NULL;
		uint32_t decrypted_data_size = 0;
		uint32_t decryption_error;
		bool ok = idService.decryptData( &mail->payload[0],
		        mail->payload.size(), decrypted_data,
		        decrypted_data_size, decryptIds,
		        decryption_error );
		ok = ok && dispatchDecryptedMail( mail, decrypted_data,
		                                  decrypted_data_size );
		if(!ok)
		{
			std::cout << "p3GxsMails::handleEcryptedMail(...) "
			          << "RSA decryption failed" << std::endl;
			free(decrypted_data);
			goto hFail;
		}
		break;
	}
	default:
		std::cout << "Unknown encryption type:"
		          << mail->cryptoType << std::endl;
		goto hFail;
	}

	return true;

hFail:
	delete mail;
	return false;
}

bool p3GxsMails::dispatchDecryptedMail( RsGxsMailItem* received_msg,
                                        uint8_t* decrypted_data,
                                        uint32_t decrypted_data_size )
{
	uint16_t csri;
	uint32_t off;
	getRawUInt16(decrypted_data, decrypted_data_size, &off, &csri);
	GxsMailsClient::GxsMailSubServices rsrvc;
	rsrvc = static_cast<GxsMailsClient::GxsMailSubServices>(csri);

	GxsMailsClient* reecipientService = NULL;
	{
		RS_STACK_MUTEX(servClientsMutex);
		reecipientService = servClients[rsrvc];
	}
	if(reecipientService)
		return reecipientService->receiveGxsMail( received_msg,
		                                          &decrypted_data[2],
		                                          decrypted_data_size-2 );
	else
	{
		std::cerr << "p3GxsMails::dispatchReceivedMail(...) "
		          << "got message for unknown service: "
		          << csri << std::endl;
		return false;
	}
}

