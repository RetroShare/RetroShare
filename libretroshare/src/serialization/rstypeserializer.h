#pragma once

#include "serialiser/rsserial.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvlist.h"

#include "retroshare/rsflags.h"
#include "retroshare/rsids.h"

#include "serialization/rsserializer.h"


class RsTypeSerializer
{
	public:
    	// This type should be used to pass a parameter to drive the serialisation if needed.

		struct TlvMemBlock_proxy: public std::pair<void*&   ,uint32_t&>
        {
            TlvMemBlock_proxy(void   *& p,uint32_t& s) : std::pair<void*&,uint32_t&>(p,s) {}
            TlvMemBlock_proxy(uint8_t*& p,uint32_t& s) : std::pair<void*&,uint32_t&>(*(void**)&p,s) {}
        };

		//=================================================================================================//
		//                                            Generic types                                        //
		//=================================================================================================//

		template<typename T>
		static void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx,T& member,const std::string& member_name)
		{
			switch(j)
			{
				case RsGenericSerializer::SIZE_ESTIMATE: ctx.mOffset += serial_size(member) ;
																break ;

				case RsGenericSerializer::DESERIALIZE:   ctx.mOk = ctx.mOk && deserialize(ctx.mData,ctx.mSize,ctx.mOffset,member) ;
																break ;

				case RsGenericSerializer::SERIALIZE:     ctx.mOk = ctx.mOk && serialize(ctx.mData,ctx.mSize,ctx.mOffset,member) ;
																break ;

				case RsGenericSerializer::PRINT:
                							print_data(member_name,member);
                break;
				default:
																ctx.mOk = false ;
																throw std::runtime_error("Unknown serial job") ;
			}
		}

		//=================================================================================================//
		//                                     Generic types + type_id                                     //
		//=================================================================================================//

		template<typename T>
		static void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx,uint16_t type_id,T& member,const std::string& member_name)
		{
			switch(j)
			{
				case RsGenericSerializer::SIZE_ESTIMATE: ctx.mOffset += serial_size(type_id,member) ;
																break ;

				case RsGenericSerializer::DESERIALIZE:   ctx.mOk = ctx.mOk && deserialize(ctx.mData,ctx.mSize,ctx.mOffset,type_id,member) ;
																break ;

				case RsGenericSerializer::SERIALIZE:     ctx.mOk = ctx.mOk && serialize(ctx.mData,ctx.mSize,ctx.mOffset,type_id,member) ;
																break ;

				case RsGenericSerializer::PRINT:
                							print_data(member_name,type_id,member);
                break;
				default:
																ctx.mOk = false ;
																throw std::runtime_error("Unknown serial job") ;
			}
		}
		//=================================================================================================//
		//                                            std::map<T,U>                                        //
		//=================================================================================================//

		template<typename T,typename U>
		static void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx,std::map<T,U>& v,const std::string& member_name)
		{
			switch(j)
			{
			case RsGenericSerializer::SIZE_ESTIMATE:
			{
				ctx.mOffset += 4 ;
				for(typename std::map<T,U>::iterator it(v.begin());it!=v.end();++it)
                {
                    ctx.mOffset += serial_size(it->first) ;
                    ctx.mOffset += serial_size(it->second) ;
				}
			}
				break ;

			case RsGenericSerializer::DESERIALIZE:
			{
                uint32_t n=0 ;
				ctx.mOk = ctx.mOk && deserialize<uint32_t>(ctx.mData,ctx.mSize,ctx.mOffset,n) ;

				for(uint32_t i=0;i<n;++i)
                {
                    T t ;
                    U u ;

					ctx.mOk = ctx.mOk && deserialize(ctx.mData,ctx.mSize,ctx.mOffset,t) ;
					ctx.mOk = ctx.mOk && deserialize(ctx.mData,ctx.mSize,ctx.mOffset,u) ;

                    v[t] = u ;
                }
			}
				break ;

			case RsGenericSerializer::SERIALIZE:
			{
				uint32_t n=v.size();
				ctx.mOk = ctx.mOk && serialize<uint32_t>(ctx.mData,ctx.mSize,ctx.mOffset,n) ;

				for(typename std::map<T,U>::iterator it(v.begin());it!=v.end();++it)
                {
					ctx.mOk = ctx.mOk && serialize(ctx.mData,ctx.mSize,ctx.mOffset,it->first) ;
					ctx.mOk = ctx.mOk && serialize(ctx.mData,ctx.mSize,ctx.mOffset,it->second) ;
                }
			}
				break ;

			case RsGenericSerializer::PRINT:
			{
                if(v.empty())
					std::cerr << "  Empty map \"" << member_name << "\"" << std::endl;
				else
					std::cerr << "  std::map of " << v.size() << " elements: \"" << member_name << "\"" << std::endl;

				for(typename std::map<T,U>::iterator it(v.begin());it!=v.end();++it)
                {
                    std::cerr << "  " ;

                    print_data("first",it->first) ;
                    print_data("second",it->second) ;
                }
			}
				break;
			default:
                break;
			}
		}

		//=================================================================================================//
		//                                            std::vector<T>                                       //
		//=================================================================================================//

		template<typename T>
		static void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx,std::vector<T>& v,const std::string& member_name)
		{
			switch(j)
			{
			case RsGenericSerializer::SIZE_ESTIMATE:
			{
				ctx.mOffset += 4 ;
				for(uint32_t i=0;i<v.size();++i)
					serial_process(j,ctx,v[i],member_name) ;
			}
				break ;

			case RsGenericSerializer::DESERIALIZE:
			{  uint32_t n=0 ;
				serial_process(j,ctx,n,"temporary size") ;

				v.resize(n) ;
				for(uint32_t i=0;i<v.size();++i)
					serial_process(j,ctx,v[i],member_name) ;
			}
				break ;

			case RsGenericSerializer::SERIALIZE:
			{
				uint32_t n=v.size();
				serial_process(j,ctx,n,"temporary size") ;
				for(uint32_t i=0;i<v.size();++i)
					serial_process(j,ctx,v[i],member_name) ;
			}
				break ;

			case RsGenericSerializer::PRINT:
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
		//                                            std::list<T>                                         //
		//=================================================================================================//

		template<typename T>
		static void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx,std::list<T>& v,const std::string& member_name)
		{
			switch(j)
			{
			case RsGenericSerializer::SIZE_ESTIMATE:
			{
				ctx.mOffset += 4 ;
				for(typename std::list<T>::iterator it(v.begin());it!=v.end();++it)
					serial_process(j,ctx,*it ,member_name) ;
			}
				break ;

			case RsGenericSerializer::DESERIALIZE:
			{  uint32_t n=0 ;
				serial_process(j,ctx,n,"temporary size") ;

				for(uint32_t i=0;i<n;++i)
                {
                    T tmp;
					serial_process<T>(j,ctx,tmp,member_name) ;
                    v.push_back(tmp);
                }
			}
				break ;

			case RsGenericSerializer::SERIALIZE:
			{
				uint32_t n=v.size();
				serial_process(j,ctx,n,"temporary size") ;
				for(typename std::list<T>::iterator it(v.begin());it!=v.end();++it)
					serial_process(j,ctx,*it ,member_name) ;
			}
				break ;

			case RsGenericSerializer::PRINT:
			{
                if(v.empty())
					std::cerr << "  Empty list"<< std::endl;
				else
					std::cerr << "  List of " << v.size() << " elements:" << std::endl;
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
		static void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx,t_RsFlags32<N>& v,const std::string& member_name)
		{
			switch(j)
			{
			case RsGenericSerializer::SIZE_ESTIMATE: ctx.mOffset += 4 ;
				break ;

			case RsGenericSerializer::DESERIALIZE:
			{
                uint32_t n=0 ;
                deserialize<uint32_t>(ctx.mData,ctx.mSize,ctx.mOffset,n) ;
                v = t_RsFlags32<N>(n) ;
			}
				break ;

			case RsGenericSerializer::SERIALIZE:
			{
                uint32_t n=v.toUInt32() ;
                serialize<uint32_t>(ctx.mData,ctx.mSize,ctx.mOffset,n) ;
			}
				break ;

			case RsGenericSerializer::PRINT:
				std::cerr << "  Flags of type " << std::hex << N << " : " << v.toUInt32() << std::endl;
                break ;
            }

		}

	protected:
		template<class T> static bool     serialize  (uint8_t data[], uint32_t size, uint32_t &offset, const T& member);
		template<class T> static bool     deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, T& member);
		template<class T> static uint32_t serial_size(const T& /* member */);
		template<class T> static void     print_data(const std::string& name,const T& /* member */);

		template<class T> static bool     serialize  (uint8_t data[], uint32_t size, uint32_t &offset, uint16_t type_id,const T& member);
		template<class T> static bool     deserialize(const uint8_t data[], uint32_t size, uint32_t &offset,uint16_t type_id, T& member);
		template<class T> static uint32_t serial_size(uint16_t type_id,const T& /* member */);
		template<class T> static void     print_data(const std::string& name,uint16_t type_id,const T& /* member */);

		template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER> static bool     serialize  (uint8_t data[], uint32_t size, uint32_t &offset, const t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& member);
		template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER> static bool     deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& member);
		template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER> static uint32_t serial_size(const t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& /* member */);
		template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER> static void     print_data(const std::string& name,const t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& /* member */);

		template<class TLV_CLASS,uint32_t TLV_TYPE> static bool     serialize  (uint8_t data[], uint32_t size, uint32_t &offset, const t_RsTlvList<TLV_CLASS,TLV_TYPE>& member);
		template<class TLV_CLASS,uint32_t TLV_TYPE> static bool     deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, t_RsTlvList<TLV_CLASS,TLV_TYPE>& member);
		template<class TLV_CLASS,uint32_t TLV_TYPE> static uint32_t serial_size(const t_RsTlvList<TLV_CLASS,TLV_TYPE>& /* member */);
		template<class TLV_CLASS,uint32_t TLV_TYPE> static void     print_data(const std::string& name,const t_RsTlvList<TLV_CLASS,TLV_TYPE>& /* member */);
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
void     RsTypeSerializer::print_data(const std::string& /* name */,const t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& member)
{
    std::cerr << "  [RsGenericId<" << std::hex << UNIQUE_IDENTIFIER << ">] : " << member << std::endl;
}

//=================================================================================================//
//                                         t_RsTlvList<>                                           //
//=================================================================================================//

template<class TLV_CLASS,uint32_t TLV_TYPE>
bool     RsTypeSerializer::serialize  (uint8_t data[], uint32_t size, uint32_t &offset, const t_RsTlvList<TLV_CLASS,TLV_TYPE>& member)
{
    return (*const_cast<const t_RsTlvList<TLV_CLASS,TLV_TYPE> *>(&member)).SetTlv(data,size,&offset) ;
}

template<class TLV_CLASS,uint32_t TLV_TYPE>
bool     RsTypeSerializer::deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, t_RsTlvList<TLV_CLASS,TLV_TYPE>& member)
{
    return member.GetTlv(const_cast<uint8_t*>(data),size,&offset) ;
}

template<class TLV_CLASS,uint32_t TLV_TYPE>
uint32_t RsTypeSerializer::serial_size(const t_RsTlvList<TLV_CLASS,TLV_TYPE>& member)
{
    return member.TlvSize();
}

template<class TLV_CLASS,uint32_t TLV_TYPE>
void     RsTypeSerializer::print_data(const std::string& /* name */,const t_RsTlvList<TLV_CLASS,TLV_TYPE>& member)
{
    std::cerr << "  [t_RsTlvString<" << std::hex << TLV_TYPE << ">] : size=" << member.mList.size() << std::endl;
}
