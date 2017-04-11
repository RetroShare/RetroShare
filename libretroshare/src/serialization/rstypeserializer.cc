#include "serialization/rsserializer.h"
#include "serialization/rstypeserializer.h"

#include "serialiser/rsbaseserial.h"
#include "serialiser/rstlvkeys.h"

#include "util/rsprint.h"

#include <iomanip>

static const uint32_t MAX_SERIALIZED_ARRAY_SIZE = 500 ;

#ifdef REMOVE
template<typename T> T ntoh(T t)
{
	if(sizeof(T) == 8) return t;
	if(sizeof(T) == 4) return ntohl(t) ;
	if(sizeof(T) == 2) return ntohs(t) ;
	if(sizeof(T) == 1) return t ;

	std::cerr << "(EE) unhandled type of size " << sizeof(T) << " in ntoh<>" << std::endl;
	return t;
}
template<typename T> T hton(T t)
{
	if(sizeof(T) == 8) return t;
	if(sizeof(T) == 4) return htonl(t) ;
	if(sizeof(T) == 2) return htons(t) ;
	if(sizeof(T) == 1) return t ;

	std::cerr << "(EE) unhandled type of size " << sizeof(T) << " in hton<>" << std::endl;
	return t;
}
#endif

//=================================================================================================//
//                                            Integer types                                        //
//=================================================================================================//

template<> bool RsTypeSerializer::serialize(uint8_t data[], uint32_t size, uint32_t &offset, const uint8_t& member) 
{ 
	return setRawUInt8(data,size,&offset,member);
}
template<> bool RsTypeSerializer::serialize(uint8_t data[], uint32_t size, uint32_t &offset, const uint32_t& member) 
{ 
	return setRawUInt32(data,size,&offset,member);
}
template<> bool RsTypeSerializer::serialize(uint8_t data[], uint32_t size, uint32_t &offset, const uint64_t& member) 
{ 
	return setRawUInt64(data,size,&offset,member);
}
template<> bool RsTypeSerializer::serialize(uint8_t data[], uint32_t size, uint32_t &offset, const time_t& member)
{
	return setRawTimeT(data,size,&offset,member);
}
template<> bool RsTypeSerializer::deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, uint8_t& member)
{ 
	return getRawUInt8(data,size,&offset,&member);
}
template<> bool RsTypeSerializer::deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, uint32_t& member) 
{ 
	return getRawUInt32(data,size,&offset,&member);
}
template<> bool RsTypeSerializer::deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, uint64_t& member) 
{ 
	return getRawUInt64(data,size,&offset,&member);
}
template<> bool RsTypeSerializer::deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, time_t& member)
{
	return getRawTimeT(data,size,&offset,member);
}
template<> uint32_t RsTypeSerializer::serial_size(const uint8_t& /* member*/)
{ 
	return 1;
}
template<> uint32_t RsTypeSerializer::serial_size(const uint32_t& /* member*/)
{ 
	return 4;
}
template<> uint32_t RsTypeSerializer::serial_size(const uint64_t& /* member*/)
{ 
	return 8;
}
template<> uint32_t RsTypeSerializer::serial_size(const time_t& /* member*/)
{
	return 8;
}

template<> void RsTypeSerializer::print_data(const std::string& n, const uint8_t & V)
{
    std::cerr << "  [uint8_t    ] " << n << ": " << V << std::endl;
}
template<> void RsTypeSerializer::print_data(const std::string& n, const uint16_t& V)
{
    std::cerr << "  [uint16_t   ] " << n << ": " << V << std::endl;
}
template<> void RsTypeSerializer::print_data(const std::string& n, const uint32_t& V)
{
    std::cerr << "  [uint32_t   ] " << n << ": " << V << std::endl;
}
template<> void RsTypeSerializer::print_data(const std::string& n, const uint64_t& V)
{
    std::cerr << "  [uint64_t   ] " << n << ": " << V << std::endl;
}
template<> void RsTypeSerializer::print_data(const std::string& n, const time_t& V)
{
    std::cerr << "  [time_t     ] " << n << ": " << V << " (" << time(NULL)-V << " secs ago)" << std::endl;
}


//=================================================================================================//
//                                            FLoats                                               //
//=================================================================================================//

template<> uint32_t RsTypeSerializer::serial_size(const float&){ return 4; }

template<> bool RsTypeSerializer::serialize(uint8_t data[], uint32_t size, uint32_t& offset, const float& f)
{
    return setRawUFloat32(data,size,&offset,f) ;
}
template<> bool RsTypeSerializer::deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, float& f)
{
    return getRawUFloat32(data,size,&offset,f) ;
}
template<> void RsTypeSerializer::print_data(const std::string& n, const float& V)
{
    std::cerr << "  [float      ] " << n << ": " << V << std::endl;
}


//=================================================================================================//
//                                    TlvString with subtype                                       //
//=================================================================================================//

template<> uint32_t RsTypeSerializer::serial_size(uint16_t /* type_subtype */,const std::string& s)
{ 
	return GetTlvStringSize(s) ;
}

template<> bool RsTypeSerializer::serialize(uint8_t data[], uint32_t size, uint32_t &offset,uint16_t type_substring,const std::string& s)
{
	return SetTlvString(data,size,&offset,type_substring,s) ;
}

template<> bool RsTypeSerializer::deserialize(const uint8_t data[], uint32_t size,uint32_t& offset,uint16_t type_substring,std::string& s)
{
	return GetTlvString((void*)data,size,&offset,type_substring,s) ;
}

template<> void RsTypeSerializer::print_data(const std::string& n, uint16_t type_substring,const std::string& V)
{
    std::cerr << "  [TlvString  ] " << n << ": type=" << std::hex <<std::setw(4)<<std::setfill('0') <<  type_substring << std::dec << " s=\"" << V<< "\"" << std::endl;
}

//=================================================================================================//
//                                       TlvInt with subtype                                       //
//=================================================================================================//

template<> uint32_t RsTypeSerializer::serial_size(uint16_t /* type_subtype */,const uint32_t& s)
{
	return GetTlvUInt32Size() ;
}

template<> bool RsTypeSerializer::serialize(uint8_t data[], uint32_t size, uint32_t &offset,uint16_t sub_type,const uint32_t& s)
{
	return SetTlvUInt32(data,size,&offset,sub_type,s) ;
}

template<> bool RsTypeSerializer::deserialize(const uint8_t data[], uint32_t size,uint32_t& offset,uint16_t sub_type,uint32_t& s)
{
	return GetTlvUInt32((void*)data,size,&offset,sub_type,&s) ;
}

template<> void RsTypeSerializer::print_data(const std::string& n, uint16_t sub_type,const uint32_t& V)
{
    std::cerr << "  [TlvUInt32  ] " << n << ": type=" << std::hex <<std::setw(4)<<std::setfill('0') <<  sub_type << std::dec << " s=\"" << V<< "\"" << std::endl;
}


//=================================================================================================//
//                                            std::string                                          //
//=================================================================================================//

template<> void RsTypeSerializer::print_data(const std::string& n, const std::string& V)
{
    std::cerr << "  [std::string] " << n << ": " << V << std::endl;
}

//=================================================================================================//
//                                            Binary blocks                                        //
//=================================================================================================//

template<> uint32_t RsTypeSerializer::serial_size(const RsTypeSerializer::TlvMemBlock_proxy& r) { return 4 + r.second ; }

template<> bool RsTypeSerializer::deserialize(const uint8_t data[],uint32_t size,uint32_t& offset,RsTypeSerializer::TlvMemBlock_proxy& r)
{
    uint32_t saved_offset = offset ;

    bool ok = deserialize<uint32_t>(data,size,offset,r.second) ;

    r.first = (uint8_t*)rs_malloc(r.second) ;

    ok = ok && NULL != r.first;

    memcpy(r.first,&data[offset],r.second) ;

    if(!ok)
        offset = saved_offset ;

    return ok;
}

template<> bool RsTypeSerializer::serialize(uint8_t data[],uint32_t size,uint32_t& offset,const RsTypeSerializer::TlvMemBlock_proxy& r)
{
    uint32_t saved_offset = offset ;

    bool ok = serialize<uint32_t>(data,size,offset,r.second) ;

    memcpy(&data[offset],r.first,r.second) ;

    if(!ok)
        offset = saved_offset ;

    return ok;
}

template<> void RsTypeSerializer::print_data(const std::string& n, const RsTypeSerializer::TlvMemBlock_proxy& s)
{
    std::cerr << "  [Binary data] " << n << ", length=" << s.second << " data=" << RsUtil::BinToHex(s.first,std::min(50u,s.second)) << ((s.second>50)?"...":"") << std::endl;
}

//=================================================================================================//
//                                            Signatures                                           //
//=================================================================================================//

template<> uint32_t RsTypeSerializer::serial_size(const RsTlvKeySignature& s)
{
	return s.TlvSize() ;
}

template<> bool RsTypeSerializer::serialize(uint8_t data[], uint32_t size, uint32_t &offset,const RsTlvKeySignature& s)
{
	return s.SetTlv(data,size,&offset) ;
}

template<> bool RsTypeSerializer::deserialize(const uint8_t data[], uint32_t size,uint32_t& offset,RsTlvKeySignature& s)
{
	return s.GetTlv((void*)data,size,&offset) ;
}

template<> void RsTypeSerializer::print_data(const std::string& n, const RsTlvKeySignature& s)
{
    std::cerr << "  [Signature] " << n << " : key_id=" << s.keyId << ", length=" << s.signData.bin_len << std::endl;
}
