#include <iostream>
#include <list>
#include <math.h>
#include <serialiser/rsserial.h>
#include <serialiser/rsbaseserial.h>

#include "pqiqos.h"

pqiQoS::pqiQoS(uint32_t nb_levels,float alpha)
	: _item_queues(nb_levels),_alpha(alpha)
{
	assert(pow(alpha,nb_levels) < 1e+20) ;

	float c = 1.0f ;
	float inc = alpha ;
	_nb_items = 0 ;

for(int i=((int)nb_levels)-1;i>=0;--i,c *= alpha)
	{
		_item_queues[i]._threshold = c ;
		_item_queues[i]._counter = 0 ;
		_item_queues[i]._inc = inc ;
	}
}

void pqiQoS::clear()
{
	void *item ;

	for(uint32_t i=0;i<_item_queues.size();++i)
		while( (item = _item_queues[i].pop()) != NULL)
			free(item) ;

	_nb_items = 0 ;
}

void pqiQoS::print() const
{
	std::cerr << "pqiQoS: " << _item_queues.size() << " levels, alpha=" << _alpha ;
	std::cerr << "  Size = " << _nb_items ;
	std::cerr << "    Queues: " ;
	for(uint32_t i=0;i<_item_queues.size();++i)
		std::cerr << _item_queues[i]._items.size() << " " ;
	std::cerr << std::endl;
}

void pqiQoS::in_rsItem(void *ptr,int priority)
{
	if(uint32_t(priority) >= _item_queues.size())
	{
		std::cerr << "pqiQoS::in_rsRawItem() ****Warning****: priority " << priority << " out of scope [0," << _item_queues.size()-1 << "]. Priority will be clamped to maximum value." << std::endl;
		priority = _item_queues.size()-1 ;
	}

	_item_queues[priority].push(ptr) ;
	++_nb_items ;
}

// int pqiQoS::gatherStatistics(std::vector<uint32_t>& per_service_count,std::vector<uint32_t>& per_priority_count) const
// {
//     assert(per_priority_count.size() == 10) ;
//     assert(per_service_count.size() == 65536) ;
//
//     for(uint32_t i=0;i<_item_queues.size();++i)
//     {
//         per_priority_count[i] += _item_queues[i].size() ;
//
//         for(std::list<void*>::const_iterator it(_item_queues[i]._items.begin());it!=_item_queues[i]._items.end();++it)
//         {
//                         uint32_t type = 0;
//                         uint32_t offset = 0;
//                         getRawUInt32((uint8_t*)(*it), 4, &offset, &type);
//
//             uint16_t service_id =  (type >> 8) & 0xffff ;
//
//             ++per_service_count[service_id] ;
//         }
//     }
//     return 1 ;
// }


void *pqiQoS::out_rsItem()
{
	// Go through the queues. Increment counters.

	if(_nb_items == 0)
		return NULL ;

	float inc = 1.0f ;
	int i = _item_queues.size()-1 ;

	while(i > 0 && _item_queues[i]._items.empty())
		--i, inc = _item_queues[i]._inc ;

	int last = i ;

	for(int j=i;j>=0;--j)
		if( (!_item_queues[j]._items.empty()) && ((_item_queues[j]._counter += inc) >= _item_queues[j]._threshold ))
		{
			last = j ;
			_item_queues[j]._counter -= _item_queues[j]._threshold ;
		}

	if(last >= 0)
	{
		assert(_nb_items > 0) ;
		--_nb_items ;
		return _item_queues[last].pop();
	}
	else
		return NULL ;
}


