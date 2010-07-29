#ifndef WINDOWS_SYS
#include <stdexcept>
#endif
#include <iostream>
#include "turtletypes.h"
#include "rsturtleitem.h"

//#define P3TURTLE_DEBUG
// -----------------------------------------------------------------------------------//
// --------------------------------  Serialization. --------------------------------- // 
// -----------------------------------------------------------------------------------//
//

//
// ---------------------------------- Packet sizes -----------------------------------//
//

uint32_t RsTurtleStringSearchRequestItem::serial_size() 
{
	uint32_t s = 0 ;

	s += 8 ; // header
	s += 4 ; // request_id
	s += 2 ; // depth
	s += GetTlvStringSize(match_string) ;	// match_string

	return s ;
}
uint32_t RsTurtleRegExpSearchRequestItem::serial_size() 
{
	uint32_t s = 0 ;

	s += 8 ; // header
	s += 4 ; // request_id
	s += 2 ; // depth

	s += 4 ; // number of strings

	for(unsigned int i=0;i<expr._strings.size();++i)
		s += GetTlvStringSize(expr._strings[i]) ;

	s += 4 ; // number of ints
	s += 4 * expr._ints.size() ;
	s += 4 ; // number of tokens
	s += expr._tokens.size() ;		// uint8_t has size 1

	return s ;
}

uint32_t RsTurtleSearchResultItem::serial_size()
{
	uint32_t s = 0 ;

	s += 8 ; // header
	s += 4 ; // search request id
	s += 2 ; // depth
	s += 4 ; // number of results

	for(std::list<TurtleFileInfo>::const_iterator it(result.begin());it!=result.end();++it)
	{
		s += 8 ;											// file size
		s += GetTlvStringSize(it->hash) ;		// file hash
		s += GetTlvStringSize(it->name) ;		// file name
	}

	return s ;
}

uint32_t RsTurtleOpenTunnelItem::serial_size()
{
	uint32_t s = 0 ;

	s += 8 ; // header
	s += GetTlvStringSize(file_hash) ;		// file hash
	s += 4 ; // tunnel request id
	s += 4 ; // partial tunnel id 
	s += 2 ; // depth

	return s ;
}

uint32_t RsTurtleTunnelOkItem::serial_size()
{
	uint32_t s = 0 ;

	s += 8 ; // header
	s += 4 ; // tunnel id 
	s += 4 ; // tunnel request id

	return s ;
}

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

uint32_t RsTurtleFileCrcRequestItem::serial_size()
{
	uint32_t s = 0 ;

	s += 8 ; // header
	s += 4 ; // tunnel id 

	return s ;
}

uint32_t RsTurtleFileCrcItem::serial_size()
{
	uint32_t s = 0 ;

	s += 8 ; // header
	s += 4 ; // tunnel id 

	s += 4 ; // size of _map
	s += 4 ; // size of _crcs

	s += 4 * crc_map._crcs.size() ;
	s += 4 * crc_map._ccmap._map.size() ;

	return s ;
}
//
// ---------------------------------- Serialization ----------------------------------//
//
RsItem *RsTurtleSerialiser::deserialise(void *data, uint32_t *size) 
{
	// look what we have...
	
	/* get the type */
	uint32_t rstype = getRsItemId(data);
#ifdef P3TURTLE_DEBUG
	std::cerr << "p3turtle: deserialising packet: " << std::endl ;
#endif
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (RS_SERVICE_TYPE_TURTLE != getRsItemService(rstype))) 
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "  Wrong type !!" << std::endl ;
#endif
		return NULL; /* wrong type */
	}

#ifndef WINDOWS_SYS
	try
	{
#endif
		switch(getRsItemSubType(rstype))
		{
			case RS_TURTLE_SUBTYPE_STRING_SEARCH_REQUEST	:	return new RsTurtleStringSearchRequestItem(data,*size) ;
			case RS_TURTLE_SUBTYPE_REGEXP_SEARCH_REQUEST	:	return new RsTurtleRegExpSearchRequestItem(data,*size) ;
			case RS_TURTLE_SUBTYPE_SEARCH_RESULT			:	return new RsTurtleSearchResultItem(data,*size) ;
			case RS_TURTLE_SUBTYPE_OPEN_TUNNEL  			:	return new RsTurtleOpenTunnelItem(data,*size) ;
			case RS_TURTLE_SUBTYPE_TUNNEL_OK    			:	return new RsTurtleTunnelOkItem(data,*size) ;
			case RS_TURTLE_SUBTYPE_FILE_REQUEST 			:	return new RsTurtleFileRequestItem(data,*size) ;
			case RS_TURTLE_SUBTYPE_FILE_DATA    			:	return new RsTurtleFileDataItem(data,*size) ;
			case RS_TURTLE_SUBTYPE_FILE_MAP_REQUEST		:	return new RsTurtleFileMapRequestItem(data,*size) ;
			case RS_TURTLE_SUBTYPE_FILE_MAP     			:	return new RsTurtleFileMapItem(data,*size) ;
			case RS_TURTLE_SUBTYPE_FILE_CRC_REQUEST		:	return new RsTurtleFileCrcRequestItem(data,*size) ;
			case RS_TURTLE_SUBTYPE_FILE_CRC     			:	return new RsTurtleFileCrcItem(data,*size) ;

			default:
																std::cerr << "Unknown packet type in RsTurtle!" << std::endl ;
																return NULL ;
		}
#ifndef WINDOWS_SYS
	}
	catch(std::exception& e)
	{
		std::cerr << "Exception raised: " << e.what() << std::endl ;
		return NULL ;
	}
#endif

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

bool RsTurtleFileCrcRequestItem::serialize(void *data,uint32_t& pktsize)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "RsTurtleFileCrcRequestItem::serialize(): serializing packet:" << std::endl ;
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

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsFileConfigSerialiser::serialiseTransfer() Size Error! " << std::endl;
	}

	return ok;
}

bool RsTurtleFileCrcItem::serialize(void *data,uint32_t& pktsize)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "RsTurtleFileCrcItem::serialize(): serializing packet:" << std::endl ;
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
	ok &= setRawUInt32(data, tlvsize, &offset, crc_map._ccmap._map.size());
	ok &= setRawUInt32(data, tlvsize, &offset, crc_map._crcs.size());

	for(uint32_t i=0;i<crc_map._ccmap._map.size() && ok;++i)
		ok &= setRawUInt32(data, tlvsize, &offset, crc_map._ccmap._map[i]);

	for(uint32_t i=0;i<crc_map._crcs.size() && ok;++i)
		ok &= setRawUInt32(data, tlvsize, &offset, crc_map._crcs[i]);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsFileConfigSerialiser::serialiseTransfer() Size Error! " << std::endl;
	}

	return ok;
}

bool RsTurtleStringSearchRequestItem::serialize(void *data,uint32_t& pktsize)
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

	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, match_string);
	ok &= setRawUInt32(data, tlvsize, &offset, request_id);
	ok &= setRawUInt16(data, tlvsize, &offset, depth);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsFileConfigSerialiser::serialiseTransfer() Size Error! " << std::endl;
#endif
	}

	return ok;
}

bool RsTurtleRegExpSearchRequestItem::serialize(void *data,uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size();
	uint32_t offset = 0;

#ifdef P3TURTLE_DEBUG
	std::cerr << "RsTurtleSerialiser::serialising RegExp search packet (size=" << tlvsize << ")" << std::endl;
#endif
	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data,tlvsize,PacketId(), tlvsize);

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */

	ok &= setRawUInt32(data, tlvsize, &offset, request_id);
	ok &= setRawUInt16(data, tlvsize, &offset, depth);

	// now serialize the regexp
	ok &= setRawUInt32(data,tlvsize,&offset,expr._tokens.size()) ;

	for(unsigned int i=0;i<expr._tokens.size();++i) ok &= setRawUInt8(data,tlvsize,&offset,expr._tokens[i]) ;

	ok &= setRawUInt32(data,tlvsize,&offset,expr._ints.size()) ;

	for(unsigned int i=0;i<expr._ints.size();++i) ok &= setRawUInt32(data,tlvsize,&offset,expr._ints[i]) ;

	ok &= setRawUInt32(data,tlvsize,&offset,expr._strings.size()) ;

	for(unsigned int i=0;i<expr._strings.size();++i) ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, expr._strings[i]); 	


	if (offset != tlvsize)
	{
		ok = false;
#ifdef P3TURTLE_DEBUG
		std::cerr << "RsTurtleSerialiser::serialiseTransfer() Size Error! (offset=" << offset << ", tlvsize=" << tlvsize << ")" << std::endl;
#endif
	}

	return ok;
}

RsTurtleStringSearchRequestItem::RsTurtleStringSearchRequestItem(void *data,uint32_t pktsize)
	: RsTurtleSearchRequestItem(RS_TURTLE_SUBTYPE_STRING_SEARCH_REQUEST)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "  deserializibg packet. type = search request (string)" << std::endl ;
#endif
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

	ok &= GetTlvString(data, pktsize, &offset, TLV_TYPE_STR_VALUE, match_string); 	// file hash
	ok &= getRawUInt32(data, pktsize, &offset, &request_id);
	ok &= getRawUInt16(data, pktsize, &offset, &depth);

#ifdef WINDOWS_SYS // No Exceptions in Windows compile. (drbobs).
	UNREFERENCED_LOCAL_VARIABLE(rssize);
#else
	if (offset != rssize)
		throw std::runtime_error("Size error while deserializing.") ;
	if (!ok)
		throw std::runtime_error("Unknown error while deserializing.") ;
#endif
}

RsTurtleRegExpSearchRequestItem::RsTurtleRegExpSearchRequestItem(void *data,uint32_t pktsize)
	: RsTurtleSearchRequestItem(RS_TURTLE_SUBTYPE_REGEXP_SEARCH_REQUEST)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "  deserializibg packet. type = search request (regexp)" << std::endl ;
#endif
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

	ok &= getRawUInt32(data, pktsize, &offset, &request_id);
	ok &= getRawUInt16(data, pktsize, &offset, &depth);

	// now serialize the regexp
	uint32_t n =0 ;
	ok &= getRawUInt32(data,pktsize,&offset,&n) ;

	expr._tokens.resize(n) ;

	for(uint32_t i=0;i<n;++i) ok &= getRawUInt8(data,pktsize,&offset,&expr._tokens[i]) ;

	ok &= getRawUInt32(data,pktsize,&offset,&n) ;
	expr._ints.resize(n) ;

	for(uint32_t i=0;i<n;++i) ok &= getRawUInt32(data,pktsize,&offset,&expr._ints[i]) ;

	ok &= getRawUInt32(data,pktsize,&offset,&n) ;
	expr._strings.resize(n) ;

	for(uint32_t i=0;i<n;++i) ok &= GetTlvString(data, pktsize, &offset, TLV_TYPE_STR_VALUE, expr._strings[i]); 	

#ifdef WINDOWS_SYS // No Exceptions in Windows compile. (drbobs).
	UNREFERENCED_LOCAL_VARIABLE(rssize);
#else
	if (offset != rssize)
		throw std::runtime_error("Size error while deserializing.") ;
	if (!ok)
		throw std::runtime_error("Unknown error while deserializing.") ;
#endif
}

bool RsTurtleSearchResultItem::serialize(void *data,uint32_t& pktsize)
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

	ok &= setRawUInt32(data, tlvsize, &offset, request_id);
	ok &= setRawUInt16(data, tlvsize, &offset, depth);
	ok &= setRawUInt32(data, tlvsize, &offset, result.size());

	for(std::list<TurtleFileInfo>::const_iterator it(result.begin());it!=result.end();++it)
	{
		ok &= setRawUInt64(data, tlvsize, &offset, it->size); 								// file size
		ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_HASH_SHA1, it->hash);	// file hash
		ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_NAME, it->name); 		// file name
	}

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsFileConfigSerialiser::serialiseTransfer() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsTurtleFileMapItem::RsTurtleFileMapItem(void *data,uint32_t pktsize)
	: RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_FILE_MAP)
{
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

RsTurtleFileCrcItem::RsTurtleFileCrcItem(void *data,uint32_t pktsize)
	: RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_FILE_CRC)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "  type = file map item" << std::endl ;
#endif
	uint32_t offset = 8; // skip the header 

	/* add mandatory parts first */

	bool ok = true ;
	uint32_t s1,s2 ;
	ok &= getRawUInt32(data, pktsize, &offset, &tunnel_id);
	ok &= getRawUInt32(data, pktsize, &offset, &s1) ;
	ok &= getRawUInt32(data, pktsize, &offset, &s2) ;

	crc_map._ccmap._map.resize(s1) ;
	crc_map._crcs.resize(s2) ;

	for(uint32_t i=0;i<s1 && ok;++i)
		ok &= getRawUInt32(data, pktsize, &offset, &(crc_map._ccmap._map[i])) ;

	for(uint32_t i=0;i<s2 && ok;++i)
		ok &= getRawUInt32(data, pktsize, &offset, &(crc_map._crcs[i])) ;

#ifdef WINDOWS_SYS // No Exceptions in Windows compile. (drbobs).
#else
	if (offset != pktsize)
		throw std::runtime_error("Size error while deserializing.") ;
	if (!ok)
		throw std::runtime_error("Unknown error while deserializing.") ;
#endif
}

RsTurtleFileCrcRequestItem::RsTurtleFileCrcRequestItem(void *data,uint32_t pktsize)
	: RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_FILE_CRC_REQUEST)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "  type = file map request item" << std::endl ;
#endif
	uint32_t offset = 8; // skip the header 

	/* add mandatory parts first */

	bool ok = true ;
	ok &= getRawUInt32(data, pktsize, &offset, &tunnel_id);

#ifdef WINDOWS_SYS // No Exceptions in Windows compile. (drbobs).
#else
	if (offset != pktsize)
		throw std::runtime_error("Size error while deserializing.") ;
	if (!ok)
		throw std::runtime_error("Unknown error while deserializing.") ;
#endif
}

RsTurtleSearchResultItem::RsTurtleSearchResultItem(void *data,uint32_t pktsize)
	: RsTurtleItem(RS_TURTLE_SUBTYPE_SEARCH_RESULT)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "  type = search result" << std::endl ;
#endif
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);

	/* add mandatory parts first */

	bool ok = true ;
	uint32_t s ;
	ok &= getRawUInt32(data, pktsize, &offset, &request_id);
	ok &= getRawUInt16(data, pktsize, &offset, &depth);
	ok &= getRawUInt32(data, pktsize, &offset, &s) ;
#ifdef P3TURTLE_DEBUG
	std::cerr << "  request_id=" << request_id << ", depth=" << depth << ", s=" << s << std::endl ;
#endif

	result.clear() ;

	for(int i=0;i<(int)s;++i)
	{
		TurtleFileInfo f ;

		ok &= getRawUInt64(data, pktsize, &offset, &(f.size)); 									// file size
		ok &= GetTlvString(data, pktsize, &offset, TLV_TYPE_STR_HASH_SHA1, f.hash); 	// file hash
		ok &= GetTlvString(data, pktsize, &offset, TLV_TYPE_STR_NAME, f.name); 			// file name

		result.push_back(f) ;
	}

#ifdef WINDOWS_SYS // No Exceptions in Windows compile. (drbobs).
	UNREFERENCED_LOCAL_VARIABLE(rssize);
#else
	if (offset != rssize)
		throw std::runtime_error("Size error while deserializing.") ;
	if (!ok)
		throw std::runtime_error("Unknown error while deserializing.") ;
#endif
}

bool RsTurtleOpenTunnelItem::serialize(void *data,uint32_t& pktsize)
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

	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_HASH_SHA1, file_hash);	// file hash
	ok &= setRawUInt32(data, tlvsize, &offset, request_id);
	ok &= setRawUInt32(data, tlvsize, &offset, partial_tunnel_id);
	ok &= setRawUInt16(data, tlvsize, &offset, depth);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsTurtleOpenTunnelItem::serialiseTransfer() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsTurtleOpenTunnelItem::RsTurtleOpenTunnelItem(void *data,uint32_t pktsize)
	: RsTurtleItem(RS_TURTLE_SUBTYPE_OPEN_TUNNEL)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "  type = open tunnel" << std::endl ;
#endif
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);

	/* add mandatory parts first */

	bool ok = true ;
	ok &= GetTlvString(data, pktsize, &offset, TLV_TYPE_STR_HASH_SHA1, file_hash); 	// file hash
	ok &= getRawUInt32(data, pktsize, &offset, &request_id);
	ok &= getRawUInt32(data, pktsize, &offset, &partial_tunnel_id) ;
	ok &= getRawUInt16(data, pktsize, &offset, &depth);
#ifdef P3TURTLE_DEBUG
	std::cerr << "  request_id=" << (void*)request_id << ", partial_id=" << (void*)partial_tunnel_id << ", depth=" << depth << ", hash=" << file_hash << std::endl ;
#endif

#ifdef WINDOWS_SYS // No Exceptions in Windows compile. (drbobs).
	UNREFERENCED_LOCAL_VARIABLE(rssize);
#else
	if (offset != rssize)
		throw std::runtime_error("RsTurtleOpenTunnelItem::() error while deserializing.") ;
	if (!ok)
		throw std::runtime_error("RsTurtleOpenTunnelItem::() unknown error while deserializing.") ;
#endif
}

bool RsTurtleTunnelOkItem::serialize(void *data,uint32_t& pktsize)
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
	ok &= setRawUInt32(data, tlvsize, &offset, request_id);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsTurtleTunnelOkItem::serialiseTransfer() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsTurtleTunnelOkItem::RsTurtleTunnelOkItem(void *data,uint32_t pktsize)
	: RsTurtleItem(RS_TURTLE_SUBTYPE_TUNNEL_OK)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "  type = tunnel ok" << std::endl ;
#endif
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);

	/* add mandatory parts first */

	bool ok = true ;
	ok &= getRawUInt32(data, pktsize, &offset, &tunnel_id) ;
	ok &= getRawUInt32(data, pktsize, &offset, &request_id);
#ifdef P3TURTLE_DEBUG
	std::cerr << "  request_id=" << (void*)request_id << ", tunnel_id=" << (void*)tunnel_id << std::endl ;
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

	chunk_data = (void*)malloc(chunk_size) ;
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
// -----------------------------------------------------------------------------------//
// -------------------------------------  IO  --------------------------------------- // 
// -----------------------------------------------------------------------------------//
//
std::ostream& RsTurtleStringSearchRequestItem::print(std::ostream& o, uint16_t)
{
	o << "Search request (string):" << std::endl ;
	o << "  direct origin: \"" << PeerId() << "\"" << std::endl ;
	o << "  match string: \"" << match_string << "\"" << std::endl ;
	o << "  Req. Id: " << (void *)request_id << std::endl ;
	o << "  Depth  : " << depth << std::endl ;

	return o ;
}
std::ostream& RsTurtleRegExpSearchRequestItem::print(std::ostream& o, uint16_t)
{
	o << "Search request (regexp):" << std::endl ;
	o << "  direct origin: \"" << PeerId() << "\"" << std::endl ;
	o << "  Req. Id: " << (void *)request_id << std::endl ;
	o << "  Depth  : " << depth << std::endl ;
	o << "  RegExp: " << std::endl ;
	o << "    Toks: " ; for(unsigned int i=0;i<expr._tokens.size();++i) std::cout << (int)expr._tokens[i] << " " ; std::cout << std::endl ;
	o << "    Ints: " ; for(unsigned int i=0;i<expr._ints.size();++i) std::cout << (int)expr._ints[i] << " " ; std::cout << std::endl ;
	o << "    Strs: " ; for(unsigned int i=0;i<expr._strings.size();++i) std::cout << expr._strings[i] << " " ; std::cout << std::endl ;

	return o ;
}

std::ostream& RsTurtleSearchResultItem::print(std::ostream& o, uint16_t)
{
	o << "Search result:" << std::endl ;

	o << "  Peer id: " << PeerId() << std::endl ;
	o << "  Depth  : " << depth << std::endl ;
	o << "  Req. Id: " << (void *)request_id << std::endl ;
	o << "  Files:" << std::endl ;
	
	for(std::list<TurtleFileInfo>::const_iterator it(result.begin());it!=result.end();++it)
		o << "    " << it->hash << "  " << it->size << " " << it->name << std::endl ;

	return o ;
}

std::ostream& RsTurtleOpenTunnelItem::print(std::ostream& o, uint16_t)
{
	o << "Open Tunnel:" << std::endl ;

	o << "  Peer id    : " << PeerId() << std::endl ;
	o << "  Partial tId: " << (void *)partial_tunnel_id << std::endl ;
	o << "  Req. Id    : " << (void *)request_id << std::endl ;
	o << "  Depth      : " << depth << std::endl ;
	o << "  Hash       : " << file_hash << std::endl ;

	return o ;
}

std::ostream& RsTurtleTunnelOkItem::print(std::ostream& o, uint16_t)
{
	o << "Tunnel Ok:" << std::endl ;

	o << "  Peer id   : " << PeerId() << std::endl ;
	o << "  tunnel id : " << (void*)tunnel_id << std::endl ;
	o << "  Req. Id   : " << (void *)request_id << std::endl ;

	return o ;
}

std::ostream& RsTurtleFileRequestItem::print(std::ostream& o, uint16_t)
{
	o << "File request item:" << std::endl ;

	o << "  tunnel id : " << (void*)tunnel_id << std::endl ;
	o << "  offset    : " << chunk_offset << std::endl ;
	o << "  chunk size: " << chunk_size << std::endl ;

	return o ;
}

std::ostream& RsTurtleFileDataItem::print(std::ostream& o, uint16_t)
{
	o << "File request item:" << std::endl ;

	o << "  tunnel id : " << (void*)tunnel_id << std::endl ;
	o << "  offset    : " << chunk_offset << std::endl ;
	o << "  chunk size: " << chunk_size << std::endl ;
	o << "  data      : " << (void*)chunk_data << std::endl ;

	return o ;
}

std::ostream& RsTurtleFileMapItem::print(std::ostream& o, uint16_t)
{
	o << "File map item:" << std::endl ;

	o << "  tunnel id : " << (void*)tunnel_id << std::endl ;
	o << "  direction : " << direction << std::endl ;
	o << "  map      : " ;

	for(uint32_t i=0;i<compressed_map._map.size();++i)
		o << (void*)compressed_map._map[i] << std::endl ;

	return o ;
}

std::ostream& RsTurtleFileMapRequestItem::print(std::ostream& o, uint16_t)
{
	o << "File map request item:" << std::endl ;

	o << "  tunnel id : " << (void*)tunnel_id << std::endl ;
	o << "  direction : " << direction << std::endl ;

	return o ;
}

std::ostream& RsTurtleFileCrcItem::print(std::ostream& o, uint16_t)
{
	o << "File CRC item:" << std::endl ;

	o << "  tunnel id : " << (void*)tunnel_id << std::endl ;
	o << "  map      : " ;

	for(uint32_t i=0;i<crc_map._ccmap._map.size();++i)
		o << (void*)crc_map._ccmap._map[i] << std::endl ;

	o << "  CRC      : " ;

	for(uint32_t i=0;i<crc_map._crcs.size();++i)
		o << (void*)crc_map._crcs[i] << std::endl ;

	return o ;
}
std::ostream& RsTurtleFileCrcRequestItem::print(std::ostream& o, uint16_t)
{
	o << "File CRC request item:" << std::endl ;

	o << "  tunnel id : " << (void*)tunnel_id << std::endl ;

	return o ;
}
