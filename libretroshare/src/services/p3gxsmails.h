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

#include "retroshare/rsgxsifacetypes.h" // For RsGxsId, RsGxsCircleId
#include "gxs/gxstokenqueue.h" // For GxsTokenQueue
#include "serialiser/rsgxsmailitems.h" // For RS_SERVICE_TYPE_GXS_MAIL
#include "services/p3idservice.h" // For p3IdService

struct p3GxsMails;

struct GxsMailsClient
{
	/// Subservices identifiers (like port for TCP)
	enum GxsMailSubServices { MSG_SERVICE };

	/**
	 * This is usually used to save a pointer to the p3GxsMails service (e.g. by
	 * coping it in a member variable), so as to be able to send mails, and to
	 * register as a mail receiver via
	 * p3GxsMails::registerGxsMailsClient(GxsMailSubServices, GxsMailsClient)
	 */
	//virtual void connectToGxsMails(p3GxsMails *pt) = 0 ;

	/**
	 * This will be called by p3GxsMails to dispatch mails to the subservice
	 * @param originalMessage message as received from GXS backend (encrypted)
	 *   GxsMailsClient take ownership of it ( aka should take free/delete it
	 *   when not needed anymore )
	 * @param data buffer containing the decrypted data
	 *   GxsMailsClient take ownership of it ( aka should take free/delete it
	 *   when not needed anymore )
	 * @param dataSize size of the buffer
	 * @return true if dispatching goes fine, false otherwise
	 */
	virtual bool receiveGxsMail( RsGxsMailItem* originalMessage,
	                             uint8_t* data, uint32_t dataSize ) = 0;
};

struct p3GxsMails : RsGenExchange, GxsTokenQueue
{
	p3GxsMails( RsGeneralDataService* gds, RsNetworkExchangeService* nes,
	            p3IdService& identities ) :
	    RsGenExchange( gds, nes, new RsGxsMailSerializer(),
	                   RS_SERVICE_TYPE_GXS_MAIL, &identities,
	                   AuthenPolicy(),
	                   RS_GXS_DEFAULT_MSG_STORE_PERIOD ), // TODO: Discuss with Cyril about this
	    GxsTokenQueue(this), idService(identities),
	    servClientsMutex("p3GxsMails client services map mutex") {}

	/**
	 * Send an email to recipient, in the process author of the email is
	 * disclosed to the network (because the sent GXS item is signed), while
	 * recipient is not @see RsGxsMailBaseItem::recipientsHint for details on
	 * recipient protection.
	 * This method is part of the public interface of this service.
	 * @return true if the mail will be sent, false if not
	 */
	bool sendMail( GxsMailsClient::GxsMailSubServices service,
	               const RsGxsId& own_gxsid, const RsGxsId& recipient,
	               const uint8_t* data, uint32_t size,
	               RsGxsMailBaseItem::EncryptionMode cm = RsGxsMailBaseItem::RSA
	        );

	/**
	 * Send an email to recipients, in the process author of the email is
	 * disclosed to the network (because the sent GXS item is signed), while
	 * recipients are not @see RsGxsMailBaseItem::recipientsHint for details on
	 * recipient protection.
	 * This method is part of the public interface of this service.
	 * @return true if the mail will be sent, false if not
	 */
	bool sendMail( GxsMailsClient::GxsMailSubServices service,
	               const RsGxsId& own_gxsid,
	               const std::vector<const RsGxsId*>& recipients,
	               const uint8_t* data, uint32_t size,
	               RsGxsMailBaseItem::EncryptionMode cm = RsGxsMailBaseItem::RSA
	        );

	/**
	 * Register a client service to p3GxsMails to receive mails via
	 * GxsMailsClient::receiveGxsMail(...) callback
	 */
	void registerGxsMailsClient( GxsMailsClient::GxsMailSubServices serviceType,
	                             GxsMailsClient* service );

	/**
	 * @see GxsTokenQueue::handleResponse(uint32_t token, uint32_t req_type)
	 */
	virtual void handleResponse(uint32_t token, uint32_t req_type);

	/// @see RsGenExchange::service_tick()
	virtual void service_tick();

	/// @see RsGenExchange::getServiceInfo()
	virtual RsServiceInfo getServiceInfo() { return RsServiceInfo( RS_SERVICE_TYPE_GXS_MAIL, "GXS Mails", 0, 1, 0, 1 ); }

	/// @see RsGenExchange::service_CreateGroup(...)
	RsGenExchange::ServiceCreate_Return service_CreateGroup(RsGxsGrpItem* grpItem, RsTlvSecurityKeySet&);

protected:
	/// @see RsGenExchange::notifyChanges(std::vector<RsGxsNotify *> &changes)
	void notifyChanges(std::vector<RsGxsNotify *> &changes);

private:
	/// Time interval of inactivity before a distribution group is unsubscribed
	const static int32_t UNUSED_GROUP_UNSUBSCRIBE_INTERVAL = 0x76A700; // 3 months approx

	/// @brief AuthenPolicy check nothing ATM TODO talk with Cyril how this should be
	static uint32_t AuthenPolicy() { return 0; }

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
	std::map<GxsMailsClient::GxsMailSubServices, GxsMailsClient*> servClients;
	RsMutex servClientsMutex;


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
		//std::cout << "supersedePreferredGroup(const RsGxsGroupId& potentialGrId) " << preferredGroupId << " <? " << potentialGrId << std::endl;
		if(preferredGroupId < potentialGrId)
		{
			std::cerr << "supersedePreferredGroup(...) " << potentialGrId
			          << " supersed " << preferredGroupId << std::endl;
			preferredGroupId = potentialGrId;
			return true;
		}
		return false;
	}

	/// @return true if has passed more then interval seconds time since timeStamp
	bool static inline olderThen(time_t timeStamp, int32_t interval)
	{ return (timeStamp + interval) < time(NULL); }


	/// Decrypt email content and pass it to dispatchDecryptedMail(...)
	bool handleEcryptedMail(RsGxsMailItem* mail);

	/// Dispatch the message to the recipient service
	bool dispatchDecryptedMail( RsGxsMailItem* received_msg,
	                            uint8_t* decrypted_data,
	                            uint32_t decrypted_data_size );
};

