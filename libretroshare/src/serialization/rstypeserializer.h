#include "serialiser/rstlvbase.h"

template<typename T> T ntoh(T t)
{
	if(sizeof(T) == 8) return t;
	if(sizeof(T) == 4) return ntohl(t) ;
	if(sizeof(T) == 2) return ntohs(t) ;

	std::cerr << "(EE) unhandled type of size " << sizeof(T) << " in ntoh<>" << std::endl;
	return t;
}
template<typename T> T hton(T t)
{
	if(sizeof(T) == 8) return t;
	if(sizeof(T) == 4) return htonl(t) ;
	if(sizeof(T) == 2) return htons(t) ;

	std::cerr << "(EE) unhandled type of size " << sizeof(T) << " in hton<>" << std::endl;
	return t;
}

class RsTypeSerializer
{
	public:
		template<typename T> 
		static void serial_process(RsItem::SerializeJob j,SerializeContext& ctx,T& member) 
		{
			switch(j)
			{
				case RsItem::SIZE_ESTIMATE: ctx.mSize += serial_size(member) ;
																break ;

				case RsItem::DESERIALIZE:   ctx.mOk = ctx.mOk && deserialize(ctx.mData,ctx.mSize,ctx.mOffset,member) ;
																break ;

				case RsItem::SERIALIZE:     ctx.mOk = ctx.mOk && serialize(ctx.mData,ctx.mSize,ctx.mOffset,member) ;
																break ;

				default:
																ctx.mOk = false ;
																throw std::runtime_error("Unknown serial job") ;
			}
		}

	protected:
		template<typename T> static bool     serialize  (uint8_t data[], uint32_t size, uint32_t &offset, const T& member);
		template<typename T> static bool     deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, T& member);
		template<typename T> static uint32_t serial_size(const T& /* member */);
};

/// Templates to generate RsSerializer for standard integral types
//
template<typename N,uint32_t SIZE> class t_SerializerNType 
{
public:
	static bool serialize(uint8_t data[], uint32_t size, uint32_t &offset, const N& member)
	{
		if (size <= offset || size < SIZE + offset) 
		{
			SERIALIZE_ERROR() << ": not enough room. SIZE+offset=" << SIZE+offset << " and size is only " << size << std::endl;
			return false;
		}

		N tmp = hton<N>(member);
		memcpy(data+offset, &tmp, SIZE);
		offset += SIZE;
		return true;
	}
	static bool deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, N& member)
	{
		if (size <= offset || size < offset + SIZE) 
		{
			SERIALIZE_ERROR() << ": not enough room. SIZE+offset=" << SIZE+offset << " and size is only " << size << std::endl;
			return false;
		}

		N tmp ;
		memcpy(&tmp, data+offset, SIZE);
		member = ntoh<N>(tmp);
		offset += SIZE;
		return true;
	}

	static uint32_t serial_size(const N& /* member */)
	{ 
		return SIZE; 
	}
};

template<> bool RsTypeSerializer::serialize(uint8_t data[], uint32_t size, uint32_t &offset, const uint8_t& member) 
{ 
	return t_SerializerNType<uint8_t,1>::serialize(data,size,offset,member); 
}
template<> bool RsTypeSerializer::serialize(uint8_t data[], uint32_t size, uint32_t &offset, const uint32_t& member) 
{ 
	return t_SerializerNType<uint32_t,4>::serialize(data,size,offset,member); 
}
template<> bool RsTypeSerializer::serialize(uint8_t data[], uint32_t size, uint32_t &offset, const uint64_t& member) 
{ 
	return t_SerializerNType<uint64_t,8>::serialize(data,size,offset,member); 
}
template<> bool RsTypeSerializer::deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, uint8_t& member) 
{ 
	return t_SerializerNType<uint8_t,1>::deserialize(data,size,offset,member); 
}
template<> bool RsTypeSerializer::deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, uint32_t& member) 
{ 
	return t_SerializerNType<uint32_t,4>::deserialize(data,size,offset,member); 
}
template<> bool RsTypeSerializer::deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, uint64_t& member) 
{ 
	return t_SerializerNType<uint64_t,8>::deserialize(data,size,offset,member); 
}
template<> uint32_t RsTypeSerializer::serial_size(const uint8_t& member) 
{ 
	return t_SerializerNType<uint8_t,1>::serial_size(member); 
}
template<> uint32_t RsTypeSerializer::serial_size(const uint32_t& member) 
{ 
	return t_SerializerNType<uint32_t,4>::serial_size(member); 
}
template<> uint32_t RsTypeSerializer::serial_size(const uint64_t& member) 
{ 
	return t_SerializerNType<uint64_t,8>::serial_size(member); 
}
//// Float
//
template<> uint32_t RsTypeSerializer::serial_size(const float&){ return 4; }

template<> bool RsTypeSerializer::serialize(uint8_t data[], uint32_t size, uint32_t& offset, const float& f)
{
	uint32_t sz = serial_size(f);

	if ( !data || size <= offset || size < sz + offset )
	{
		SERIALIZE_ERROR() << ": not enough room. SIZE+offset=" << sz+offset << " and size is only " << size << std::endl;
		return false;
	}

	const float tmp = f;
	if(tmp < 0.0f)
	{
		SERIALIZE_ERROR() << "Cannot serialise invalid negative float value " << tmp << std::endl;
		return false;
	}

	/* This serialisation is quite accurate. The max relative error is approx.
	 * 0.01% and most of the time less than 1e-05% The error is well distributed
	 * over numbers also. */

	uint32_t n;
	if(tmp < 1e-7) n = (~(uint32_t)0);
	else n = ((uint32_t)( (1.0f/(1.0f+tmp) * (~(uint32_t)0))));
	n = hton<uint32_t>(n);
	memcpy(data+offset, &n, sz);
	offset += sz;
	return true;
}


template<> bool RsTypeSerializer::deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, float& f)
{
	uint32_t sz = serial_size(f);

	if ( !data || size <= offset || size < offset + sz )
	{
		SERIALIZE_ERROR() << "Cannot deserialise float value. Not enough room. size=" << size << ", offset=" << offset << std::endl;
		return false;
	}

	uint32_t n;
	memcpy(&n, data+offset, sz);
	n = ntoh<uint32_t>(n);
	f = 1.0f/ ( n/(float)(~(uint32_t)0)) - 1.0f;
	return true;
}

typedef std::pair<std::string&,uint16_t> TlvString;

/// Serializer for std::string
template<> uint32_t RsTypeSerializer::serial_size<TlvString>(const TlvString& s) 
{ 
	return GetTlvStringSize(s.first) ;
}

template<> bool RsTypeSerializer::serialize(uint8_t data[], uint32_t size, uint32_t &offset,const TlvString& s)
{
	return SetTlvString(data,size,&offset,s.second,s.first) ;
}

template<> bool RsTypeSerializer::deserialize(const uint8_t data[], uint32_t size,uint32_t& offset,TlvString& s)
{
	return GetTlvString((void*)data,size,&offset,s.second,s.first) ;
}

