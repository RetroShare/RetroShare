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

#include "pqi/pqistreamer.h"
#include "rsserver/p3face.h"

#include "serialiser/rsserial.h" 

const int pqistreamerzone = 8221;

const int PQISTREAM_ABS_MAX = 100000000; /* 100 MB/sec (actually per loop) */

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
	mPkt_wpending(NULL), 
	mTotalRead(0), mTotalSent(0),
	mCurrRead(0), mCurrSent(0),
	mAvgReadCount(0), mAvgSentCount(0)
{
	RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/

    mAvgLastUpdate = mCurrReadTS = mCurrSentTS = time(NULL);
    mIncomingSize = 0 ;

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

// // PQInterface
int	pqistreamer::tick()
{

#ifdef DEBUG_PQISTREAMER
	{
		RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/
		std::string out = "pqistreamer::tick()\n" + PeerId();
		rs_sprintf_append(out, ": currRead/Sent: %d/%d", mCurrRead, mCurrSent);

		pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out);
	}
#endif

	if (!tick_bio())
	{
		return 0;
	}

	tick_recv(0);
	tick_send(0);

	RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/
#ifdef DEBUG_PQISTREAMER
	/* give details of the packets */
	{
		std::list<void *>::iterator it;

		std::string out = "pqistreamer::tick() Queued Data: for " + PeerId();

		if (mBio->isactive())
		{
			out += " (active)";
		}
		else
		{
			out += " (waiting)";
		}
		out += "\n";

		{
			int total = locked_compute_out_pkt_size() ;

			rs_sprintf_append(out, "\t Out Packets [%d] => %d bytes\n", locked_out_queue_size(), total);
            rs_sprintf_append(out, "\t Incoming    [%d]\n", mIncomingSize);
		}

		pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, out);
	}
#endif

	/* if there is more stuff in the queues */
    if ((!mIncoming.empty()) || (locked_out_queue_size() > 0))
	{
		return 1;
	}
	return 0;
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

void pqistreamer::locked_storeInOutputQueue(void *ptr,int)
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
	void *ptr = malloc(pktsize);

#ifdef DEBUG_PQISTREAMER
	std::cerr << "pqistreamer::queue_outpqi() serializing packet with packet size : " << pktsize << std::endl;
#endif
	if (mRsSerialiser->serialise(pqi, ptr, &pktsize))
	{
		locked_storeInOutputQueue(ptr,pqi->priority_level()) ;

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

int 	pqistreamer::handleincomingitem_locked(RsItem *pqi)
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
	return 1;
}

time_t	pqistreamer::getLastIncomingTS()
{
	RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/

	return mLastIncomingTs;
}

int	pqistreamer::handleoutgoing_locked()
{
#ifdef DEBUG_PQISTREAMER
	pqioutput(PQL_DEBUG_ALL, pqistreamerzone, "pqistreamer::handleoutgoing_locked()");
#endif

	int maxbytes = outAllowedBytes_locked();
	int sentbytes = 0;
	int len;
	int ss;
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
		}

		outSentBytes_locked(sentbytes);
		return 0;
	}

	// a very simple round robin

	bool sent = true;
	while(sent) // catch if all items sent.
	{
		sent = false;

		if ((!(mBio->cansend(0))) || (maxbytes < sentbytes))
		{

#ifdef DEBUG_TRANSFERS
			if (maxbytes < sentbytes)
			{
				std::cerr << "pqistreamer::handleoutgoing_locked() Stopped sending sentbytes > maxbytes. Sent " << sentbytes << " bytes ";
				std::cerr << std::endl;
			}
			else
			{
				std::cerr << "pqistreamer::handleoutgoing_locked() Stopped sending at cansend() is false";
				std::cerr << std::endl;
			}
#endif

			outSentBytes_locked(sentbytes);
			return 0;
		}

		// send a out_pkt., else send out_data. unless
		// there is a pending packet.
		if (!mPkt_wpending)
			mPkt_wpending = locked_pop_out_data() ;

		if (mPkt_wpending)
		{
			// write packet.
			len = getRsItemSize(mPkt_wpending);

#ifdef DEBUG_PQISTREAMER
                        std::cout << "Sending Out Pkt of size " << len << " !" << std::endl;
#endif

			if (len != (ss = mBio->senddata(mPkt_wpending, len)))
			{
#ifdef DEBUG_PQISTREAMER
				std::string out;
				rs_sprintf(out, "Problems with Send Data! (only %d bytes sent, total pkt size=%d)", ss, len);
//				std::cerr << out << std::endl ;
				pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, out);
#endif

				outSentBytes_locked(sentbytes);
				// pkt_wpending will kept til next time.
				// ensuring exactly the same data is written (openSSL requirement).
				return -1;
			}

#ifdef DEBUG_TRANSFERS
			std::cerr << "pqistreamer::handleoutgoing_locked() Sent Packet len: " << len << " @ " << RsUtil::AccurateTimeString();
			std::cerr << std::endl;
#endif

			free(mPkt_wpending);
			mPkt_wpending = NULL;

			sentbytes += len;
			sent = true;
		}
	}
	outSentBytes_locked(sentbytes);
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
		inReadBytes_locked(readbytes);
        free_rpend_locked();
		return 0;
	}
    else
        allocate_rpend_locked();

	// enough space to read any packet.
	int maxlen = mPkt_rpend_size; 
	void *block = mPkt_rpending; 

	// initial read size: basic packet.
	int blen = getRsPktBaseSize();

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

			inReadBytes_locked(readbytes);

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
		int extralen = getRsItemSize(block) - blen;

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

//		if(pktlen == 17306)
//		{
//			FILE *f = RsDirUtil::rs_fopen("dbug.packet.bin","w");
//			fwrite(block,pktlen,1,f) ;
//			fclose(f) ;
//			exit(-1) ;
//		}
		RsItem *pkt = mRsSerialiser->deserialise(block, &pktlen);

		if ((pkt != NULL) && (0  < handleincomingitem_locked(pkt)))
		{
#ifdef DEBUG_PQISTREAMER
			pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, "Successfully Read a Packet!");
#endif
		}
		else
		{
#ifdef DEBUG_PQISTREAMER
			pqioutput(PQL_ALERT, pqistreamerzone, "Failed to handle Packet!");
#endif
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

	inReadBytes_locked(readbytes);
	return 0;
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


static const float AVG_PERIOD = 5; // sec
static const float AVG_FRAC = 0.8; // for low pass filter.

void    pqistreamer::outSentBytes_locked(int outb)
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

	int t = time(NULL); // get current timestep.
	if (t - mAvgLastUpdate > AVG_PERIOD)
	{
		float avgReadpSec = getRate(true);
		float avgSentpSec = getRate(false);

		avgReadpSec *= AVG_FRAC;
		avgReadpSec += (1.0 - AVG_FRAC) * mAvgReadCount / 
				(1000.0 * (t - mAvgLastUpdate));

		avgSentpSec *= AVG_FRAC;
		avgSentpSec += (1.0 - AVG_FRAC) * mAvgSentCount / 
				(1000.0 * (t - mAvgLastUpdate));

	
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
			setRate(true, 0);
			setRate(false, 0);
		}


		mAvgLastUpdate = t;
		mAvgReadCount = 0;
		mAvgSentCount = 0;
	}
	return;
}

void    pqistreamer::inReadBytes_locked(int inb)
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
    mPkt_rpending = malloc(mPkt_rpend_size);

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

int     pqistreamer::gatherOutQueueStatistics(std::vector<uint32_t>& per_service_count,std::vector<uint32_t>& per_priority_count)
{
    RsStackMutex stack(mStreamerMtx); /**** LOCKED MUTEX ****/

    return locked_gatherStatistics(per_service_count,per_priority_count);
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

int pqistreamer::locked_gatherStatistics(std::vector<uint32_t>&,std::vector<uint32_t>&) const
{
    std::cerr << "(II) called overloaded function locked_gatherStatistics(). This is probably an error" << std::endl;
    return 1 ;
}

void *pqistreamer::locked_pop_out_data() 
{
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
