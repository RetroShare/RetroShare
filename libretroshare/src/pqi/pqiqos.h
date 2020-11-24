/*******************************************************************************
 * libretroshare/src/pqi: pqiqos.h                                             *
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

// This class handles the prioritisation of RsItem, based on the 
// priority level. The QoS algorithm must ensure that:
//
// - lower priority items get out with lower rate than high priority items
// - items of equal priority get out of the queue in the same order than they got in
// - items of level n+1 are output \alpha times more often than items of level n. 
//   \alpha is a constant that is not necessarily an integer, but strictly > 1.
// - the set of possible priority levels is finite, and pre-determined.
//
#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <list>

#include <util/rsmemory.h>

class pqiQoS
{
public:
	pqiQoS(uint32_t max_levels,float alpha) ;

	struct ItemRecord
	{
		void *data ;
		uint32_t current_offset ;
		uint32_t size ;
		uint32_t id ;
	};

	class ItemQueue 
	{
	public:
		ItemQueue()
		  : _threshold(0.0)
		  , _counter(0.0)
		  , _inc(0.0)
		{}
		void *pop() 
		{
			if(_items.empty())
				return NULL ;

			void *item = _items.front().data ;
			_items.pop_front() ;

			return item ;
		}

		void *slice(uint32_t max_size,uint32_t& size,bool& starts,bool& ends,uint32_t& packet_id) 
		{
			if(_items.empty())
				return NULL ;

			ItemRecord& rec(_items.front()) ;
			packet_id = rec.id ;

			// readily get rid of the item if it can be sent as a whole

			if(rec.current_offset == 0 && rec.size < max_size)
			{
				starts = true ;
				ends = true ;
				size = rec.size ;

				return pop() ;
			}
			starts = (rec.current_offset == 0) ;
			ends   = (rec.current_offset + max_size >= rec.size) ;

			if(rec.size <= rec.current_offset)
			{
				std::cerr << "(EE) severe error in slicing in QoS." << std::endl;
				pop() ;
				return NULL ;
			}

			size = std::min(max_size, uint32_t((int)rec.size - (int)rec.current_offset)) ;
			void *mem = rs_malloc(size) ;

			if(!mem)
			{
				std::cerr << "(EE) memory allocation error in QoS." << std::endl;
				pop() ;
				return NULL ;
			}

			memcpy(mem,&((unsigned char*)rec.data)[rec.current_offset],size) ;

			if(ends)	// we're taking the whole stuff. So we can delete the entry.
			{
				free(rec.data) ;
				_items.pop_front() ;
			}
			else
				rec.current_offset += size ;	// by construction, !ends  implies  rec.current_offset < rec.size

			return mem ;
		}

		void push(void *item,uint32_t size,uint32_t id) 
		{
			ItemRecord rec ;

			rec.data = item ;
			rec.current_offset = 0 ;
			rec.size = size ;
			rec.id = id ;

			_items.push_back(rec) ;
		}

        uint32_t size() const { return _items.size() ; }

		float _threshold ;
		float _counter ;
		float _inc ;

		std::list<ItemRecord> _items ;
	};

	// This function pops items from the queue, y order of priority
	//
	void *out_rsItem(uint32_t max_slice_size,uint32_t& size,bool& starts,bool& ends,uint32_t& packet_id) ;

	// This function is used to queue items.
	//
	void in_rsItem(void *item, int size, int priority) ;

	void print() const ;
	uint64_t qos_queue_size() const { return _nb_items ; }

	// kills all waiting items.
	void clear() ;

	// get some stats about what's going on. service_packets will contain the number of
	// packets per service, and queue_sizes will contain the size of the different priority queues.

	//int gatherStatistics(std::vector<uint32_t>& per_service_count,std::vector<uint32_t>& per_priority_count) const ;

	void computeTotalItemSize() const ;
	int debug_computeTotalItemSize() const ;
private:
	// This vector stores the lists of items with equal priorities.
	//
	std::vector<ItemQueue> _item_queues ;
	float _alpha ;
	uint64_t _nb_items ;
	uint32_t _id_counter ;

	static const uint32_t MAX_PACKET_COUNTER_VALUE ;
};


