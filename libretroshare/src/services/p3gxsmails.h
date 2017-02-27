#pragma once
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

#include <stdint.h>
#include <unordered_map>
#include <map>

#include "retroshare/rsgxsifacetypes.h" // For RsGxsId, RsGxsCircleId
#include "gxs/gxstokenqueue.h" // For GxsTokenQueue
#include "serialiser/rsgxsmailitems.h" // For RS_SERVICE_TYPE_GXS_MAIL
#include "services/p3idservice.h" // For p3IdService
#include "util/rsthreads.h"

struct p3GxsMails;

/// Services who want to make use of p3GxsMails should inherit this struct
struct GxsMailsClient
{
	/**
	 * This will be called by p3GxsMails to dispatch mails to the subservice
	 * @param originalMessage message as received from GXS backend (encrypted)
	 * @param data buffer containing the decrypted data
	 * @param dataSize size of the buffer
	 * @return true if dispatching goes fine, false otherwise
	 */
	virtual bool receiveGxsMail( const RsGxsMailItem& originalMessage,
	                             const uint8_t* data, uint32_t dataSize ) = 0;

	/**
	 * This will be called by p3GxsMails to notify the subservice about the
	 * status of a sent email.
	 * @param originalMessage message for with the notification is made
	 * @param status the new status of the message
	 * @return true if notification goes fine, false otherwise (ignored ATM)
	 */
	virtual bool notifySendMailStatus( const RsGxsMailItem& originalMessage,
	                                   GxsMailStatus status ) = 0;
};

/**
 * @brief p3GxsMails is a mail delivery service based on GXS.
 * p3GxsMails is capable of asynchronous mail delivery and acknowledgement.
 * p3GxsMails is meant to be capable of multiple encryption options,
 * @see RsGxsMailEncryptionMode at moment messages are encrypted using RSA
 * unless the user ask for them being sent in clear text ( this is not supposed
 * to happen in non testing environment so warnings and stack traces are printed
 * in the log if an attempt to send something in clear text is made ).
 * p3GxsMails try to hide metadata so the travelling message signed by the author
 * but the recipient is not disclosed, instead to avoid everyone trying to
 * decrypt every message a hint has been introduced, the hint is calculated in a
 * way that one can easily prove that a message is not destined to someone, but
 * cannot prove the message is destined to someone
 * @see RsGxsMailItem::recipientsHint for more details.
 * p3GxsMails expose a simple API to send and receive mails, the API also
 * provide notification for the sending mail status @see sendMail(...),
 * @see querySendMailStatus(...), @see registerGxsMailsClient(...),
 * @see GxsMailsClient::receiveGxsMail(...),
 * @see GxsMailsClient::notifySendMailStatus(...).
 */
struct p3GxsMails : RsGenExchange, GxsTokenQueue, p3Config
{
	p3GxsMails( RsGeneralDataService* gds, RsNetworkExchangeService* nes,
	            p3IdService& identities ) :
	    RsGenExchange( gds, nes, new RsGxsMailSerializer(),
	                   RS_SERVICE_TYPE_GXS_MAIL, &identities,
	                   AuthenPolicy(), GXS_STORAGE_PERIOD ),
	    GxsTokenQueue(this), idService(identities),
	    servClientsMutex("p3GxsMails client services map mutex"),
	    outgoingMutex("p3GxsMails outgoing queue map mutex"),
	    ingoingMutex("p3GxsMails ingoing queue map mutex") {}
	~p3GxsMails();

	/**
	 * Send an email to recipient, in the process author of the email is
	 * disclosed to the network (because the sent GXS item is signed), while
	 * recipient is not @see RsGxsMailBaseItem::recipientsHint for details on
	 * recipient protection.
	 * This method is part of the public interface of this service.
	 * @return true if the mail will be sent, false if not
	 */
	bool sendMail( RsGxsMailId& mailId,
	               GxsMailSubServices service,
	               const RsGxsId& own_gxsid, const RsGxsId& recipient,
	               const uint8_t* data, uint32_t size,
	               RsGxsMailEncryptionMode cm = RsGxsMailEncryptionMode::RSA
	        );

	/**
	 * This method is part of the public interface of this service.
	 * @return false if mail is not found in outgoing queue, true otherwise
	 */
	bool querySendMailStatus( RsGxsMailId mailId, GxsMailStatus& st );

	/**
	 * Register a client service to p3GxsMails to receive mails via
	 * GxsMailsClient::receiveGxsMail(...) callback
	 * This method is part of the public interface of this service.
	 */
	void registerGxsMailsClient( GxsMailSubServices serviceType,
	                             GxsMailsClient* service );

	/// @see RsGenExchange::getServiceInfo()
	virtual RsServiceInfo getServiceInfo() { return RsServiceInfo( RS_SERVICE_TYPE_GXS_MAIL, "GXS Mails", 0, 1, 0, 1 ); }

private:
	/** Time interval of inactivity before a distribution group is unsubscribed.
	 * Approximatively 3 months seems ok ATM. */
	const static int32_t UNUSED_GROUP_UNSUBSCRIBE_INTERVAL = 0x76A700;

	/**
	 * This should be as little as possible as the size of the database can grow
	 * very fast taking in account we are handling mails for the whole network.
	 * We do prefer to resend a not acknowledged yet mail after
	 * GXS_STORAGE_PERIOD has passed and keep it little.
	 * Tought it can't be too little as this may cause signed receipts to
	 * get lost thus causing resend and fastly grow perceived async latency, in
	 * case two sporadically connected users sends mails each other.
	 * While it is ok for signed acknowledged to stays in the DB for a
	 * full GXS_STORAGE_PERIOD, mails should be removed as soon as a valid
	 * signed acknowledged is received for each of them.
	 * Two weeks seems fair ATM.
	 */
	const static uint32_t GXS_STORAGE_PERIOD = 0x127500;

	/// Define how the backend should handle authentication based on signatures
	static uint32_t AuthenPolicy();

	/// Types to mark queries in tokens queue
	enum GxsReqResTypes
	{
		GROUPS_LIST        = 1,
		GROUP_CREATE       = 2,
		MAILS_UPDATE       = 3
	};

	/// Store the id of the preferred GXS group to send emails
	RsGxsGroupId preferredGroupId;

	/// Used for items {de,en}cryption
	p3IdService& idService;

	/// Stores pointers to client services to notify them about new mails
	std::map<GxsMailSubServices, GxsMailsClient*> servClients;
	RsMutex servClientsMutex;

	/**
	 * @brief Keep track of outgoing mails.
	 * Records enter the queue when a mail is sent, and are removed when a
	 * receipt has been received or sending is considered definetly failed.
	 * Items are saved in config for consistence accross RetroShare shutdowns.
	 */
	typedef std::map<RsGxsMailId, OutgoingRecord> prMap;
	prMap outgoingQueue;
	RsMutex outgoingMutex;
	void processOutgoingRecord(OutgoingRecord& r);

	/**
	 * @brief Ingoing mail and receipt processing queue.
	 * At shutdown remaining items are saved in config and then deleted in
	 * destructor for consistence accross RetroShare instances.
	 * In order to avoid malicious messages ( non malicious collision has 1/2^64
	 * probablity ) to smash items in the queue thus causing previous incoming
	 * item to not being processed and memleaked multimap is used instead of map
	 * for incoming queue.
	 */
	typedef std::unordered_multimap<RsGxsMailId, RsGxsMailBaseItem*> inMap;
	inMap ingoingQueue;
	RsMutex ingoingMutex;

	/// @see GxsTokenQueue::handleResponse(uint32_t token, uint32_t req_type)
	virtual void handleResponse(uint32_t token, uint32_t req_type);

	/// @see RsGenExchange::service_tick()
	virtual void service_tick();

	/// @see RsGenExchange::service_CreateGroup(...)
	RsGenExchange::ServiceCreate_Return service_CreateGroup(
	        RsGxsGrpItem* grpItem, RsTlvSecurityKeySet& );

	/// @see RsGenExchange::notifyChanges(std::vector<RsGxsNotify *> &changes)
	void notifyChanges(std::vector<RsGxsNotify *> &changes);

	/// @see p3Config::setupSerialiser()
	virtual RsSerialiser* setupSerialiser();

	/// @see p3Config::saveList(bool &cleanup, std::list<RsItem *>&)
	virtual bool saveList(bool &cleanup, std::list<RsItem *>&saveList);

	/// @see p3Config::saveDone()
	void saveDone();

	/// @see p3Config::loadList(std::list<RsItem *>&)
	virtual bool loadList(std::list<RsItem *>& loadList);

	/// Request groups list to GXS backend. Async method.
	bool requestGroupsData(const std::list<RsGxsGroupId>* groupIds = NULL);

	/**
	 * Check if current preferredGroupId is the best against potentialGrId, if
	 * the passed one is better update it.
	 * Useful when GXS backend notifies groups changes, or when a reponse to an
	 * async grop request (@see GXS_REQUEST_TYPE_GROUP_*) is received.
	 * @return true if preferredGroupId has been supeseded by potentialGrId
	 *   false otherwise.
	 */
	bool inline supersedePreferredGroup(const RsGxsGroupId& potentialGrId)
	{
		if(preferredGroupId < potentialGrId)
		{
			std::cerr << "supersedePreferredGroup(...) " << potentialGrId
			          << " supersed " << preferredGroupId << std::endl;
			preferredGroupId = potentialGrId;
			return true;
		}
		return false;
	}

	/** @return true if has passed more then interval seconds between timeStamp
	 * and ref. @param ref by default now is taked as reference. */
	bool static inline olderThen(time_t timeStamp, int32_t interval,
	                             time_t ref = time(NULL))
	{ return (timeStamp + interval) < ref; }


	/// Decrypt email content and pass it to dispatchDecryptedMail(...)
	bool handleEcryptedMail(const RsGxsMailItem* mail);

	/// Dispatch the message to the recipient service
	bool dispatchDecryptedMail( const RsGxsMailItem* received_msg,
	                            const uint8_t* decrypted_data,
	                            uint32_t decrypted_data_size );

	void notifyClientService(const OutgoingRecord& pr);
};

#ifdef TEST_RS_GXS_MAIL

struct TestGxsMailClientService : GxsMailsClient, RsSingleJobThread
{
	TestGxsMailClientService( p3GxsMails& gxsMailService,
	                          p3IdService& gxsIdService ) :
	    mailService(gxsMailService), idService(gxsIdService)
	{
		mailService.registerGxsMailsClient( GxsMailSubServices::TEST_SERVICE,
		                                    this );
	}

	/// @see GxsMailsClient::receiveGxsMail(...)
	virtual bool receiveGxsMail( const RsGxsMailItem& originalMessage,
	                             const uint8_t* data, uint32_t dataSize )
	{
		std::cout << "TestGxsMailClientService::receiveGxsMail(...) got message"
		          << " from: " << originalMessage.meta.mAuthorId << std::endl
		          << "\t>" << std::string((char*)data, dataSize) << "<"
		          << std::endl;
		return true;
	}

	/// @see GxsMailsClient::notifyMailStatus(...)
	virtual bool notifySendMailStatus( const RsGxsMailItem& originalMessage,
	                                GxsMailStatus status )
	{
		std::cout << "TestGxsMailClientService::notifyMailsStatus(...) for: "
		          << originalMessage.mailId << " status: "
		          << static_cast<uint>(status) << std::endl;
		if( status == GxsMailStatus::RECEIPT_RECEIVED )
			std::cout << "\t It mean Receipt has been Received!" << std::endl;
		return true;
	}

	/// @see RsSingleJobThread::run()
	virtual void run()
	{
#if 0
		usleep(10*1000*1000);
		RsGxsId gxsidA("d0df7474bdde0464679e6ef787890287");
		RsGxsId gxsidB("d060bea09dfa14883b5e6e517eb580cd");
		RsGxsMailId mailId = 0;
		if(idService.isOwnId(gxsidA))
		{
			std::string ciao("CiAone!");
			mailService.sendMail( mailId, GxsMailSubServices::TEST_SERVICE,
			                      gxsidA, gxsidB,
			                      reinterpret_cast<const uint8_t*>(ciao.data()),
			                      ciao.size() );
		}
		else if(idService.isOwnId(gxsidB))
		{
			std::string ciao("CiBuono!");
			mailService.sendMail( mailId, GxsMailSubServices::TEST_SERVICE,
			                      gxsidB, gxsidA,
			                      reinterpret_cast<const uint8_t*>(ciao.data()),
			                      ciao.size() );
		}
#endif
	}

private:
	p3GxsMails& mailService;
	p3IdService& idService;
};

#endif // TEST_RS_GXS_MAIL

