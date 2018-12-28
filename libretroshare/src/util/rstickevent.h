/*******************************************************************************
 * libretroshare/src/util: rstickevent.h                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2012 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef RS_UTIL_TICK_EVENT
#define RS_UTIL_TICK_EVENT


/* 
 *
 * A simple event queue... to avoid having to continually write separate one.
 */

#include <map>
#include "util/rstime.h"

#include "util/rsthreads.h"

class RsTickEvent
{
	public:
	RsTickEvent():mEventMtx("TickEventMtx") { return; }

void	tick_events();

void    schedule_now(uint32_t event_type);
void    schedule_now(uint32_t event_type, const std::string &elabel);

void    schedule_event(uint32_t event_type, rstime_t when, const std::string &elabel);

void    schedule_in(uint32_t event_type, uint32_t in_secs);
void    schedule_in(uint32_t event_type, uint32_t in_secs, const std::string &elabel);

int32_t event_count(uint32_t event_type);
bool 	prev_event_ago(uint32_t event_type, int32_t &age);

	protected:

	// Overloaded to handle the events.
virtual void    handle_event(uint32_t event_type, const std::string &event_label);

	private:

	class EventData
	{
		public:
		EventData() :mEventType(0) { return; }
		EventData(uint32_t etype) :mEventType(etype) { return; }
		EventData(uint32_t etype, std::string elabel) :mEventLabel(elabel), mEventType(etype) { return; }

		std::string mEventLabel;
		uint32_t mEventType;
	};

void 	count_adjust_locked(uint32_t event_type, int32_t change);
void 	note_event_locked(uint32_t event_type);

	RsMutex mEventMtx;
	std::map<uint32_t, int32_t>    mEventCount;
	std::map<uint32_t, rstime_t>      mPreviousEvent;
	std::multimap<rstime_t, EventData> mEvents;
};

#endif // RS_UTIL_TICK_EVENT
