/*
 * libretroshare/src/pqi pqistreamer.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2012-2012 by Cyril Soler
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
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#pragma once

#include "pqiqos.h"
#include "pqistreamer.h"

class pqiQoSstreamer: public pqistreamer, public pqiQoS
{
	public:
		pqiQoSstreamer(RsSerialiser *rss, std::string peerid, BinInterface *bio_in, int bio_flagsin);

		static const uint32_t PQI_QOS_STREAMER_MAX_LEVELS =  10 ;
		static const float    PQI_QOS_STREAMER_ALPHA      = 2.0 ;

		virtual void locked_storeInOutputQueue(void *ptr,int priority) ;
		virtual int locked_out_queue_size() const { return _total_item_count ; }
		virtual void locked_clear_out_queue() ;
		virtual int locked_compute_out_pkt_size() const { return _total_item_size ; }
		virtual void *locked_pop_out_data() ;

		virtual int getQueueSize(bool in) ;

	private:
		uint32_t _total_item_size ;
		uint32_t _total_item_count ;
};

