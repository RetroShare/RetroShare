/*
 * libretroshare/src/services: ftturtlefiletransferitem.cc
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

#include <iostream>
#include <stdexcept>

#include <util/rsmemory.h>
#include <serialiser/itempriorities.h>
#include <ft/ftturtlefiletransferitem.h>

uint32_t RsTurtleFileRequestItem::serial_size()
{
	uint32_t s = 0 ;

	s += 8 ; // header
	s += 4 ; // tunnel id 
	s += 8 ; // file offset
	s += 4 ; // chunk size

	return s ;
}

uint32_t RsTurtleFileDataItem::serial_size()
{
	uint32_t s = 0 ;

	s += 8 ; // header
	s += 4 ; // tunnel id 
	s += 8 ; // file offset
	s += 4 ; // chunk size
	s += chunk_size ;	// actual data size.

	return s ;
}

uint32_t RsTurtleFileMapRequestItem::serial_size()
{
	uint32_t s = 0 ;

	s += 8 ; // header
	s += 4 ; // tunnel id 
	s += 4 ; // direction

	return s ;
}

uint32_t RsTurtleFileMapItem::serial_size()
{
	uint32_t s = 0 ;

	s += 8 ; // header
	s += 4 ; // tunnel id 
	s += 4 ; // direction
	s += 4 ; // compressed_map.size()

	s += 4 * compressed_map._map.size() ;

	return s ;
}

uint32_t RsTurtleChunkCrcItem::serial_size()
{
	uint32_t s = 0 ;

	s += 8 ; // header
	s += 4 ; // tunnel id 
	s += 4 ; // chunk number
	s += check_sum.serial_size() ; // check_sum 

	return s ;
}
uint32_t RsTurtleChunkCrcRequestItem::serial_size()
{
	uint32_t s = 0 ;

	s += 8 ; // header
	s += 4 ; // tunnel id 
	s += 4 ; // chunk number

	return s ;
}
bool RsTurtleFileMapRequestItem::serialize(void *data,uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size();
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data,tlvsize,PacketId(), tlvsize);

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */

	ok &= setRawUInt32(data, tlvsize, &offset, tunnel_id);
	ok &= setRawUInt32(data, tlvsize, &offset, direction);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsFileConfigSerialiser::serialiseTransfer() Size Error! " << std::endl;
#endif
	}

	return ok;
}

bool RsTurtleFileMapItem::serialize(void *data,uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size();
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data,tlvsize,PacketId(), tlvsize);

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */

	ok &= setRawUInt32(data, tlvsize, &offset, tunnel_id);
	ok &= setRawUInt32(data, tlvsize, &offset, direction);
	ok &= setRawUInt32(data, tlvsize, &offset, compressed_map._map.size());

	for(uint32_t i=0;i<compressed_map._map.size() && ok;++i)
		ok &= setRawUInt32(data, tlvsize, &offset, compressed_map._map[i]);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsFileConfigSerialiser::serialiseTransfer() Size Error! " << std::endl;
#endif
	}

	return ok;
}

bool RsTurtleChunkCrcRequestItem::serialize(void *data,uint32_t& pktsize)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "RsTurtleChunkCrcRequestItem::serialize(): serializing packet:" << std::endl ;
	print(std::cerr,2) ;
#endif
	uint32_t tlvsize = serial_size();
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data,tlvsize,PacketId(), tlvsize);

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */

	ok &= setRawUInt32(data, tlvsize, &offset, tunnel_id);
	ok &= setRawUInt32(data, tlvsize, &offset, chunk_number);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsFileConfigSerialiser::serialiseTransfer() Size Error! " << std::endl;
	}

	return ok;
}

bool RsTurtleChunkCrcItem::serialize(void *data,uint32_t& pktsize)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "RsTurtleChunkCrcRequestItem::serialize(): serializing packet:" << std::endl ;
	print(std::cerr,2) ;
#endif
	uint32_t tlvsize = serial_size();
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data,tlvsize,PacketId(), tlvsize);

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */

	ok &= setRawUInt32(data, tlvsize, &offset, tunnel_id);
	ok &= setRawUInt32(data, tlvsize, &offset, chunk_number);
	ok &= check_sum.serialise(data, tlvsize, offset) ;

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsFileConfigSerialiser::serialiseTransfer() Size Error! " << std::endl;
	}

	return ok;
}
RsTurtleFileMapItem::RsTurtleFileMapItem(void *data,uint32_t pktsize)
	: RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_FILE_MAP)
{
	setPriorityLevel(QOS_PRIORITY_RS_TURTLE_FILE_MAP) ;
#ifdef P3TURTLE_DEBUG
	std::cerr << "  type = file map item" << std::endl ;
#endif
	uint32_t offset = 8; // skip the header 

	/* add mandatory parts first */

	bool ok = true ;
	uint32_t s,d ;
	ok &= getRawUInt32(data, pktsize, &offset, &tunnel_id);
	ok &= getRawUInt32(data, pktsize, &offset, &d);
	direction = d ;
	ok &= getRawUInt32(data, pktsize, &offset, &s) ;

	compressed_map._map.resize(s) ;

	for(uint32_t i=0;i<s && ok;++i)
		ok &= getRawUInt32(data, pktsize, &offset, &(compressed_map._map[i])) ;

#ifdef WINDOWS_SYS // No Exceptions in Windows compile. (drbobs).
#else
	if (offset != pktsize)
		throw std::runtime_error("Size error while deserializing.") ;
	if (!ok)
		throw std::runtime_error("Unknown error while deserializing.") ;
#endif
}

RsTurtleFileMapRequestItem::RsTurtleFileMapRequestItem(void *data,uint32_t pktsize)
	: RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_FILE_MAP_REQUEST)
{
	setPriorityLevel(QOS_PRIORITY_RS_TURTLE_FILE_MAP_REQUEST) ;
#ifdef P3TURTLE_DEBUG
	std::cerr << "  type = file map request item" << std::endl ;
#endif
	uint32_t offset = 8; // skip the header 

	/* add mandatory parts first */

	bool ok = true ;
	ok &= getRawUInt32(data, pktsize, &offset, &tunnel_id);
	ok &= getRawUInt32(data, pktsize, &offset, &direction);

#ifdef WINDOWS_SYS // No Exceptions in Windows compile. (drbobs).
#else
	if (offset != pktsize)
		throw std::runtime_error("Size error while deserializing.") ;
	if (!ok)
		throw std::runtime_error("Unknown error while deserializing.") ;
#endif
}

RsTurtleChunkCrcItem::RsTurtleChunkCrcItem(void *data,uint32_t pktsize)
	: RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_CHUNK_CRC)
{
	setPriorityLevel(QOS_PRIORITY_RS_TURTLE_CHUNK_CRC) ;
#ifdef P3TURTLE_DEBUG
	std::cerr << "  type = file map item" << std::endl ;
#endif
	uint32_t offset = 8; // skip the header 

	/* add mandatory parts first */

	bool ok = true ;
	ok &= getRawUInt32(data, pktsize, &offset, &tunnel_id) ;
	ok &= getRawUInt32(data, pktsize, &offset, &chunk_number) ;
	ok &= check_sum.deserialise(data, pktsize, offset) ;

#ifdef WINDOWS_SYS // No Exceptions in Windows compile. (drbobs).
#else
	if (offset != pktsize)
		throw std::runtime_error("Size error while deserializing.") ;
	if (!ok)
		throw std::runtime_error("Unknown error while deserializing.") ;
#endif
}

RsTurtleChunkCrcRequestItem::RsTurtleChunkCrcRequestItem(void *data,uint32_t pktsize)
	: RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_CHUNK_CRC_REQUEST)
{
	setPriorityLevel(QOS_PRIORITY_RS_TURTLE_CHUNK_CRC_REQUEST) ;
#ifdef P3TURTLE_DEBUG
	std::cerr << "  type = file map item" << std::endl ;
#endif
	uint32_t offset = 8; // skip the header 

	/* add mandatory parts first */

	bool ok = true ;
	ok &= getRawUInt32(data, pktsize, &offset, &tunnel_id) ;
	ok &= getRawUInt32(data, pktsize, &offset, &chunk_number) ;

#ifdef WINDOWS_SYS // No Exceptions in Windows compile. (drbobs).
#else
	if (offset != pktsize)
		throw std::runtime_error("Size error while deserializing.") ;
	if (!ok)
		throw std::runtime_error("Unknown error while deserializing.") ;
#endif
}
bool RsTurtleFileRequestItem::serialize(void *data,uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size();
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data,tlvsize,PacketId(), tlvsize);

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */

	ok &= setRawUInt32(data, tlvsize, &offset, tunnel_id) ;
	ok &= setRawUInt64(data, tlvsize, &offset, chunk_offset);
	ok &= setRawUInt32(data, tlvsize, &offset, chunk_size);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsTurtleTunnelOkItem::serialiseTransfer() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsTurtleFileRequestItem::RsTurtleFileRequestItem(void *data,uint32_t pktsize)
	: RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_FILE_REQUEST)
{
	setPriorityLevel(QOS_PRIORITY_RS_TURTLE_FILE_REQUEST) ;
#ifdef P3TURTLE_DEBUG
	std::cerr << "  type = file request" << std::endl ;
#endif
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);

	/* add mandatory parts first */

	bool ok = true ;
	ok &= getRawUInt32(data, pktsize, &offset, &tunnel_id) ;
	ok &= getRawUInt64(data, pktsize, &offset, &chunk_offset);
	ok &= getRawUInt32(data, pktsize, &offset, &chunk_size);
#ifdef P3TURTLE_DEBUG
	std::cerr << "  tunnel_id=" << (void*)tunnel_id << ", chunk_offset=" << chunk_offset << ", chunk_size=" << chunk_size << std::endl ;
#endif

#ifdef WINDOWS_SYS // No Exceptions in Windows compile. (drbobs).
	UNREFERENCED_LOCAL_VARIABLE(rssize);
#else
	if (offset != rssize)
		throw std::runtime_error("RsTurtleTunnelOkItem::() error while deserializing.") ;
	if (!ok)
		throw std::runtime_error("RsTurtleTunnelOkItem::() unknown error while deserializing.") ;
#endif
}
	
RsTurtleFileDataItem::~RsTurtleFileDataItem()
{
	free(chunk_data) ;
}
RsTurtleFileDataItem::RsTurtleFileDataItem(void *data,uint32_t pktsize)
	: RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_FILE_DATA)
{
	setPriorityLevel(QOS_PRIORITY_RS_TURTLE_FILE_DATA) ;
#ifdef P3TURTLE_DEBUG
	std::cerr << "  type = file request" << std::endl ;
#endif
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);

	bool ok = true ;
    
    	if(rssize > pktsize)
            ok = false ;
        
	/* add mandatory parts first */
	ok &= getRawUInt32(data, pktsize, &offset, &tunnel_id) ;
	ok &= getRawUInt64(data, pktsize, &offset, &chunk_offset);
	ok &= getRawUInt32(data, pktsize, &offset, &chunk_size);

    	if(chunk_size > rssize || rssize - chunk_size < offset)
	    throw std::runtime_error("RsTurtleFileDataItem::() error while deserializing.") ;
        
	chunk_data = (void*)rs_safe_malloc(chunk_size) ;
        
        if(chunk_data == NULL)
	    throw std::runtime_error("RsTurtleFileDataItem::() cannot allocate memory.") ;
            
	memcpy(chunk_data,(void*)((unsigned char*)data+offset),chunk_size) ;

	offset += chunk_size ;

#ifdef P3TURTLE_DEBUG
	std::cerr << "  tunnel_id=" << (void*)tunnel_id << ", chunk_offset=" << chunk_offset << ", chunk_size=" << chunk_size << std::endl ;
#endif

#ifdef WINDOWS_SYS // No Exceptions in Windows compile. (drbobs).
	UNREFERENCED_LOCAL_VARIABLE(rssize);
#else
	if (offset != rssize)
		throw std::runtime_error("RsTurtleFileDataItem::() error while deserializing.") ;
	if (!ok)
		throw std::runtime_error("RsTurtleFileDataItem::() unknown error while deserializing.") ;
#endif
}

bool RsTurtleFileDataItem::serialize(void *data,uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size();
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data,tlvsize,PacketId(), tlvsize);

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */

	ok &= setRawUInt32(data, tlvsize, &offset, tunnel_id) ;
	ok &= setRawUInt64(data, tlvsize, &offset, chunk_offset);
	ok &= setRawUInt32(data, tlvsize, &offset, chunk_size);

	memcpy((void*)((unsigned char*)data+offset),chunk_data,chunk_size) ;
	offset += chunk_size ;

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsTurtleTunnelOkItem::serialiseTransfer() Size Error! " << std::endl;
#endif
	}

	return ok;
}
std::ostream& RsTurtleFileRequestItem::print(std::ostream& o, uint16_t)
{
	o << "File request item:" << std::endl ;

	o << "  tunnel id : " << std::hex << tunnel_id << std::dec << std::endl ;
	o << "  offset    : " << chunk_offset << std::endl ;
	o << "  chunk size: " << chunk_size << std::endl ;

	return o ;
}

std::ostream& RsTurtleFileDataItem::print(std::ostream& o, uint16_t)
{
	o << "File request item:" << std::endl ;

	o << "  tunnel id : " << std::hex << tunnel_id << std::dec << std::endl ;
	o << "  offset    : " << chunk_offset << std::endl ;
	o << "  chunk size: " << chunk_size << std::endl ;
	o << "  data      : " << std::hex << chunk_data << std::dec << std::endl ;

	return o ;
}

std::ostream& RsTurtleFileMapItem::print(std::ostream& o, uint16_t)
{
	o << "File map item:" << std::endl ;

	o << "  tunnel id : " << std::hex << tunnel_id << std::dec << std::endl ;
	o << "  direction : " << direction << std::endl ;
	o << "  map      : " ;

	for(uint32_t i=0;i<compressed_map._map.size();++i)
		o << std::hex << compressed_map._map[i] << std::dec << std::endl ;

	return o ;
}

std::ostream& RsTurtleFileMapRequestItem::print(std::ostream& o, uint16_t)
{
	o << "File map request item:" << std::endl ;

	o << "  tunnel id : " << std::hex << tunnel_id << std::dec << std::endl ;
	o << "  direction : " << direction << std::endl ;

	return o ;
}


std::ostream& RsTurtleChunkCrcRequestItem::print(std::ostream& o, uint16_t)
{
	o << "Chunk CRC request item:" << std::endl ;

	o << "  tunnel id : " << std::hex << tunnel_id << std::dec << std::endl ;
	o << "  chunk num : " << chunk_number << std::endl ;

	return o ;
}
std::ostream& RsTurtleChunkCrcItem::print(std::ostream& o, uint16_t)
{
	o << "Chunk CRC item:" << std::endl ;

	o << "  tunnel id : " << std::hex << tunnel_id << std::dec << std::endl ;
	o << "  chunk num : " << chunk_number << std::endl ;
	o << "   sha1 sum : " << check_sum.toStdString() << std::endl ;

	return o ;
}

