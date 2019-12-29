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
#include <deque>

#include "retroshare/rsevents.h"
#include "util/rsthreads.h"
#include "util/rsdebug.h"

class RsEventsService :
        public RsEvents, public RsTickingThread
{
public:
	RsEventsService():
	    mHandlerMapMtx("RsEventsService::mHandlerMapMtx"), mLastHandlerId(1),
        mHandlerMaps(static_cast<int>(RsEventType::MAX)),
	    mEventQueueMtx("RsEventsService::mEventQueueMtx") {}

	/// @see RsEvents
	bool postEvent(
	        std::shared_ptr<const RsEvent> event,
	        std::string& errorMessage = RS_DEFAULT_STORAGE_PARAM(std::string)
	        ) override;

	/// @see RsEvents
	bool sendEvent(
	        std::shared_ptr<const RsEvent> event,
	        std::string& errorMessage = RS_DEFAULT_STORAGE_PARAM(std::string)
	        ) override;

	/// @see RsEvents
	RsEventsHandlerId_t generateUniqueHandlerId() override;

	/// @see RsEvents
	bool registerEventsHandler(
            RsEventType eventType,
	        std::function<void(std::shared_ptr<const RsEvent>)> multiCallback,
	        RsEventsHandlerId_t& hId = RS_DEFAULT_STORAGE_PARAM(RsEventsHandlerId_t, 0)
	        ) override;

	/// @see RsEvents
	bool unregisterEventsHandler(RsEventsHandlerId_t hId) override;

protected:
	RsMutex mHandlerMapMtx;
	RsEventsHandlerId_t mLastHandlerId;

    std::vector<
		std::map<
		    RsEventsHandlerId_t,
		    std::function<void(std::shared_ptr<const RsEvent>)> > > mHandlerMaps;

	RsMutex mEventQueueMtx;
	std::deque< std::shared_ptr<const RsEvent> > mEventQueue;

	/// @see RsTickingThread
	void data_tick() override;

	void handleEvent(std::shared_ptr<const RsEvent> event);
	RsEventsHandlerId_t generateUniqueHandlerId_unlocked();

	RS_SET_CONTEXT_DEBUG_LEVEL(3)
};
