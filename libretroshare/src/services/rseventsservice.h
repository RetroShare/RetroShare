/*******************************************************************************
 * Retroshare events service                                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2019-2020  Gioacchino Mazzurco <gio@eigenlab.org>             *
 * Copyright (C) 2020  Asociación Civil Altermundi <info@altermundi.net>       *
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
#include <deque>
#include <array>

#include "retroshare/rsevents.h"
#include "util/rsthreads.h"
#include "util/rsdebug.h"

class RsEventsService :
        public RsEvents, public RsTickingThread
{
public:
	RsEventsService():
	    mHandlerMapMtx("RsEventsService::mHandlerMapMtx"), mLastHandlerId(1),
	    mEventQueueMtx("RsEventsService::mEventQueueMtx") {}

	/// @see RsEvents
	std::error_condition postEvent(
	        std::shared_ptr<const RsEvent> event ) override;

	/// @see RsEvents
	std::error_condition sendEvent(
	        std::shared_ptr<const RsEvent> event ) override;

	/// @see RsEvents
	RsEventsHandlerId_t generateUniqueHandlerId() override;

	/// @see RsEvents
	std::error_condition registerEventsHandler(
	        std::function<void(std::shared_ptr<const RsEvent>)> multiCallback,
	        RsEventsHandlerId_t& hId = RS_DEFAULT_STORAGE_PARAM(RsEventsHandlerId_t, 0),
	        RsEventType eventType = RsEventType::__NONE ) override;

	/// @see RsEvents
	std::error_condition unregisterEventsHandler(
	        RsEventsHandlerId_t hId ) override;

protected:
	std::error_condition isEventTypeInvalid(RsEventType eventType);
	std::error_condition isEventInvalid(std::shared_ptr<const RsEvent> event);

	RsMutex mHandlerMapMtx;
	RsEventsHandlerId_t mLastHandlerId;

	/** Storage for event handlers, keep 10 extra types for plugins that might
	 * be released indipendently */
	std::array<
	    std::map<
	        RsEventsHandlerId_t,
	        std::function<void(std::shared_ptr<const RsEvent>)> >,
	    static_cast<std::size_t>(RsEventType::__MAX) + 10
	> mHandlerMaps;

	RsMutex mEventQueueMtx;
	std::deque< std::shared_ptr<const RsEvent> > mEventQueue;

	void threadTick() override; /// @see RsTickingThread

	void handleEvent(std::shared_ptr<const RsEvent> event);
	RsEventsHandlerId_t generateUniqueHandlerId_unlocked();

	RS_SET_CONTEXT_DEBUG_LEVEL(3)
};
