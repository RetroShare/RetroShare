/*******************************************************************************
 * libretroshare/src/pqi: pqiqosstreamer.cc                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012-2012  Cyril Soler <csoler@users.sourceforge.net>         *
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
#include "pqiqosstreamer.h"

//#define DEBUG_PQIQOSSTREAMER 1

const float    pqiQoSstreamer::PQI_QOS_STREAMER_ALPHA      = 2.0f ;

pqiQoSstreamer::pqiQoSstreamer(PQInterface *parent, RsSerialiser *rss, const RsPeerId& peerid, BinInterface *bio_in, int bio_flagsin)
	: pqithreadstreamer(parent,rss,peerid,bio_in,bio_flagsin), pqiQoS(PQI_QOS_STREAMER_MAX_LEVELS, PQI_QOS_STREAMER_ALPHA)
{
	_total_item_size = 0 ;
	_total_item_count = 0 ;
}

int pqiQoSstreamer::getQueueSize(bool in) 
{
	if(in)
		return pqistreamer::getQueueSize(in) ;
	else
	{
		RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/
		return qos_queue_size() ;
	}
}

//int  pqiQoSstreamer::locked_gatherStatistics(std::vector<uint32_t>& per_service_count,std::vector<uint32_t>& per_priority_count) const // extracting data.
//{
//    return pqiQoS::gatherStatistics(per_service_count,per_priority_count) ;
//}

void pqiQoSstreamer::locked_storeInOutputQueue(void *ptr,int size,int priority)
{
	_total_item_size += size ;
	++_total_item_count ;

	pqiQoS::in_rsItem(ptr,size,priority) ;
}

void pqiQoSstreamer::locked_clear_out_queue()
{
#ifdef DEBUG_PQIQOSSTREAMER
    if(qos_queue_size() > 0)
	    std::cerr << "  pqiQoSstreamer::locked_clear_out_queue(): clearing " << qos_queue_size() << " pending outqueue elements." << std::endl;
#endif
    
	pqiQoS::clear() ;
	_total_item_size = 0 ;
	_total_item_count = 0 ;
}

void *pqiQoSstreamer::locked_pop_out_data(uint32_t max_slice_size, uint32_t& size, bool& starts, bool& ends, uint32_t& packet_id)
{
	void *out = pqiQoS::out_rsItem(max_slice_size,size,starts,ends,packet_id) ;

	if(out != NULL) 
	{
		_total_item_size -= size ;
        
        	if(ends)
			--_total_item_count ;
	}

	return out ;
}

