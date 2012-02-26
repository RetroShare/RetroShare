#ifndef RS_VOIP_ITEMS_H
#define RS_VOIP_ITEMS_H

/*
 * libretroshare/src/serialiser: rsvoipitems.h
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
#include "serialiser/rstlvtypes.h"

/**************************************************************************/

const uint16_t RS_SERVICE_TYPE_VOIP       = 0xa021;

const uint8_t RS_PKT_SUBTYPE_VOIP_PING 	= 0x01;
const uint8_t RS_PKT_SUBTYPE_VOIP_PONG 	= 0x02;
const uint8_t RS_PKT_SUBTYPE_VOIP_PROTOCOL= 0x03 ;
const uint8_t RS_PKT_SUBTYPE_VOIP_DATA   	= 0x04 ;

const uint8_t QOS_PRIORITY_RS_VOIP = 9 ;

class RsVoipItem: public RsItem
{
	public:
		RsVoipItem(uint8_t voip_subtype) 
			: RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_VOIP,voip_subtype) 
		{ 
			setPriorityLevel(QOS_PRIORITY_RS_VOIP) ;
		}	

		virtual ~RsVoipItem() {};
		virtual void clear() {};
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0) = 0 ;

		virtual bool serialise(void *data,uint32_t& size) = 0 ;	// Isn't it better that items can serialise themselves ?
		virtual uint32_t serial_size() const = 0 ; 							// deserialise is handled using a constructor
};

class RsVoipPingItem: public RsVoipItem
{
	public:
		RsVoipPingItem() :RsVoipItem(RS_PKT_SUBTYPE_VOIP_PING) {}
		RsVoipPingItem(void *data,uint32_t size) ;

		virtual bool serialise(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() const ; 						

		virtual ~RsVoipPingItem() {}
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		uint32_t mSeqNo;
		uint64_t mPingTS;
};

class RsVoipDataItem: public RsVoipItem
{
	public:
		RsVoipDataItem() :RsVoipItem(RS_PKT_SUBTYPE_VOIP_DATA) {}
		RsVoipDataItem(void *data,uint32_t size) ; // de-serialization

		virtual bool serialise(void *data,uint32_t& size) ;
		virtual uint32_t serial_size() const ; 							

		virtual ~RsVoipDataItem() 
		{
			free(voip_data) ;
			voip_data = NULL ;
		}
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		uint32_t flags ;
		uint32_t data_size ;
		void *voip_data ;
};

class RsVoipProtocolItem: public RsVoipItem
{
	public:
		RsVoipProtocolItem() :RsVoipItem(RS_PKT_SUBTYPE_VOIP_PROTOCOL) {}
		RsVoipProtocolItem(void *data,uint32_t size) ;

		enum { VoipProtocol_Ring = 1, VoipProtocol_Ackn = 2, VoipProtocol_Close = 3 } ;

		virtual bool serialise(void *data,uint32_t& size) ;
		virtual uint32_t serial_size() const ; 							

		virtual ~RsVoipProtocolItem() {}
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		uint32_t protocol ;
		uint32_t flags ;
};

class RsVoipPongItem: public RsVoipItem
{
	public:
		RsVoipPongItem() :RsVoipItem(RS_PKT_SUBTYPE_VOIP_PONG) {}
		RsVoipPongItem(void *data,uint32_t size) ;

		virtual bool serialise(void *data,uint32_t& size) ;
		virtual uint32_t serial_size() const ; 							

		virtual ~RsVoipPongItem() {}
	virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		uint32_t mSeqNo;
		uint64_t mPingTS;
		uint64_t mPongTS;
};

class RsVoipSerialiser: public RsSerialType
{
	public:
		RsVoipSerialiser()
			:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_VOIP)
		{ 
		}
		virtual ~RsVoipSerialiser() {}

		virtual uint32_t 	size (RsItem *item) 
		{ 
			return dynamic_cast<RsVoipItem *>(item)->serial_size() ;
		}

		virtual	bool serialise  (RsItem *item, void *data, uint32_t *size)
		{ 
			return dynamic_cast<RsVoipItem *>(item)->serialise(data,*size) ;
		}
		virtual	RsItem *deserialise(void *data, uint32_t *size);
};

/**************************************************************************/

#endif /* RS_VOIP_ITEMS_H */


