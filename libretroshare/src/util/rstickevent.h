/*
 * libretroshare/src/util: rstickevent.h
 *
 * Identity interface for RetroShare.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */
#ifndef RS_UTIL_TICK_EVENT
#define RS_UTIL_TICK_EVENT


/* 
 *
 * A simple event queue... to avoid having to continually write separate one.
 */

#include <map>
#include <time.h>

#include "util/rsthreads.h"

class RsTickEvent
{
	public:
	RsTickEvent():mEventMtx("TickEventMtx") { return; }

void	tick_events();

void    schedule_now(uint32_t event_type);
void    schedule_now(uint32_t event_type, const std::string &elabel);

void    schedule_event(uint32_t event_type, time_t when, const std::string &elabel);

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
	std::map<uint32_t, time_t>      mPreviousEvent;
	std::multimap<time_t, EventData> mEvents;
};

#endif // RS_UTIL_TICK_EVENT
