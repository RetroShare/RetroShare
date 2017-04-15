#ifndef WINDOWS_SYS
#include <stdexcept>
#endif
#include <iostream>
#include "turtletypes.h"
#include "rsturtleitem.h"
#include "turtleclientservice.h"

#include "serialization/rstypeserializer.h"

//#define P3TURTLE_DEBUG
// -----------------------------------------------------------------------------------//
// --------------------------------  Serialization. --------------------------------- // 
// -----------------------------------------------------------------------------------//
//

//
// ---------------------------------- Packet sizes -----------------------------------//
//

#ifdef TO_REMOVE
uint32_t RsTurtleStringSearchRequestItem::serial_size() const
{
	uint32_t s = 0 ;

	s += 8 ; // header
	s += 4 ; // request_id
	s += 2 ; // depth
	s += GetTlvStringSize(match_string) ;	// match_string

	return s ;
}
uint32_t RsTurtleRegExpSearchRequestItem::serial_size() const
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

uint32_t RsTurtleSearchResultItem::serial_size()const
{
	uint32_t s = 0 ;

	s += 8 ; // header
	s += 4 ; // search request id
	s += 2 ; // depth
	s += 4 ; // number of results

	for(std::list<TurtleFileInfo>::const_iterator it(result.begin());it!=result.end();++it)
	{
		s += 8 ;											// file size
        s += it->hash.serial_size();			// file hash
		s += GetTlvStringSize(it->name) ;		// file name
	}

	return s ;
}

uint32_t RsTurtleOpenTunnelItem::serial_size()const
{
	uint32_t s = 0 ;

	s += 8 ; // header
    s += file_hash.serial_size() ;		// file hash
	s += 4 ; // tunnel request id
	s += 4 ; // partial tunnel id 
	s += 2 ; // depth

	return s ;
}

uint32_t RsTurtleTunnelOkItem::serial_size() const
{
	uint32_t s = 0 ;

	s += 8 ; // header
	s += 4 ; // tunnel id 
	s += 4 ; // tunnel request id

	return s ;
}

uint32_t RsTurtleGenericDataItem::serial_size() const
{
	uint32_t s = 0 ;

	s += 8 ; // header
	s += 4 ; // tunnel id 
	s += 4 ; // data size
	s += data_size ; // data

	return s ;
}
#endif
//
// ---------------------------------- Serialization ----------------------------------//
//
RsItem *RsTurtleSerialiser::create_item(uint16_t service,uint8_t item_subtype)
{
	if (RS_SERVICE_TYPE_TURTLE != service)
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "  Wrong type !!" << std::endl ;
#endif
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(item_subtype))
	{
	case RS_TURTLE_SUBTYPE_STRING_SEARCH_REQUEST	:	return new RsTurtleStringSearchRequestItem();
	case RS_TURTLE_SUBTYPE_REGEXP_SEARCH_REQUEST	:	return new RsTurtleRegExpSearchRequestItem();
	case RS_TURTLE_SUBTYPE_SEARCH_RESULT			:	return new RsTurtleSearchResultItem();
	case RS_TURTLE_SUBTYPE_OPEN_TUNNEL  			:	return new RsTurtleOpenTunnelItem();
	case RS_TURTLE_SUBTYPE_TUNNEL_OK    			:	return new RsTurtleTunnelOkItem();
	case RS_TURTLE_SUBTYPE_GENERIC_DATA 			:	return new RsTurtleGenericDataItem();

	default:
		break ;
	}
	// now try all client services
	//
	RsItem *item = NULL ;

	for(uint32_t i=0;i<_client_services.size();++i)
		if((item = _client_services[i]->create_item(service,item_subtype)) != NULL)
			return item ;

	std::cerr << "Unknown packet type in RsTurtle (not even handled by client services)!" << std::endl ;
	return NULL ;
}

void RsTurtleStringSearchRequestItem::serial_process(SerializeJob j,SerializeContext& ctx)
{
    RsTypeSerializer::serial_process          (j,ctx,TLV_TYPE_STR_VALUE,match_string,"match_string") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,request_id,"request_id") ;
    RsTypeSerializer::serial_process<uint16_t>(j,ctx,depth     ,"depth") ;
}

#ifdef TO_REMOVE
bool RsTurtleStringSearchRequestItem::serialize(void *data,uint32_t& pktsize) const
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

bool RsTurtleRegExpSearchRequestItem::serialize(void *data,uint32_t& pktsize) const
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
#endif

void RsTurtleRegExpSearchRequestItem::serial_process(SerializeJob j,SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,request_id,"request_id") ;
    RsTypeSerializer::serial_process<uint16_t>(j,ctx,depth,"depth") ;
    RsTypeSerializer::serial_process(j,ctx,expr,"expr") ;
}

template<> uint32_t RsTypeSerializer::serial_size(const RsRegularExpression::LinearizedExpression& r)
{
    uint32_t s = 0 ;

	s += 4 ; // number of strings

	for(unsigned int i=0;i<r._strings.size();++i)
		s += GetTlvStringSize(r._strings[i]) ;

	s += 4 ; // number of ints
	s += 4 * r._ints.size() ;
	s += 4 ; // number of tokens
	s += r._tokens.size() ;		// uint8_t has size 1

    return s;
}

template<> bool RsTypeSerializer::deserialize(const uint8_t data[],uint32_t size,uint32_t& offset,RsRegularExpression::LinearizedExpression& expr)
{
    uint32_t saved_offset = offset ;

	uint32_t n =0 ;
	bool ok = true ;

    ok &= getRawUInt32(data,size,&offset,&n) ;

	if(ok) expr._tokens.resize(n) ;

	for(uint32_t i=0;i<n && ok;++i) ok &= getRawUInt8(data,size,&offset,&expr._tokens[i]) ;

	ok &= getRawUInt32(data,size,&offset,&n) ;

	if(ok) expr._ints.resize(n) ;

	for(uint32_t i=0;i<n && ok;++i) ok &= getRawUInt32(data,size,&offset,&expr._ints[i]) ;

	ok &= getRawUInt32(data,size,&offset,&n);

	if (ok) expr._strings.resize(n);

	for(uint32_t i=0;i<n && ok;++i) ok &= GetTlvString(data, size, &offset, TLV_TYPE_STR_VALUE, expr._strings[i]);

    if(!ok)
        offset = saved_offset ;

    return ok;
}

template<> bool RsTypeSerializer::serialize(uint8_t data[],uint32_t size,uint32_t& offset,const RsRegularExpression::LinearizedExpression& expr)
{
    uint32_t saved_offset = offset ;

	bool ok = true ;

    ok &= setRawUInt32(data,size,&offset,expr._tokens.size()) ;

	for(unsigned int i=0;i<expr._tokens.size();++i) ok &= setRawUInt8(data,size,&offset,expr._tokens[i]) ;

	ok &= setRawUInt32(data,size,&offset,expr._ints.size()) ;

	for(unsigned int i=0;i<expr._ints.size();++i) ok &= setRawUInt32(data,size,&offset,expr._ints[i]) ;

	ok &= setRawUInt32(data,size,&offset,expr._strings.size()) ;

	for(unsigned int i=0;i<expr._strings.size();++i) ok &= SetTlvString(data, size, &offset, TLV_TYPE_STR_VALUE, expr._strings[i]);


    if(!ok)
        offset = saved_offset ;

    return ok;
}

template<> void RsTypeSerializer::print_data(const std::string& n, const RsRegularExpression::LinearizedExpression& expr)
{
    std::cerr << "  [RegExpr    ] " << n << ", tokens=" << expr._tokens.size() << " ints=" << expr._ints.size() << " strings=" << expr._strings.size() << std::endl;
}


#ifdef TO_REMOVE
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

	if(ok) expr._tokens.resize(n) ;

	for(uint32_t i=0;i<n && ok;++i) ok &= getRawUInt8(data,pktsize,&offset,&expr._tokens[i]) ;

	ok &= getRawUInt32(data,pktsize,&offset,&n) ;

	if(ok) expr._ints.resize(n) ;

	for(uint32_t i=0;i<n && ok;++i) ok &= getRawUInt32(data,pktsize,&offset,&expr._ints[i]) ;

	ok &= getRawUInt32(data,pktsize,&offset,&n);

	if (ok) expr._strings.resize(n);

	for(uint32_t i=0;i<n && ok;++i) ok &= GetTlvString(data, pktsize, &offset, TLV_TYPE_STR_VALUE, expr._strings[i]); 	

#ifdef WINDOWS_SYS // No Exceptions in Windows compile. (drbobs).
	UNREFERENCED_LOCAL_VARIABLE(rssize);
#else
	if (offset != rssize)
		throw std::runtime_error("Size error while deserializing.") ;
	if (!ok)
		throw std::runtime_error("Unknown error while deserializing.") ;
#endif
}
#endif

void RsTurtleSearchResultItem::serial_process(SerializeJob j,SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,request_id,"request_id") ;
    RsTypeSerializer::serial_process<uint16_t>(j,ctx,depth     ,"depth") ;
    RsTypeSerializer::serial_process          (j,ctx,result    ,"result") ;
}

template<> uint32_t RsTypeSerializer::serial_size(const TurtleFileInfo& i)
{
    uint32_t s = 0 ;

    s += 8 ; // size
    s += i.hash.SIZE_IN_BYTES ;
    s += GetTlvStringSize(i.name) ;

    return s;
}

template<> bool RsTypeSerializer::deserialize(const uint8_t data[],uint32_t size,uint32_t& offset,TurtleFileInfo& i)
{
    uint32_t saved_offset = offset ;
    bool ok = true ;

	ok &= getRawUInt64(data, size, &offset, &i.size); 					 // file size
	ok &= i.hash.deserialise(data, size, offset);                        // file hash
	ok &= GetTlvString(data, size, &offset, TLV_TYPE_STR_NAME, i.name);  // file name

    if(!ok)
        offset = saved_offset ;

    return ok;
}

template<> bool RsTypeSerializer::serialize(uint8_t data[],uint32_t size,uint32_t& offset,const TurtleFileInfo& i)
{
	uint32_t saved_offset = offset ;
    bool ok = true ;

	ok &= setRawUInt64(data, size, &offset, i.size); 					 // file size
	ok &= i.hash.serialise(data, size, offset);					         // file hash
	ok &= SetTlvString(data, size, &offset, TLV_TYPE_STR_NAME, i.name);  // file name

	if(!ok)
		offset = saved_offset ;

	return ok;
}

template<> void RsTypeSerializer::print_data(const std::string& n, const TurtleFileInfo& i)
{
    std::cerr << "  [FileInfo  ] " << n << " size=" << i.size << " hash=" << i.hash << ", name=" << i.name << std::endl;
}

#ifdef TO_REMOVE

bool RsTurtleSearchResultItem::serialize(void *data,uint32_t& pktsize) const
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
        ok &= setRawUInt64(data, tlvsize, &offset, it->size); 					// file size
        ok &= it->hash.serialise(data, tlvsize, offset);					// file hash
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

RsTurtleSearchResultItem::RsTurtleSearchResultItem(void *data,uint32_t pktsize)
	: RsTurtleItem(RS_TURTLE_SUBTYPE_SEARCH_RESULT)
{
	setPriorityLevel(QOS_PRIORITY_RS_TURTLE_SEARCH_RESULT) ;
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
        ok &= f.hash.deserialise(data, pktsize, offset); 	// file hash
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
#endif

void RsTurtleOpenTunnelItem::serial_process(SerializeJob j,SerializeContext& ctx)
{
    RsTypeSerializer::serial_process          (j,ctx,file_hash        ,"file_hash") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,request_id       ,"request_id") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,partial_tunnel_id,"partial_tunnel_id") ;
    RsTypeSerializer::serial_process<uint16_t>(j,ctx,depth            ,"depth") ;
}

#ifdef TO_REMOVE
bool RsTurtleOpenTunnelItem::serialize(void *data,uint32_t& pktsize) const
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

    ok &= file_hash.serialise(data, tlvsize, offset);	// file hash
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
	setPriorityLevel(QOS_PRIORITY_RS_TURTLE_OPEN_TUNNEL) ;
#ifdef P3TURTLE_DEBUG
	std::cerr << "  type = open tunnel" << std::endl ;
#endif
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);

	/* add mandatory parts first */

	bool ok = true ;
    ok &= file_hash.deserialise(data, pktsize, offset); 	// file hash
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
#endif

void RsTurtleTunnelOkItem::serial_process(SerializeJob j,SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,tunnel_id ,"tunnel_id") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,request_id,"request_id") ;
}

#ifdef TO_REMOVE
bool RsTurtleTunnelOkItem::serialize(void *data,uint32_t& pktsize) const
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
	setPriorityLevel(QOS_PRIORITY_RS_TURTLE_TUNNEL_OK) ;
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
#endif

void RsTurtleGenericDataItem::serial_process(SerializeJob j,SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,tunnel_id ,"tunnel_id") ;

    RsTypeSerializer::TlvMemBlock_proxy prox(data_bytes,data_size) ;

    RsTypeSerializer::serial_process(j,ctx,prox,"data bytes") ;
}

#ifdef TO_REMOVE
RsTurtleGenericDataItem::RsTurtleGenericDataItem(void *data,uint32_t pktsize)
	: RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_GENERIC_DATA)
{
	setPriorityLevel(QOS_PRIORITY_RS_TURTLE_GENERIC_DATA) ;
#ifdef P3TURTLE_DEBUG
	std::cerr << "  type = tunnel ok" << std::endl ;
#endif
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);

    	if(rssize > pktsize)
		throw std::runtime_error("RsTurtleTunnelOkItem::() wrong rssize (exceeds pktsize).") ;
            
	/* add mandatory parts first */

	bool ok = true ;

	ok &= getRawUInt32(data, pktsize, &offset, &tunnel_id) ;
	ok &= getRawUInt32(data, pktsize, &offset, &data_size);
#ifdef P3TURTLE_DEBUG
	std::cerr << "  request_id=" << (void*)request_id << ", tunnel_id=" << (void*)tunnel_id << std::endl ;
#endif
    
    	if(data_size > rssize || rssize - data_size < offset)
		throw std::runtime_error("RsTurtleTunnelOkItem::() wrong data_size (exceeds rssize).") ;
            
	data_bytes = rs_malloc(data_size) ;

	if(data_bytes != NULL)
	{
		memcpy(data_bytes,(void *)((uint8_t *)data+offset),data_size) ;
		offset += data_size ;
	}
	else
		offset = 0 ; // generate an error

#ifdef WINDOWS_SYS // No Exceptions in Windows compile. (drbobs).
	UNREFERENCED_LOCAL_VARIABLE(rssize);
#else
	if (offset != rssize)
		throw std::runtime_error("RsTurtleTunnelOkItem::() error while deserializing.") ;
	if (!ok)
		throw std::runtime_error("RsTurtleTunnelOkItem::() unknown error while deserializing.") ;
#endif
}

bool RsTurtleGenericDataItem::serialize(void *data,uint32_t& pktsize) const
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
	ok &= setRawUInt32(data, tlvsize, &offset, data_size);

	memcpy((void *)((uint8_t *)data+offset),data_bytes,data_size) ;
	offset += data_size ;

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
	o << "  Req. Id: " << std::hex << request_id << std::dec << std::endl ;
	o << "  Depth  : " << depth << std::endl ;

	return o ;
}
std::ostream& RsTurtleRegExpSearchRequestItem::print(std::ostream& o, uint16_t)
{
	o << "Search request (regexp):" << std::endl ;
	o << "  direct origin: \"" << PeerId() << "\"" << std::endl ;
	o << "  Req. Id: " << std::hex << request_id << std::dec << std::endl ;
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
	o << "  Req. Id: " << std::hex << request_id << std::dec << std::endl ;
	o << "  Files:" << std::endl ;
	
	for(std::list<TurtleFileInfo>::const_iterator it(result.begin());it!=result.end();++it)
		o << "    " << it->hash << "  " << it->size << " " << it->name << std::endl ;

	return o ;
}

std::ostream& RsTurtleOpenTunnelItem::print(std::ostream& o, uint16_t)
{
	o << "Open Tunnel:" << std::endl ;

	o << "  Peer id    : " << PeerId() << std::endl ;
	o << "  Partial tId: " << std::hex << partial_tunnel_id << std::dec << std::endl ;
	o << "  Req. Id    : " << std::hex << request_id << std::dec << std::endl ;
	o << "  Depth      : " << depth << std::endl ;
	o << "  Hash       : " << file_hash << std::endl ;

	return o ;
}

std::ostream& RsTurtleTunnelOkItem::print(std::ostream& o, uint16_t)
{
	o << "Tunnel Ok:" << std::endl ;

	o << "  Peer id   : " << PeerId() << std::endl ;
	o << "  tunnel id : " << std::hex << tunnel_id << std::dec << std::endl ;
	o << "  Req. Id   : " << std::hex << request_id << std::dec << std::endl ;

	return o ;
}

std::ostream& RsTurtleGenericDataItem::print(std::ostream& o, uint16_t)
{
	o << "Generic Data item:" << std::endl ;

	o << "  Peer id   : " << PeerId() << std::endl ;
	o << "  data size : " << data_size << std::endl ;
	o << "  data bytes: " << std::hex << (void*)data_bytes << std::dec << std::endl ;

	return o ;
}
#endif
