#include "serialiser/rsserial.h"
#include "serialiser/rstlvbase.h"

class RsTypeSerializer
{
	public:
    	// This type should be used to pass a parameter to drive the serialisation if needed.

		typedef std::pair<std::string&,uint16_t> TlvString;

		template<typename T>
		static void serial_process(RsItem::SerializeJob j,SerializeContext& ctx,T& member,const std::string& member_name)
		{
			switch(j)
			{
				case RsItem::SIZE_ESTIMATE: ctx.mSize += serial_size(member) ;
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

	protected:
		template<typename T> static bool     serialize  (uint8_t data[], uint32_t size, uint32_t &offset, const T& member);
		template<typename T> static bool     deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, T& member);
		template<typename T> static uint32_t serial_size(const T& /* member */);
		template<typename T> static void     print_data(const std::string& name,const T& /* member */);
};


