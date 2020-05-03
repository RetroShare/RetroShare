/*******************************************************************************
 * libretroshare/src/pqi: pqistreamer.h                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2008 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef MRK_PQI_STREAMER_HEADER
#define MRK_PQI_STREAMER_HEADER

#include <stdint.h>               // for uint32_t
#include "util/rstime.h"                 // for rstime_t
#include <iostream>               // for operator<<, basic_ostream, cerr, endl
#include <list>                   // for list
#include <map>                    // for map

#include "pqi/pqi_base.h"         // for BinInterface (ptr only), PQInterface
#include "retroshare/rsconfig.h"  // for RSTrafficClue
#include "retroshare/rstypes.h"   // for RsPeerId
#include "util/rsthreads.h"       // for RsMutex

struct RsItem;
class RsSerialiser;

struct PartialPacketRecord
{
    void *mem ;
    uint32_t size ;
};

/**
 * @brief Fully implements the PQInterface and communicates with peer etc via
 *	the BinInterface.
 * The interface does not handle connection, just communication.
 * Possible BIN_FLAGS: BIN_FLAGS_NO_CLOSE | BIN_FLAGS_NO_DELETE
 */
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
		virtual int     status();

		rstime_t  getLastIncomingTS(); 	// Time of last data packet, for checking a connection is alive.
		virtual void    getRates(RsBwRates &rates);
		virtual int     getQueueSize(bool in); // extracting data.
		virtual int     gatherStatistics(std::list<RSTrafficClue>& outqueue_stats,std::list<RSTrafficClue>& inqueue_stats); // extracting data.
        
            	// mutex protected versions of RateInterface calls.
            	virtual void setRate(bool b,float f) ;
            	virtual void setMaxRate(bool b,float f) ;
            	virtual float getRate(bool b) ;

    protected:
        		virtual int reset() ;

		int tick_bio();
		int tick_send(uint32_t timeout);
		int tick_recv(uint32_t timeout);

		/* Implementation */

		// These methods are redefined in pqiQoSstreamer
		//
		virtual void locked_storeInOutputQueue(void *ptr, int size, int priority) ;
		virtual int locked_out_queue_size() const ;
		virtual void locked_clear_out_queue() ;
		virtual int locked_compute_out_pkt_size() const ;
		virtual void *locked_pop_out_data(uint32_t max_slice_size,uint32_t& size,bool& starts,bool& ends,uint32_t& packet_id);
		virtual int   locked_gatherStatistics(std::list<RSTrafficClue>& outqueue_stats,std::list<RSTrafficClue>& inqueue_stats); // extracting data.

        	void updateRates() ;
            	
	protected:
		RsMutex mStreamerMtx ; // Protects data, fns below, protected so pqiqos can use it too.

		// Binary Interface for IO, initialisated at startup.
		BinInterface *mBio;
		unsigned int  mBio_flags; // BIN_FLAGS_NO_CLOSE | BIN_FLAGS_NO_DELETE

	private:
		int queue_outpqi_locked(RsItem *i,uint32_t& serialized_size);
		int handleincomingitem(RsItem *i, int len);

		// ticked regularly (manages out queues and sending
		// via above interfaces.
		virtual int	handleoutgoing_locked();
		virtual int	handleincoming();

		// Bandwidth/Streaming Management.
		float	outTimeSlice_locked();

		int	outAllowedBytes_locked();
		void	outSentBytes_locked(uint32_t );

		int	inAllowedBytes();
		void	inReadBytes(uint32_t );

        		// cleans up everything that's pending / half finished.
		void free_pend();

		// RsSerialiser - determines which packets can be serialised.
		RsSerialiser *mRsSerialiser;

		void *mPkt_wpending; // storage for pending packet to write.
        	uint32_t mPkt_wpending_size; // ... and its size.

        void allocate_rpend(); // use these two functions to allocate/free the buffer below
        
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
        
        double mCurrReadTS; // TS from which these are measured.
        double mCurrSentTS;

		double mAvgLastUpdate; // TS from which these are measured.
		uint32_t mAvgReadCount;
		uint32_t mAvgSentCount;

		double mAvgDtOut;	// average time diff between 2 rounds of sending data
		double mAvgDtIn;	// average time diff between 2 rounds of receiving data

		rstime_t mLastIncomingTs;
	
        	// traffic statistics

        	std::list<RSTrafficClue> mPreviousStatsChunk_In ;
        	std::list<RSTrafficClue> mPreviousStatsChunk_Out ;
        	std::list<RSTrafficClue> mCurrentStatsChunk_In ;
        	std::list<RSTrafficClue> mCurrentStatsChunk_Out ;
		rstime_t mStatisticsTimeStamp ;

        bool mAcceptsPacketSlicing ;
        rstime_t mLastSentPacketSlicingProbe ;
        void locked_addTrafficClue(const RsItem *pqi, uint32_t pktsize, std::list<RSTrafficClue> &lst);
        RsItem *addPartialPacket(const void *block, uint32_t len, uint32_t slice_packet_id,bool packet_starting,bool packet_ending,uint32_t& total_len);
        
        std::map<uint32_t,PartialPacketRecord> mPartialPackets ;
};

#endif //MRK_PQI_STREAMER_HEADER
