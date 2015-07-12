/*
 * libretroshare/src/pqi: pqiqos.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2008 by Cyril Soler
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "csoler@users.sourceforge.net"
 *
 */

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
#include <vector>
#include <list>

class pqiQoS
{
	public:
		pqiQoS(uint32_t max_levels,float alpha) ;

		class ItemQueue 
		{
            public:
            ItemQueue()
            {
                _item_count =0 ;
            }
				void *pop() 
				{
					if(_items.empty())
						return NULL ;

					void *item = _items.front() ;
                    _items.pop_front() ;
                    --_item_count ;

					return item ;
				}

				void push(void *item) 
				{
                    _items.push_back(item) ;
                    ++_item_count ;
                }

                uint32_t size() const { return _item_count ; }

				float _threshold ;
				float _counter ;
                float _inc ;
                uint32_t _item_count ;
				std::list<void*> _items ;
		};

		// This function pops items from the queue, y order of priority
		//
		void *out_rsItem() ;

		// This function is used to queue items.
		//
		void in_rsItem(void *item,int priority) ;

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
};


