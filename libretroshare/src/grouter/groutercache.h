/*
 * libretroshare/src/services: groutercache.h
 *
 * Services for RetroShare.
 *
 * Copyright 2013 by Cyril Soler
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
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
 * Please report all bugs and problems to "csoler@users.sourceforge.net".
 *
 */

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
		time_t last_activity ;
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
};

