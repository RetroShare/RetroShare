/*******************************************************************************
 * libretroshare/src/serialiser: rsserial.cc                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2008 by Robert Fernie <retroshare@lunamutt.com>              *
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

#include "serialiser/rsbaseserial.h"

#include "util/rsthreads.h"
#include "util/rsstring.h"
#include "util/rsprint.h"

#include "rsitems/rsitem.h"
#include "rsitems/itempriorities.h"

#include <math.h>
#include <map>
#include <vector>
#include <iostream>
#include <typeinfo>


/***
 * #define RSSERIAL_DEBUG 		1
 * #define RSSERIAL_ERROR_DEBUG		1
***/

// As these represent SERIOUS ERRORs, this debug should be left one.
#define RSSERIAL_ERROR_DEBUG		1


#if defined(RSSERIAL_DEBUG) || defined(RSSERIAL_ERROR_DEBUG)
	#include <sstream>
#endif
	
RsItem::RsItem(uint32_t t)
:type(t) 
{
	_priority_level = QOS_PRIORITY_UNKNOWN ;	// This value triggers PQIInterface to complain about undefined priorities
}


#ifdef DO_STATISTICS
class Counter
{
	public: 
		Counter(int i): _i(i) {}
		Counter(): _i(0) {} 

		int v() const { return _i ; }
		int& v() { return _i ; }
	private:
		int _i ;
};

	static RsMutex smtx ;
	static std::map<int,Counter> size_hits ;
	static int nb_rsitem_creations = 0 ;
	static int total_rsitem_mallocs = 0 ;
	static int total_rsitem_frees = 0 ;
	static int total_rsitem_freed = 0 ;
	static rstime_t last_time = 0 ;

void *RsItem::operator new(size_t s)
{
//	std::cerr << "New RsItem: s=" << s << std::endl;

	RsStackMutex m(smtx) ;

	++size_hits[ s ].v() ;

	rstime_t now = time(NULL);
	++nb_rsitem_creations ;
	total_rsitem_mallocs += s ;

	if(last_time + 20 < now)
	{
		std::cerr << "Memory statistics:" << std::endl;
		std::cerr << "  Total RsItem memory:       " << total_rsitem_mallocs << std::endl;
		std::cerr << "  Total RsItem creations:    " << nb_rsitem_creations << std::endl;
		std::cerr << "  Total RsItem freed memory: " << total_rsitem_freed << std::endl;
		std::cerr << "  Total RsItem deletions:    " << total_rsitem_frees << std::endl;
		std::cerr << "Now printing histogram:" << std::endl;

		for(std::map<int,Counter>::const_iterator it(size_hits.begin());it!=size_hits.end();++it)
			std::cerr << it->first << "  " << it->second.v() << std::endl;
		last_time = now ;
	}

	RsItem *a = static_cast<RsItem*>(::operator new(s)) ;
	return a ;
}
void RsItem::operator delete(void *p,size_t s)
{
//	std::cerr << "Delete RsItem: s=" << s << std::endl;

	RsStackMutex m(smtx) ;
	total_rsitem_freed += s ;
	++total_rsitem_frees ;

	::operator delete(p) ;
}
#endif

RsItem::RsItem(uint8_t ver, uint8_t cls, uint8_t t, uint8_t subtype)
{
	_priority_level = QOS_PRIORITY_UNKNOWN ;	// This value triggers PQIInterface to complain about undefined priorities

	type = (ver << 24) + (cls << 16) + (t << 8) + subtype;
}

RsItem::~RsItem()
{
}

void RsItem::print_string(std::string &out, uint16_t indent)
{
	std::ostringstream stream;
	print(stream, indent);

	out += stream.str();
}
uint32_t    RsItem::PacketId() const
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

	
uint8_t    RsItem::PacketSubType() const
{
	return (type & 0xFF);
}


	/* For Service Packets */	
RsItem::RsItem(uint8_t ver, uint16_t service, uint8_t subtype)
{
	_priority_level = QOS_PRIORITY_UNKNOWN ;	// This value triggers PQIInterface to complain about undefined priorities
	type = (ver << 24) + (service << 8) + subtype;
	return;
}

uint16_t    RsItem::PacketService() const
{
	return (type >> 8) & 0xFFFF;
}

void    RsItem::setPacketService(uint16_t service)
{
	type &= 0xFF0000FF;
	type |= (uint32_t) (service << 8);
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

RsSerialType::RsSerialType(uint8_t ver, uint16_t service)
{
	type = (ver << 24) + (service << 8);
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

bool        RsSerialType::serialise(RsItem */*item*/, void */*data*/, uint32_t */*size*/)
{
    std::cerr << "(EE) Empty method called for missing serialize() method in serializer class " << typeid(this).name() << std::endl;
#ifdef  RSSERIAL_DEBUG
	std::cerr << "RsSerialType::serialise()" << std::endl;
#endif
	return false;
}

RsItem *    RsSerialType::deserialise(void */*data*/, uint32_t */*size*/)
{
#ifdef  RSSERIAL_DEBUG
	std::cerr << "RsSerialType::deserialise()" << std::endl;
#endif
	return NULL;
}

uint32_t    RsSerialType::PacketId() const
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
	for(it = serialisers.begin(); it != serialisers.end(); ++it)
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
		/* remove 8 more bits -> try again */
		type &= 0xFFFF0000;
		if (serialisers.end() == (it = serialisers.find(type)))
		{
			/* one more try */
			type &= 0xFF000000;
			if (serialisers.end() == (it = serialisers.find(type)))
			{

#ifdef  RSSERIAL_ERROR_DEBUG
				std::cerr << "RsSerialiser::size() ERROR serialiser missing!" << std::endl;
			
				std::string out;
				rs_sprintf(out, "%x", item->PacketId());

				std::cerr << "RsSerialiser::size() PacketId: ";
				std::cerr << out;
				std::cerr << std::endl;
#endif
				return 0;
			}
		}
	}

#ifdef  RSSERIAL_DEBUG
	std::string out;
	rs_sprintf(out, "RsSerialiser::size() Item->PacketId(): %x matched to Serialiser Type: %lu", item->PacketId(), type);
	std::cerr << out << std::endl;
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
		/* remove 8 more bits -> try again */
		type &= 0xFFFF0000;
		if (serialisers.end() == (it = serialisers.find(type)))
		{
			/* one more try */
			type &= 0xFF000000;
			if (serialisers.end() == (it = serialisers.find(type)))
			{

#ifdef  RSSERIAL_ERROR_DEBUG
				std::cerr << "RsSerialiser::serialise() ERROR serialiser missing!";
				std::string out;
				rs_sprintf(out, "%x", item->PacketId());

				std::cerr << "RsSerialiser::serialise() PacketId: ";
				std::cerr << out;
				std::cerr << std::endl;
#endif
				return false;
			}
		}
	}

#ifdef  RSSERIAL_DEBUG
	std::string out;
	rs_sprintf(out, "RsSerialiser::serialise() Item->PacketId(): %x matched to Serialiser Type: %lu", item->PacketId(), type);
	std::cerr << out << std::endl;
#endif

	return (it->second)->serialise(item, data, size);
}



RsItem *    RsSerialiser::deserialise(void *data, uint32_t *size)
{
	/* find the type */
	if (*size < 8)
	{
#ifdef  RSSERIAL_ERROR_DEBUG
		std::cerr << "RsSerialiser::deserialise() ERROR Not Enough Data(1)";
		std::cerr << std::endl;
#endif
		return NULL;
	}

	uint32_t type = (getRsItemId(data) & 0xFFFFFF00);
	uint32_t pkt_size = getRsItemSize(data);

	//std::cerr << "RsSerialiser::deserialise() RsItem Type: " << std::hex << getRsItemId(data) << " Size: " << pkt_size;
	//std::cerr << std::endl;

	if (pkt_size > *size)
	{
#ifdef  RSSERIAL_ERROR_DEBUG
		std::cerr << "RsSerialiser::deserialise() ERROR Size mismatch(2)";
		std::cerr << std::endl;
#endif
		return NULL;
	}
	if(pkt_size > getRsPktMaxSize())
	{
	   std::cerr << "(EE) trying to deserialise a packet with absurdely large size " << pkt_size << ". This means there's a bug upward or packet corruption. Packet content: " << RsUtil::BinToHex((unsigned char*)data,std::min(300u,pkt_size)) ;
	   return NULL ;
	}

	/* store the packet size to return the amount we should use up */
	*size = pkt_size;

	std::map<uint32_t, RsSerialType *>::iterator it;
	if (serialisers.end() == (it = serialisers.find(type)))
	{
		/* remove 8 more bits -> try again */
		type &= 0xFFFF0000;
		if (serialisers.end() == (it = serialisers.find(type)))
		{
			/* one more try */
			type &= 0xFF000000;
			if (serialisers.end() == (it = serialisers.find(type)))
			{

#ifdef  RSSERIAL_ERROR_DEBUG
				std::cerr << "RsSerialiser::deserialise() ERROR deserialiser missing!";
				std::string out;
				rs_sprintf(out, "%x", getRsItemId(data));

				std::cerr << "RsSerialiser::deserialise() PacketId: ";
				std::cerr << out << std::endl;
#endif
				return NULL;
			}
		}
	}

	RsItem *item = (it->second)->deserialise(data, &pkt_size);
	if (!item)
	{
#ifdef  RSSERIAL_ERROR_DEBUG
		std::cerr << "RsSerialiser::deserialise() ERROR Failed!";
		std::cerr << std::endl;
		std::cerr << "RsSerialiser::deserialise() pkt_size: " << pkt_size << " vs *size: " << *size;
        std::cerr << std::endl;

        //RsItem *item2 = (it->second)->deserialise(data, &pkt_size);

		uint32_t failedtype = getRsItemId(data);
		std::cerr << "RsSerialiser::deserialise() FAILED PACKET Size: ";
		std::cerr << getRsItemSize(data) << " ID: ";
		std::cerr << std::hex << failedtype << std::endl;
		std::cerr << "RsSerialiser::deserialise() FAILED PACKET: ";
		std::cerr << " Version: " << std::hex << (uint32_t) getRsItemVersion(failedtype) << std::dec;
		std::cerr << " Class: " << std::hex << (uint32_t) getRsItemClass(failedtype) << std::dec;
		std::cerr << " Type: " << std::hex << (uint32_t) getRsItemType(failedtype) << std::dec;
		std::cerr << " SubType: " << std::hex << (uint32_t) getRsItemSubType(failedtype) << std::dec;
        std::cerr << " Data: " << RsUtil::BinToHex((char*)data,pkt_size).substr(0,300) << std::endl;
        std::cerr << std::endl;
#endif
		return NULL;
	}

	if (pkt_size != *size)
	{
#ifdef  RSSERIAL_ERROR_DEBUG
		std::cerr << "RsSerialiser::deserialise() ERROR: size mismatch!";
		std::cerr << std::endl;
		std::cerr << "RsSerialiser::deserialise() pkt_size: " << pkt_size << " vs *size: " << *size;
		std::cerr << std::endl;

		uint32_t failedtype = getRsItemId(data);
		std::cerr << "RsSerialiser::deserialise() FAILED PACKET Size: ";
		std::cerr << getRsItemSize(data) << " ID: ";
		std::cerr << std::hex << failedtype << std::dec;
		std::cerr << "RsSerialiser::deserialise() FAILED PACKET: ";
		std::cerr << " Version: " << std::hex << (uint32_t) getRsItemVersion(failedtype) << std::dec;
		std::cerr << " Class: " << std::hex << (uint32_t) getRsItemClass(failedtype) << std::dec;
		std::cerr << " Type: " << std::hex << (uint32_t) getRsItemType(failedtype) << std::dec;
        std::cerr << " SubType: " << std::hex << (uint32_t) getRsItemSubType(failedtype) << std::dec;
        std::cerr << " Data: " << RsUtil::BinToHex((char*)data,pkt_size).substr(0,300) << std::endl;
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
	uint32_t type = 0;
	uint32_t offset = 0;
	getRawUInt32(data, 4, &offset, &type);
	return type;
}


uint32_t getRsItemSize(void *data)
{
	uint32_t size = 0;
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

uint16_t  getRsItemService(uint32_t type)
{
	return (type >> 8) & 0xFFFF;
}


std::ostream &printRsItemBase(std::ostream &out, std::string clsName, uint16_t indent)
{
        printIndent(out, indent);
	out << "RsItem: " << clsName << " ####################################";
        out << std::endl;
        return out;
}

std::ostream &printRsItemEnd(std::ostream &out, std::string clsName, uint16_t indent)
{
        printIndent(out, indent);
        out << "###################### " << clsName << " #####################";
        out << std::endl;
	return out;
}

std::ostream &RsRawItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsRawItem", indent);
	printIndent(out, indent);
	out << "Size: " << len << std::endl;
	printRsItemEnd(out, "RsRawItem", indent);
	return out;
}


uint32_t getRsPktMaxSize()
{
	//return 65535; /* 2^16 (old artifical low size) */
	//return 1048575; /* 2^20 -1 (Too Big! - must remove fixed static buffers first) */
	/* Remember that every pqistreamer allocates an input buffer of this size!
	 * So don't make it too big!
	 */
	return 262143; /* 2^18 -1 */
}


uint32_t getRsPktBaseSize()
{
	return 8; /* 4 + 4 */
}

