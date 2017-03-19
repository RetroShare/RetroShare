// COMPILE_LINE: g++ -g serial_test.cc -I.. -o serial_test ../lib/libretroshare.a -lssl -lcrypto -lstdc++ -lpthread
//
//

#include "serializer.h"
#include "util/rsmemory.h"
#include "util/rsprint.h"
#include "serialiser/rsserial.h"

static const uint16_t RS_SERVICE_TYPE_TEST  = 0xffff;
static const uint8_t  RS_ITEM_SUBTYPE_TEST1 = 0x01 ;

class RsTestItem: public RsSerializable
{
	public:
		RsTestItem() : RsSerializable(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_TEST,RS_ITEM_SUBTYPE_TEST1) {}

		virtual void serial_process(RsSerializable::SerializeJob j, SerializeContext& ctx) 
		{
			RsTypeSerializer<uint64_t>   ().serial_process(j,ctx,ts) ;
			RsTypeSerializer<std::string>().serial_process(j,ctx,str) ;
		}

		// derived from RsItem
		//
		virtual void clear() {}
		virtual std::ostream& print(std::ostream&,uint16_t indent) {}


	private:
		std::string str ;
		uint64_t ts ;
};

class RsTestSerializer: public RsSerializer
{
	public:

		virtual RsSerializable *create_item(uint16_t service,uint8_t subtype)
		{
			switch(subtype)
			{
				case RS_ITEM_SUBTYPE_TEST1: return new RsTestItem();

				default:
					return NULL ;
			}
		}
};

int main(int argc,char *argv[])
{
	try
	{
		RsTestItem t1 ;

		uint32_t size = RsTestSerializer().size_item(&t1);

		std::cerr << "t1.serial_size() = " << size << std::endl;

		RsTemporaryMemory mem1(size);

		RsTestSerializer().serialize_item(&t1,mem1,mem1.size()) ;

		RsSerializable *t2 = RsTestSerializer().deserialize_item(mem1,mem1.size()) ;

		std::cerr << "Serialized t1: " << RsUtil::BinToHex(mem1,mem1.size()) << std::endl;

		return 0;
	}
	catch(std::exception& e)
	{
		std::cerr << "Exception caught: " << e.what() << std::endl;
		return 1;
	}
}


