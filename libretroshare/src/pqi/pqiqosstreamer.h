/*******************************************************************************
 * libretroshare/src/pqi: pqiqosstreamer.h                                     *
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
#pragma once

#include "pqiqos.h"
#include "pqithreadstreamer.h"

class pqiQoSstreamer: public pqithreadstreamer, public pqiQoS
{
	public:
		pqiQoSstreamer(PQInterface *parent, RsSerialiser *rss, const RsPeerId& peerid, BinInterface *bio_in, int bio_flagsin);

		static const uint32_t PQI_QOS_STREAMER_MAX_LEVELS =  10 ;
        static const float    PQI_QOS_STREAMER_ALPHA ;

		virtual void locked_storeInOutputQueue(void *ptr, int size, int priority) ;
		virtual int locked_out_queue_size() const { return _total_item_count ; }
		virtual void locked_clear_out_queue() ;
		virtual int locked_compute_out_pkt_size() const { return _total_item_size ; }
		virtual  void *locked_pop_out_data(uint32_t max_slice_size,uint32_t& size,bool& starts,bool& ends,uint32_t& packet_id);
                //virtual int  locked_gatherStatistics(std::vector<uint32_t>& per_service_count,std::vector<uint32_t>& per_priority_count) const; // extracting data.


		virtual int getQueueSize(bool in) ;

	private:
		uint32_t _total_item_size ;
		uint32_t _total_item_count ;
};

