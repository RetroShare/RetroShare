// COMPILE_LINE: g++ -g serial_test.cc -I.. -o serial_test ../lib/libretroshare.a -lssl -lcrypto -lstdc++ -lpthread
//
//

#include <set>

#include "serializer.h"
#include "util/rsmemory.h"
#include "util/rsprint.h"
#include "serialiser/rsserial.h"

static const uint16_t RS_SERVICE_TYPE_TEST  = 0xffff;
static const uint8_t  RS_ITEM_SUBTYPE_TEST1 = 0x01 ;

// Template serialization of RsTypeSerialiser::serial_process() for unknown/new types
//
//
template<> uint32_t RsTypeSerializer::serial_size(const std::set<uint32_t>& s)
{
	return s.size() * 4 + 4 ;
}
template<> bool RsTypeSerializer::serialize(uint8_t data[], uint32_t size, uint32_t &offset, const std::set<uint32_t>& member)
{
	uint32_t tlvsize = serial_size(member) ;
	bool ok = true ;
	uint32_t offset_save = offset ;

	if(tlvsize + offset >= size)
		return false ;

	ok = ok && RsTypeSerializer::serialize(data,size,offset,member.size()) ;

	for(std::set<uint32_t>::const_iterator it(member.begin());it!=member.end();++it)
		ok = ok && RsTypeSerializer::serialize(data,size,offset,*it) ;

	if(!ok)
	{
		std::cerr << "(EE) Cannot serialize std::set<uint32_t>" << std::endl;
		offset = offset_save ;	// return the data in the same condition
	}
	return ok ;
}

template<> bool RsTypeSerializer::deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, std::set<uint32_t>& member)
{
	bool ok = true ;
	uint32_t n = 0 ;
	uint32_t offset_save = offset ;
	member.clear();

	ok = ok && RsTypeSerializer::deserialize(data,size,offset,n);

	for(uint32_t i=0;i<n;++i)
	{
		uint32_t x;
		ok = ok && RsTypeSerializer::deserialize(data,size,offset,x) ;

		member.insert(x);
	}

	if(!ok)
	{
		std::cerr << "(EE) Cannot deserialize std::set<uint32_t>" << std::endl;
		offset = offset_save ;	// return the data in the same condition
	}
	return ok;
}

// New item class. This class needs to define:
// - a serial_process method that tells which members to serialize
// - overload the clear() and print() methods of RsItem
//
class RsTestItem: public RsSerializable
{
	public:
		RsTestItem() : RsSerializable(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_TEST,RS_ITEM_SUBTYPE_TEST1) {}

		// Derived from RsSerializable
		//
		virtual void serial_process(RsSerializable::SerializeJob j, SerializeContext& ctx) 
		{
			RsTypeSerializer::serial_process(j,ctx,ts ) ;
			RsTypeSerializer::serial_process(j,ctx,str) ;
			RsTypeSerializer::serial_process(j,ctx,int_set ) ;
		}

		// Derived from RsItem
		//
		virtual void clear() {}
		virtual std::ostream& print(std::ostream&,uint16_t indent) {}


	private:
		std::string str ;
		uint64_t ts ;
		std::set<uint32_t> int_set ;
};

// New user-defined serializer class. 
// The only required member is the create_item() method, which creates the correct RsSerializable 
// that corresponds to a specific (service,subtype) couple.
//
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


