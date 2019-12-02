/*******************************************************************************
 * Retroshare events service                                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2019  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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

#include <memory>
#include <cstdint>

#include "util/rsmemory.h"
#include "util/rsurl.h"
#include "serialiser/rsserializable.h"
#include "serialiser/rstypeserializer.h"

class RsEvents;

/**
 * Pointer to global instance of RsEvents service implementation
 * @jsonapi{development}
 *
 * TODO: this should become std::weak_ptr once we have a reasonable services
 * management.
 */
extern RsEvents* rsEvents;

/**
 * @brief Events types.
 * When creating a new type of event, add a new type here and use that to
 * initialize mType in the constructor of your derivative of @see RsEvent
 */
enum class RsEventType : uint32_t
{
	NONE = 0, /// Used to detect uninitialized event

	/// @see RsBroadcastDiscovery
	BROADCAST_DISCOVERY_PEER_FOUND                          = 1,

	/// @see RsDiscPendingPgpReceivedEvent
	GOSSIP_DISCOVERY_INVITE_RECEIVED                        = 2,

	/// @see AuthSSL
	AUTHSSL_CONNECTION_AUTENTICATION                        = 3,

	/// @see pqissl
	PEER_CONNECTION                                         = 4,

	/// @see RsGxsChanges
	GXS_CHANGES                                             = 5,

	/// Emitted when a peer state changes, @see RsPeers
	PEER_STATE_CHANGED                                      = 6,

	/// @see RsMailStatusEvent
	MAIL_STATUS_CHANGE                                      = 7,

	MAX       /// Used to detect invalid event type passed
};

/**
 * This struct is not meant to be used directly, you should create events type
 * deriving from it.
 */
struct RsEvent : RsSerializable
{
protected:
	RsEvent(RsEventType type) :
	    mType(type), mTimePoint(std::chrono::system_clock::now()) {}

	RsEvent() = delete;

public:
	RsEventType mType;
	std::chrono::system_clock::time_point mTimePoint;

	/**
	 * Derived types must call this method at beginning of their implementation
	 * of serial_process
	 * @see RsSerializable
	 */
	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx) override
	{
		RS_SERIAL_PROCESS(mType);

		rstime_t mTime = std::chrono::system_clock::to_time_t(mTimePoint);
		RS_SERIAL_PROCESS(mTime);
		mTimePoint = std::chrono::system_clock::from_time_t(mTime);
	}

	~RsEvent() override;
};

typedef uint32_t RsEventsHandlerId_t;

class RsEvents
{
public:
	/**
	 * @brief Post event to the event queue.
	 * @param[in] event
	 * @param[out] errorMessage Optional storage for error messsage, meaningful
	 *                          only on failure.
	 * @return False on error, true otherwise.
	 */
	virtual bool postEvent(
	        std::shared_ptr<const RsEvent> event,
	        std::string& errorMessage = RS_DEFAULT_STORAGE_PARAM(std::string)
	        ) = 0;

	/**
	 * @brief Send event directly to handlers. Blocking API
	 * The handlers get exectuded on the caller thread.
	 * @param[in] event
	 * @param[out] errorMessage Optional storage for error messsage, meaningful
	 *                          only on failure.
	 * @return False on error, true otherwise.
	 */
	virtual bool sendEvent(
	        std::shared_ptr<const RsEvent> event,
	        std::string& errorMessage = RS_DEFAULT_STORAGE_PARAM(std::string)
	        ) = 0;

	/**
	 * @brief Generate unique handler identifier
	 * @return generate Id
	 */
	virtual RsEventsHandlerId_t generateUniqueHandlerId() = 0;

	/**
	 * @brief Register events handler
	 * Every time an event is dispatced the registered events handlers will get
	 * their method handleEvent called with the event passed as paramether.
	 * @jsonapi{development,manualwrapper}
	 * @param multiCallback     Function that will be called each time an event
	 *                          is dispatched.
	 * @param[inout] hId        Optional storage for handler id, useful to
	 *                          eventually unregister the handler later. The
	 *                          value may be provided to the function call but
	 *                          must habe been generated with
	 *                          @see generateUniqueHandlerId()
	 * @return False on error, true otherwise.
	 */
	virtual bool registerEventsHandler(
	        std::function<void(std::shared_ptr<const RsEvent>)> multiCallback,
	        RsEventsHandlerId_t& hId = RS_DEFAULT_STORAGE_PARAM(RsEventsHandlerId_t, 0)
	        ) = 0;

	/**
	 * @brief Unregister event handler
	 * @param[in] hId Id of the event handler to unregister
	 * @return True if the handler id has been found, false otherwise.
	 */
	virtual bool unregisterEventsHandler(RsEventsHandlerId_t hId) = 0;

	virtual ~RsEvents();
};

//===================================================================================================//
//                               Connexion and security events                                       //
//===================================================================================================//

/**
 * Event triggered by AuthSSL when authentication of a connection attempt either
 * fail or success
 */
struct RsAuthSslConnectionAutenticationEvent : RsEvent
{
	RsAuthSslConnectionAutenticationEvent() : RsEvent(RsEventType::AUTHSSL_CONNECTION_AUTENTICATION) {}

    enum ConnectionErrorCode: uint8_t {
        NO_ERROR                        = 0x00,
        MISSING_AUTHENTICATION_INFO     = 0x01,
        PGP_SIGNATURE_VALIDATION_FAILED = 0x02,
        MISMATCHED_PGP_ID               = 0x03,
        NO_CERTIFICATE_SUPPLIED         = 0x04,
        NOT_A_FRIEND                    = 0x05,
        MISSING_CERTIFICATE             = 0x06,
        IP_IS_BLACKLISTED               = 0x07,
        UNKNOWN_ERROR                   = 0x08,
    };

	RsPeerId mSslId;
	std::string mSslCn;
	RsPgpId mPgpId;
    RsUrl mLocator;
	std::string mErrorMsg;
	ConnectionErrorCode mErrorCode;

	///* @see RsEvent @see RsSerializable
	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx) override
	{
		RsEvent::serial_process(j, ctx);
		RS_SERIAL_PROCESS(mSslId);
		RS_SERIAL_PROCESS(mSslCn);
		RS_SERIAL_PROCESS(mPgpId);
		RS_SERIAL_PROCESS(mLocator);
		RS_SERIAL_PROCESS(mErrorMsg);
		RS_SERIAL_PROCESS(mErrorCode);
	}
};

struct RsConnectionEvent : RsEvent
{
	RsConnectionEvent()
	    : RsEvent(RsEventType::PEER_CONNECTION),
	      mType(UNKNOWN) {}

	enum ConnectionType: uint8_t {
        UNKNOWN                 = 0x00,
        PEER_CONNECTED          = 0x01,
        PEER_DISCONNECTED       = 0x02,
        PEER_REFUSED_CONNECTION = 0x03,
    };

    ConnectionType mType;
	RsPeerId mSslId;

	///* @see RsEvent @see RsSerializable
	void serial_process( RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx) override
	{
		RsEvent::serial_process(j, ctx);
		RS_SERIAL_PROCESS(mType);
		RS_SERIAL_PROCESS(mSslId);
	}
};

