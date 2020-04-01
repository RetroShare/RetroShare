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

#include <string>
#include <thread>

#include "services/rseventsservice.h"


/*extern*/ RsEvents* rsEvents = nullptr;
RsEvent::~RsEvent() {};
RsEvents::~RsEvents() {};

bool isEventValid(
        std::shared_ptr<const RsEvent> event, std::string& errorMessage )
{
	if(!event)
	{
		errorMessage = "Event is null!";
		return false;
	}

	if(event->mType <= RsEventType::__NONE)
	{
		errorMessage = "Event has type NONE: " +
		        std::to_string(
		            static_cast<std::underlying_type<RsEventType>::type >(
		                event->mType ) );
		return false;
	}

	if(event->mType >= RsEventType::__MAX)
	{
		errorMessage = "Event has type >= RsEventType::__MAX: " +
		        std::to_string(
		            static_cast<std::underlying_type<RsEventType>::type >(
		                event->mType ) );
	}

	return true;
}

bool RsEventsService::postEvent( std::shared_ptr<const RsEvent> event,
                                 std::string& errorMessage )
{
	if(!isEventValid(event, errorMessage))
	{
		std::cerr << __PRETTY_FUNCTION__ << " Error: "<< errorMessage
		          << std::endl;
		return false;
	}

	RS_STACK_MUTEX(mEventQueueMtx);
	mEventQueue.push_back(event);
	return true;
}

bool RsEventsService::sendEvent( std::shared_ptr<const RsEvent> event,
                                 std::string& errorMessage )
{
	if(!isEventValid(event, errorMessage))
	{
		RsErr() << __PRETTY_FUNCTION__ << " "<< errorMessage << std::endl;
		return false;
	}

	handleEvent(event);
	return true;
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

bool RsEventsService::registerEventsHandler(
        std::function<void(std::shared_ptr<const RsEvent>)> multiCallback,
        RsEventsHandlerId_t& hId, RsEventType eventType )
{
	RS_STACK_MUTEX(mHandlerMapMtx);

	if( eventType >= RsEventType::__MAX)
	{
		RsErr() << __PRETTY_FUNCTION__ << " Invalid event type: "
		        << static_cast<uint32_t>(eventType) << " >= RsEventType::__MAX:"
		        << static_cast<uint32_t>(RsEventType::__MAX) << std::endl;
		print_stacktrace();
		return false;
	}

	if(!hId) hId = generateUniqueHandlerId_unlocked();
	else if (hId > mLastHandlerId)
	{
		RsErr() << __PRETTY_FUNCTION__ << " Invalid handler id: " << hId
		        << " how did you generate it? " << std::endl;
		print_stacktrace();
		return false;
	}

	mHandlerMaps[static_cast<std::size_t>(eventType)][hId] = multiCallback;
	return true;
}

bool RsEventsService::unregisterEventsHandler(RsEventsHandlerId_t hId)
{
	RS_STACK_MUTEX(mHandlerMapMtx);

	for(uint32_t i=0; i<mHandlerMaps.size(); ++i)
	{
		auto it = mHandlerMaps[i].find(hId);
		if(it != mHandlerMaps[i].end())
		{
			mHandlerMaps[i].erase(it);
			return true;
		}
	}
	return false;
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
	uint32_t event_type_index = static_cast<uint32_t>(event->mType);

	if(RsEventType::__NONE >= event->mType || event->mType >= RsEventType::__MAX )
	{
		RsErr() << __PRETTY_FUNCTION__ << " Invalid event type: "
		        << event_type_index << std::endl;
		print_stacktrace();
		return;
	}

	{
		RS_STACK_MUTEX(mHandlerMapMtx);
		/* It is important to also call the callback under mutex protection to
		 * ensure they are not unregistered in the meanwhile.
		 * If a callback try to fiddle with registering/unregistering it will
		 * deadlock */

		// Call all clients that registered a callback for this event type
		for(auto cbit: mHandlerMaps[event_type_index]) cbit.second(event);

		/* Also call all clients that registered with NONE, meaning that they
		 * expect all events */
		for(auto cbit: mHandlerMaps[static_cast<uint32_t>(RsEventType::__NONE)])
			cbit.second(event);
	}
}
