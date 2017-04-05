// COMPILE_LINE: g++ -g serial_test.cc -I.. -o serial_test ../lib/libretroshare.a -lssl -lcrypto -lstdc++ -lpthread
//
//

#include <set>
#include <vector>

#include "util/rsmemory.h"
#include "util/rsprint.h"
#include "serialiser/rsserial.h"

#include "rsserializer.h"
#include "rstypeserializer.h"

#define GET_VARIABLE_NAME(str) #str

static const uint16_t RS_SERVICE_TYPE_TEST  = 0xffff;
static const uint8_t  RS_ITEM_SUBTYPE_TEST1 = 0x01 ;

// Template serialization of RsTypeSerialiser::serial_process() for unknown/new types
// Here we do it for std::set<uint32_t> as an example.
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

	if(tlvsize + offset > size)
	{
		std::cerr << "RsTypeSerializer::serialize: error. tlvsize+offset > size, where tlvsize=" << tlvsize << ", offset=" << offset << ", size=" << size << std::endl;
		return false ;
	}

	ok = ok && RsTypeSerializer::serialize<uint32_t>(data,size,offset,member.size()) ;

	for(std::set<uint32_t>::const_iterator it(member.begin());it!=member.end();++it)
		ok = ok && RsTypeSerializer::serialize<uint32_t>(data,size,offset,*it) ;

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

	for(uint32_t i=0;i<n && ok;++i)
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

template<> void RsTypeSerializer::print_data(const std::string& s,const std::set<uint32_t>& set)
{
	std::cerr << "  [set<uint32_t>] " << s << " : set of size " << set.size() << std::endl;
}

// New item class. This class needs to define:
// - a serial_process method that tells which members to serialize
// - overload the clear() and print() methods of RsItem
//
class RsTestItem: public RsItem
{
	public:
		RsTestItem() : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_TEST,RS_ITEM_SUBTYPE_TEST1) 
		{
			str = "test string";
			ts = time(NULL) ;

			int_set.push_back(lrand48()) ;
			int_set.push_back(lrand48()) ;
			int_set.push_back(lrand48()) ;
		}

		// Derived from RsItem
		//
		virtual void serial_process(RsItem::SerializeJob j, SerializeContext& ctx) 
		{
			RsTypeSerializer::TlvString tt(str,TLV_TYPE_STR_DESCR) ;

			RsTypeSerializer::serial_process(j,ctx,ts     ,GET_VARIABLE_NAME(ts)      ) ;
			RsTypeSerializer::serial_process(j,ctx,tt     ,GET_VARIABLE_NAME(str)     ) ;
			RsTypeSerializer::serial_process(j,ctx,int_set,GET_VARIABLE_NAME(int_set) ) ;
		}

		// Derived from RsItem, because they are pure virtuals. Normally print() should disappear soon.
		//
		virtual void clear() {}
		virtual std::ostream& print(std::ostream&,uint16_t) {}

	private:
		std::string str ;
		uint64_t ts ;
		std::vector<uint32_t> int_set ;

		friend int main(int argc,char *argv[]);
};

// New user-defined serializer class. 
// The only required member is the create_item() method, which creates the correct RsItem 
// that corresponds to a specific (service,subtype) couple.
//
class RsTestSerializer: public RsSerializer
{
	public:
		RsTestSerializer() : RsSerializer(RS_SERVICE_TYPE_TEST) {}

		virtual RsItem *create_item(uint16_t service_id,uint8_t subtype)
		{
			if(service_id != RS_SERVICE_TYPE_TEST)
				return NULL ;

			switch(subtype)
			{
				case RS_ITEM_SUBTYPE_TEST1: return new RsTestItem();

				default:
					return NULL ;
			}
		}
};

// Methods to check the equality of items.
//
void check(const std::string& s1,const std::string& s2) { assert(s1 == s2) ; }
void check(const uint64_t& s1,const uint64_t& s2) { assert(s1 == s2) ; }
void check(const std::set<uint32_t>& s1,const std::set<uint32_t>& s2) { assert(s1 == s2) ; }
void check(const std::vector<uint32_t>& s1,const std::vector<uint32_t>& s2) { assert(s1 == s2) ; }

int main(int argc,char *argv[])
{
	try
	{
		RsTestItem t1 ;

		uint32_t size = RsTestSerializer().size(&t1);

		std::cerr << "t1.serial_size() = " << size << std::endl;

		// Allocate some memory to serialise to
		//
		RsTemporaryMemory mem1(size);

		std::cerr << "Item to be serialized:" << std::endl;

		RsTestSerializer().print(&t1) ;
		RsTestSerializer().serialise(&t1,mem1,mem1.size()) ;

		std::cerr << "Serialized t1: " << RsUtil::BinToHex(mem1,mem1.size()) << std::endl;

		// Now deserialise into a new item
		//
		RsItem *t2 = RsTestSerializer().deserialise(mem1,mem1.size()) ;

		// make sure t1 is equal to t2
		//
		check(t1.str,dynamic_cast<RsTestItem*>(t2)->str) ;
		check(t1.ts,dynamic_cast<RsTestItem*>(t2)->ts) ;
		check(t1.int_set,dynamic_cast<RsTestItem*>(t2)->int_set) ;

		delete t2;

		return 0;
	}
	catch(std::exception& e)
	{
		std::cerr << "Exception caught: " << e.what() << std::endl;
		return 1;
	}
}


