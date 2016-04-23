/*
 * "$Id: pqistreamer.cc,v 1.19 2007-02-18 21:46:50 rmf24 Exp $"
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
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


#include <iostream>
#include <fstream>
#include <time.h>
#include "util/rsdebug.h"
#include "util/rsstring.h"
#include "util/rsprint.h"
#include "util/rsscopetimer.h"

#include "pqi/pqistreamer.h"
#include "rsserver/p3face.h"

#include "serialiser/rsserial.h" 

const int pqistreamerzone = 8221;

static const int   PQISTREAM_ABS_MAX    			= 100000000; /* 100 MB/sec (actually per loop) */
static const int   PQISTREAM_AVG_PERIOD 			= 5; 		// update speed estimate every 5 seconds
static const float PQISTREAM_AVG_FRAC   			= 0.8; 		// for bandpass filter over speed estimate.
static const int   PQISTREAM_OPTIMAL_PACKET_SIZE  		= 512;		// It is believed that this value should be lower than TCP slices and large enough as compare to encryption padding.
										// most importantly, it should be constant, so as to allow correct QoS.
static const int   PQISTREAM_OPTIMAL_SLICE_OFFSET_UNIT 	= 16 ; 		// slices offset in units of 16 bits. That allows bigger numbers encoded in 4 less bits. 
static const int   PQISTREAM_SLICE_FLAG_ENDS 			= 0x01;		// these flags should be kept in the range 0x01-0x08
static const int   PQISTREAM_SLICE_FLAG_STARTS			= 0x02;		// 
static const int   PQISTREAM_SLICE_PROTOCOL_VERSION_ID		= 0x10;		// Protocol version ID. Should hold on the 4 lower bits.
static const int   PQISTREAM_PARTIAL_PACKET_HEADER_SIZE	= 8;   		// Same size than normal header, to make the code simpler.

/* This removes the print statements (which hammer pqidebug) */
/***
#define RSITEM_DEBUG 1
#define DEBUG_TRANSFERS	 1
#define DEBUG_PQISTREAMER 1
 ***/

#ifdef DEBUG_TRANSFERS
	#include "util/rsprint.h"
#endif


pqistreamer::pqistreamer(RsSerialiser *rss, const RsPeerId& id, BinInterface *bio_in, int bio_flags_in)
	:PQInterface(id), mStreamerMtx("pqistreamer"),
	mBio(bio_in), mBio_flags(bio_flags_in), mRsSerialiser(rss), 
	mPkt_wpending(NULL), mPkt_wpending_size(0),
	mTotalRead(0), mTotalSent(0),
	mCurrRead(0), mCurrSent(0),
	mAvgReadCount(0), mAvgSentCount(0)
{
	RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/

    mAvgLastUpdate = mCurrReadTS = mCurrSentTS = time(NULL);
    mIncomingSize = 0 ;

    mStatisticsTimeStamp = 0 ;
	/* allocated once */
    mPkt_rpend_size = 0;
    mPkt_rpending = 0;
	mReading_state = reading_state_initial ;

	// 100 B/s (minimal)
	setMaxRate(true, 0.1);
	setMaxRate(false, 0.1);
	setRate(true, 0);
	setRate(false, 0);

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

	// clean up outgoing. (cntrl packets)
	locked_clear_out_queue() ;

	if (mPkt_wpending)
	{
		free(mPkt_wpending);
		mPkt_wpending = NULL;
        	mPkt_wpending_size = 0 ;
	}

    free_rpend_locked();

	// clean up incoming.
    while(!mIncoming.empty())
	{
		RsItem *i = mIncoming.front();
        mIncoming.pop_front();
        --mIncomingSize ;
		delete i;
    }

    if(mIncomingSize != 0)
        std::cerr << "(EE) inconsistency after deleting pqistreamer queue. Remaining items: " << mIncomingSize << std::endl;
	return;
}


// Get/Send Items.
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

	RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/

	if(mIncoming.empty())
		return NULL; 

	RsItem *osr = mIncoming.front() ;
    mIncoming.pop_front() ;
    --mIncomingSize;

	return osr;
}

void pqistreamer::updateRates()
{
    // now update rates both ways.

    time_t t = time(NULL); // get current timestep.

    if (t > mAvgLastUpdate + PQISTREAM_AVG_PERIOD)
    {
        int64_t diff = int64_t(t) - int64_t(mAvgLastUpdate) ;

        float avgReadpSec = getRate(true) * PQISTREAM_AVG_FRAC   +     (1.0 - PQISTREAM_AVG_FRAC) * mAvgReadCount/(1000.0 * float(diff));
        float avgSentpSec = getRate(false) * PQISTREAM_AVG_FRAC   +     (1.0 - PQISTREAM_AVG_FRAC) * mAvgSentCount/(1000.0 * float(diff));

#ifdef DEBUG_PQISTREAMER
	    std::cerr << "Peer " << PeerId() << ": Current speed estimates: " << avgReadpSec << " / " << avgSentpSec << std::endl;
#endif
	    /* pretend our rate is zero if we are 
		 * not bandwidthLimited().
		 */
	    if (mBio->bandwidthLimited())
	    {
		    setRate(true, avgReadpSec);
		    setRate(false, avgSentpSec);
	    }
	    else
	    {
		    std::cerr << "Warning: setting to 0" << std::endl;
		    setRate(true, 0);
		    setRate(false, 0);
	    }

	    mAvgLastUpdate = t;
	    mAvgReadCount = 0;
	    mAvgSentCount = 0;
    }
}
 
int 	pqistreamer::tick_bio()
{
	RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/
	mBio->tick();
	
	/* short circuit everything is bio isn't active */
	if (!(mBio->isactive()))
	{
		return 0;
	}
	return 1;
}


int 	pqistreamer::tick_recv(uint32_t timeout)
{
	RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/

	if (mBio->moretoread(timeout))
	{
		handleincoming_locked();
	}
    if(!(mBio->isactive()))
    {
        free_rpend_locked();
    }
	return 1;
}


int 	pqistreamer::tick_send(uint32_t timeout)
{
	RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/

	/* short circuit everything is bio isn't active */
	if (!(mBio->isactive()))
	{
		return 0;
	}

	if (mBio->cansend(timeout))
	{
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

void pqistreamer::locked_storeInOutputQueue(void *ptr,int,int)
{
	mOutPkts.push_back(ptr);
}
//
/**************** HANDLE OUTGOING TRANSLATION + TRANSMISSION ******/

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

int 	pqistreamer::handleincomingitem_locked(RsItem *pqi,int len)
{

#ifdef DEBUG_PQISTREAMER
	pqioutput(PQL_DEBUG_ALL, pqistreamerzone, "pqistreamer::handleincomingitem_locked()");
#endif
	// timestamp last received packet.
	mLastIncomingTs = time(NULL);

	// Use overloaded Contact function 
	pqi -> PeerId(PeerId());
    mIncoming.push_back(pqi);
    ++mIncomingSize ;

            /*******************************************************************************************/
    	// keep info for stats for a while. Only keep the items for the last two seconds. sec n is ongoing and second n-1
    	// is a full statistics chunk that can be used in the GUI

    	locked_addTrafficClue(pqi,len,mCurrentStatsChunk_In) ;

        /*******************************************************************************************/

	return 1;
}

void pqistreamer::locked_addTrafficClue(const RsItem *pqi,uint32_t pktsize,std::list<RSTrafficClue>& lst)
{
    time_t now = time(NULL) ;

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

time_t	pqistreamer::getLastIncomingTS()
{
	RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/

	return mLastIncomingTs;
}

// Packet slicing:
//
//          Old :    02 0014 03 00000026  [data,  26 bytes] =>   [version 1B] [service 2B][subpacket 1B] [size 4B]
//          New1:    fv 0014 03 xxxxx sss [data, sss bytes] =>   [flags 0.5B version 0.5B] [service 2B][subpacket 1B] [packet counter 2.5B size 1.5B]
//          New2:    fx xxxxxx oooo ssss  [data, sss bytes] =>   [flags 0.5B] [2^28 packet count] [2^16 offset (in units of 16)] [size 2^16] 
//
//          Flags:  0x1 => incomplete packet continued after
//          Flags:  0x2 => packet ending a previously incomplete packet
//
//	- backward compatibility:
//		* send one packet with service + subpacket = ffffff. Old peers will silently ignore such packets.
//		* if received, mark the peer as able to decode the new packet type
//
//      Mode 1:
//		- Encode length    on 1.5 Bytes (10 bits) => max slice size = 1024
//		- Encode packet ID on 2.5 Bytes (20 bits) => packet counter = [0...1056364]
//      Mode 2:
//           	- Encode flags     on 0.5 Bytes ( 4 bits)
//		- Encode packet ID on 3.5 Bytes (28 bits) => packet counter = [0...16777216]
//		- Encode offset    on 2.0 Bytes (16 bits) => 65536 * 16 = 				// ax packet size = 1048576
//		- Encode size      on 2.0 Bytes (16 bits) => 65536					// max slice size = 65536
//
//      - limit packet grouping to max size 1024.
//      - new peers need to read flux, and properly extract partial sizes, and combine packets based on packet counter.
//	- on sending, RS should grab slices of max size 1024 from pqiQoS. If smaller, possibly pack them together.
//	  pqiQoS keeps track of sliced packets and makes sure the output is consistent:
//		* when a large packet needs to be send, only takes a slice and return it, and update the remaining part
//		* always consider priority when taking new slices => a newly arrived fast packet will always get through.
//
//	Max slice size should be customisable, depending on bandwidth.

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

//#ifdef DEBUG_PQISTREAMER
		    if (maxbytes < sentbytes)
			    std::cerr << "pqistreamer::handleoutgoing_locked() Stopped sending: bio not ready. maxbytes=" << maxbytes << ", sentbytes=" << sentbytes << std::endl;
		    else
			    std::cerr << "pqistreamer::handleoutgoing_locked() Stopped sending: sentbytes=" << sentbytes << ", max=" << maxbytes << std::endl;
//#endif

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

		uint32_t slice_offset =0 ;
        	uint32_t slice_size=0;
		bool slice_starts=true ;
		bool slice_ends=true ;
		uint32_t slice_packet_id=0 ;

		do
		{
			dta = locked_pop_out_data(PQISTREAM_OPTIMAL_PACKET_SIZE,PQISTREAM_OPTIMAL_SLICE_OFFSET_UNIT,slice_offset,slice_size,slice_starts,slice_ends,slice_packet_id) ;

			if(!dta)
				break ;

                            if(slice_size > 0xffff)
                            {
                                std::cerr << "(EE) protocol error in pqitreamer: slice size is too large and cannot be encoded." ;
                                free(mPkt_wpending) ;
                                mPkt_wpending_size = 0;
                            }
                
			    if(slice_offset > 0xfffff || (slice_offset & 0xff)!=0)	// 5 f, on purpose. Not a bug.
                            {
                                std::cerr << "(EE) protocol error in pqitreamer: slice size is too large and cannot be encoded." ;
                                free(mPkt_wpending) ;
                                mPkt_wpending_size = 0;
                            }
                
            		if(slice_starts && slice_ends)	// good old method. Send the packet as is, since it's a full packet.
		    	{
			    mPkt_wpending = realloc(mPkt_wpending,slice_size+mPkt_wpending_size) ;
			    memcpy( &((char*)mPkt_wpending)[mPkt_wpending_size],dta,slice_size) ;
			    free(dta);
			    mPkt_wpending_size += slice_size ;
			    ++k ;
		    	}
                    	else	// partial packet. We make a special header for it and insert it in the stream
                    	{
			    mPkt_wpending = realloc(mPkt_wpending,slice_size+mPkt_wpending_size+PQISTREAM_PARTIAL_PACKET_HEADER_SIZE) ;
			    memcpy( &((char*)mPkt_wpending)[mPkt_wpending_size+PQISTREAM_PARTIAL_PACKET_HEADER_SIZE],dta,slice_size) ;
			    free(dta);
                
			// New2: fp xxxxxx oooo ssss  [data, sss bytes] =>   [flags 0.5B] [protocol version 0.5B] [2^24 packet count] [2^16 offset (in units of 16)] [size 2^16] 
                
                		uint8_t partial_flags = PQISTREAM_SLICE_PROTOCOL_VERSION_ID ;	// includes version. Flags are in the first half-byte
                        	if(slice_starts) partial_flags |= PQISTREAM_SLICE_FLAG_STARTS  ;
                        	if(slice_ends  ) partial_flags |= PQISTREAM_SLICE_FLAG_ENDS  ;
                            
			    ((char*)mPkt_wpending)[mPkt_wpending_size+0x00] = partial_flags ;
			    ((char*)mPkt_wpending)[mPkt_wpending_size+0x01] = uint8_t(slice_packet_id >> 16) & 0xff ;
			    ((char*)mPkt_wpending)[mPkt_wpending_size+0x02] = uint8_t(slice_packet_id >>  8) & 0xff ;
			    ((char*)mPkt_wpending)[mPkt_wpending_size+0x03] = uint8_t(slice_packet_id >>  0) & 0xff ;
			    ((char*)mPkt_wpending)[mPkt_wpending_size+0x04] = uint8_t(slice_offset    >> 12) & 0xff ;
			    ((char*)mPkt_wpending)[mPkt_wpending_size+0x05] = uint8_t(slice_offset    >>  4) & 0xff ;	// not a bug. The last 4 bits are discarded because they are always 0
			    ((char*)mPkt_wpending)[mPkt_wpending_size+0x06] = uint8_t(slice_size      >>  8) & 0xff ;
			    ((char*)mPkt_wpending)[mPkt_wpending_size+0x07] = uint8_t(slice_size      >>  0) & 0xff ;
                            
			    mPkt_wpending_size += slice_size + PQISTREAM_PARTIAL_PACKET_HEADER_SIZE;
			    ++k ;
                    	}
		} 
       		 while(mPkt_wpending_size < (uint32_t)maxbytes && mPkt_wpending_size < PQISTREAM_OPTIMAL_PACKET_SIZE ) ;
             
#ifdef DEBUG_PQISTREAMER
		if(k > 1)
			std::cerr << "Packed " << k << " packets into " << mPkt_wpending_size << " bytes." << std::endl;
#endif
	}
        
	    if (mPkt_wpending)
	    {
            	RsScopeTimer tmer("pqistreamer:"+PeerId().toStdString()) ;
                
		    // write packet.
#ifdef DEBUG_PQISTREAMER
		    std::cout << "Sending Out Pkt of size " << mPkt_wpending_size << " !" << std::endl;
#endif
            		int ss=0;

		    if (mPkt_wpending_size != (ss = mBio->senddata(mPkt_wpending, mPkt_wpending_size)))
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
            else
                		std::cerr << PeerId() << ": sent " << ss << " bytes " << std::endl;
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
int pqistreamer::handleincoming_locked()
{
	int readbytes = 0;
	static const int max_failed_read_attempts = 2000 ;

#ifdef DEBUG_PQISTREAMER
	pqioutput(PQL_DEBUG_ALL, pqistreamerzone, "pqistreamer::handleincoming_locked()");
#endif

	if(!(mBio->isactive()))
	{
		mReading_state = reading_state_initial ;
        free_rpend_locked();
		return 0;
	}
    else
        allocate_rpend_locked();

	// enough space to read any packet.
	int maxlen = mPkt_rpend_size; 
	void *block = mPkt_rpending; 

	// initial read size: basic packet.
	int blen = getRsPktBaseSize();	// this is valid for both packet slices and normal un-sliced packets (same header size)

	int maxin = inAllowedBytes_locked();

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
		std::cerr << "[" << (void*)pthread_self() << "] " << "block 0 : " << (int)(((unsigned char*)block)[0]) << " " << (int)(((unsigned char*)block)[1]) << " " << (int)(((unsigned char*)block)[2]) << " "
							<< (int)(((unsigned char*)block)[3]) << " "
							<< (int)(((unsigned char*)block)[4]) << " "
							<< (int)(((unsigned char*)block)[5]) << " "
							<< (int)(((unsigned char*)block)[6]) << " "
							<< (int)(((unsigned char*)block)[7]) << " " << std::endl ;
#endif

		readbytes += blen;
        mReading_state = reading_state_packet_started ;
		mFailed_read_attempts = 0 ;						// reset failed read, as the packet has been totally read.
	}
continue_packet:
	{
		// workout how much more to read.
        
        	bool is_partial_packet = false ;
            
		int extralen =0;
        	int slice_offset = 0 ;
            	int slice_packet_id =0;
        
        	if( ((char*)block)[0] == 0x10 || ((char*)block)[0] == 0x11 || ((char*)block)[0] == 0x12)
            	{
                   extralen        = (int(((char*)block)[6]) << 8) + (int(((char*)block)[7]));
                   slice_offset    = (int(((char*)block)[5]) << 4) + (int(((char*)block)[4]) << 12);
                   slice_packet_id = (int(((char*)block)[3]) << 0) + (int(((char*)block)[2]) << 8) + (int(((char*)block)[1]) << 16);
                   
                   is_partial_packet = true ;
            	}
	        else
                   extralen = getRsItemSize(block) - blen;

#ifdef DEBUG_PQISTREAMER
                std::cerr << "[" << (void*)pthread_self() << "] " << "continuing packet getRsItemSize(block) = " << getRsItemSize(block) << std::endl ;
                std::cerr << "[" << (void*)pthread_self() << "] " << "continuing packet extralen = " << extralen << std::endl ;

				std::cerr << "[" << (void*)pthread_self() << "] " << "continuing packet state=" << mReading_state << std::endl ;
		std::cerr << "[" << (void*)pthread_self() << "] " << "block 1 : " << (int)(((unsigned char*)block)[0]) << " " << (int)(((unsigned char*)block)[1]) << " " << (int)(((unsigned char*)block)[2]) << " " << (int)(((unsigned char*)block)[3])  << " "
							<< (int)(((unsigned char*)block)[4]) << " "
							<< (int)(((unsigned char*)block)[5]) << " "
							<< (int)(((unsigned char*)block)[6]) << " "
							<< (int)(((unsigned char*)block)[7]) << " " << std::endl ;
#endif
		if (extralen > maxlen - blen)
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
				rs_sprintf_append(msg, "block = %d %d %d %d %d %d %d %d\n",
							(int)(((unsigned char*)block)[0]),
							(int)(((unsigned char*)block)[1]),
							(int)(((unsigned char*)block)[2]),
							(int)(((unsigned char*)block)[3]),
							(int)(((unsigned char*)block)[4]),
							(int)(((unsigned char*)block)[5]),
							(int)(((unsigned char*)block)[6]),
							(int)(((unsigned char*)block)[7])) ;
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

			if (extralen != (tmplen = mBio->readdata(extradata, extralen)))
			{
#ifdef DEBUG_PQISTREAMER
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

						rs_sprintf_append(msgout, "block = %d %d %d %d %d %d %d %d\n",
							(int)(((unsigned char*)block)[0]),
							(int)(((unsigned char*)block)[1]),
							(int)(((unsigned char*)block)[2]),
							(int)(((unsigned char*)block)[3]),
							(int)(((unsigned char*)block)[4]),
							(int)(((unsigned char*)block)[5]),
							(int)(((unsigned char*)block)[6]),
							(int)(((unsigned char*)block)[7]));

						//notify->AddSysMessage(0, RS_SYS_WARNING, title, msgout.str());

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
#ifdef DEBUG_PQISTREAMER
		std::cerr << "[" << (void*)pthread_self() << "] " << "continuing packet state=" << mReading_state << std::endl ;
		std::cerr << "[" << (void*)pthread_self() << "] " << "block 2 : " << (int)(((unsigned char*)extradata)[0]) << " " << (int)(((unsigned char*)extradata)[1]) << " " << (int)(((unsigned char*)extradata)[2]) << " " << (int)(((unsigned char*)extradata)[3])  << " "
							<< (int)(((unsigned char*)extradata)[4]) << " "
							<< (int)(((unsigned char*)extradata)[5]) << " "
							<< (int)(((unsigned char*)extradata)[6]) << " "
							<< (int)(((unsigned char*)extradata)[7]) << " " << std::endl ;
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

                //		std::cerr << "Deserializing packet of size " << pktlen <<std::endl ;

		uint32_t pktlen = blen+extralen ;
#ifdef DEBUG_PQISTREAMER
		std::cerr << "[" << (void*)pthread_self() << "] " << "deserializing. Size=" << pktlen << std::endl ;
#endif
        RsItem *pkt ;
        
        if(is_partial_packet)
        {
        	std::cerr << "Inputing partial packet " << RsUtil::BinToHex((char*)block,8) << std::endl;
                         
		pkt = addPartialPacket(block,extralen,slice_offset,slice_packet_id) ;
        }
        else
		pkt = mRsSerialiser->deserialise(block, &pktlen);
            

        std::cerr << "Got packet with header " << RsUtil::BinToHex((char*)block,8) << std::endl;

		if ((pkt != NULL) && (0  < handleincomingitem_locked(pkt,pktlen)))
		{
#ifdef DEBUG_PQISTREAMER
			pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, "Successfully Read a Packet!");
#endif
			inReadBytes_locked(pktlen);	// only count deserialised packets, because that's what is actually been transfered.
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

#ifdef DEBUG_TRANSFERS
	if (readbytes >= maxin)
	{
		std::cerr << "pqistreamer::handleincoming() Stopped reading as readbytes >= maxin. Read " << readbytes << " bytes ";
		std::cerr << std::endl;
	}
#endif

	return 0;
}

RsItem *pqistreamer::addPartialPacket(const void *block,uint32_t len,uint32_t slice_offset,uint32_t slice_packet_id) 
{
    std::map<uint32_t,PartialPacketRecord>::iterator it = mPartialPackets.find(slice_packet_id) ;
    
    if(it == mPartialPackets.end())
    {
        // make sure we really have  starting packet. Otherwise this is an error.
    }
    else
    {
        // make sure this is a continuing packet, otherwise this is an error.
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

// very simple..... 
int     pqistreamer::outAllowedBytes_locked()
{
	int t = time(NULL); // get current timestep.

	/* allow a lot if not bandwidthLimited */
	if (!mBio->bandwidthLimited())
	{
		mCurrSent = 0;
		mCurrSentTS = t;
		return PQISTREAM_ABS_MAX;
	}

	int dt = t - mCurrSentTS;
	// limiter -> for when currSentTs -> 0.
	if (dt > 5)
		dt = 5;

	int maxout = (int) (getMaxRate(false) * 1000.0);
	mCurrSent -= dt * maxout;
	if (mCurrSent < 0)
	{
		mCurrSent = 0;
	}

	mCurrSentTS = t;

#ifdef DEBUG_PQISTREAMER
	{
		std::string out;
		rs_sprintf(out, "pqistreamer::outAllowedBytes() is %d/%d", maxout - mCurrSent, maxout);
		pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out);
	}
#endif


	return maxout - mCurrSent;
}

int     pqistreamer::inAllowedBytes_locked()
{
	int t = time(NULL); // get current timestep.

	/* allow a lot if not bandwidthLimited */
	if (!mBio->bandwidthLimited())
	{
		mCurrReadTS = t;
		mCurrRead = 0;
		return PQISTREAM_ABS_MAX;
	}

	int dt = t - mCurrReadTS;
	// limiter -> for when currReadTs -> 0.
	if (dt > 5)
		dt = 5;

	int maxin = (int) (getMaxRate(true) * 1000.0);
	mCurrRead -= dt * maxin;
	if (mCurrRead < 0)
	{
		mCurrRead = 0;
	}

	mCurrReadTS = t;

#ifdef DEBUG_PQISTREAMER
	{
		std::string out;
		rs_sprintf(out, "pqistreamer::inAllowedBytes() is %d/%d", maxin - mCurrRead, maxin);
		pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out);
	}
#endif


	return maxin - mCurrRead;
}


void    pqistreamer::outSentBytes_locked(uint32_t outb)
{
#ifdef DEBUG_PQISTREAMER
	{
		std::string out;
		rs_sprintf(out, "pqistreamer::outSentBytes(): %d@%gkB/s", outb, getRate(false));
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

	return;
}

void    pqistreamer::inReadBytes_locked(uint32_t inb)
{
#ifdef DEBUG_PQISTREAMER
	{
		std::string out;
		rs_sprintf(out, "pqistreamer::inReadBytes(): %d@%gkB/s", inb, getRate(true));
		pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out);
	}
#endif

	mTotalRead += inb;
	mCurrRead += inb;
	mAvgReadCount += inb;

	return;
}

void pqistreamer::allocate_rpend_locked()
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

void pqistreamer::free_rpend_locked()
{
    if(!mPkt_rpending)
        return;

    free(mPkt_rpending);
    mPkt_rpending = 0;
    mPkt_rpend_size = 0;
}

int     pqistreamer::gatherStatistics(std::list<RSTrafficClue>& outqueue_lst,std::list<RSTrafficClue>& inqueue_lst)
{
    RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/

    return locked_gatherStatistics(outqueue_lst,inqueue_lst);
}
int     pqistreamer::getQueueSize(bool in)
{
	RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/

	if (in)
        return mIncomingSize;
    else
        return locked_out_queue_size();
}

void    pqistreamer::getRates(RsBwRates &rates)
{
	RateInterface::getRates(rates);

	RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/

    rates.mQueueIn = mIncomingSize;
	rates.mQueueOut = locked_out_queue_size();
}

int pqistreamer::locked_out_queue_size() const
{
	// Warning: because out_pkt is a list, calling size
	//	is O(n) ! Makign algorithms pretty inefficient. We should record how many
	//	items get stored and discarded to have a proper size value at any time
	//
	return mOutPkts.size() ; 
}

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

void *pqistreamer::locked_pop_out_data(uint32_t max_slice_size,uint32_t offset_unit,uint32_t& offset,uint32_t& size,bool& starts,bool& ends,uint32_t& packet_id)
{
    offset = 0 ;
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

    
