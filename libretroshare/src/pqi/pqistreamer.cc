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
#include "util/rsdebug.h"
#include "util/rsstring.h"

#include "pqi/pqistreamer.h"
#include "pqi/pqinotify.h"

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


pqistreamer::pqistreamer(RsSerialiser *rss, std::string id, BinInterface *bio_in, int bio_flags_in)
	:PQInterface(id), rsSerialiser(rss), bio(bio_in), bio_flags(bio_flags_in), 
	pkt_wpending(NULL), 
	totalRead(0), totalSent(0),
	currRead(0), currSent(0),
	avgReadCount(0), avgSentCount(0), streamerMtx("pqistreamer")
{
	avgLastUpdate = currReadTS = currSentTS = time(NULL);

	/* allocated once */
	pkt_rpend_size = getRsPktMaxSize();
	pkt_rpending = malloc(pkt_rpend_size);
	reading_state = reading_state_initial ;

//	thread_id = pthread_self() ;
	// avoid uninitialized (and random) memory read.
	memset(pkt_rpending,0,pkt_rpend_size) ;

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

	failed_read_attempts = 0 ;						// reset failed read, as no packet is still read.

	return;
}

pqistreamer::~pqistreamer()
{
	RsStackMutex stack(streamerMtx) ;		// lock out_pkt and out_data

	pqioutput(PQL_DEBUG_ALL, pqistreamerzone, "pqistreamer::~pqistreamer() Destruction!");

	if (bio_flags & BIN_FLAGS_NO_CLOSE)
	{
		pqioutput(PQL_DEBUG_ALL, pqistreamerzone, "pqistreamer::~pqistreamer() Not Closing BinInterface!");
	}
	else if (bio)
	{
		pqioutput(PQL_DEBUG_ALL, pqistreamerzone, "pqistreamer::~pqistreamer() Deleting BinInterface!");

		delete bio;
	}

	/* clean up serialiser */
	if (rsSerialiser)
		delete rsSerialiser;

	// clean up outgoing. (cntrl packets)
	locked_clear_out_queue() ;

	if (pkt_wpending)
	{
		free(pkt_wpending);
		pkt_wpending = NULL;
	}

	free(pkt_rpending);

	// clean up incoming.
	while(incoming.size() > 0)
	{
		RsItem *i = incoming.front();
		incoming.pop_front();
		delete i;
	}
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

	return queue_outpqi(si,out_size);
}

RsItem *pqistreamer::GetItem()
{
#ifdef DEBUG_PQISTREAMER
	pqioutput(PQL_DEBUG_ALL, pqistreamerzone, "pqistreamer::GetItem()");
#endif


	if(incoming.empty())
		return NULL; 

	RsItem *osr = incoming.front() ;
	incoming.pop_front() ;

	return osr;
}

// // PQInterface
int	pqistreamer::tick()
{
#ifdef DEBUG_PQISTREAMER
	{
		std::string out = "pqistreamer::tick()\n" + PeerId();
		rs_sprintf_append(out, ": currRead/Sent: %d/%d", currRead, currSent);

		pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out);
	}
#endif

	bio->tick();

	/* short circuit everything is bio isn't active */
	if (!(bio->isactive()))
	{
		return 0;
	}


	/* must do both, as outgoing will catch some bad sockets, 
	 * that incoming will not 
	 */

	handleincoming();
	handleoutgoing();

#ifdef DEBUG_PQISTREAMER
	/* give details of the packets */
	{
		std::list<void *>::iterator it;

		std::string out = "pqistreamer::tick() Queued Data: for " + PeerId();

		if (bio->isactive())
		{
			out += " (active)";
		}
		else
		{
			out += " (waiting)";
		}
		out += "\n";

		{
			RsStackMutex stack(streamerMtx) ;		// lock out_pkt and out_data
			int total = compute_out_pkt_size() ;

			rs_sprintf_append(out, "\t Out Packets [%d] => %d bytes\n", out_queue_size(), total);
			rs_sprintf_append(out, "\t Incoming    [%d]\n", incoming.size());
		}

		pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, out);
	}
#endif

	/* if there is more stuff in the queues */
	if ((incoming.size() > 0) || (out_queue_size() > 0))
	{
		return 1;
	}
	return 0;
}

int	pqistreamer::status()
{
#ifdef DEBUG_PQISTREAMER
	pqioutput(PQL_DEBUG_ALL, pqistreamerzone, "pqistreamer::status()");

	if (bio->isactive())
	{
		std::string out;
		rs_sprintf(out, "Data in:%d out:%d", totalRead, totalSent);
		pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, out);
	}
#endif

	return 0;
}

void pqistreamer::locked_storeInOutputQueue(void *ptr,int)
{
	out_pkt.push_back(ptr);
}
//
/**************** HANDLE OUTGOING TRANSLATION + TRANSMISSION ******/

int	pqistreamer::queue_outpqi(RsItem *pqi,uint32_t& pktsize)
{
	pktsize = 0 ;
#ifdef DEBUG_PQISTREAMER
        std::cerr << "pqistreamer::queue_outpqi() called." << std::endl;
#endif
        RsStackMutex stack(streamerMtx) ;		// lock out_pkt and out_data

	// This is called by different threads, and by threads that are not the handleoutgoing thread,
	// so it should be protected by a mutex !!
	
#ifdef DEBUG_PQISTREAMER
	if(dynamic_cast<RsFileData*>(pqi)!=NULL && (bio_flags & BIN_FLAGS_NO_DELETE))
	{
		std::cerr << "Having file data with flags = " << bio_flags << std::endl ;
	}

	pqioutput(PQL_DEBUG_ALL, pqistreamerzone, "pqistreamer::queue_outpqi()");
#endif

	/* decide which type of packet it is */

	pktsize = rsSerialiser->size(pqi);
	void *ptr = malloc(pktsize);

#ifdef DEBUG_PQISTREAMER
	std::cerr << "pqistreamer::queue_outpqi() serializing packet with packet size : " << pktsize << std::endl;
#endif
	if (rsSerialiser->serialise(pqi, ptr, &pktsize))
	{
		locked_storeInOutputQueue(ptr,pqi->priority_level()) ;

		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
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

	if (!(bio_flags & BIN_FLAGS_NO_DELETE))
	{
		delete pqi;
	}
	return 1; // keep error internal.
}

int 	pqistreamer::handleincomingitem(RsItem *pqi)
{
#ifdef DEBUG_PQISTREAMER
	pqioutput(PQL_DEBUG_ALL, pqistreamerzone, "pqistreamer::handleincomingitem()");
#endif
	// timestamp last received packet.
	mLastIncomingTs = time(NULL);

	// Use overloaded Contact function 
	pqi -> PeerId(PeerId());
	incoming.push_back(pqi);
	return 1;
}

time_t	pqistreamer::getLastIncomingTS()
{
	return mLastIncomingTs;
}

int	pqistreamer::handleoutgoing()
{
	RsStackMutex stack(streamerMtx) ;		// lock out_pkt and out_data

#ifdef DEBUG_PQISTREAMER
	pqioutput(PQL_DEBUG_ALL, pqistreamerzone, "pqistreamer::handleoutgoing()");
#endif

	int maxbytes = outAllowedBytes();
	int sentbytes = 0;
	int len;
	int ss;
	//	std::cerr << "pqistreamer: maxbytes=" << maxbytes<< std::endl ; 

	std::list<void *>::iterator it;

	// if not connection, or cannot send anything... pause.
	if (!(bio->isactive()))
	{
		/* if we are not active - clear anything in the queues. */
		locked_clear_out_queue() ;

		/* also remove the pending packets */
		if (pkt_wpending)
		{
			free(pkt_wpending);
			pkt_wpending = NULL;
		}

		outSentBytes(sentbytes);
		return 0;
	}

	// a very simple round robin

	bool sent = true;
	while(sent) // catch if all items sent.
	{
		sent = false;

		if ((!(bio->cansend())) || (maxbytes < sentbytes))
		{

#ifdef DEBUG_TRANSFERS
			if (maxbytes < sentbytes)
			{
				std::cerr << "pqistreamer::handleoutgoing() Stopped sending sentbytes > maxbytes. Sent " << sentbytes << " bytes ";
				std::cerr << std::endl;
			}
			else
			{
				std::cerr << "pqistreamer::handleoutgoing() Stopped sending at cansend() is false";
				std::cerr << std::endl;
			}
#endif

			outSentBytes(sentbytes);
			return 0;
		}

		// send a out_pkt., else send out_data. unless
		// there is a pending packet.
		if (!pkt_wpending)
			pkt_wpending = locked_pop_out_data() ;

		if (pkt_wpending)
		{
			// write packet.
			len = getRsItemSize(pkt_wpending);

#ifdef DEBUG_PQISTREAMER
                        std::cout << "Sending Out Pkt of size " << len << " !" << std::endl;
#endif

			if (len != (ss = bio->senddata(pkt_wpending, len)))
			{
#ifdef DEBUG_PQISTREAMER
				std::string out;
				rs_sprintf(out, "Problems with Send Data! (only %d bytes sent, total pkt size=%d)", ss, len);
//				std::cerr << out << std::endl ;
				pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, out);
#endif

				outSentBytes(sentbytes);
				// pkt_wpending will kept til next time.
				// ensuring exactly the same data is written (openSSL requirement).
				return -1;
			}

#ifdef DEBUG_TRANSFERS
			std::cerr << "pqistreamer::handleoutgoing() Sent Packet len: " << len << " @ " << RsUtil::AccurateTimeString();
			std::cerr << std::endl;
#endif

			free(pkt_wpending);
			pkt_wpending = NULL;

			sentbytes += len;
			sent = true;
		}
	}
	outSentBytes(sentbytes);
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

	if(!(bio->isactive()))
	{
		reading_state = reading_state_initial ;
		inReadBytes(readbytes);
		return 0;
	}

	// enough space to read any packet.
	int maxlen = pkt_rpend_size; 
	void *block = pkt_rpending; 

	// initial read size: basic packet.
	int blen = getRsPktBaseSize();

	int maxin = inAllowedBytes();

#ifdef DEBUG_PQISTREAMER
	std::cerr << "[" << (void*)pthread_self() << "] " << "reading state = " << reading_state << std::endl ;
#endif
	switch(reading_state)
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

		if (blen != (tmplen = bio->readdata(block, blen)))
		{
			pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, "pqistreamer::handleincoming() Didn't read BasePkt!");

			inReadBytes(readbytes);

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
					std::cerr << "[" << (void*)pthread_self() << "] " << "given up 2, state = " << reading_state << std::endl ;
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
		reading_state = reading_state_packet_started ;
		failed_read_attempts = 0 ;						// reset failed read, as the packet has been totally read.
	}
continue_packet:
	{
		// workout how much more to read.
		int extralen = getRsItemSize(block) - blen;

#ifdef DEBUG_PQISTREAMER
                std::cerr << "[" << (void*)pthread_self() << "] " << "continuing packet getRsItemSize(block) = " << getRsItemSize(block) << std::endl ;
                std::cerr << "[" << (void*)pthread_self() << "] " << "continuing packet extralen = " << extralen << std::endl ;

				std::cerr << "[" << (void*)pthread_self() << "] " << "continuing packet state=" << reading_state << std::endl ;
		std::cerr << "[" << (void*)pthread_self() << "] " << "block 1 : " << (int)(((unsigned char*)block)[0]) << " " << (int)(((unsigned char*)block)[1]) << " " << (int)(((unsigned char*)block)[2]) << " " << (int)(((unsigned char*)block)[3])  << " "
							<< (int)(((unsigned char*)block)[4]) << " "
							<< (int)(((unsigned char*)block)[5]) << " "
							<< (int)(((unsigned char*)block)[6]) << " "
							<< (int)(((unsigned char*)block)[7]) << " " << std::endl ;
#endif
		if (extralen > maxlen - blen)
		{
			pqioutput(PQL_ALERT, pqistreamerzone, "ERROR: Read Packet too Big!");

			pqiNotify *notify = getPqiNotify();
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
			bio->close();	
			reading_state = reading_state_initial ;	// restart at state 1.
			failed_read_attempts = 0 ;
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

			if (extralen != (tmplen = bio->readdata(extradata, extralen)))
			{
#ifdef DEBUG_PQISTREAMER
				if(tmplen > 0)
					std::cerr << "[" << (void*)pthread_self() << "] " << "Incomplete packet read ! This is a real problem ;-)" << std::endl ;
#endif

				if(++failed_read_attempts > max_failed_read_attempts)
				{
					std::string out;
					rs_sprintf(out, "Error Completing Read (read %d/%d)", tmplen, extralen);
					std::cerr << out << std::endl ;
					pqioutput(PQL_ALERT, pqistreamerzone, out);

					pqiNotify *notify = getPqiNotify();
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

					bio->close();	
					reading_state = reading_state_initial ;	// restart at state 1.
					failed_read_attempts = 0 ;
					return -1;
				}
				else
				{
#ifdef DEBUG_PQISTREAMER
					std::cerr << "[" << (void*)pthread_self() << "] " << "given up 5, state = " << reading_state << std::endl ;
#endif
					return 0 ;	// this is just a SSL_WANT_READ error. Don't panic, we'll re-try the read soon.
									// we assume readdata() returned either -1 or the complete read size.
				}
			}
#ifdef DEBUG_PQISTREAMER
		std::cerr << "[" << (void*)pthread_self() << "] " << "continuing packet state=" << reading_state << std::endl ;
		std::cerr << "[" << (void*)pthread_self() << "] " << "block 2 : " << (int)(((unsigned char*)extradata)[0]) << " " << (int)(((unsigned char*)extradata)[1]) << " " << (int)(((unsigned char*)extradata)[2]) << " " << (int)(((unsigned char*)extradata)[3])  << " "
							<< (int)(((unsigned char*)extradata)[4]) << " "
							<< (int)(((unsigned char*)extradata)[5]) << " "
							<< (int)(((unsigned char*)extradata)[6]) << " "
							<< (int)(((unsigned char*)extradata)[7]) << " " << std::endl ;
#endif

			failed_read_attempts = 0 ;
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
		RsItem *pkt = rsSerialiser->deserialise(block, &pktlen);

		if ((pkt != NULL) && (0  < handleincomingitem(pkt)))
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

		reading_state = reading_state_initial ;	// restart at state 1.
		failed_read_attempts = 0 ;						// reset failed read, as the packet has been totally read.
	}

	if(maxin > readbytes && bio->moretoread())
		goto start_packet_read ;

#ifdef DEBUG_TRANSFERS
	if (readbytes >= maxin)
	{
		std::cerr << "pqistreamer::handleincoming() Stopped reading as readbytes >= maxin. Read " << readbytes << " bytes ";
		std::cerr << std::endl;
	}
#endif

	inReadBytes(readbytes);
	return 0;
}


/* BandWidth Management Assistance */

float   pqistreamer::outTimeSlice()
{
#ifdef DEBUG_PQISTREAMER
	pqioutput(PQL_DEBUG_ALL, pqistreamerzone, "pqistreamer::outTimeSlice()");
#endif

	//fixme("pqistreamer::outTimeSlice()", 1);
	return 1;
}

// very simple..... 
int     pqistreamer::outAllowedBytes()
{
	int t = time(NULL); // get current timestep.

	/* allow a lot if not bandwidthLimited */
	if (!bio->bandwidthLimited())
	{
		currSent = 0;
		currSentTS = t;
		return PQISTREAM_ABS_MAX;
	}

	int dt = t - currSentTS;
	// limiter -> for when currSentTs -> 0.
	if (dt > 5)
		dt = 5;

	int maxout = (int) (getMaxRate(false) * 1000.0);
	currSent -= dt * maxout;
	if (currSent < 0)
	{
		currSent = 0;
	}

	currSentTS = t;

#ifdef DEBUG_PQISTREAMER
	{
		std::string out;
		rs_sprintf(out, "pqistreamer::outAllowedBytes() is %d/%d", maxout - currSent, maxout);
		pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out);
	}
#endif


	return maxout - currSent;
}

int     pqistreamer::inAllowedBytes()
{
	int t = time(NULL); // get current timestep.

	/* allow a lot if not bandwidthLimited */
	if (!bio->bandwidthLimited())
	{
		currReadTS = t;
		currRead = 0;
		return PQISTREAM_ABS_MAX;
	}

	int dt = t - currReadTS;
	// limiter -> for when currReadTs -> 0.
	if (dt > 5)
		dt = 5;

	int maxin = (int) (getMaxRate(true) * 1000.0);
	currRead -= dt * maxin;
	if (currRead < 0)
	{
		currRead = 0;
	}

	currReadTS = t;

#ifdef DEBUG_PQISTREAMER
	{
		std::string out;
		rs_sprintf(out, "pqistreamer::inAllowedBytes() is %d/%d", maxin - currRead, maxin);
		pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out);
	}
#endif


	return maxin - currRead;
}


static const float AVG_PERIOD = 5; // sec
static const float AVG_FRAC = 0.8; // for low pass filter.

void    pqistreamer::outSentBytes(int outb)
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
	
	




	totalSent += outb;
	currSent += outb;
	avgSentCount += outb;

	int t = time(NULL); // get current timestep.
	if (t - avgLastUpdate > AVG_PERIOD)
	{
		float avgReadpSec = getRate(true);
		float avgSentpSec = getRate(false);

		avgReadpSec *= AVG_FRAC;
		avgReadpSec += (1.0 - AVG_FRAC) * avgReadCount / 
				(1000.0 * (t - avgLastUpdate));

		avgSentpSec *= AVG_FRAC;
		avgSentpSec += (1.0 - AVG_FRAC) * avgSentCount / 
				(1000.0 * (t - avgLastUpdate));

	
		/* pretend our rate is zero if we are 
		 * not bandwidthLimited().
		 */
		if (bio->bandwidthLimited())
		{
			setRate(true, avgReadpSec);
			setRate(false, avgSentpSec);
		}
		else
		{
			setRate(true, 0);
			setRate(false, 0);
		}


		avgLastUpdate = t;
		avgReadCount = 0;
		avgSentCount = 0;
	}
	return;
}

void    pqistreamer::inReadBytes(int inb)
{
#ifdef DEBUG_PQISTREAMER
	{
		std::string out;
		rs_sprintf(out, "pqistreamer::inReadBytes(): %d@%gkB/s", inb, getRate(true));
		pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out);
	}
#endif

	totalRead += inb;
	currRead += inb;
	avgReadCount += inb;

	return;
}

int     pqistreamer::getQueueSize(bool in)
{
	if (in)
		return incoming.size();
	return out_queue_size();
}

void    pqistreamer::getRates(RsBwRates &rates)
{
	RateInterface::getRates(rates);
	rates.mQueueIn = incoming.size();	
	rates.mQueueOut = out_queue_size();
}

int pqistreamer::out_queue_size() const
{
	// Warning: because out_pkt is a list, calling size
	//	is O(n) ! Makign algorithms pretty inefficient. We should record how many
	//	items get stored and discarded to have a proper size value at any time
	//
	return out_pkt.size() ; 
}

void pqistreamer::locked_clear_out_queue()
{
	for(std::list<void*>::iterator it = out_pkt.begin(); it != out_pkt.end(); )
	{
		free(*it);
		it = out_pkt.erase(it);
#ifdef DEBUG_PQISTREAMER
		std::string out = "pqistreamer::handleoutgoing() Not active -> Clearing Pkt!";
		//			std::cerr << out ;
		pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, out);
#endif
	}
}

int pqistreamer::locked_compute_out_pkt_size() const
{
	int total = 0 ;

	for(std::list<void*>::const_iterator it = out_pkt.begin(); it != out_pkt.end(); ++it)
		total += getRsItemSize(*it);

	return total ;
}

void *pqistreamer::locked_pop_out_data() 
{
	void *res = NULL ;

	if (!out_pkt.empty())
	{
		res = *(out_pkt.begin()); 
		out_pkt.pop_front();
#ifdef DEBUG_TRANSFERS
		std::cerr << "pqistreamer::handleoutgoing() getting next pkt from out_pkt queue";
		std::cerr << std::endl;
#endif
	}
	return res ;
}
