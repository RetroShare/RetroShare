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

#include <stdexcept>
#include "serialiser/rsbaseserial.h"
#include "serialiser/rstlvbase.h"

#include "services/rsVOIPItems.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

/*************************************************************************/

std::ostream& RsVOIPPingItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsVOIPPingItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "SeqNo: " << mSeqNo << std::endl;

	printIndent(out, int_Indent);
	out << "PingTS: " << std::hex << mPingTS << std::dec << std::endl;

	printRsItemEnd(out, "RsVOIPPingItem", indent);
	return out;
}

std::ostream& RsVOIPPongItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsVOIPPongItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "SeqNo: " << mSeqNo << std::endl;

	printIndent(out, int_Indent);
	out << "PingTS: " << std::hex << mPingTS << std::dec << std::endl;

	printIndent(out, int_Indent);
	out << "PongTS: " << std::hex << mPongTS << std::dec << std::endl;

	printRsItemEnd(out, "RsVOIPPongItem", indent);
	return out;
}
std::ostream& RsVOIPProtocolItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsVOIPProtocolItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "flags: " << flags << std::endl;

	printIndent(out, int_Indent);
	out << "protocol: " << std::hex << protocol << std::dec << std::endl;

	printRsItemEnd(out, "RsVOIPProtocolItem", indent);
	return out;
}
std::ostream& RsVOIPDataItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsVOIPDataItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "flags: " << flags << std::endl;

	printIndent(out, int_Indent);
	out << "data size: " << std::hex << data_size << std::dec << std::endl;

	printRsItemEnd(out, "RsVOIPDataItem", indent);
	return out;
}

/*************************************************************************/
uint32_t RsVOIPDataItem::serial_size() const
{
	uint32_t s = 8; /* header */
	s += 4; /* flags */
	s += 4; /* data_size  */
	s += data_size; /* data */

	return s;
}
uint32_t RsVOIPProtocolItem::serial_size() const
{
	uint32_t s = 8; /* header */
	s += 4; /* flags */
	s += 4; /* protocol */

	return s;
}
uint32_t RsVOIPPingItem::serial_size() const
{
	uint32_t s = 8; /* header */
	s += 4; /* seqno */
	s += 8; /* pingTS  */

	return s;
}
bool RsVOIPProtocolItem::serialise(void *data, uint32_t& pktsize) 
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsVOIPSerialiser::serialiseVOIPDataItem() Header: " << ok << std::endl;
	std::cerr << "RsVOIPSerialiser::serialiseVOIPDataItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, protocol);
	ok &= setRawUInt32(data, tlvsize, &offset, flags);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsVOIPSerialiser::serialiseVOIPPingItem() Size Error! " << std::endl;
	}

	return ok;
}
/* serialise the data to the buffer */
bool RsVOIPDataItem::serialise(void *data, uint32_t& pktsize) 
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsVOIPSerialiser::serialiseVOIPDataItem() Header: " << ok << std::endl;
	std::cerr << "RsVOIPSerialiser::serialiseVOIPDataItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, flags);
	ok &= setRawUInt32(data, tlvsize, &offset, data_size);

	memcpy( &((uint8_t*)data)[offset],voip_data,data_size) ;
	offset += data_size ;

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsVOIPSerialiser::serialiseVOIPPingItem() Size Error! " << std::endl;
	}

	return ok;
}
/* serialise the data to the buffer */
bool RsVOIPPingItem::serialise(void *data, uint32_t& pktsize) 
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsVOIPSerialiser::serialiseVOIPPingItem() Header: " << ok << std::endl;
	std::cerr << "RsVOIPSerialiser::serialiseVOIPPingItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, mSeqNo);
	ok &= setRawUInt64(data, tlvsize, &offset, mPingTS);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsVOIPSerialiser::serialiseVOIPPingItem() Size Error! " << std::endl;
	}

	return ok;
}

RsVOIPProtocolItem::RsVOIPProtocolItem(void *data, uint32_t pktsize)
	: RsVOIPItem(RS_PKT_SUBTYPE_VOIP_PROTOCOL)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (RS_SERVICE_TYPE_VOIP_PLUGIN != getRsItemService(rstype)) || (RS_PKT_SUBTYPE_VOIP_PROTOCOL != getRsItemSubType(rstype)))
		throw std::runtime_error("Wrong packet type!") ;

	if (pktsize < rssize)    /* check size */
		throw std::runtime_error("Not enough size!") ;

	bool ok = true;

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &protocol);
	ok &= getRawUInt32(data, rssize, &offset, &flags);

	if (offset != rssize)
		throw std::runtime_error("Deserialisation error!") ;

	if (!ok)
		throw std::runtime_error("Deserialisation error!") ;
}
RsVOIPPingItem::RsVOIPPingItem(void *data, uint32_t pktsize)
	: RsVOIPItem(RS_PKT_SUBTYPE_VOIP_PING)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (RS_SERVICE_TYPE_VOIP_PLUGIN != getRsItemService(rstype)) || (RS_PKT_SUBTYPE_VOIP_PING != getRsItemSubType(rstype)))
		throw std::runtime_error("Wrong packet type!") ;

	if (pktsize < rssize)    /* check size */
		throw std::runtime_error("Not enough size!") ;

	bool ok = true;

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &mSeqNo);
	ok &= getRawUInt64(data, rssize, &offset, &mPingTS);

	if (offset != rssize)
		throw std::runtime_error("Deserialisation error!") ;

	if (!ok)
		throw std::runtime_error("Deserialisation error!") ;
}

/*************************************************************************/
/*************************************************************************/


uint32_t RsVOIPPongItem::serial_size() const
{
	uint32_t s = 8; /* header */
	s += 4; /* seqno */
	s += 8; /* pingTS  */
	s += 8; /* pongTS */

	return s;
}

/* serialise the data to the buffer */
bool RsVOIPPongItem::serialise(void *data, uint32_t& pktsize) 
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsVOIPSerialiser::serialiseVOIPPongItem() Header: " << ok << std::endl;
	std::cerr << "RsVOIPSerialiser::serialiseVOIPPongItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, mSeqNo);
	ok &= setRawUInt64(data, tlvsize, &offset, mPingTS);
	ok &= setRawUInt64(data, tlvsize, &offset, mPongTS);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsVOIPSerialiser::serialiseVOIPPongItem() Size Error! " << std::endl;
	}

	return ok;
}
RsVOIPDataItem::RsVOIPDataItem(void *data, uint32_t pktsize)
	: RsVOIPItem(RS_PKT_SUBTYPE_VOIP_DATA)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (RS_SERVICE_TYPE_VOIP_PLUGIN != getRsItemService(rstype)) || (RS_PKT_SUBTYPE_VOIP_DATA != getRsItemSubType(rstype)))
		throw std::runtime_error("Wrong packet subtype") ;

	if (pktsize < rssize)    /* check size */
		throw std::runtime_error("Not enough space") ;

	bool ok = true;

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &flags);
	ok &= getRawUInt32(data, rssize, &offset, &data_size);

	voip_data = malloc(data_size) ;
	memcpy(voip_data,&((uint8_t*)data)[offset],data_size) ;
	offset += data_size ;

	if (offset != rssize)
		throw std::runtime_error("Serialization error.") ;

	if (!ok)
		throw std::runtime_error("Serialization error.") ;
}
RsVOIPPongItem::RsVOIPPongItem(void *data, uint32_t pktsize)
	: RsVOIPItem(RS_PKT_SUBTYPE_VOIP_PONG)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (RS_SERVICE_TYPE_VOIP_PLUGIN != getRsItemService(rstype)) || (RS_PKT_SUBTYPE_VOIP_PONG != getRsItemSubType(rstype)))
		throw std::runtime_error("Wrong packet subtype") ;

	if (pktsize < rssize)    /* check size */
		throw std::runtime_error("Not enough space") ;

	bool ok = true;

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &mSeqNo);
	ok &= getRawUInt64(data, rssize, &offset, &mPingTS);
	ok &= getRawUInt64(data, rssize, &offset, &mPongTS);

	if (offset != rssize)
		throw std::runtime_error("Serialization error.") ;

	if (!ok)
		throw std::runtime_error("Serialization error.") ;
}

/*************************************************************************/

RsItem* RsVOIPSerialiser::deserialise(void *data, uint32_t *pktsize)
{
#ifdef RSSERIAL_DEBUG
	std::cerr << "RsVOIPSerialiser::deserialise()" << std::endl;
#endif

	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (RS_SERVICE_TYPE_VOIP_PLUGIN != getRsItemService(rstype)))
		return NULL ;
	
	try
	{
		switch(getRsItemSubType(rstype))
		{
			case RS_PKT_SUBTYPE_VOIP_PING: 		return new RsVOIPPingItem(data, *pktsize);
			case RS_PKT_SUBTYPE_VOIP_PONG: 		return new RsVOIPPongItem(data, *pktsize);
			case RS_PKT_SUBTYPE_VOIP_PROTOCOL: 	return new RsVOIPProtocolItem(data, *pktsize);
			case RS_PKT_SUBTYPE_VOIP_DATA: 		return new RsVOIPDataItem(data, *pktsize);

			default:
				return NULL;
		}
	}
	catch(std::exception& e)
	{
		std::cerr << "RsVOIPSerialiser: deserialization error: " << e.what() << std::endl;
		return NULL;
	}
}


/*************************************************************************/

