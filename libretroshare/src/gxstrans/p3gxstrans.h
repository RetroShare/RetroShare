/*******************************************************************************
 * RetroShare GxsTrans asyncronous redundant small mail trasport on top of GXS *
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
#pragma once

#include <stdint.h>
#include <unordered_map>
#include <map>

#include "retroshare/rsgxsifacetypes.h" // For RsGxsId, RsGxsCircleId
#include "gxs/gxstokenqueue.h" // For GxsTokenQueue
#include "gxstrans/p3gxstransitems.h"
#include "services/p3idservice.h" // For p3IdService
#include "util/rsthreads.h"
#include "retroshare/rsgxstrans.h"

class p3GxsTrans;

/// Services who want to make use of p3GxsTrans should inherit this struct
struct GxsTransClient
{
	/**
	 * This will be called by p3GxsTrans to dispatch mails to the subservice
	 * @param authorId message sender
	 * @param decryptId recipient id
	 * @param data buffer containing the decrypted data
	 * @param dataSize size of the buffer
	 * @return true if dispatching goes fine, false otherwise
	 */
	virtual bool receiveGxsTransMail( const RsGxsId& authorId,
	                                  const RsGxsId& recipientId,
	                                  const uint8_t* data, uint32_t dataSize
	                                  ) = 0;

	/**
	 * This will be called by p3GxsTrans to notify the subservice about the
	 * status of a sent email.
	 * @param originalMessage message for with the notification is made
	 * @param status the new status of the message
	 * @return true if notification goes fine, false otherwise (ignored ATM)
	 */
	virtual bool notifyGxsTransSendStatus( RsGxsTransId mailId,
	                                       GxsTransSendStatus status ) = 0;
};

struct MsgSizeCount
{
	MsgSizeCount() : size(0),count(0) {}

	uint32_t size ;
	uint32_t count ;
};

/**
 * @brief p3GxsTrans is a mail delivery service based on GXS.
 * p3GxsTrans is capable of asynchronous mail delivery and acknowledgement.
 * p3GxsTrans is meant to be capable of multiple encryption options,
 * @see RsGxsTransEncryptionMode at moment messages are encrypted using RSA
 * unless the user ask for them being sent in clear text ( this is not supposed
 * to happen in non testing environment so warnings and stack traces are printed
 * in the log if an attempt to send something in clear text is made ).
 * p3GxsTrans try to hide metadata so the travelling message signed by the author
 * but the recipient is not disclosed, instead to avoid everyone trying to
 * decrypt every message a hint has been introduced, the hint is calculated in a
 * way that one can easily prove that a message is not destined to someone, but
 * cannot prove the message is destined to someone
 * @see RsGxsTransMailItem::recipientHint for more details.
 * p3GxsTrans expose a simple API to send and receive mails, the API also
 * provide notification for the sending mail status @see sendMail(...),
 * @see querySendStatus(...), @see registerGxsTransClient(...),
 * @see GxsTransClient::receiveGxsTransMail(...),
 * @see GxsTransClient::notifyGxsTransSendStatus(...).
 */
class p3GxsTrans : public RsGenExchange, public GxsTokenQueue, public p3Config, public RsGxsTrans
{
public:
	p3GxsTrans( RsGeneralDataService* gds, RsNetworkExchangeService* nes,
	            p3IdService& identities ) :
	    RsGenExchange( gds, nes, new RsGxsTransSerializer(),
	                   RS_SERVICE_TYPE_GXS_TRANS, &identities,
	                   AuthenPolicy()),
	    GxsTokenQueue(this),
	    RsGxsTrans(static_cast<RsGxsIface&>(*this)),
	    // always check 30 secs after start)
	    mLastMsgCleanup(time(NULL) - MAX_DELAY_BETWEEN_CLEANUPS + 30),
	    mIdService(identities),
	    mServClientsMutex("p3GxsTrans client services map mutex"),
	    mOutgoingMutex("p3GxsTrans outgoing queue map mutex"),
	    mIngoingMutex("p3GxsTrans ingoing queue map mutex"),
	    mCleanupThread(nullptr),
	    mPerUserStatsMutex("p3GxsTrans user stats mutex"),
	    mDataMutex("p3GxsTrans data mutex") {}

	virtual ~p3GxsTrans();

    /*!
     * \brief getStatistics
     * 				Gathers all sorts of statistics about the internals of p3GxsTrans, in order to display info about the running status,
     * 			message transport, etc.
     * \param stats This structure contains all statistics information.
     * \return 		true is the call succeeds.
     */

	virtual bool getStatistics(GxsTransStatistics& stats);

	/**
	 * Send an email to recipient, in the process author of the email is
	 * disclosed to the network (because the sent GXS item is signed), while
	 * recipient is not @see RsGxsTransMailItem::recipientHint for details on
	 * recipient protection.
	 * This method is part of the public interface of this service.
	 * @return true if the mail will be sent, false if not
	 */
	bool sendData( RsGxsTransId& mailId,
	               GxsTransSubServices service,
	               const RsGxsId& own_gxsid, const RsGxsId& recipient,
	               const uint8_t* data, uint32_t size,
	               RsGxsTransEncryptionMode cm = RsGxsTransEncryptionMode::RSA
	        );

	/**
	 * This method is part of the public interface of this service.
	 * @return false if mail is not found in outgoing queue, true otherwise
	 */
	bool querySendStatus( RsGxsTransId mailId, GxsTransSendStatus& st );

	/**
	 * Register a client service to p3GxsTrans to receive mails via
	 * GxsTransClient::receiveGxsTransMail(...) callback
	 * This method is part of the public interface of this service.
	 */
	void registerGxsTransClient( GxsTransSubServices serviceType,
	                             GxsTransClient* service );

	/// @see RsGenExchange::getServiceInfo()
	virtual RsServiceInfo getServiceInfo() { return RsServiceInfo( RS_SERVICE_TYPE_GXS_TRANS, "GXS Mails", 0, 1, 0, 1 ); }

	static const uint32_t GXS_STORAGE_PERIOD = 15*86400;	// 15 days.
	static const uint32_t GXS_SYNC_PERIOD    = 15*86400;
private:
	/** Time interval of inactivity before a distribution group is unsubscribed.
	 * Approximatively 3 months seems ok ATM. */
	const static int32_t UNUSED_GROUP_UNSUBSCRIBE_INTERVAL = 16*86400; // 16 days

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
	static const uint32_t MAX_DELAY_BETWEEN_CLEANUPS ; // every 20 mins. Could be less.

    rstime_t mLastMsgCleanup ;

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
	RsGxsGroupId mPreferredGroupId;

	/// Used for items {de,en}cryption
	p3IdService& mIdService;

	/// Stores pointers to client services to notify them about new mails
	std::map<GxsTransSubServices, GxsTransClient*> mServClients;
	RsMutex mServClientsMutex;

	/**
	 * @brief Keep track of outgoing mails.
	 * Records enter the queue when a mail is sent, and are removed when a
	 * receipt has been received or sending is considered definetly failed.
	 * Items are saved in config for consistence accross RetroShare shutdowns.
	 */
	typedef std::map<RsGxsTransId, OutgoingRecord> prMap;
	prMap mOutgoingQueue;
	RsMutex mOutgoingMutex;
	void locked_processOutgoingRecord(OutgoingRecord& r);

	/**
	 * @brief Ingoing mail and receipt processing queue.
	 * At shutdown remaining items are saved in config and then deleted in
	 * destructor for consistence accross RetroShare instances.
	 * In order to avoid malicious messages ( non malicious collision has 1/2^64
	 * probablity ) to smash items in the queue thus causing previous incoming
	 * item to not being processed and memleaked multimap is used instead of map
	 * for incoming queue.
	 */
	typedef std::unordered_multimap<RsGxsTransId, RsGxsTransBaseMsgItem*> inMap;
	inMap mIncomingQueue;
	RsMutex mIngoingMutex;

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
	bool inline locked_supersedePreferredGroup(const RsGxsGroupId& potentialGrId)
	{
		if(mPreferredGroupId < potentialGrId)
		{
			std::cerr << "supersedePreferredGroup(...) " << potentialGrId
			          << " supersed " << mPreferredGroupId << std::endl;
			mPreferredGroupId = potentialGrId;
			return true;
		}
		return false;
	}

	/** @return true if has passed more then interval seconds between timeStamp
	 * and ref. @param ref by default now is taked as reference. */
	bool static inline olderThen(rstime_t timeStamp, int32_t interval,
	                             rstime_t ref = time(NULL))
	{ return (timeStamp + interval) < ref; }


	/// Decrypt email content and pass it to dispatchDecryptedMail(...)
	bool handleEncryptedMail(const RsGxsTransMailItem* mail);

	/// Dispatch the message to the recipient service
	bool dispatchDecryptedMail( const RsGxsId& authorId,
	                            const RsGxsId& decryptId,
	                            const uint8_t* decrypted_data,
	                            uint32_t decrypted_data_size );

	void notifyClientService(const OutgoingRecord& pr);

	/*!
	 * Checks the integrity message and groups
	 */
	class GxsTransIntegrityCleanupThread : public RsSingleJobThread
	{
		enum CheckState { CheckStart, CheckChecking };

	public:
        explicit GxsTransIntegrityCleanupThread(RsGeneralDataService *const dataService): mDs(dataService),mMtx("GxsTransIntegrityCheck") { mDone=false;}

		bool isDone();
		void run();

		void getDeletedIds(std::list<RsGxsGroupId>& grpIds, std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >& msgIds);

		void getMessagesToDelete(GxsMsgReq& req) ;
		void getPerUserStatistics(std::map<RsGxsId,MsgSizeCount>& m) ;

	private:
		RsGeneralDataService* const mDs;
		RsMutex mMtx ;

		GxsMsgReq mMsgToDel ;
		std::map<RsGxsId,MsgSizeCount> total_message_size_and_count;
        bool mDone ;
	};

	// Overloaded from RsGenExchange.

	bool acceptNewMessage(const RsGxsMsgMetaData *msgMeta, uint32_t size) ;

    GxsTransIntegrityCleanupThread *mCleanupThread ;

	// statistics of the load across all groups, per user.

	RsMutex mPerUserStatsMutex;
	std::map<RsGxsId,MsgSizeCount> per_user_statistics ;

	// Mutex to protect local data

	RsMutex mDataMutex;
};

