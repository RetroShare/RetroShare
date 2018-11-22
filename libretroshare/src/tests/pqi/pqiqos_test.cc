#include "util/utest.h"

#include <iostream>
#include <math.h>
#include <stdint.h>
#include "serialiser/rsserial.h"
#include "util/rsrandom.h"
#include "pqi/pqiqos.h"

INITTEST();

// Class of RsItem with ids to check order
//
class testRawItem: public RsItem
{
	public:
		testRawItem()
			: RsItem(0,0,0), _id(_static_id++)
		{
		}

		virtual void clear() {}
		virtual std::ostream& print(std::ostream& o, uint16_t) { return o ; }

		static const uint32_t rs_rawitem_size = 10 ;
		static uint64_t _static_id ;
		uint64_t _id ;
};
uint64_t testRawItem::_static_id = 1;

int main()
{
	float alpha = 3    ;
	int nb_levels = 10 ;

	//////////////////////////////////////////////
	// 1 - Test consistency of output and order //
	//////////////////////////////////////////////
	{
		pqiQoS qos(nb_levels,alpha) ;

		// 0 - Fill the queue with fake RsItem.
		//
		static const uint32_t pushed_items = 10000 ;

		for(uint32_t i=0;i<pushed_items;++i)
		{
			RsItem *item = new testRawItem ;
			item->setPriorityLevel(i % nb_levels) ;

			qos.in_rsItem(item,item->priority_level()) ;
		}
		std::cerr << "QOS is filled with: " << std::endl;
		qos.print() ;

		// 1 - checks that all items eventually got out in the same order for
		// items of equal priority
		//
		uint32_t poped = 0;
		std::vector<uint64_t> last_ids(nb_levels,0) ;

		while(testRawItem *item = static_cast<testRawItem*>(qos.out_rsItem())) 
		{
			CHECK(last_ids[item->priority_level()] < item->_id) ;

			last_ids[item->priority_level()] = item->_id ;
			delete item, ++poped ;
		}

		std::cerr << "Push " << pushed_items << " items, poped " << poped << std::endl;

		if(pushed_items != poped)
		{
			std::cerr << "Queues are: " << std::endl;
			qos.print() ;
		}
		CHECK(pushed_items == poped) ;
	}

	//////////////////////////////////////////////////
	// 2 - tests proportionality                    //
	//////////////////////////////////////////////////
	{
		// Now we feed the QoS, and check that items get out with probability proportional 
		// to the priority
		//
		pqiQoS qos(nb_levels,alpha) ;

		std::cerr << "Feeding 10^6 packets to the QoS service." << std::endl;
		for(int i=0;i<1000000;++i)
		{
			if(i%10000 == 0)
			{
				fprintf(stderr,"%1.2f %% completed.\r",i/(float)1000000*100.0f) ;
				fflush(stderr) ;
			}
			testRawItem *item = new testRawItem ;

			switch(i%5)
			{
				case 0: item->setPriorityLevel( 1 ) ; break ;
				case 1: item->setPriorityLevel( 4 ) ; break ;
				case 2: item->setPriorityLevel( 6 ) ; break ;
				case 3: item->setPriorityLevel( 7 ) ; break ;
				case 4: item->setPriorityLevel( 8 ) ; break ;
			}

			qos.in_rsItem(item,item->priority_level()) ;
		}

		// Now perform stats on outputs for the 10000 first elements

		std::vector<int> hist(nb_levels,0) ;

		for(uint32_t i=0;i<10000;++i)
		{
			testRawItem *item = static_cast<testRawItem*>(qos.out_rsItem()) ;
			hist[item->priority_level()]++ ;
			delete item ;
		}

		std::cerr << "Histogram: " ;
		for(uint32_t i=0;i<hist.size();++i)
			std::cerr << hist[i] << " " ;
		std::cerr << std::endl;

	}

	///////////////////////////////////////////////////////////////////////////////////
	// 3 - Now do a test with a thread filling the queue and another getting from it //
	///////////////////////////////////////////////////////////////////////////////////

	{
		pqiQoS qos(nb_levels,alpha) ;

		static const rstime_t duration = 60 ;
		static const int average_packet_load = 10000 ;
		rstime_t start = time(NULL) ;
		rstime_t now ;

		while( (now = time(NULL)) < duration+start )
		{
			float in_out_ratio = 1.0f;// - 0.3*cos( (now-start)*M_PI ) ; // out over in

			// feed a random number of packets in

			uint32_t in_packets = RSRandom::random_u32() % average_packet_load ;
			uint32_t out_packets = (uint32_t)((RSRandom::random_u32() % average_packet_load)*in_out_ratio) ;
			
			for(uint32_t i=0;i<in_packets;++i)
			{
				testRawItem *item = new testRawItem ;
				item->setPriorityLevel(i%nb_levels) ;
				qos.in_rsItem(item,item->priority_level()) ;
			}

			// pop a random number of packets out

			std::vector<uint64_t> last_ids(nb_levels,0) ;

			for(uint32_t i=0;i<out_packets;++i)
			{
				testRawItem *item = static_cast<testRawItem*>(qos.out_rsItem()) ;

				if(item == NULL)
				{
					std::cerr << "Null output !" << std::endl;
					break ;
				}

				CHECK(last_ids[item->priority_level()] < item->_id) ;

				last_ids[item->priority_level()] = item->_id ;
				delete item ;
			}

			// print some info
			static rstime_t last = 0 ;
			if(now > last)
			{
				qos.print() ;
				last = now ;
			}
		}
	}

	FINALREPORT("pqiqos_test");
	return TESTRESULT() ;
}


