
/*
 * libretroshare/src/serialiser: rsvoipitems.cc
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

#include <stdexcept>
#include "serialiser/rsbaseserial.h"
#include "serialiser/rstlvbase.h"

#include "services/rsvoipitems.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

/*************************************************************************/

std::ostream& RsVoipPingItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsVoipPingItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "SeqNo: " << mSeqNo << std::endl;

	printIndent(out, int_Indent);
	out << "PingTS: " << std::hex << mPingTS << std::dec << std::endl;

	printRsItemEnd(out, "RsVoipPingItem", indent);
	return out;
}

std::ostream& RsVoipPongItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsVoipPongItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "SeqNo: " << mSeqNo << std::endl;

	printIndent(out, int_Indent);
	out << "PingTS: " << std::hex << mPingTS << std::dec << std::endl;

	printIndent(out, int_Indent);
	out << "PongTS: " << std::hex << mPongTS << std::dec << std::endl;

	printRsItemEnd(out, "RsVoipPongItem", indent);
	return out;
}
std::ostream& RsVoipProtocolItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsVoipProtocolItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "flags: " << flags << std::endl;

	printIndent(out, int_Indent);
	out << "protocol: " << std::hex << protocol << std::dec << std::endl;

	printRsItemEnd(out, "RsVoipProtocolItem", indent);
	return out;
}
std::ostream& RsVoipDataItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsVoipDataItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "flags: " << flags << std::endl;

	printIndent(out, int_Indent);
	out << "data size: " << std::hex << data_size << std::dec << std::endl;

	printRsItemEnd(out, "RsVoipDataItem", indent);
	return out;
}

/*************************************************************************/
uint32_t RsVoipDataItem::serial_size() const
{
	uint32_t s = 8; /* header */
	s += 4; /* flags */
	s += 4; /* data_size  */
	s += data_size; /* data */

	return s;
}
uint32_t RsVoipProtocolItem::serial_size() const
{
	uint32_t s = 8; /* header */
	s += 4; /* flags */
	s += 4; /* protocol */

	return s;
}
uint32_t RsVoipPingItem::serial_size() const
{
	uint32_t s = 8; /* header */
	s += 4; /* seqno */
	s += 8; /* pingTS  */

	return s;
}
bool RsVoipProtocolItem::serialise(void *data, uint32_t& pktsize) 
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsVoipSerialiser::serialiseVoipDataItem() Header: " << ok << std::endl;
	std::cerr << "RsVoipSerialiser::serialiseVoipDataItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, protocol);
	ok &= setRawUInt32(data, tlvsize, &offset, flags);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsVoipSerialiser::serialiseVoipPingItem() Size Error! " << std::endl;
	}

	return ok;
}
/* serialise the data to the buffer */
bool RsVoipDataItem::serialise(void *data, uint32_t& pktsize) 
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsVoipSerialiser::serialiseVoipDataItem() Header: " << ok << std::endl;
	std::cerr << "RsVoipSerialiser::serialiseVoipDataItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, flags);
	ok &= setRawUInt32(data, tlvsize, &offset, data_size);
        std::cerr << "data_size : " << data_size << std::endl;
        //memcpy(data+offset,voip_data,data_size) ;
        memcpy( &((uint8_t*)data)[offset],voip_data,data_size) ;
	offset += data_size ;

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsVoipSerialiser::serialiseVoipPingItem() Size Error! " << std::endl;
	}

	return ok;
}
/* serialise the data to the buffer */
bool RsVoipPingItem::serialise(void *data, uint32_t& pktsize) 
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsVoipSerialiser::serialiseVoipPingItem() Header: " << ok << std::endl;
	std::cerr << "RsVoipSerialiser::serialiseVoipPingItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, mSeqNo);
	ok &= setRawUInt64(data, tlvsize, &offset, mPingTS);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsVoipSerialiser::serialiseVoipPingItem() Size Error! " << std::endl;
	}

	return ok;
}

RsVoipProtocolItem::RsVoipProtocolItem(void *data, uint32_t pktsize)
	: RsVoipItem(RS_PKT_SUBTYPE_VOIP_PROTOCOL)
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
	ok &= getRawUInt32(data, rssize, &offset, &protocol);
	ok &= getRawUInt32(data, rssize, &offset, &flags);

	if (offset != rssize)
		throw std::runtime_error("Deserialisation error!") ;

	if (!ok)
		throw std::runtime_error("Deserialisation error!") ;
}
RsVoipPingItem::RsVoipPingItem(void *data, uint32_t pktsize)
	: RsVoipItem(RS_PKT_SUBTYPE_VOIP_PING)
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


uint32_t RsVoipPongItem::serial_size() const
{
	uint32_t s = 8; /* header */
	s += 4; /* seqno */
	s += 8; /* pingTS  */
	s += 8; /* pongTS */

	return s;
}

/* serialise the data to the buffer */
bool RsVoipPongItem::serialise(void *data, uint32_t& pktsize) 
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsVoipSerialiser::serialiseVoipPongItem() Header: " << ok << std::endl;
	std::cerr << "RsVoipSerialiser::serialiseVoipPongItem() Size: " << tlvsize << std::endl;
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
		std::cerr << "RsVoipSerialiser::serialiseVoipPongItem() Size Error! " << std::endl;
	}

	return ok;
}
RsVoipDataItem::RsVoipDataItem(void *data, uint32_t pktsize)
	: RsVoipItem(RS_PKT_SUBTYPE_VOIP_DATA)
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
RsVoipPongItem::RsVoipPongItem(void *data, uint32_t pktsize)
	: RsVoipItem(RS_PKT_SUBTYPE_VOIP_PONG)
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

RsItem* RsVoipSerialiser::deserialise(void *data, uint32_t *pktsize)
{
#ifdef RSSERIAL_DEBUG
	std::cerr << "RsVoipSerialiser::deserialise()" << std::endl;
#endif

	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (RS_SERVICE_TYPE_VOIP_PLUGIN != getRsItemService(rstype)))
		return NULL ;
	
	try
	{
		switch(getRsItemSubType(rstype))
		{
			case RS_PKT_SUBTYPE_VOIP_PING: 		return new RsVoipPingItem(data, *pktsize);
			case RS_PKT_SUBTYPE_VOIP_PONG: 		return new RsVoipPongItem(data, *pktsize);
			case RS_PKT_SUBTYPE_VOIP_PROTOCOL: 	return new RsVoipProtocolItem(data, *pktsize);
			case RS_PKT_SUBTYPE_VOIP_DATA: 		return new RsVoipDataItem(data, *pktsize);

			default:
				return NULL;
		}
	}
	catch(std::exception& e)
	{
		std::cerr << "RsVoipSerialiser: deserialization error: " << e.what() << std::endl;
		return NULL;
	}
}


/*************************************************************************/

