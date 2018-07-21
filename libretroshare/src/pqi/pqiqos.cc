/*******************************************************************************
 * libretroshare/src/pqi: pqiqos.cc                                            *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2004-2008  Cyril Soler <csoler@users.sourceforge.net>         *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#include <iostream>
#include <list>
#include <math.h>
#include <serialiser/rsserial.h>
#include <serialiser/rsbaseserial.h>

#include "pqiqos.h"

const uint32_t pqiQoS::MAX_PACKET_COUNTER_VALUE = (1 << 24) ;

pqiQoS::pqiQoS(uint32_t nb_levels,float alpha)
	: _item_queues(nb_levels),_alpha(alpha)
{
#ifdef DEBUG
	assert(pow(alpha,nb_levels) < 1e+20) ;
#endif

	float c = 1.0f ;
	float inc = alpha ;
	_nb_items = 0 ;
    	_id_counter = 0 ;

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

void pqiQoS::in_rsItem(void *ptr,int size,int priority)
{
	if(uint32_t(priority) >= _item_queues.size())
	{
		std::cerr << "pqiQoS::in_rsRawItem() ****Warning****: priority " << priority << " out of scope [0," << _item_queues.size()-1 << "]. Priority will be clamped to maximum value." << std::endl;
		priority = _item_queues.size()-1 ;
	}

	_item_queues[priority].push(ptr,size,_id_counter++) ;
	++_nb_items ;
    
    	if(_id_counter >= MAX_PACKET_COUNTER_VALUE)
            _id_counter = 0 ;
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


void *pqiQoS::out_rsItem(uint32_t max_slice_size, uint32_t& size, bool& starts, bool& ends, uint32_t& packet_id) 
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
#ifdef DEBUG
		assert(_nb_items > 0) ;
#endif
        
        	// now chop a slice of this item
        
        	void *res = _item_queues[last].slice(max_slice_size,size,starts,ends,packet_id) ;
            
            	if(ends)
			--_nb_items ;
                
		return res ;
	}
	else
		return NULL ;
}



    
    
