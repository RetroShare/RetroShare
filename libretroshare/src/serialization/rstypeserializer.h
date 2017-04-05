#pragma once

#include <typeinfo>

#include "serialiser/rsserial.h"
#include "serialiser/rstlvbase.h"

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
protected:
	class BinaryDataBlock_ref
        {
        public:
            BinaryDataBlock_ref(unsigned char *_mem,uint32_t& _size) : mem(_mem),size(_size){}

            // This allows to pass Temporary objects as modifiable. This is valid only because all members of this class
            // are pointers and references.

            BinaryDataBlock_ref& modifiable() const { return *const_cast<BinaryDataBlock_ref*>(this) ; }

            unsigned char *& mem ;
            uint32_t& size ;
        };


	public:
    	// This type should be used to pass a parameter to drive the serialisation if needed.

		typedef std::pair<std::string&,uint16_t> TlvString;

        static BinaryDataBlock_ref& block_ref(unsigned char *mem,uint32_t& size) { return BinaryDataBlock_ref(mem,size).modifiable() ; }

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

		// Arrays of stuff

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
					std::cerr << "  Array of \"" << typeid(v[0]).name() << "\"" << " with " << v.size() << " elements:";
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

	protected:
		template<class T> static bool     serialize  (uint8_t data[], uint32_t size, uint32_t &offset, const T& member);
		template<class T> static bool     deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, T& member);
		template<class T> static uint32_t serial_size(const T& /* member */);
		template<class T> static void     print_data(const std::string& name,const T& /* member */);


};


