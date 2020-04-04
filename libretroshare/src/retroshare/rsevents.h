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
#include <chrono>
#include <functional>

#include "util/rsmemory.h"
#include "util/rsurl.h"
#include "serialiser/rsserializable.h"
#include "serialiser/rstypeserializer.h"
#include "util/rstime.h"

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
	__NONE = 0, /// Used internally to detect invalid event type passed

	/// @see RsBroadcastDiscovery
	BROADCAST_DISCOVERY                                     = 1,

	/// @see RsDiscPendingPgpReceivedEvent
	GOSSIP_DISCOVERY                                        = 2,

	/// @see AuthSSL
	AUTHSSL_CONNECTION_AUTENTICATION                        = 3,

	/// @see pqissl
	PEER_CONNECTION                                         = 4,

	/// @see RsGxsChanges, used also in @see RsGxsBroadcast
	GXS_CHANGES                                             = 5,

	/// Emitted when a peer state changes, @see RsPeers
	PEER_STATE_CHANGED                                      = 6,

	/// @see RsMailStatusEvent
	MAIL_STATUS                                             = 7,

    /// @see RsGxsCircleEvent
    GXS_CIRCLES                                             = 8,

    /// @see RsGxsChannelEvent
    GXS_CHANNELS                                            = 9,

    /// @see RsGxsForumEvent
    GXS_FORUMS                                              = 10,

    /// @see RsGxsPostedEvent
    GXS_POSTED                                              = 11,

    /// @see RsGxsPostedEvent
    GXS_IDENTITY                                            = 12,

    /// @see RsFiles
    SHARED_DIRECTORIES                                      = 13,

    /// @see RsFiles
    FILE_TRANSFER                                           = 14,

	/// @see RsMsgs
	CHAT_MESSAGE                                            = 15,

	__MAX /// Used internally to detect invalid event type passed
};

/**
 * This struct is not meant to be used directly, you should create events type
 * deriving from it.
 */
struct RsEvent : RsSerializable
{
protected:
	explicit RsEvent(RsEventType type) :
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
	 * @attention Callbacks must not fiddle internally with methods of this
	 * class otherwise a deadlock will happen.
	 * @jsonapi{development,manualwrapper}
	 * @param multiCallback     Function that will be called each time an event
	 *                          is dispatched.
	 * @param[inout] hId        Optional storage for handler id, useful to
	 *                          eventually unregister the handler later. The
	 *                          value may be provided to the function call but
	 *                          must habe been generated with
	 *                          @see generateUniqueHandlerId()
	 * @param[in] eventType     Optional type of event for which the callback is
	 *                          called, if __NONE is passed multiCallback is
	 *                          called for every events without filtering.
	 * @return False on error, true otherwise.
	 */
	virtual bool registerEventsHandler(
	        std::function<void(std::shared_ptr<const RsEvent>)> multiCallback,
	        RsEventsHandlerId_t& hId = RS_DEFAULT_STORAGE_PARAM(RsEventsHandlerId_t, 0),
	        RsEventType eventType = RsEventType::__NONE ) = 0;

	/**
	 * @brief Unregister event handler
	 * @param[in] hId Id of the event handler to unregister
	 * @return True if the handler id has been found, false otherwise.
	 */
	virtual bool unregisterEventsHandler(RsEventsHandlerId_t hId) = 0;

	virtual ~RsEvents();
};
