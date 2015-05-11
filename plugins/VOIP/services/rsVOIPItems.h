/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2015
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#pragma once

/*
 * libretroshare/src/serialiser: rsVOIPItems.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2011 by Robert Fernie.
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
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include <map>

#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"

/**************************************************************************/

const uint16_t RS_SERVICE_TYPE_VOIP_PLUGIN = 0xa021;

const uint8_t RS_PKT_SUBTYPE_VOIP_PING 	   = 0x01;
const uint8_t RS_PKT_SUBTYPE_VOIP_PONG 	   = 0x02;
const uint8_t RS_PKT_SUBTYPE_VOIP_PROTOCOL   = 0x03 ;
													     // 0x04 is unused because of a change in the protocol
const uint8_t RS_PKT_SUBTYPE_VOIP_DATA      	= 0x05 ;

const uint8_t QOS_PRIORITY_RS_VOIP = 9 ;

const uint32_t RS_VOIP_FLAGS_VIDEO_DATA = 0x0001 ;
const uint32_t RS_VOIP_FLAGS_AUDIO_DATA = 0x0002 ;

class RsVOIPItem: public RsItem
{
	public:
		RsVOIPItem(uint8_t voip_subtype)
			: RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_VOIP_PLUGIN,voip_subtype) 
		{ 
			setPriorityLevel(QOS_PRIORITY_RS_VOIP) ;
		}	

		virtual ~RsVOIPItem() {};
		virtual void clear() {};
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0) = 0 ;

		virtual bool serialise(void *data,uint32_t& size) = 0 ;	// Isn't it better that items can serialise themselves ?
		virtual uint32_t serial_size() const = 0 ; 							// deserialise is handled using a constructor
};

class RsVOIPPingItem: public RsVOIPItem
{
	public:
		RsVOIPPingItem() :RsVOIPItem(RS_PKT_SUBTYPE_VOIP_PING) {}
		RsVOIPPingItem(void *data,uint32_t size) ;

		virtual bool serialise(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() const ; 						

		virtual ~RsVOIPPingItem() {}
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		uint32_t mSeqNo;
		uint64_t mPingTS;
};

class RsVOIPDataItem: public RsVOIPItem
{
	public:
		RsVOIPDataItem() :RsVOIPItem(RS_PKT_SUBTYPE_VOIP_DATA) {}
		RsVOIPDataItem(void *data,uint32_t size) ; // de-serialization

		virtual bool serialise(void *data,uint32_t& size) ;
		virtual uint32_t serial_size() const ; 							

		virtual ~RsVOIPDataItem() 
		{
			free(voip_data) ;
			voip_data = NULL ;
		}
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		uint32_t flags ;
		uint32_t data_size ;
		void *voip_data ;
};

class RsVOIPProtocolItem: public RsVOIPItem
{
	public:
		RsVOIPProtocolItem() :RsVOIPItem(RS_PKT_SUBTYPE_VOIP_PROTOCOL) {}
		RsVOIPProtocolItem(void *data,uint32_t size) ;

		enum { VoipProtocol_Ring = 1, VoipProtocol_Ackn = 2, VoipProtocol_Close = 3, VoipProtocol_Bandwidth = 4 } ;

		virtual bool serialise(void *data,uint32_t& size) ;
		virtual uint32_t serial_size() const ; 							

		virtual ~RsVOIPProtocolItem() {}
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		uint32_t protocol ;
		uint32_t flags ;
};

class RsVOIPPongItem: public RsVOIPItem
{
	public:
		RsVOIPPongItem() :RsVOIPItem(RS_PKT_SUBTYPE_VOIP_PONG) {}
		RsVOIPPongItem(void *data,uint32_t size) ;

		virtual bool serialise(void *data,uint32_t& size) ;
		virtual uint32_t serial_size() const ; 							

		virtual ~RsVOIPPongItem() {}
	virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		uint32_t mSeqNo;
		uint64_t mPingTS;
		uint64_t mPongTS;
};

class RsVOIPSerialiser: public RsSerialType
{
	public:
		RsVOIPSerialiser()
			:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_VOIP_PLUGIN)
		{ 
		}
		virtual ~RsVOIPSerialiser() {}

		virtual uint32_t 	size (RsItem *item) 
		{ 
			return dynamic_cast<RsVOIPItem *>(item)->serial_size() ;
		}

		virtual	bool serialise  (RsItem *item, void *data, uint32_t *size)
		{ 
			return dynamic_cast<RsVOIPItem *>(item)->serialise(data,*size) ;
		}
		virtual	RsItem *deserialise(void *data, uint32_t *size);
};

/**************************************************************************/
