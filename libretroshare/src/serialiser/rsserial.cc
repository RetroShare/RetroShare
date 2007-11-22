
/*
 * libretroshare/src/serialiser: rsserial.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Robert Fernie.
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

#include "serialiser/rsbaseserial.h"
#include "serialiser/rsserial.h"

#include <map>

#define RSSERIAL_DEBUG 1

#ifdef RSSERIAL_DEBUG
	#include <iostream>
#endif
	
RsItem::RsItem(uint32_t t)
:type(t) 
{
	return;
}
	
RsItem::RsItem(uint8_t ver, uint8_t cls, uint8_t t, uint8_t subtype)
{
	type = (ver << 24) + (cls << 16) + (t << 8) + subtype;
	return;
}

RsItem::~RsItem()
{
	return;
}

	
uint32_t    RsItem::PacketId()
{
	return type;
}

uint8_t    RsItem::PacketVersion()
{
	return (type >> 24);
}

	
uint8_t    RsItem::PacketClass()
{
	return (type >> 16) & 0xFF;
}

	
uint8_t    RsItem::PacketType()
{
	return (type >> 8) & 0xFF;
}

	
uint8_t    RsItem::PacketSubType()
{
	return (type & 0xFF);
}



RsSerialType::RsSerialType(uint32_t t)
	:type(t & 0xFFFFFF00)
{
	return;
}
	
RsSerialType::RsSerialType(uint8_t ver, uint8_t cls, uint8_t t)
{
	type = (ver << 24) + (cls << 16) + (t << 8);
	return;
}


RsSerialType::~RsSerialType()
{
	return;
}
	
uint32_t    RsSerialType::size(RsItem *)
{
#ifdef  RSSERIAL_DEBUG
	std::cerr << "RsSerialType::size()" << std::endl;
#endif

	/* base size: type + length */
	return 8; 
}

bool        RsSerialType::serialise(RsItem *item, void *data, uint32_t *size)
{
#ifdef  RSSERIAL_DEBUG
	std::cerr << "RsSerialType::serialise()" << std::endl;
#endif
	return false;
}

RsItem *    RsSerialType::deserialise(void *data, uint32_t *size)
{
#ifdef  RSSERIAL_DEBUG
	std::cerr << "RsSerialType::deserialise()" << std::endl;
#endif
	return NULL;
}

uint32_t    RsSerialType::PacketId()
{
	return type;
}




RsSerialiser::RsSerialiser()
{
	return;
}


RsSerialiser::~RsSerialiser()
{
	/* clean up the map */
	std::map<uint32_t, RsSerialType *>::iterator it;
	for(it = serialisers.begin(); it != serialisers.end(); it++)
	{
		delete (it->second);
	}
	serialisers.clear();
	return;
}



bool        RsSerialiser::addSerialType(RsSerialType *serialiser)
{
	uint32_t type = (serialiser->PacketId() & 0xFFFFFF00);
	std::map<uint32_t, RsSerialType *>::iterator it;
	if (serialisers.end() != (it = serialisers.find(type)))
	{
#ifdef  RSSERIAL_DEBUG
		std::cerr << "RsSerialiser::addSerialType() Error Serialiser already exists!";
		std::cerr << std::endl;
#endif
		return false;
	}

	serialisers[type] = serialiser;
	return true;
}



uint32_t    RsSerialiser::size(RsItem *item)
{
	/* find the type */
	uint32_t type = (item->PacketId() & 0xFFFFFF00);
	std::map<uint32_t, RsSerialType *>::iterator it;

	if (serialisers.end() == (it = serialisers.find(type)))
	{
#ifdef  RSSERIAL_DEBUG
		std::cerr << "RsSerialiser::size() serialiser missing!";
		std::cerr << std::endl;
#endif
		return 0;
	}

#ifdef  RSSERIAL_DEBUG
	std::cerr << "RsSerialiser::serialise() Found type: " << type;
	std::cerr << std::endl;
#endif


	return (it->second)->size(item);
}

bool        RsSerialiser::serialise  (RsItem *item, void *data, uint32_t *size)
{
	/* find the type */
	uint32_t type = (item->PacketId() & 0xFFFFFF00);
	std::map<uint32_t, RsSerialType *>::iterator it;

	if (serialisers.end() == (it = serialisers.find(type)))
	{
#ifdef  RSSERIAL_DEBUG
		std::cerr << "RsSerialiser::serialise() serialiser missing!";
		std::cerr << std::endl;
#endif
		return false;
	}

#ifdef  RSSERIAL_DEBUG
	std::cerr << "RsSerialiser::serialise() Found type: " << type;
	std::cerr << std::endl;
#endif

	return (it->second)->serialise(item, data, size);
}



RsItem *    RsSerialiser::deserialise(void *data, uint32_t *size)
{
	/* find the type */
	if (*size < 8)
	{
#ifdef  RSSERIAL_DEBUG
		std::cerr << "RsSerialiser::deserialise() Not Enough Data(1)";
		std::cerr << std::endl;
#endif
		return NULL;
	}

	uint32_t type = (getRsItemId(data) & 0xFFFFFF00);
	uint32_t pkt_size = getRsItemSize(data);

	if (pkt_size < *size)
	{
#ifdef  RSSERIAL_DEBUG
		std::cerr << "RsSerialiser::deserialise() Not Enough Data(2)";
		std::cerr << std::endl;
#endif
		return NULL;
	}

	/* store the packet size to return the amount we should use up */
	*size = pkt_size;

	std::map<uint32_t, RsSerialType *>::iterator it;
	if (serialisers.end() == (it = serialisers.find(type)))
	{
#ifdef  RSSERIAL_DEBUG
		std::cerr << "RsSerialiser::deserialise() deserialiser missing!";
		std::cerr << std::endl;
#endif
		return NULL;
	}

	RsItem *item = (it->second)->deserialise(data, &pkt_size);
	if (!item)
	{
		return NULL;
	}

	if (pkt_size != *size)
	{
#ifdef  RSSERIAL_DEBUG
		std::cerr << "RsSerialiser::deserialise() Warning: size mismatch!";
		std::cerr << std::endl;
#endif
	}
	return item;
}


bool   setRsItemHeader(void *data, uint32_t size, uint32_t type, uint32_t pktsize)
{
	if (size < 8)
		return false;

	uint32_t offset = 0;
	bool ok = true;
	ok &= setRawUInt32(data, 8, &offset, type);
	ok &= setRawUInt32(data, 8, &offset, pktsize);

	return ok;
}

	

uint32_t getRsItemId(void *data)
{
	uint32_t type;
	uint32_t offset = 0;
	getRawUInt32(data, 4, &offset, &type);
	return type;
}


uint32_t getRsItemSize(void *data)
{
	uint32_t size;
	uint32_t offset = 4;
	getRawUInt32(data, 8, &offset, &size);
	return size;
}

uint8_t  getRsItemVersion(uint32_t type)
{
	return (type >> 24);
}

uint8_t  getRsItemClass(uint32_t type)
{
	return (type >> 16) & 0xFF;
}

uint8_t  getRsItemType(uint32_t type)
{
	return (type >> 8) & 0xFF;
}

uint8_t  getRsItemSubType(uint32_t type)
{
	return (type & 0xFF);
}




