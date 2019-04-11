/*******************************************************************************
 * libretroshare/src/grouter: groutercache.h                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2013 by Cyril Soler <csoler@users.sourceforge.net>                *
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

#include "retroshare/rsflags.h"
#include "rsgrouter.h"

#define FLAGS_TAG_GROUTER_CACHE 0x22948eb1

typedef t_RsFlags32<FLAGS_TAG_GROUTER_CACHE> GRouterCacheInfoFlags ;
typedef uint64_t GRouterMessageId ;

const uint32_t GROUTER_CACHE_INFO_FLAGS_WAITING_ACK = 0x0001 ;

class GRouterMessageDataItem
{
	public:
		uint8_t *data_bytes ;				// data to be sent
		uint32_t data_size ;					// size of the data
		GRouterMessageId message_id ;
		GRouterKeyId destination ;

	private:
		// Make this class non copiable to avoid memory issues
		//
		GRouterMessageDataItem& operator=(const GRouterMessageDataItem&) ;
		GRouterMessageDataItem(const GRouterMessageDataItem&) ;
};

class GRouterCacheInfo
{
	public:
		GRouterCacheInfoFlags flags ;		
		rstime_t last_activity ;
};

class GRouterCache
{
	public:
		// Stored transitting messages
		//
		std::list<GRouterMessageDataItem *> _pending_messages ;

		// Cache of which message is pending, waiting for an ACK, etc.
		//
		std::map<GRouterMessageId,GRouterCacheInfo> _cache_info ;

		// debug stuff
		//
		void debugDump() ;
};

