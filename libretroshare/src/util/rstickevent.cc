/*******************************************************************************
 * libretroshare/src/util: rstickevent.cc                                      *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012  Robert Fernie <retroshare@lunamutt.com>                 *
 * Copyright (C) 2019  Gioacchino Mazzurco <gio@altermundi.net>                *
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
#include "util/rstickevent.h"

#include <iostream>
#include <list>
#include <algorithm>

//#define DEBUG_EVENTS	1

void	RsTickEvent::tick_events()
{
#ifdef DEBUG_EVENTS
	std::cerr << "RsTickEvent::tick_events() Event List:";
	std::cerr << std::endl;
#endif

	rstime_t now = time(NULL);
	{
		RsStackMutex stack(mEventMtx); /********** STACK LOCKED MTX ******/

#ifdef DEBUG_EVENTS
		if (!mEvents.empty())
		{
			std::multimap<rstime_t, uint32_t>::iterator it;

			for(it = mEvents.begin(); it != mEvents.end(); ++it)
			{
				std::cerr << "\tEvent type: ";
				std::cerr << it->second << " in " << it->first - now << " secs";
				std::cerr << std::endl;
			}
		}
#endif

		if (mEvents.empty())
		{
			return;
		}

		/* all events in the future */
		if (mEvents.begin()->first > now)
		{
			return;
		}
	}

	std::list<EventData> toProcess;
	std::list<EventData>::iterator it;

	{
		RsStackMutex stack(mEventMtx); /********** STACK LOCKED MTX ******/
		while((!mEvents.empty()) && (mEvents.begin()->first <= now))
		{
			std::multimap<rstime_t, EventData>::iterator it = mEvents.begin();
			uint32_t event_type = it->second.mEventType;
			toProcess.push_back(it->second);
			mEvents.erase(it);

			count_adjust_locked(event_type, -1);
			note_event_locked(event_type);
		}
	}

	for(it = toProcess.begin(); it != toProcess.end(); ++it)
	{
#ifdef DEBUG_EVENTS
		std::cerr << "RsTickEvent::tick_events() calling handle_event(";
		std::cerr << it->mEventType << ", " << it->mEventLabel << ")";
		std::cerr << std::endl;
#endif // DEBUG_EVENTS
		handle_event(it->mEventType, it->mEventLabel);
	}
}

void RsTickEvent::schedule_now(uint32_t event_type)
{
	std::string elabel;
	RsTickEvent::schedule_in(event_type, 0, elabel);
}


void RsTickEvent::schedule_now(uint32_t event_type, const std::string &elabel)
{
	RsTickEvent::schedule_in(event_type, 0, elabel);
}

void RsTickEvent::schedule_event(uint32_t event_type, rstime_t when, const std::string &elabel)
{
	RsStackMutex stack(mEventMtx); /********** STACK LOCKED MTX ******/
	mEvents.insert(std::make_pair(when, EventData(event_type, elabel)));

	count_adjust_locked(event_type, 1);
}

void RsTickEvent::schedule_in(uint32_t event_type, uint32_t in_secs)
{
	std::string elabel;
	RsTickEvent::schedule_in(event_type, in_secs, elabel);
}


void RsTickEvent::schedule_in(uint32_t event_type, uint32_t in_secs, const std::string &elabel)
{
#ifdef DEBUG_EVENTS
	std::cerr << "RsTickEvent::schedule_in(" << event_type << ", " << elabel << ") in " << in_secs << " secs";
	std::cerr << std::endl;
#endif // DEBUG_EVENTS

	rstime_t event_time = time(NULL) + in_secs;
	RsTickEvent::schedule_event(event_type, event_time, elabel);
}


void RsTickEvent::handle_event(uint32_t event_type, const std::string& elabel)
{
	RsErr() << __PRETTY_FUNCTION__ << " event_type: " << event_type
	        << " elabel: " << elabel << " Not Handled!" << std::endl;
	print_stacktrace();
}


int32_t RsTickEvent::event_count(uint32_t event_type)
{
	RS_STACK_MUTEX(mEventMtx);
	std::map<uint32_t, int32_t>::iterator it = mEventCount.find(event_type);
	if (it == mEventCount.end()) return 0;
	return it->second;
}


bool RsTickEvent::prev_event_ago(uint32_t event_type, uint32_t& age)
{
	RS_STACK_MUTEX(mEventMtx);
	const auto it = mPreviousEvent.find(event_type);
	if (it == mPreviousEvent.end()) return false;

	age = static_cast<uint32_t>(
	            std::max(time_t(0), time_t(time(nullptr) - it->second)) );
	return true;
}

	
void RsTickEvent::count_adjust_locked(uint32_t event_type, int32_t change)
{
	std::map<uint32_t, int32_t>::iterator it;

	it = mEventCount.find(event_type);
	if (it == mEventCount.end())
	{
		mEventCount[event_type] = 0;
		it = mEventCount.find(event_type);
	}

	it->second += change;
	if (it->second < 0)
	{
		std::cerr << "RsTickEvent::count_adjust() ERROR: COUNT < 0";
		std::cerr << std::endl;

		it->second = 0;
	}
}


void RsTickEvent::note_event_locked(uint32_t event_type)
{
	mPreviousEvent[event_type] = time(NULL);
}


