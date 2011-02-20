#ifdef LINUX
#include <fenv.h>
#endif
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <math.h>
#include "util/utest.h"
#include "serialiser/rsserial.h"

INITTEST();

// Make a fake class of size n.
//
template<int n> class RsTestItem: public RsItem
{
	public:
		unsigned char buff[n] ;

		RsTestItem(uint32_t s) : RsItem(s) {}
		virtual void clear() {}
		virtual std::ostream& print(std::ostream& o, uint16_t) { return o ; }
};

int main(int argc, char **argv)
{
#ifdef LINUX
	feenableexcept(FE_INVALID) ;
	feenableexcept(FE_DIVBYZERO) ;
#endif
	typedef RsTestItem<17> Test17 ;
	typedef RsTestItem<31> Test31 ;

	std::vector<RsItem*> v;

	for(int i=0;i<300;++i)
		v.push_back(new Test17(0)) ;
	for(int i=0;i<700;++i)
		v.push_back(new Test31(0)) ;

	RsMemoryManagement::printStatistics() ;

	// Now delete objects randomly.
	//
	for(int i=0;i<1000;++i)
	{
		int indx = lrand48()%(int)v.size() ;

		delete v[indx] ;
		v[indx] = v.back() ;
		v.pop_back() ;
	}

	std::cerr << "After memory free: " << std::endl;

	RsMemoryManagement::printStatistics() ;

	FINALREPORT("memory_management_test");
	exit(TESTRESULT());
}

