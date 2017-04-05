#pragma once

#include "serialiser/rsserial.h"
#include "serialiser/rstlvbase.h"

#include "retroshare/rsflags.h"
#include "retroshare/rsids.h"

class SerializeContext
{
	public:

	SerializeContext(uint8_t *data,uint32_t size)
		: mData(data),mSize(size),mOffset(0),mOk(true) {}

	unsigned char *mData ;
	uint32_t mSize ;
	uint32_t mOffset ;
	bool mOk ;
};


class RsTypeSerializer
{
	public:
    	// This type should be used to pass a parameter to drive the serialisation if needed.

		typedef std::pair<std::string&,uint16_t > TlvString_proxy;
		typedef std::pair<uint8_t*&   ,uint32_t&> TlvMemBlock_proxy;

		//=================================================================================================//
		//                                            Generic types                                        //
		//=================================================================================================//

		template<typename T>
		static void serial_process(RsItem::SerializeJob j,SerializeContext& ctx,T& member,const std::string& member_name)
		{
			switch(j)
			{
				case RsItem::SIZE_ESTIMATE: ctx.mOffset += serial_size(member) ;
																break ;

				case RsItem::DESERIALIZE:   ctx.mOk = ctx.mOk && deserialize(ctx.mData,ctx.mSize,ctx.mOffset,member) ;
																break ;

				case RsItem::SERIALIZE:     ctx.mOk = ctx.mOk && serialize(ctx.mData,ctx.mSize,ctx.mOffset,member) ;
																break ;

				case RsItem::PRINT:
                							print_data(member_name,member);
                break;
				default:
																ctx.mOk = false ;
																throw std::runtime_error("Unknown serial job") ;
			}
		}

		//=================================================================================================//
		//                                            std::vector<T>                                       //
		//=================================================================================================//

		template<typename T>
		static void serial_process(RsItem::SerializeJob j,SerializeContext& ctx,std::vector<T>& v,const std::string& member_name)
		{
			switch(j)
			{
			case RsItem::SIZE_ESTIMATE:
			{
				ctx.mOffset += 4 ;
				for(uint32_t i=0;i<v.size();++i)
					serial_process(j,ctx,v[i],member_name) ;
			}
				break ;

			case RsItem::DESERIALIZE:
			{  uint32_t n=0 ;
				serial_process(j,ctx,n,"temporary size") ;

				v.resize(n) ;
				for(uint32_t i=0;i<v.size();++i)
					serial_process(j,ctx,v[i],member_name) ;
			}
				break ;

			case RsItem::SERIALIZE:
			{
				uint32_t n=v.size();
				serial_process(j,ctx,n,"temporary size") ;
				for(uint32_t i=0;i<v.size();++i)
					serial_process(j,ctx,v[i],member_name) ;
			}
				break ;

			case RsItem::PRINT:
			{
                if(v.empty())
					std::cerr << "  Empty array"<< std::endl;
				else
					std::cerr << "  Array of " << v.size() << " elements:" << std::endl;
				for(uint32_t i=0;i<v.size();++i)
                {
                    std::cerr << "  " ;
					serial_process(j,ctx,v[i],member_name) ;
                }
			}
				break;
			default:
                break;
			}
		}
		//=================================================================================================//
		//                                         t_RsFlags32<> types                                     //
		//=================================================================================================//

		template<int N>
		static void serial_process(RsItem::SerializeJob j,SerializeContext& ctx,t_RsFlags32<N>& v,const std::string& member_name)
		{
			switch(j)
			{
			case RsItem::SIZE_ESTIMATE: ctx.mOffset += 4 ;
				break ;

			case RsItem::DESERIALIZE:
			{
                uint32_t n=0 ;
                deserialize<uint32_t>(ctx.mData,ctx.mSize,ctx.mOffset,n) ;
                v = t_RsFlags32<N>(n) ;
			}
				break ;

			case RsItem::SERIALIZE:
			{
                uint32_t n=v.toUInt32() ;
                serialize<uint32_t>(ctx.mData,ctx.mSize,ctx.mOffset,n) ;
			}
				break ;

			case RsItem::PRINT:
				std::cerr << "  Flags of type " << std::hex << N << " : " << v.toUInt32() << std::endl;
                break ;
            }

		}

	protected:
		template<class T> static bool     serialize  (uint8_t data[], uint32_t size, uint32_t &offset, const T& member);
		template<class T> static bool     deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, T& member);
		template<class T> static uint32_t serial_size(const T& /* member */);
		template<class T> static void     print_data(const std::string& name,const T& /* member */);

		template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER> static bool     serialize  (uint8_t data[], uint32_t size, uint32_t &offset, const t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& member);
		template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER> static bool     deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& member);
		template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER> static uint32_t serial_size(const t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& /* member */);
		template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER> static void     print_data(const std::string& name,const t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& /* member */);
};

//=================================================================================================//
//                                         t_RsGenericId<>                                         //
//=================================================================================================//

template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER>
bool     RsTypeSerializer::serialize  (uint8_t data[], uint32_t size, uint32_t &offset, const t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& member)
{
    return (*const_cast<const t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER> *>(&member)).serialise(data,size,offset) ;
}

template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER>
bool     RsTypeSerializer::deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& member)
{
    return member.deserialise(data,size,offset) ;
}

template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER>
uint32_t RsTypeSerializer::serial_size(const t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& member)
{
    return member.serial_size();
}

template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER>
void     RsTypeSerializer::print_data(const std::string& name,const t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& member)
{
    std::cerr << "  [RsGenericId<" << std::hex << UNIQUE_IDENTIFIER << ">] : " << member << std::endl;
}
