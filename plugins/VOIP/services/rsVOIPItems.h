/*******************************************************************************
 * plugins/VOIP/services/rsVOIPItem.h                                          *
 *                                                                             *
 * Copyright 2011 by Robert Fernie <retroshare.project@gmail.com>              *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#pragma once

#include <map>

#include "rsitems/rsserviceids.h"
#include "rsitems/rsitem.h"

/**************************************************************************/

const uint16_t RS_SERVICE_TYPE_VOIP_PLUGIN = 0xa021;

const uint8_t RS_PKT_SUBTYPE_VOIP_PING 	= 0x01;
const uint8_t RS_PKT_SUBTYPE_VOIP_PONG 	= 0x02;
const uint8_t RS_PKT_SUBTYPE_VOIP_PROTOCOL    	= 0x03 ;
													     // 0x04,0x05 is unused because of a change in the protocol
const uint8_t RS_PKT_SUBTYPE_VOIP_BANDWIDTH 	= 0x06 ;
const uint8_t RS_PKT_SUBTYPE_VOIP_DATA      	= 0x07 ;

const uint8_t QOS_PRIORITY_RS_VOIP = 9 ;

const uint32_t RS_VOIP_FLAGS_VIDEO_DATA = 0x0001 ;
const uint32_t RS_VOIP_FLAGS_AUDIO_DATA = 0x0002 ;

class RsVOIPItem: public RsItem
{
	public:
		RsVOIPItem(uint8_t voip_subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_VOIP_PLUGIN,voip_subtype)
		{ 
			setPriorityLevel(QOS_PRIORITY_RS_VOIP) ;
		}	

		virtual ~RsVOIPItem() {}
		virtual void clear() {}
};

class RsVOIPPingItem: public RsVOIPItem
{
	public:
		RsVOIPPingItem() :RsVOIPItem(RS_PKT_SUBTYPE_VOIP_PING), mSeqNo(0), mPingTS(0) {}
		virtual ~RsVOIPPingItem() {}

		virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

		uint32_t mSeqNo;
		uint64_t mPingTS;
};

class RsVOIPDataItem: public RsVOIPItem
{
	public:
		RsVOIPDataItem() :RsVOIPItem(RS_PKT_SUBTYPE_VOIP_DATA) {}

		virtual ~RsVOIPDataItem() 
		{
			free(voip_data) ;
			voip_data = NULL ;
		}
		virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

		uint32_t flags ;
		uint32_t data_size ;
        
		void *voip_data ;
};

#ifdef TODO
class RsVOIPBandwidthItem: public RsVOIPItem
{
	public:
		RsVOIPBandwidthItem() :RsVOIPItem(RS_PKT_SUBTYPE_VOIP_BANDWIDTH) {}

		virtual ~RsVOIPBandwidthItem()  {}
		virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

		uint32_t flags ;			// is that incoming or expected bandwidth?
		uint32_t bytes_per_sec ;		// bandwidth in bytes per sec.
};
#endif

class RsVOIPProtocolItem: public RsVOIPItem
{
	public:
		RsVOIPProtocolItem() :RsVOIPItem(RS_PKT_SUBTYPE_VOIP_PROTOCOL) {}

		typedef enum { VoipProtocol_Ring = 1, VoipProtocol_Ackn = 2, VoipProtocol_Close = 3, VoipProtocol_Bandwidth = 4 } en_Protocol;

		virtual ~RsVOIPProtocolItem() {}
		virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

		uint32_t protocol ;
		uint32_t flags ;
};

class RsVOIPPongItem: public RsVOIPItem
{
	public:
		RsVOIPPongItem() :RsVOIPItem(RS_PKT_SUBTYPE_VOIP_PONG) {}

		virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
		virtual ~RsVOIPPongItem() {}

		uint32_t mSeqNo;
		uint64_t mPingTS;
		uint64_t mPongTS;
};

class RsVOIPSerialiser: public RsServiceSerializer
{
	public:
		RsVOIPSerialiser() :RsServiceSerializer(RS_SERVICE_TYPE_VOIP_PLUGIN) {}
		virtual ~RsVOIPSerialiser() {}

        virtual RsItem *create_item(uint16_t service,uint8_t item_subtype) const ;
};

/**************************************************************************/
