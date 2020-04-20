/*******************************************************************************
 * Retroshare events service                                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2019-2020  Gioacchino Mazzurco <gio@eigenlab.org>             *
 * Copyright (C) 2019-2020  Retroshare Team <contact@retroshare.cc>            *
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

#include <string>
#include <thread>

#include "services/rseventsservice.h"


/*extern*/ RsEvents* rsEvents = nullptr;

RsEvent::~RsEvent() = default;
RsEvents::~RsEvents() = default;

/*static*/ const RsEventsErrorCategory RsEventsErrorCategory::instance;

std::error_condition RsEventsErrorCategory::default_error_condition(int ev)
const noexcept
{
	switch(static_cast<RsEventsErrorNum>(ev))
	{
	case RsEventsErrorNum::INVALID_HANDLER_ID: // [[fallthrough]];
	case RsEventsErrorNum::NULL_EVENT_POINTER: // [[fallthrough]];
	case RsEventsErrorNum::EVENT_TYPE_UNDEFINED: // [[fallthrough]];
	case RsEventsErrorNum::EVENT_TYPE_OUT_OF_RANGE:
		return std::errc::invalid_argument;
	default:
		return std::error_condition(ev, *this);
	}
}

std::error_condition RsEventsService::isEventTypeInvalid(RsEventType eventType)
{
	if(eventType == RsEventType::__NONE)
		return RsEventsErrorNum::EVENT_TYPE_UNDEFINED;

	if( eventType < RsEventType::__NONE ||
	        eventType >= static_cast<RsEventType>(mHandlerMaps.size()) )
		return RsEventsErrorNum::EVENT_TYPE_OUT_OF_RANGE;

	return std::error_condition();
}

std::error_condition RsEventsService::isEventInvalid(
        std::shared_ptr<const RsEvent> event)
{
	if(!event) return RsEventsErrorNum::NULL_EVENT_POINTER;
	return isEventTypeInvalid(event->mType);
}

std::error_condition RsEventsService::postEvent(
        std::shared_ptr<const RsEvent> event )
{
	if(std::error_condition ec = isEventInvalid(event)) return ec;

	RS_STACK_MUTEX(mEventQueueMtx);
	mEventQueue.push_back(event);
	return std::error_condition();
}

std::error_condition RsEventsService::sendEvent(
        std::shared_ptr<const RsEvent> event )
{
	if(std::error_condition ec = isEventInvalid(event)) return ec;
	handleEvent(event);
	return std::error_condition();
}

RsEventsHandlerId_t RsEventsService::generateUniqueHandlerId()
{
	RS_STACK_MUTEX(mHandlerMapMtx);
	return generateUniqueHandlerId_unlocked();
}

RsEventsHandlerId_t RsEventsService::generateUniqueHandlerId_unlocked()
{
	if(++mLastHandlerId) return mLastHandlerId; // Avoid 0 after overflow
	return 1;
}

std::error_condition RsEventsService::registerEventsHandler(
        std::function<void(std::shared_ptr<const RsEvent>)> multiCallback,
        RsEventsHandlerId_t& hId, RsEventType eventType )
{
	RS_STACK_MUTEX(mHandlerMapMtx);

	if(eventType != RsEventType::__NONE)
		if(std::error_condition ec = isEventTypeInvalid(eventType))
			return ec;

	if(!hId) hId = generateUniqueHandlerId_unlocked();
	else if (hId > mLastHandlerId)
	{
		print_stacktrace();
		return RsEventsErrorNum::INVALID_HANDLER_ID;
	}

	mHandlerMaps[static_cast<std::size_t>(eventType)][hId] = multiCallback;
	return std::error_condition();
}

std::error_condition RsEventsService::unregisterEventsHandler(
        RsEventsHandlerId_t hId )
{
	RS_STACK_MUTEX(mHandlerMapMtx);

	for(uint32_t i=0; i<mHandlerMaps.size(); ++i)
	{
		auto it = mHandlerMaps[i].find(hId);
		if(it != mHandlerMaps[i].end())
		{
			mHandlerMaps[i].erase(it);
			return std::error_condition();
		}
	}
	return RsEventsErrorNum::INVALID_HANDLER_ID;
}

void RsEventsService::threadTick()
{
	auto nextRunAt = std::chrono::system_clock::now() +
	        std::chrono::milliseconds(200);

	std::shared_ptr<const RsEvent> eventPtr(nullptr);
	size_t futureEventsCounter = 0;

dispatchEventFromQueueLock:
	mEventQueueMtx.lock();
	if(mEventQueue.size() > futureEventsCounter)
	{
		eventPtr = mEventQueue.front();
		mEventQueue.pop_front();

		if(eventPtr->mTimePoint >= nextRunAt)
		{
			mEventQueue.push_back(eventPtr);
			++futureEventsCounter;
		}
	}
	mEventQueueMtx.unlock();

	if(eventPtr)
	{
		/* It is relevant that this stays out of mEventQueueMtx */
		handleEvent(eventPtr);
		eventPtr = nullptr; // ensure refcounter is decremented before sleep
		goto dispatchEventFromQueueLock;
	}

	std::this_thread::sleep_until(nextRunAt);
}

void RsEventsService::handleEvent(std::shared_ptr<const RsEvent> event)
{
	if(std::error_condition ec = isEventInvalid(event))
	{
		RsErr() << __PRETTY_FUNCTION__ << " " << ec << std::endl;
		print_stacktrace();
		return;
	}

	RS_STACK_MUTEX(mHandlerMapMtx);
	/* It is important to also call the callback under mutex protection to
	 * ensure they are not unregistered in the meanwhile.
	 * If a callback try to fiddle with registering/unregistering it will
	 * deadlock */

	// Call all clients that registered a callback for this event type
	for(auto cbit: mHandlerMaps[static_cast<uint32_t>(event->mType)])
		cbit.second(event);

	/* Also call all clients that registered with NONE, meaning that they
	 * expect all events */
	for(auto cbit: mHandlerMaps[static_cast<uint32_t>(RsEventType::__NONE)])
		cbit.second(event);
}
