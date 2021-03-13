/*******************************************************************************
 * libretroshare/src/pqi: pqistreamer.cc                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2006 by Robert Fernie <retroshare@lunamutt.com>              *
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
#include "pqi/pqistreamer.h"

#include <sys/time.h>             // for gettimeofday
#include <stdlib.h>               // for free, realloc, exit
#include <string.h>               // for memcpy, memset, memcmp
#include "util/rstime.h"                 // for NULL, time, rstime_t
#include <algorithm>              // for min
#include <iostream>               // for operator<<, ostream, basic_ostream
#include <string>                 // for string, allocator, operator<<, oper...
#include <utility>                // for pair

#include "pqi/p3notify.h"         // for p3Notify
#include "retroshare/rsids.h"     // for operator<<
#include "retroshare/rsnotify.h"  // for RS_SYS_WARNING
#include "rsserver/p3face.h"      // for RsServer
#include "serialiser/rsserial.h"  // for RsItem, RsSerialiser, getRsItemSize
#include "util/rsdebug.h"         // for pqioutput, PQL_ALERT, PQL_DEBUG_ALL
#include "util/rsmemory.h"        // for rs_malloc
#include "util/rsprint.h"         // for BinToHex
#include "util/rsstring.h"        // for rs_sprintf_append, rs_sprintf

static struct RsLog::logInfo pqistreamerzoneInfo = {RsLog::Default, "pqistreamer"};
#define pqistreamerzone &pqistreamerzoneInfo

static const int   PQISTREAM_ABS_MAX    			= 100000000; /* 100 MB/sec (actually per loop) */
static const int   PQISTREAM_AVG_PERIOD 			= 1; 		// update speed estimate every second
static const float PQISTREAM_AVG_FRAC   			= 0.8; 		// for bandpass filter over speed estimate.
static const float PQISTREAM_AVG_DT_FRAC                        = 0.99;         // for low pass filter over elapsed time

static const int   PQISTREAM_OPTIMAL_PACKET_SIZE  		= 512;		// It is believed that this value should be lower than TCP slices and large enough as compare to encryption padding.
										// most importantly, it should be constant, so as to allow correct QoS.
static const int   PQISTREAM_SLICE_FLAG_STARTS			= 0x01;		// 
static const int   PQISTREAM_SLICE_FLAG_ENDS 			= 0x02;		// these flags should be kept in the range 0x01-0x08
static const int   PQISTREAM_SLICE_PROTOCOL_VERSION_ID_01     = 0x10;		// Protocol version ID. Should hold on the 4 lower bits.
static const int   PQISTREAM_PARTIAL_PACKET_HEADER_SIZE	= 8;   		// Same size than normal header, to make the code simpler.
static const int   PQISTREAM_PACKET_SLICING_PROBE_DELAY	= 60;  		// send every 60 secs.

// This is a probe packet, that won't deserialise (it's empty) but will not cause problems to old peers either, since they will ignore
// it. This packet however will be understood by new peers as a signal to enable packet slicing. This should go when all peers use the
// same protocol.

static uint8_t PACKET_SLICING_PROBE_BYTES[8] =  { 0x02, 0xaa, 0xbb, 0xcc, 0x00, 0x00, 0x00,  0x08 } ;

/* Change to true to disable packet slicing and/or packet grouping, if needed */
#define DISABLE_PACKET_SLICING  false
#define DISABLE_PACKET_GROUPING false

/* This removes the print statements (which hammer pqidebug) */
/***
#define RSITEM_DEBUG 1
#define DEBUG_TRANSFERS	 1
#define DEBUG_PQISTREAMER 1
#define DEBUG_PACKET_SLICING 1
 ***/

#ifdef DEBUG_TRANSFERS
	#include "util/rsprint.h"
#endif

static double getCurrentTS()
{
#ifndef WINDOWS_SYS
        struct timeval cts_tmp;
        gettimeofday(&cts_tmp, NULL);
        double cts =  (cts_tmp.tv_sec) + ((double) cts_tmp.tv_usec) / 1000000.0;
#else
        struct _timeb timebuf;
        _ftime( &timebuf);
        double cts =  (timebuf.time) + ((double) timebuf.millitm) / 1000.0;
#endif
        return cts;
}

pqistreamer::pqistreamer(RsSerialiser *rss, const RsPeerId& id, BinInterface *bio_in, int bio_flags_in)
	:PQInterface(id), mStreamerMtx("pqistreamer"),
	mBio(bio_in), mBio_flags(bio_flags_in), mRsSerialiser(rss), 
	mPkt_wpending(NULL), mPkt_wpending_size(0),
	mTotalRead(0), mTotalSent(0),
	mCurrRead(0), mCurrSent(0),
	mAvgReadCount(0), mAvgSentCount(0),
	mAvgDtOut(0), mAvgDtIn(0)
{

	// 100 B/s (minimal)
	setMaxRate(true, 0.1);
	setMaxRate(false, 0.1);
	setRate(true, 0);		// needs to be off-mutex
	setRate(false, 0);

	RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/

	mAcceptsPacketSlicing = false ; // by default. Will be turned into true when everyone's ready.
	mLastSentPacketSlicingProbe = 0 ;

	mAvgLastUpdate = mCurrSentTS = mCurrReadTS = getCurrentTS();

	mIncomingSize = 0 ;
	mIncomingSize_bytes = 0;

	mStatisticsTimeStamp = 0 ;
	/* allocated once */
	mPkt_rpend_size = 0;
	mPkt_rpending = 0;
	mReading_state = reading_state_initial ;

	pqioutput(PQL_DEBUG_ALL, pqistreamerzone, "pqistreamer::pqistreamer() Initialisation!");

	if (!bio_in)
	{
		pqioutput(PQL_ALERT, pqistreamerzone, "pqistreamer::pqistreamer() NULL bio, FATAL ERROR!");
		exit(1);
	}

	mFailed_read_attempts = 0;  // reset failed read, as no packet is still read.

	return;
}

pqistreamer::~pqistreamer()
{
	RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/
#ifdef DEBUG_PQISTREAMER
    	std::cerr << "Closing pqistreamer." << std::endl;
#endif
	pqioutput(PQL_DEBUG_ALL, pqistreamerzone, "pqistreamer::~pqistreamer() Destruction!");

	if (mBio_flags & BIN_FLAGS_NO_CLOSE)
	{
		pqioutput(PQL_DEBUG_ALL, pqistreamerzone, "pqistreamer::~pqistreamer() Not Closing BinInterface!");
	}
	else if (mBio)
	{
		pqioutput(PQL_DEBUG_ALL, pqistreamerzone, "pqistreamer::~pqistreamer() Deleting BinInterface!");

		delete mBio;
	}

	/* clean up serialiser */
	if (mRsSerialiser)
		delete mRsSerialiser;

	free_pend() ;

	// clean up incoming.
	while (!mIncoming.empty())
	{
		RsItem *i = mIncoming.front();
		mIncoming.pop_front() ;
		--mIncomingSize;
		delete i;
	}

	if (mIncomingSize != 0)
		std::cerr << "(EE) inconsistency after deleting pqistreamer queue. Remaining items: " << mIncomingSize << std::endl ;
	return;
}


// Get/Send Items.
// This is the entry poing for methods willing to send items through our out queue
int	pqistreamer::SendItem(RsItem *si,uint32_t& out_size)
{
#ifdef RSITEM_DEBUG 
	{
		std::string out = "pqistreamer::SendItem():\n";
		si -> print_string(out);
		pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out);
		std::cerr << out;
	}
#endif

	RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/
	
	return queue_outpqi_locked(si,out_size);
}

RsItem *pqistreamer::GetItem()
{
#ifdef DEBUG_PQISTREAMER
	pqioutput(PQL_DEBUG_ALL, pqistreamerzone, "pqistreamer::GetItem()");
#endif

	if(mIncoming.empty())
		return NULL; 

	RsItem *osr = mIncoming.front() ;
	mIncoming.pop_front() ;
	--mIncomingSize;
// for future use
//	mIncomingSize_bytes -= 

	return osr;
}


float pqistreamer::getMaxRate(bool b)
{
        RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/
        return getMaxRate_locked(b);
}

float pqistreamer::getMaxRate_locked(bool b)
{
        return RateInterface::getMaxRate(b) ;
}

float pqistreamer::getRate(bool b)
{
	RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/
    	return RateInterface::getRate(b) ;
}

void pqistreamer::setMaxRate(bool b,float f)
{
        RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/
        setMaxRate_locked(b,f);
}

void pqistreamer::setMaxRate_locked(bool b,float f)
{
        RateInterface::setMaxRate(b,f) ;
}

void pqistreamer::setRate(bool b,float f)
{
	RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/
    	RateInterface::setRate(b,f) ;
}


void pqistreamer::updateRates()
{
	// update actual rates both ways.

	double t = getCurrentTS(); // get current timestamp.
	double diff = t - mAvgLastUpdate;

	if (diff > PQISTREAM_AVG_PERIOD)
	{
		float avgReadpSec = PQISTREAM_AVG_FRAC * getRate(true ) + (1.0 - PQISTREAM_AVG_FRAC) * mAvgReadCount/(1024.0 * diff);
		float avgSentpSec = PQISTREAM_AVG_FRAC * getRate(false) + (1.0 - PQISTREAM_AVG_FRAC) * mAvgSentCount/(1024.0 * diff);

#ifdef DEBUG_PQISTREAMER
		uint64_t t_now = 1000 * getCurrentTS();
		std::cerr << std::dec << t_now << " DEBUG_PQISTREAMER pqistreamer::updateRates PeerId " << this->PeerId().toStdString() << " Current speed estimates: down " << std::dec << (int)(1024 * avgReadpSec)  << " B/s / up " << (int)(1024 * avgSentpSec) << " B/s"  << std::endl;
#endif

		// now store the new rates, zero meaning that we are not bandwidthLimited()

		if (mBio->bandwidthLimited())
		{
			setRate(true, avgReadpSec);
			setRate(false, avgSentpSec);
		}
		else
		{
			setRate(true, 0);
			setRate(false, 0);
		}

		mAvgLastUpdate = t;
		mAvgReadCount = 0;

		{
			RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/
			mAvgSentCount = 0;
		}
	}
}
 
int 	pqistreamer::tick_bio()
{
	RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/
	mBio->tick();
	
	/* short circuit everything if bio isn't active */
	if (!(mBio->isactive()))
	{
		return 0;
	}
	return 1;
}

int 	pqistreamer::tick_recv(uint32_t timeout)
{
	if (mBio->moretoread(timeout))
	{
		handleincoming();
	}
	if(!(mBio->isactive()))
	{
		RsStackMutex stack(mStreamerMtx);
		free_pend();
	}
	return 1;
}

int 	pqistreamer::tick_send(uint32_t timeout)
{
	/* short circuit everything if bio isn't active */
	if (!(mBio->isactive()))
	{
		RsStackMutex stack(mStreamerMtx);
		free_pend();
		return 0;
	}

	if (mBio->cansend(timeout))
	{
		RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/
		handleoutgoing_locked();
	}
    
	return 1;
}

int	pqistreamer::status()
{
	RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/


#ifdef DEBUG_PQISTREAMER
	pqioutput(PQL_DEBUG_ALL, pqistreamerzone, "pqistreamer::status()");

	if (mBio->isactive())
	{
		std::string out;
		rs_sprintf(out, "Data in:%d out:%d", mTotalRead, mTotalSent);
		pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, out);
	}
#endif

	return 0;
}

// this method is overloaded by pqiqosstreamer
void pqistreamer::locked_storeInOutputQueue(void *ptr,int,int)
{
	mOutPkts.push_back(ptr);
}

int	pqistreamer::queue_outpqi_locked(RsItem *pqi,uint32_t& pktsize)
{
	pktsize = 0 ;
#ifdef DEBUG_PQISTREAMER
        std::cerr << "pqistreamer::queue_outpqi() called." << std::endl;
#endif

	/* decide which type of packet it is */

	pktsize = mRsSerialiser->size(pqi);
	void *ptr = rs_malloc(pktsize);
    
    	if(ptr == NULL)
            return 0 ;

#ifdef DEBUG_PQISTREAMER
	std::cerr << "pqistreamer::queue_outpqi() serializing packet with packet size : " << pktsize << std::endl;
#endif

        /*******************************************************************************************/
    	// keep info for stats for a while. Only keep the items for the last two seconds. sec n is ongoing and second n-1
    	// is a full statistics chunk that can be used in the GUI

    	locked_addTrafficClue(pqi,pktsize,mCurrentStatsChunk_Out) ;

        /*******************************************************************************************/

	if (mRsSerialiser->serialise(pqi, ptr, &pktsize))
	{
		locked_storeInOutputQueue(ptr,pktsize,pqi->priority_level()) ;

		if (!(mBio_flags & BIN_FLAGS_NO_DELETE))
		{
			delete pqi;
		}
		return 1;
	}
	else
	{
		/* cleanup serialiser */
		free(ptr);
	}

	std::string out = "pqistreamer::queue_outpqi() Null Pkt generated!\nCaused By:\n";
	pqi -> print_string(out);
	pqioutput(PQL_ALERT, pqistreamerzone, out);

	if (!(mBio_flags & BIN_FLAGS_NO_DELETE))
	{
		delete pqi;
	}
	return 1; // keep error internal.
}

int 	pqistreamer::handleincomingitem(RsItem *pqi,int len)
{

#ifdef DEBUG_PQISTREAMER
	pqioutput(PQL_DEBUG_ALL, pqistreamerzone, "pqistreamer::handleincomingitem()");
#endif
	// timestamp last received packet.
	mLastIncomingTs = time(NULL);

	// Use overloaded Contact function 
	pqi -> PeerId(PeerId());

	mIncoming.push_back(pqi);
	++mIncomingSize;
	// for future use
	//	mIncomingSize_bytes += len;

	/*******************************************************************************************/
	// keep info for stats for a while. Only keep the items for the last two seconds. sec n is ongoing and second n-1
	// is a full statistics chunk that can be used in the GUI
	{
		RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/
		locked_addTrafficClue(pqi,len,mCurrentStatsChunk_In) ;
	}
	/*******************************************************************************************/

	return 1;
}

void pqistreamer::locked_addTrafficClue(const RsItem *pqi,uint32_t pktsize,std::list<RSTrafficClue>& lst)
{
    rstime_t now = time(NULL) ;

    if(now > mStatisticsTimeStamp)	// new chunk => get rid of oldest, replace old list by current list, clear current list.
    {
	    mPreviousStatsChunk_Out = mCurrentStatsChunk_Out ;
	    mPreviousStatsChunk_In = mCurrentStatsChunk_In ;
	    mCurrentStatsChunk_Out.clear() ;
	    mCurrentStatsChunk_In.clear() ;

	    mStatisticsTimeStamp = now ;
    }

    RSTrafficClue tc ;
    tc.TS = now ;
    tc.size = pktsize ;
    tc.priority = pqi->priority_level() ;
    tc.peer_id = pqi->PeerId() ;
    tc.count = 1 ;
    tc.service_id = pqi->PacketService() ;
    tc.service_sub_id = pqi->PacketSubType() ;

    lst.push_back(tc) ;
}

rstime_t	pqistreamer::getLastIncomingTS()
{
	// This is the only case where another thread (rs main for pqiperson) will access our data
	// Still a mutex lock is not needed because the operation is atomic
	return mLastIncomingTs;
}

// Packet slicing:
//
//          Old :    02 0014 03 00000026  [data,  26 bytes] =>   [version 1B] [service 2B][subpacket 1B] [size 4B]
//          New2:    pp ff xxxxxxxx ssss  [data, sss bytes] =>   [protocol version 1B] [flags 1B] [2^32 packet count] [2^16 size]
//
//           	Encode protocol  on 1.0 Bytes ( 8 bits)
//           	Encode flags     on 1.0 Bytes ( 8 bits)
//          		0x01 => incomplete packet continued after
//          		0x02 => packet ending a previously incomplete packet
//
//		Encode packet ID on 4.0 Bytes (32 bits) => packet counter = [0...2^32]
//		Encode size      on 2.0 Bytes (16 bits) => 65536					// max slice size = 65536
//
//	Backward compatibility:
//		* send one packet with service + subpacket = aabbcc. Old peers will silently ignore such packets. Full packet header is: 02aabbcc 00000008 
//		* if received, mark the peer as able to decode the new packet type
//	In pqiQoS:
//		- limit packet grouping to max size 512.
//		- new peers need to read flux, and properly extract partial sizes, and combine packets based on packet counter.
//		- on sending, RS grabs slices of max size 1024 from pqiQoS. If smaller, possibly pack them together.
//		  pqiQoS keeps track of sliced packets and makes sure the output is consistent:
//				* when a large packet needs to be send, only takes a slice and return it, and update the remaining part
//				* always consider priority when taking new slices => a newly arrived fast packet will always get through.
//
//	Max slice size should be customisable, depending on bandwidth. To be tested...
//

int	pqistreamer::handleoutgoing_locked()
{
#ifdef DEBUG_PQISTREAMER
    pqioutput(PQL_DEBUG_ALL, pqistreamerzone, "pqistreamer::handleoutgoing_locked()");
#endif

    int maxbytes = outAllowedBytes_locked();
    int sentbytes = 0;
    
    //	std::cerr << "pqistreamer: maxbytes=" << maxbytes<< std::endl ; 

    std::list<void *>::iterator it;

    // if not connection, or cannot send anything... pause.
    if (!(mBio->isactive()))
    {
	    /* if we are not active - clear anything in the queues. */
	    locked_clear_out_queue() ;
#ifdef DEBUG_PACKET_SLICING 
        	std::cerr << "(II) Switching off packet slicing." << std::endl;
#endif
        	mAcceptsPacketSlicing = false ;

	    /* also remove the pending packets */
	    if (mPkt_wpending)
	    {
		    free(mPkt_wpending);
		    mPkt_wpending = NULL;
		    	mPkt_wpending_size = 0 ;
	    }

	    return 0;
    }

    // a very simple round robin

    bool sent = true;
    int nsent = 0 ;
    while(sent) // catch if all items sent.
    {
	    sent = false;

	    if ((!(mBio->cansend(0))) || (maxbytes < sentbytes))
	    {
#ifdef DEBUG_PQISTREAMER
		if (sentbytes > maxbytes)
			RsDbg() << "PQISTREAMER pqistreamer::handleoutgoing_locked() stopped sending max reached, sentbytes " << std::dec << sentbytes << " maxbytes " << maxbytes;
		else
			RsDbg() << "PQISTREAMER pqistreamer::handleoutgoing_locked() stopped sending bio not ready, sentbytes " << std::dec << sentbytes << " maxbytes " << maxbytes;
#endif
		    return 0;
	    }
	    // send a out_pkt., else send out_data. unless there is a pending packet. The strategy is to
            //	- grab as many packets as possible while below the optimal packet size, so as to allow some packing and decrease encryption padding overhead (suposeddly)
            //	- limit packets size to OPTIMAL_PACKET_SIZE when sending big packets so as to keep as much QoS as possible.
        
	    if (!mPkt_wpending)
	{
		void *dta;
		mPkt_wpending_size = 0 ;
		int k=0;

        	// Checks for inserting a packet slicing probe. We do that to send the other peer the information that packet slicing can be used.
        	// if so, we enable it for the session. This should be removed (because it's unnecessary) when all users have switched to the new version.
		rstime_t now = time(NULL) ;
        
            if(now > mLastSentPacketSlicingProbe + PQISTREAM_PACKET_SLICING_PROBE_DELAY)
        	{
#ifdef DEBUG_PACKET_SLICING
                	std::cerr << "(II) Inserting packet slicing probe in traffic" << std::endl;
#endif
                    
                    	mPkt_wpending_size = 8 ;
                    	mPkt_wpending = rs_malloc(8) ;
                        memcpy(mPkt_wpending,PACKET_SLICING_PROBE_BYTES,8) ;
                        
                	mLastSentPacketSlicingProbe = now ;
        	}
            
        	uint32_t slice_size=0;
		bool slice_starts=true ;
		bool slice_ends=true ;
		uint32_t slice_packet_id=0 ;

		do
		{
            		int desired_packet_size = mAcceptsPacketSlicing?PQISTREAM_OPTIMAL_PACKET_SIZE:(getRsPktMaxSize());
                    
			dta = locked_pop_out_data(desired_packet_size,slice_size,slice_starts,slice_ends,slice_packet_id) ;

			if(!dta)
				break ;

			if(slice_starts && slice_ends)	// good old method. Send the packet as is, since it's a full packet.
			{
#ifdef DEBUG_PACKET_SLICING
				std::cerr << "sending full slice, old style. Size=" << slice_size << std::endl;
#endif
				mPkt_wpending = realloc(mPkt_wpending,slice_size+mPkt_wpending_size) ;
				memcpy( &((char*)mPkt_wpending)[mPkt_wpending_size],dta,slice_size) ;
				free(dta);
				mPkt_wpending_size += slice_size ;
				++k ;
			}
			else	// partial packet. We make a special header for it and insert it in the stream
			{
				if(slice_size > 0xffff || !mAcceptsPacketSlicing)
				{
					std::cerr << "(EE) protocol error in pqitreamer: slice size is too large and cannot be encoded." ;
					free(mPkt_wpending) ;
					mPkt_wpending_size = 0;
					return -1 ;
				}
#ifdef DEBUG_PACKET_SLICING
				std::cerr << "sending partial slice, packet ID=" << std::hex << slice_packet_id << std::dec << ", size=" << slice_size << std::endl;
#endif

				mPkt_wpending = realloc(mPkt_wpending,slice_size+mPkt_wpending_size+PQISTREAM_PARTIAL_PACKET_HEADER_SIZE) ;
				memcpy( &((char*)mPkt_wpending)[mPkt_wpending_size+PQISTREAM_PARTIAL_PACKET_HEADER_SIZE],dta,slice_size) ;
				free(dta);

				// New2: pp ff xxxxxxxx ssss  [data, sss bytes] => [flags 1B] [protocol version 1B] [2^32 packet count] [2^16 size]

				uint8_t partial_flags = 0 ;
				if(slice_starts) partial_flags |= PQISTREAM_SLICE_FLAG_STARTS  ;
				if(slice_ends  ) partial_flags |= PQISTREAM_SLICE_FLAG_ENDS  ;

				((char*)mPkt_wpending)[mPkt_wpending_size+0x00] = PQISTREAM_SLICE_PROTOCOL_VERSION_ID_01 ;
				((char*)mPkt_wpending)[mPkt_wpending_size+0x01] = partial_flags ;
				((char*)mPkt_wpending)[mPkt_wpending_size+0x02] = uint8_t(slice_packet_id >> 24) & 0xff ;
				((char*)mPkt_wpending)[mPkt_wpending_size+0x03] = uint8_t(slice_packet_id >> 16) & 0xff ;
				((char*)mPkt_wpending)[mPkt_wpending_size+0x04] = uint8_t(slice_packet_id >>  8) & 0xff ;
				((char*)mPkt_wpending)[mPkt_wpending_size+0x05] = uint8_t(slice_packet_id >>  0) & 0xff ;	
				((char*)mPkt_wpending)[mPkt_wpending_size+0x06] = uint8_t(slice_size      >>  8) & 0xff ;
				((char*)mPkt_wpending)[mPkt_wpending_size+0x07] = uint8_t(slice_size      >>  0) & 0xff ;

				mPkt_wpending_size += slice_size + PQISTREAM_PARTIAL_PACKET_HEADER_SIZE;
				++k ;
			}
		} 
                 while(mPkt_wpending_size < (uint32_t)maxbytes && mPkt_wpending_size < PQISTREAM_OPTIMAL_PACKET_SIZE && !DISABLE_PACKET_GROUPING) ;
             
#ifdef DEBUG_PQISTREAMER
		if(k > 1)
			std::cerr << "Packed " << k << " packets into " << mPkt_wpending_size << " bytes." << std::endl;
#endif
	}
        
	    if (mPkt_wpending)
	    {
		    // write packet.
#ifdef DEBUG_PQISTREAMER
		std::cout << "Sending Out Pkt of size " << mPkt_wpending_size << " !" << std::endl;
#endif
            		int ss=0;

		    if (mPkt_wpending_size != (uint32_t)(ss = mBio->senddata(mPkt_wpending, mPkt_wpending_size)))
		    {
#ifdef DEBUG_PQISTREAMER
			    std::string out;
			    rs_sprintf(out, "Problems with Send Data! (only %d bytes sent, total pkt size=%d)", ss, mPkt_wpending_size);
			    //				std::cerr << out << std::endl ;
			    pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, out);
#endif
                		std::cerr << PeerId() << ": sending failed. Only " << ss << " bytes sent over " << mPkt_wpending_size << std::endl;

			    // pkt_wpending will kept til next time.
			    // ensuring exactly the same data is written (openSSL requirement).
			    return -1;
		    }
#ifdef DEBUG_PQISTREAMER
            else
                		std::cerr << PeerId() << ": sent " << ss << " bytes " << std::endl;
#endif
            
		    ++nsent;
            
            outSentBytes_locked(mPkt_wpending_size);	// this is the only time where we know exactly what was sent.

#ifdef DEBUG_TRANSFERS
		    std::cerr << "pqistreamer::handleoutgoing_locked() Sent Packet len: " << mPkt_wpending_size << " @ " << RsUtil::AccurateTimeString();
		    std::cerr << std::endl;
#endif

		    sentbytes += mPkt_wpending_size;
            
		    free(mPkt_wpending);
		    mPkt_wpending = NULL;
		    mPkt_wpending_size = 0 ;

		    sent = true;
	    }
    }
#ifdef DEBUG_PQISTREAMER
    if(nsent > 0)
	    std::cerr << "nsent = " << nsent << ", total bytes=" << sentbytes << std::endl;
#endif
    return 1;
}


/* Handles reading from input stream.
 */
int pqistreamer::handleincoming()
{
    int readbytes = 0;
    static const int max_failed_read_attempts = 2000 ;

#ifdef DEBUG_PQISTREAMER
    pqioutput(PQL_DEBUG_ALL, pqistreamerzone, "pqistreamer::handleincoming()");
#endif

    if(!(mBio->isactive()))
    {
	    RsStackMutex stack(mStreamerMtx);
	    mReading_state = reading_state_initial ;
	    free_pend();
	    return 0;
    }
    else
	    allocate_rpend();

    // enough space to read any packet.
    uint32_t maxlen = mPkt_rpend_size; 
    void *block = mPkt_rpending; 

    // initial read size: basic packet.
    int blen = getRsPktBaseSize();	// this is valid for both packet slices and normal un-sliced packets (same header size)

    int maxin = inAllowedBytes();

#ifdef DEBUG_PQISTREAMER
    std::cerr << "[" << (void*)pthread_self() << "] " << "reading state = " << mReading_state << std::endl ;
#endif
    switch(mReading_state)
    {
    case reading_state_initial: 			/*std::cerr << "jumping to start" << std::endl; */ goto start_packet_read ;
    case reading_state_packet_started:	/*std::cerr << "jumping to middle" << std::endl;*/ goto continue_packet ;
    }

start_packet_read:
    {	// scope to ensure variable visibility
	    // read the basic block (minimum packet size)
	    int tmplen;
#ifdef DEBUG_PQISTREAMER
	    std::cerr << "[" << (void*)pthread_self() << "] " << "starting packet" << std::endl ;
#endif
	    memset(block,0,blen) ;	// reset the block, to avoid uninitialized memory reads.

	    if (blen != (tmplen = mBio->readdata(block, blen)))
	    {
		    pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, "pqistreamer::handleincoming() Didn't read BasePkt!");

		    // error.... (either blocked or failure)
		    if (tmplen == 0)
		    {
#ifdef DEBUG_PQISTREAMER
			    // most likely blocked!
			    pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, "pqistreamer::handleincoming() read blocked");
			    std::cerr << "[" << (void*)pthread_self() << "] " << "given up 1" << std::endl ;
#endif
			    return 0;
		    }
		    else if (tmplen < 0)
		    {
			    // Most likely it is that the packet is pending but could not be read by pqissl because of stream flow.
			    // So we return without an error, and leave the machine state in 'start_read'.
			    //
			    //pqioutput(PQL_WARNING, pqistreamerzone, "pqistreamer::handleincoming() Error in bio read");
#ifdef DEBUG_PQISTREAMER
			    std::cerr << "[" << (void*)pthread_self() << "] " << "given up 2, state = " << mReading_state << std::endl ;
#endif
			    return 0;
		    }
		    else // tmplen > 0
		    {
			    // strange case....This should never happen as partial reads are handled by pqissl below.
#ifdef DEBUG_PQISTREAMER
			    std::string out = "pqistreamer::handleincoming() Incomplete ";
			    rs_sprintf_append(out, "(Strange) read of %d bytes", tmplen);
			    pqioutput(PQL_ALERT, pqistreamerzone, out);

			    std::cerr << "[" << (void*)pthread_self() << "] " << "given up 3" << std::endl ;
#endif
			    return -1;
		    }
	    }
#ifdef DEBUG_PQISTREAMER
	    std::cerr << "[" << (void*)pthread_self() << "] " << "block 0 : " << RsUtil::BinToHex((unsigned char*)block,8) << std::endl;
#endif

	    readbytes += blen;
	    mReading_state = reading_state_packet_started ;
	    mFailed_read_attempts = 0 ;						// reset failed read, as the packet has been totally read.

	    // Check for packet slicing probe (04/26/2016). To be removed when everyone uses it.

	    if(!memcmp(block,PACKET_SLICING_PROBE_BYTES,8))
	    {
                    mAcceptsPacketSlicing = !DISABLE_PACKET_SLICING;
#ifdef DEBUG_PACKET_SLICING
		    std::cerr << "(II) Enabling packet slicing!" << std::endl;
#endif
	    }
    }
continue_packet:
    {
	    // workout how much more to read.

	    bool is_partial_packet  = false ;
	    bool is_packet_starting = (((char*)block)[1] == PQISTREAM_SLICE_FLAG_STARTS) ; 	// STARTS and ENDS flags are actually never combined.
	    bool is_packet_ending   = (((char*)block)[1] == PQISTREAM_SLICE_FLAG_ENDS) ; 
	    bool is_packet_middle   = (((char*)block)[1] == 0x00) ; 

	    uint32_t extralen =0;
	    uint32_t slice_packet_id =0;

	    if( ((char*)block)[0] == PQISTREAM_SLICE_PROTOCOL_VERSION_ID_01 && ( is_packet_starting || is_packet_middle || is_packet_ending))
	    {
		    extralen        = (uint32_t(((uint8_t*)block)[6]) << 8 ) + (uint32_t(((uint8_t*)block)[7]));
		    slice_packet_id = (uint32_t(((uint8_t*)block)[2]) << 24) + (uint32_t(((uint8_t*)block)[3]) << 16) + (uint32_t(((uint8_t*)block)[4]) << 8) + (uint32_t(((uint8_t*)block)[5]) << 0);

#ifdef DEBUG_PACKET_SLICING
		    std::cerr << "Reading partial packet from mem block " << RsUtil::BinToHex((char*)block,8) << ": packet_id=" << std::hex << slice_packet_id << std::dec << ", len=" << extralen << std::endl;
#endif
		    is_partial_packet = true ;
            
                        mAcceptsPacketSlicing = !DISABLE_PACKET_SLICING; // this is needed
	    }
	    else
		    extralen = getRsItemSize(block) - blen;	// old style packet type

#ifdef DEBUG_PACKET_SLICING
	    std::cerr << "[" << (void*)pthread_self() << "] " << "continuing packet getRsItemSize(block) = " << getRsItemSize(block) << std::endl ;
	    std::cerr << "[" << (void*)pthread_self() << "] " << "continuing packet extralen = " << extralen << std::endl ;

	    std::cerr << "[" << (void*)pthread_self() << "] " << "continuing packet state=" << mReading_state << std::endl ;
	    std::cerr << "[" << (void*)pthread_self() << "] " << "block 1 : " << RsUtil::BinToHex((unsigned char*)block,8) << std::endl;
#endif
	    if (extralen + (uint32_t)blen > maxlen)
	    {
		    pqioutput(PQL_ALERT, pqistreamerzone, "ERROR: Read Packet too Big!");

		    p3Notify *notify = RsServer::notify();
		    if (notify)
		    {
			    std::string title =
			                    "Warning: Bad Packet Read";

			    std::string msg;
			    msg =   "               **** WARNING ****     \n";
			    msg +=  "Retroshare has caught a BAD Packet Read";
			    msg +=  "\n";
			    msg +=  "This is normally caused by connecting to an";
			    msg +=  " OLD version of Retroshare";
			    msg +=  "\n";
			    rs_sprintf_append(msg, "(M:%d B:%d E:%d)\n", maxlen, blen, extralen);
			    msg +=  "\n";
			    msg +=  "block = " ;
                	    msg += RsUtil::BinToHex((char*)block,8);

			    msg +=  "\n";
			    msg +=  "Please get your friends to upgrade to the latest version";
			    msg +=  "\n";
			    msg +=  "\n";
			    msg +=  "If you are sure the error was not caused by an old version";
			    msg +=  "\n";
			    msg +=  "Please report the problem to Retroshare's developers";
			    msg +=  "\n";

			    notify->AddLogMessage(0, RS_SYS_WARNING, title, msg);

			    std::cerr << "pqistreamer::handle_incoming() ERROR: Read Packet too Big" << std::endl;
			    std::cerr << msg;
			    std::cerr << std::endl;

		    }
		    mBio->close();	
		    mReading_state = reading_state_initial ;	// restart at state 1.
		    mFailed_read_attempts = 0 ;
		    return -1;

		    // Used to exit now! exit(1);
	    }

	    if (extralen > 0)
	    {
		    void *extradata = (void *) (((char *) block) + blen);
		    int tmplen ;

		    // Don't reset the block now! If pqissl is in the middle of a multiple-chunk
		    // packet (larger than 16384 bytes), and pqistreamer jumped directly yo
		    // continue_packet:, then readdata is going to write after the beginning of
		    // extradata, yet not exactly at start -> the start of the packet would be wiped out.
		    //
		    // so, don't do that:
		    //		memset( extradata,0,extralen ) ;	

		    if (extralen != (uint32_t)(tmplen = mBio->readdata(extradata, extralen)))
		    {
#ifdef DEBUG_PACKET_SLICING
			    if(tmplen > 0)
				    std::cerr << "[" << (void*)pthread_self() << "] " << "Incomplete packet read ! This is a real problem ;-)" << std::endl ;
#endif

			    if(++mFailed_read_attempts > max_failed_read_attempts)
			    {
				    std::string out;
				    rs_sprintf(out, "Error Completing Read (read %d/%d)", tmplen, extralen);
				    std::cerr << out << std::endl ;
				    pqioutput(PQL_ALERT, pqistreamerzone, out);

				    p3Notify *notify = RsServer::notify();
				    if (notify)
				    {
					    std::string title = "Warning: Error Completing Read";

					    std::string msgout;
					    msgout =   "               **** WARNING ****     \n";
					    msgout +=  "Retroshare has experienced an unexpected Read ERROR";
					    msgout +=  "\n";
					    rs_sprintf_append(msgout, "(M:%d B:%d E:%d R:%d)\n", maxlen, blen, extralen, tmplen);
					    msgout +=  "\n";
					    msgout +=  "Note: this error might as well happen (rarely) when a peer disconnects in between a transmission of a large packet.\n";
					    msgout +=  "If it happens manny time, please contact the developers, and send them these numbers:";
					    msgout +=  "\n";

					    msgout +=  "block = " ;
                        			msgout += RsUtil::BinToHex((char*)block,8) + "\n" ;

					    std::cerr << msgout << std::endl;
				    }

				    mBio->close();	
				    mReading_state = reading_state_initial ;	// restart at state 1.
				    mFailed_read_attempts = 0 ;
				    return -1;
			    }
			    else
			    {
#ifdef DEBUG_PQISTREAMER
				    std::cerr << "[" << (void*)pthread_self() << "] " << "given up 5, state = " << mReading_state << std::endl ;
#endif
				    return 0 ;	// this is just a SSL_WANT_READ error. Don't panic, we'll re-try the read soon.
				    // we assume readdata() returned either -1 or the complete read size.
			    }
		    }
#ifdef DEBUG_PACKET_SLICING
		    std::cerr << "[" << (void*)pthread_self() << "] " << "continuing packet state=" << mReading_state << std::endl ;
		    std::cerr << "[" << (void*)pthread_self() << "] " << "block 2 : " << RsUtil::BinToHex((unsigned char*)extradata,8) << std::endl;
#endif

		    mFailed_read_attempts = 0 ;
		    readbytes += extralen;
	    }

	    // create packet, based on header.
#ifdef DEBUG_PQISTREAMER
	    {
		    std::string out;
		    rs_sprintf(out, "Read Data Block -> Incoming Pkt(%d)", blen + extralen);
		    //std::cerr << out ;
		    pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, out);
	    }
#endif

	    uint32_t pktlen = blen+extralen ;
#ifdef DEBUG_PQISTREAMER
	    std::cerr << "[" << (void*)pthread_self() << "] " << RsUtil::BinToHex((char*)block,8) << "...: deserializing. Size=" << pktlen << std::endl ;
#endif
	    RsItem *pkt ;

	    if(is_partial_packet)
	    {
#ifdef DEBUG_PACKET_SLICING
		    std::cerr << "Inputing partial packet " << RsUtil::BinToHex((char*)block,8) << std::endl;
#endif
            		uint32_t packet_length = 0 ;
		    pkt = addPartialPacket(block,pktlen,slice_packet_id,is_packet_starting,is_packet_ending,packet_length) ;
            
            		pktlen = packet_length ;
	    }
	    else
		    pkt = mRsSerialiser->deserialise(block, &pktlen);

	    if ((pkt != NULL) && (0  < handleincomingitem(pkt,pktlen)))
	    {
#ifdef DEBUG_PQISTREAMER
		    pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, "Successfully Read a Packet!");
#endif
		    inReadBytes(pktlen);	// only count deserialised packets, because that's what is actually been transfered.
	    }
	    else if (!is_partial_packet)
	    {
#ifdef DEBUG_PQISTREAMER
		    pqioutput(PQL_ALERT, pqistreamerzone, "Failed to handle Packet!");
#endif
		    std::cerr << "Incoming Packet  could not be deserialised:" << std::endl;
		    std::cerr << "  Incoming peer id: " << PeerId() << std::endl;
		    if(pktlen >= 8)
			    std::cerr << "  Packet header   : " << RsUtil::BinToHex((unsigned char*)block,8) << std::endl;
		    if(pktlen >  8)
			    std::cerr << "  Packet data     : " << RsUtil::BinToHex((unsigned char*)block+8,std::min(50u,pktlen-8)) << ((pktlen>58)?"...":"") << std::endl;
	    }

	    mReading_state = reading_state_initial ;	// restart at state 1.
	    mFailed_read_attempts = 0 ;						// reset failed read, as the packet has been totally read.
    }

    if(maxin > readbytes && mBio->moretoread(0))
	    goto start_packet_read ;

#ifdef DEBUG_PQISTREAMER
	if (readbytes > maxin)
		RsDbg() << "PQISTREAMER pqistreamer::handleincoming() stopped reading max reached, readbytes " << std::dec << readbytes << " maxin " << maxin;
	else
		RsDbg() << "PQISTREAMER pqistreamer::handleincoming() stopped reading no more to read, readbytes " << std::dec << readbytes << " maxin " << maxin;
#endif

    return 0;
}

RsItem *pqistreamer::addPartialPacket(const void *block, uint32_t len, uint32_t slice_packet_id, bool is_packet_starting, bool is_packet_ending, uint32_t &total_len) 
{
#ifdef DEBUG_PACKET_SLICING
    std::cerr << "Receiving partial packet. size=" << len << ", ID=" << std::hex << slice_packet_id << std::dec << ", starting:" << is_packet_starting << ", ending:" << is_packet_ending ;
#endif

    if(is_packet_starting && is_packet_ending)
    {
	    std::cerr << " (EE) unexpected situation: both starting and ending" << std::endl;
	    return NULL ;
    }

    uint32_t slice_length = len - PQISTREAM_PARTIAL_PACKET_HEADER_SIZE ;
    unsigned char *slice_data = &((unsigned char*)block)[PQISTREAM_PARTIAL_PACKET_HEADER_SIZE] ;

    std::map<uint32_t,PartialPacketRecord>::iterator it = mPartialPackets.find(slice_packet_id) ;

    if(it == mPartialPackets.end())
    {
	    // make sure we really have a starting packet. Otherwise this is an error.

	    if(!is_packet_starting)
	    {
		    std::cerr << " (EE) non starting packet has no record. Dropping" << std::endl;
		    return NULL ;
	    }
	    PartialPacketRecord& rec = mPartialPackets[slice_packet_id] ;

	    rec.mem = rs_malloc(slice_length) ;

	    if(!rec.mem)
	    {
		    std::cerr << " (EE) Cannot allocate memory for slice of size " << slice_length << std::endl;
		    return NULL ;
	    }

	    memcpy(rec.mem, slice_data, slice_length) ; ;
	    rec.size = slice_length ;

#ifdef DEBUG_PACKET_SLICING
	    std::cerr << " => stored in new record (size=" << rec.size << std::endl;
#endif

	    return NULL ;	// no need to check for ending
    }
    else
    {
	    PartialPacketRecord& rec = it->second ;

	    if(is_packet_starting)
	    {
		    std::cerr << "(WW) dropping unfinished existing packet that gets to be replaced by new starting packet." << std::endl;
		    free(rec.mem);
            		rec.mem = NULL ;
		    rec.size = 0 ;
	    }
	    // make sure this is a continuing packet, otherwise this is an error.

	    rec.mem = realloc(rec.mem, rec.size + slice_length) ;
	    memcpy( &((char*)rec.mem)[rec.size],slice_data,slice_length) ;
	    rec.size += slice_length ;

#ifdef DEBUG_PACKET_SLICING
	    std::cerr << " => added to existing record size=" << rec.size ;
#endif

	    if(is_packet_ending)
	    {
#ifdef DEBUG_PACKET_SLICING
		    std::cerr << " => deserialising: mem=" << RsUtil::BinToHex((char*)rec.mem,std::min(8u,rec.size)) << std::endl;
#endif
		    RsItem *item = mRsSerialiser->deserialise(rec.mem, &rec.size);

		    total_len = rec.size ;
		    free(rec.mem) ;
		    mPartialPackets.erase(it) ;
		    return item ;
	    }
	    else
	    {
#ifdef DEBUG_PACKET_SLICING
		    std::cerr << std::endl;
#endif
		    return NULL ;
	    }
    }
}

/* BandWidth Management Assistance */

float   pqistreamer::outTimeSlice_locked()
{
#ifdef DEBUG_PQISTREAMER
	pqioutput(PQL_DEBUG_ALL, pqistreamerzone, "pqistreamer::outTimeSlice()");
#endif

	//fixme("pqistreamer::outTimeSlice()", 1);
	return 1;
}

int     pqistreamer::outAllowedBytes_locked()
{
	double t = getCurrentTS() ; // in sec, with high accuracy

	// allow a lot if not bandwidthLimited()
	if (!mBio->bandwidthLimited())
	{
		mCurrSent = 0;
		mCurrSentTS = t;
		return PQISTREAM_ABS_MAX;
	}

	// dt is the time elapsed since the last round of sending data
	double dt = t - mCurrSentTS;

	// ignore cases where dt > 1s
	if (dt > 1)
		dt = 1;

	// low pass filter on mAvgDtOut
	mAvgDtOut = PQISTREAM_AVG_DT_FRAC * mAvgDtOut + (1 - PQISTREAM_AVG_DT_FRAC) * dt;
	
	double maxout = getMaxRate_locked(false) * 1024.0;

	// this is used to take into account a possible excess of data sent during the previous round
	mCurrSent -= int(dt * maxout);

	// we dont allow negative value, any quota not used during the previous round is therefore lost
	if (mCurrSent < 0)
		mCurrSent = 0;

	mCurrSentTS = t;

	// now calculate the amount of data allowed to be sent during the next round
	// we take into account the possible excess (but not deficit) of the previous round
	// (this is handled differently when reading data, see below)
	double quota = mAvgDtOut * maxout - mCurrSent;

#ifdef DEBUG_PQISTREAMER
	RsDbg() << "PQISTREAMER pqistreamer::outAllowedBytes_locked() dt " << std::dec << (int)(1000 * dt) << "ms, mAvgDtOut " << (int)(1000 * mAvgDtOut) << "ms, maxout " << (int)(maxout) << " bytes/s, mCurrSent " << mCurrSent << " bytes, quota " << (int)(quota) << " bytes";
#endif

	return quota;
}

int     pqistreamer::inAllowedBytes()
{
	double t = getCurrentTS(); // in sec, with high accuracy

	// allow a lot if not bandwidthLimited()
	if (!mBio->bandwidthLimited())
	{
 		mCurrRead = 0;
		mCurrReadTS = t;
		return PQISTREAM_ABS_MAX;
	}

	// dt is the time elapsed since the last round of receiving data
	double dt = t - mCurrReadTS;

	// limit dt to 1s
	if (dt > 1)
        	dt = 1;

	// low pass filter on mAvgDtIn
	mAvgDtIn = PQISTREAM_AVG_DT_FRAC * mAvgDtIn + (1 - PQISTREAM_AVG_DT_FRAC) * dt;

	double maxin = getMaxRate(true) * 1024.0;

	// this is used to take into account a possible excess/deficit of data received during the previous round
	mCurrRead -= int(dt * maxin);

	// we allow negative value up to the average amount of data received during one round
	// in that case we will use this credit during the next around
	if (mCurrRead < - mAvgDtIn * maxin)
		mCurrRead = - mAvgDtIn * maxin;

	mCurrReadTS = t;

	// we now calculate the max amount of data allowed to be received during the next round
	// we take into account the excess/deficit of the previous round
	double quota = mAvgDtIn * maxin - mCurrRead;

#ifdef DEBUG_PQISTREAMER
	RsDbg() << "PQISTREAMER pqistreamer::inAllowedBytes() dt " << std::dec << (int)(1000 * dt) << "ms, mAvgDtIn " << (int)(1000 * mAvgDtIn) << "ms, maxin " << (int)(maxin) << " bytes/s, mCurrRead " << mCurrRead << " bytes, quota " << (int)(quota) << " bytes";
#endif

	return quota;
}

void    pqistreamer::outSentBytes_locked(uint32_t outb)
{
#ifdef DEBUG_PQISTREAMER
	{
		std::string out;
		rs_sprintf(out, "pqistreamer::outSentBytes(): %d@%gkB/s", outb, RateInterface::getRate(false));
		pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out);
	}
#endif

	/*** One theory for the massive delays - is that the queue here is filling up ****/
//#define DEBUG_LAG	1
#ifdef DEBUG_LAG

#define MIN_PKTS_FOR_MSG	100
	if (out_queue_size() > MIN_PKTS_FOR_MSG)
	{
		std::cerr << "pqistreamer::outSentBytes() for: " << PeerId();
		std::cerr << " End of Write and still " << out_queue_size() << " pkts left";
		std::cerr << std::endl;
	}

#endif
	mTotalSent += outb;
	mCurrSent += outb;
	mAvgSentCount += outb;
	PQInterface::traf_out += outb;
	return;
}

void    pqistreamer::inReadBytes(uint32_t inb)
{
#ifdef DEBUG_PQISTREAMER
	{
		std::string out;
		rs_sprintf(out, "pqistreamer::inReadBytes(): %d@%gkB/s", inb, RateInterface::getRate(true));
		pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out);
	}
#endif

	mTotalRead += inb;
	mCurrRead += inb;
	mAvgReadCount += inb;
	PQInterface::traf_in += inb;
	return;
}

void pqistreamer::allocate_rpend()
{
    if(mPkt_rpending)
        return;

    mPkt_rpend_size = getRsPktMaxSize();
    mPkt_rpending = rs_malloc(mPkt_rpend_size);
    
    if(mPkt_rpending == NULL)
        return ;

    // avoid uninitialized (and random) memory read.
    memset(mPkt_rpending,0,mPkt_rpend_size) ;
}

// clean everything that is half-finished, to avoid causing issues when re-connecting later on.

int pqistreamer::reset()
{
	RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/
#ifdef DEBUG_PQISTREAMER
	std::cerr << "pqistreamer::reset()" << std::endl;
#endif
	free_pend();
    
    return 1 ;
}

void pqistreamer::free_pend()
{
	if(mPkt_rpending)
	{
#ifdef DEBUG_PQISTREAMER
        		std::cerr << "pqistreamer::free_pend(): pending input packet buffer" << std::endl;
#endif
		free(mPkt_rpending);
		mPkt_rpending = 0;
	}
	mPkt_rpend_size = 0;

	if (mPkt_wpending)
	{
#ifdef DEBUG_PQISTREAMER
        		std::cerr << "pqistreamer::free_pend(): pending output packet buffer" << std::endl;
#endif
		free(mPkt_wpending);
		mPkt_wpending = NULL;
	}
	mPkt_wpending_size = 0 ;

#ifdef DEBUG_PQISTREAMER
    if(!mPartialPackets.empty())
        		std::cerr << "pqistreamer::free_pend(): " << mPartialPackets.size() << " pending input partial packets" << std::endl;
#endif
	// also delete any incoming partial packet
	for(std::map<uint32_t,PartialPacketRecord>::iterator it(mPartialPackets.begin());it!=mPartialPackets.end();++it)
		free(it->second.mem) ;

	mPartialPackets.clear() ;
    
	// clean up outgoing. (cntrl packets)
	locked_clear_out_queue() ;
}

int     pqistreamer::gatherStatistics(std::list<RSTrafficClue>& outqueue_lst,std::list<RSTrafficClue>& inqueue_lst)
{
    RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/

    return locked_gatherStatistics(outqueue_lst,inqueue_lst);
}

// this method is overloaded by pqiqosstreamer
int     pqistreamer::getQueueSize(bool in)
{
	if (in)
// no mutex is needed here because this is atomic
		return mIncomingSize;
	else
	{
		RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/
		return locked_out_queue_size();
	}
}

int     pqistreamer::getQueueSize_bytes(bool in)
{
        if (in)
// no mutex is needed here because this is atomic
// for future use, mIncomingSize_bytes is not updated yet
                return mIncomingSize_bytes;
        else
        {
                RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/
                return locked_compute_out_pkt_size();
        }
}

void    pqistreamer::getRates(RsBwRates &rates)
{
	RateInterface::getRates(rates);

// no mutex is needed here because this is atomic
	rates.mQueueIn = mIncomingSize;

	{
		RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/
		rates.mQueueOut = locked_out_queue_size();
	}
}

// this method is overloaded by pqiqosstreamer
int pqistreamer::locked_out_queue_size() const
{
	// Warning: because out_pkt is a list, calling size
	//	is O(n) ! Makign algorithms pretty inefficient. We should record how many
	//	items get stored and discarded to have a proper size value at any time
	//
	return mOutPkts.size() ; 
}

// this method is overloaded by pqiqosstreamer
void pqistreamer::locked_clear_out_queue()
{
	for(std::list<void*>::iterator it = mOutPkts.begin(); it != mOutPkts.end(); )
	{
		free(*it);
		it = mOutPkts.erase(it);
#ifdef DEBUG_PQISTREAMER
		std::string out = "pqistreamer::locked_clear_out_queue() Not active -> Clearing Pkt!";
		std::cerr << out << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, out);
#endif
	}
}

// this method is overloaded by pqiqosstreamer
int pqistreamer::locked_compute_out_pkt_size() const
{
	int total = 0 ;

	for(std::list<void*>::const_iterator it = mOutPkts.begin(); it != mOutPkts.end(); ++it)
		total += getRsItemSize(*it);

	return total ;
}

int pqistreamer::locked_gatherStatistics(std::list<RSTrafficClue>& out_lst,std::list<RSTrafficClue>& in_lst)
{
    out_lst = mPreviousStatsChunk_Out ;
     in_lst = mPreviousStatsChunk_In ;

    return 1 ;
}

// this method is overloaded by pqiqosstreamer
void *pqistreamer::locked_pop_out_data(uint32_t /*max_slice_size*/, uint32_t &size, bool &starts, bool &ends, uint32_t &packet_id)
{
    size = 0 ;
    starts = true ;
    ends = true ;
    packet_id = 0 ;
    
	void *res = NULL ;

	if (!mOutPkts.empty())
	{
		res = *(mOutPkts.begin()); 
		mOutPkts.pop_front();
#ifdef DEBUG_TRANSFERS
		std::cerr << "pqistreamer::locked_pop_out_data() getting next pkt from mOutPkts queue";
		std::cerr << std::endl;
#endif
	}
	return res ;
}

