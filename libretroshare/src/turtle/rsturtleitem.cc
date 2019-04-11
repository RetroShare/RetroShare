/*******************************************************************************
 * libretroshare/src/turtle: rsturtleitem.cc                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2009-2018 by Cyril Soler <csoler@users.sourceforge.net>           *
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
#ifndef WINDOWS_SYS
#include <stdexcept>
#endif
#include <iostream>
#include "turtletypes.h"
#include "rsturtleitem.h"
#include "turtleclientservice.h"

#include "serialiser/rstypeserializer.h"

//#define P3TURTLE_DEBUG
// -----------------------------------------------------------------------------------//
// --------------------------------  Serialization. --------------------------------- // 
// -----------------------------------------------------------------------------------//
//

RsItem *RsTurtleSerialiser::create_item(uint16_t service,uint8_t item_subtype) const
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
	case RS_TURTLE_SUBTYPE_FT_SEARCH_RESULT			:	return new RsTurtleFTSearchResultItem();
	case RS_TURTLE_SUBTYPE_OPEN_TUNNEL  			:	return new RsTurtleOpenTunnelItem();
	case RS_TURTLE_SUBTYPE_TUNNEL_OK    			:	return new RsTurtleTunnelOkItem();
	case RS_TURTLE_SUBTYPE_GENERIC_DATA 			:	return new RsTurtleGenericDataItem();
	case RS_TURTLE_SUBTYPE_GENERIC_FAST_DATA 		:	return new RsTurtleGenericFastDataItem();
	case RS_TURTLE_SUBTYPE_GENERIC_SEARCH_REQUEST	:	return new RsTurtleGenericSearchRequestItem();
	case RS_TURTLE_SUBTYPE_GENERIC_SEARCH_RESULT	:	return new RsTurtleGenericSearchResultItem();

	default:
		break ;
	}
	// now try all client services
	//
	RsItem *item = NULL ;

	for(uint32_t i=0;i<_client_services.size();++i)
		if((_client_services[i]->serializer() != NULL) && (item = _client_services[i]->serializer()->create_item(service,item_subtype)) != NULL)
			return item ;

	std::cerr << "Unknown packet type in RsTurtle (not even handled by client services)!" << std::endl ;
	return NULL ;
}

std::string RsTurtleGenericSearchRequestItem::GetKeywords()
{
    return std::string("Generic search : " + RsUtil::BinToHex(search_data,search_data_len,10));
}

void RsTurtleStringSearchRequestItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process          (j,ctx,TLV_TYPE_STR_VALUE,match_string,"match_string") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,request_id,"request_id") ;
    RsTypeSerializer::serial_process<uint16_t>(j,ctx,depth     ,"depth") ;
}
void RsTurtleRegExpSearchRequestItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,request_id,"request_id") ;
    RsTypeSerializer::serial_process<uint16_t>(j,ctx,depth,"depth") ;
    RsTypeSerializer::serial_process(j,ctx,expr,"expr") ;
}
void RsTurtleGenericSearchRequestItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,request_id,"request_id") ;
    RsTypeSerializer::serial_process<uint16_t>(j,ctx,depth,"depth") ;
    RsTypeSerializer::serial_process<uint16_t>(j,ctx,service_id,"service_id") ;
    RsTypeSerializer::serial_process<uint8_t >(j,ctx,request_type,"request_type") ;

    RsTypeSerializer::TlvMemBlock_proxy prox(search_data,search_data_len) ;
    RsTypeSerializer::serial_process(j,ctx,prox,"search_data") ;
}
RsTurtleSearchRequestItem *RsTurtleGenericSearchRequestItem::clone() const
{
    return new RsTurtleGenericSearchRequestItem(*this) ;
}

RsTurtleGenericSearchRequestItem::RsTurtleGenericSearchRequestItem(const RsTurtleGenericSearchRequestItem& it)
		: RsTurtleSearchRequestItem(it)
{
	search_data_len = it.search_data_len ;
    search_data = (unsigned char*)rs_malloc(it.search_data_len) ;
	service_id = it.service_id ;
	request_type = it.request_type ;

    memcpy(search_data,it.search_data,it.search_data_len) ;
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

RS_TYPE_SERIALIZER_TO_JSON_NOT_IMPLEMENTED_DEF(RsRegularExpression::LinearizedExpression)
RS_TYPE_SERIALIZER_FROM_JSON_NOT_IMPLEMENTED_DEF(RsRegularExpression::LinearizedExpression)

void RsTurtleFTSearchResultItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,request_id,"request_id") ;

	// This depth was previously a member of SearchResult parent class that was set to be always 0. It's removed, but we have to stay backward compatible.
	uint16_t depth_retrocompat_unused_placeholder = 0 ;
    RsTypeSerializer::serial_process<uint16_t>(j,ctx,depth_retrocompat_unused_placeholder,"depth") ;

    RsTypeSerializer::serial_process          (j,ctx,result    ,"result") ;
}
void RsTurtleGenericSearchResultItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,request_id,"request_id") ;

	// This depth was previously a member of SearchResult parent class that was set to be always 0. It's removed, but we have to stay backward compatible.
	uint16_t depth_retrocompat_unused_placeholder = 0 ;
    RsTypeSerializer::serial_process<uint16_t>(j,ctx,depth_retrocompat_unused_placeholder,"depth") ;

    RsTypeSerializer::TlvMemBlock_proxy prox(result_data,result_data_len) ;
    RsTypeSerializer::serial_process(j,ctx,prox,"search_data") ;
}
RsTurtleSearchResultItem *RsTurtleGenericSearchResultItem::duplicate() const
{
    RsTurtleGenericSearchResultItem *sr = new RsTurtleGenericSearchResultItem ;

    sr->result_data = (unsigned char*)rs_malloc(result_data_len) ;
    memcpy(sr->result_data,result_data,result_data_len) ;
    sr->result_data_len = result_data_len ;
    sr->request_id = request_id ;
    return sr ;
}

void RsTurtleOpenTunnelItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process          (j,ctx,file_hash        ,"file_hash") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,request_id       ,"request_id") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,partial_tunnel_id,"partial_tunnel_id") ;
    RsTypeSerializer::serial_process<uint16_t>(j,ctx,depth            ,"depth") ;
}

void RsTurtleTunnelOkItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,tunnel_id ,"tunnel_id") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,request_id,"request_id") ;
}

void RsTurtleGenericDataItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,tunnel_id ,"tunnel_id") ;
    RsTypeSerializer::TlvMemBlock_proxy prox(data_bytes,data_size) ;
    RsTypeSerializer::serial_process(j,ctx,prox,"data bytes") ;
}
void RsTurtleGenericFastDataItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,tunnel_id ,"tunnel_id") ;
    RsTypeSerializer::TlvMemBlock_proxy prox(data_bytes,data_size) ;
    RsTypeSerializer::serial_process(j,ctx,prox,"data bytes") ;
}
