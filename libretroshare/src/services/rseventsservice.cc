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

	if(event->mType <= RsEventType::NONE)
	{
		errorMessage = "Event has type NONE: " +
		        std::to_string(
		            static_cast<std::underlying_type<RsEventType>::type >(
		                event->mType ) );
		return false;
	}

	if(event->mType >= RsEventType::MAX)
	{
		errorMessage = "Event has type >= RsEventType::MAX: " +
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
        RsEventsHandlerId_t& hId )
{
	RS_STACK_MUTEX(mHandlerMapMtx);
	if(!hId) hId = generateUniqueHandlerId_unlocked();
	mHandlerMap[hId] = multiCallback;
	return true;
}

bool RsEventsService::unregisterEventsHandler(RsEventsHandlerId_t hId)
{
	RS_STACK_MUTEX(mHandlerMapMtx);
	auto it = mHandlerMap.find(hId);
	if(it == mHandlerMap.end()) return false;
	mHandlerMap.erase(it);
	return true;
}

void RsEventsService::data_tick()
{
	auto nextRunAt = std::chrono::system_clock::now() +
	        std::chrono::milliseconds(1);

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
	std::function<void(std::shared_ptr<const RsEvent>)> mCallback;

	mHandlerMapMtx.lock();
	auto cbpt = mHandlerMap.begin();
	mHandlerMapMtx.unlock();

getHandlerFromMapLock:
	mHandlerMapMtx.lock();
	if(cbpt != mHandlerMap.end())
	{
		mCallback = cbpt->second;
		++cbpt;
	}
	mHandlerMapMtx.unlock();

	if(mCallback)
	{
		mCallback(event); // It is relevant that this happens outside mutex
		mCallback = std::function<void(std::shared_ptr<const RsEvent>)>(nullptr);
		goto getHandlerFromMapLock;
	}
}
