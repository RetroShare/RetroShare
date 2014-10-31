/*
 * libretroshare/src/pqi pqistreamer.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2008 by Robert Fernie.
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


#ifndef MRK_PQI_STREAMER_HEADER
#define MRK_PQI_STREAMER_HEADER

// Only dependent on the base stuff.
#include "pqi/pqi_base.h"
#include "util/rsthreads.h"
#include "retroshare/rstypes.h"

#include <list>

// Fully implements the PQInterface.
// and communicates with peer etc via the BinInterface.
//
// The interface does not handle connection, just communication.
// possible bioflags: BIN_FLAGS_NO_CLOSE | BIN_FLAGS_NO_DELETE

class pqistreamer: public PQInterface
{
	public:
		pqistreamer(RsSerialiser *rss, const RsPeerId& peerid, BinInterface *bio_in, int bio_flagsin);
		virtual ~pqistreamer();

		// PQInterface
		virtual int     SendItem(RsItem *item)
		{
			std::cerr << "Warning pqistreamer::sendItem(RsItem*) should not be called. Plz call SendItem(RsItem *,uint32_t& serialized_size) instead." << std::endl;
			uint32_t serialized_size ;
			return SendItem(item,serialized_size) ;
		}
		virtual int     SendItem(RsItem *,uint32_t& serialized_size);
		virtual RsItem *GetItem();

		virtual int     tick();
		virtual int     status();

		time_t  getLastIncomingTS(); 	// Time of last data packet, for checking a connection is alive.
		virtual void    getRates(RsBwRates &rates);
		virtual int     getQueueSize(bool in); // extracting data.
        virtual int     gatherOutQueueStatistics(std::vector<uint32_t>& per_service_count,std::vector<uint32_t>& per_priority_count); // extracting data.
    protected:

		int tick_bio();
		int tick_send(uint32_t timeout);
		int tick_recv(uint32_t timeout);

		/* Implementation */

		// These methods are redefined in pqiQoSstreamer
		//
		virtual void locked_storeInOutputQueue(void *ptr,int priority) ;
		virtual int locked_out_queue_size() const ;
		virtual void locked_clear_out_queue() ;
		virtual int locked_compute_out_pkt_size() const ;
		virtual void *locked_pop_out_data() ;
        virtual int  locked_gatherStatistics(std::vector<uint32_t>& per_service_count,std::vector<uint32_t>& per_priority_count) const; // extracting data.


	protected:
		RsMutex mStreamerMtx ; // Protects data, fns below, protected so pqiqos can use it too.

		// Binary Interface for IO, initialisated at startup.
		BinInterface *mBio;
		unsigned int  mBio_flags; // BIN_FLAGS_NO_CLOSE | BIN_FLAGS_NO_DELETE

	private:
		int queue_outpqi_locked(RsItem *i,uint32_t& serialized_size);
		int handleincomingitem_locked(RsItem *i);

		// ticked regularly (manages out queues and sending
		// via above interfaces.
		virtual int	handleoutgoing_locked();
		virtual int	handleincoming_locked();

		// Bandwidth/Streaming Management.
		float	outTimeSlice_locked();

		int	outAllowedBytes_locked();
		void	outSentBytes_locked(int );

		int	inAllowedBytes_locked();
		void	inReadBytes_locked(int );



		// RsSerialiser - determines which packets can be serialised.
		RsSerialiser *mRsSerialiser;

		void *mPkt_wpending; // storage for pending packet to write.
		int   mPkt_rpend_size; // size of pkt_rpending.
		void *mPkt_rpending; // storage for read in pending packets.

		enum {reading_state_packet_started=1,
			reading_state_initial=0 } ;

		int   mReading_state ;
		int   mFailed_read_attempts ;

		// Temp Storage for transient data.....
		std::list<void *> mOutPkts; // Cntrl / Search / Results queue
		std::list<RsItem *> mIncoming;

        uint32_t mIncomingSize; // size of mIncoming. To avoid calling linear cost std::list::size()

		// data for network stats.
		int mTotalRead;
		int mTotalSent;

		// these are representative (but not exact)
		int mCurrRead;
		int mCurrSent;
		int mCurrReadTS; // TS from which these are measured.
		int mCurrSentTS;

		int mAvgLastUpdate; // TS from which these are measured.
		float mAvgReadCount;
		float mAvgSentCount;

		time_t mLastIncomingTs;
	

};

#endif //MRK_PQI_STREAMER_HEADER
