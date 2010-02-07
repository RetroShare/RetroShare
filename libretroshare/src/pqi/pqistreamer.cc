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
#include <sstream>
#include "util/rsdebug.h"

#include "pqi/pqistreamer.h"
#include "pqi/pqinotify.h"

#include "serialiser/rsserial.h" 
#include "serialiser/rsbaseitems.h"  /***** For RsFileData *****/


const int pqistreamerzone = 8221;

const int PQISTREAM_ABS_MAX = 100000000; /* 100 MB/sec (actually per loop) */

/* This removes the print statements (which hammer pqidebug) */
/***
#define RSITEM_DEBUG 1
 ***/


pqistreamer::pqistreamer(RsSerialiser *rss, std::string id, BinInterface *bio_in, int bio_flags_in)
	:PQInterface(id), rsSerialiser(rss), bio(bio_in), bio_flags(bio_flags_in), 
	pkt_wpending(NULL), 
	totalRead(0), totalSent(0),
	currRead(0), currSent(0),
	avgReadCount(0), avgSentCount(0)
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

	{
		std::ostringstream out;
		out << "pqistreamer::pqistreamer()";
		out << " Initialisation!" << std::endl;
		pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out.str());
	}

	if (!bio_in)
	{
		std::ostringstream out;
		out << "pqistreamer::pqistreamer()";
		out << " NULL bio, FATAL ERROR!" << std::endl;
		pqioutput(PQL_ALERT, pqistreamerzone, out.str());
		exit(1);
	}

	failed_read_attempts = 0 ;						// reset failed read, as no packet is still read.

	return;
}

pqistreamer::~pqistreamer()
{
	RsStackMutex stack(streamerMtx) ;		// lock out_pkt and out_data

	{
		std::ostringstream out;
		out << "pqistreamer::~pqistreamer()";
		out << " Destruction!" << std::endl;
		pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out.str());
	}

	if (bio_flags & BIN_FLAGS_NO_CLOSE)
	{
		std::ostringstream out;
		out << "pqistreamer::~pqistreamer()";
		out << " Not Closing BinInterface!" << std::endl;
		pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out.str());
	}
	else if (bio)
	{
		std::ostringstream out;
		out << "pqistreamer::~pqistreamer()";
		out << " Deleting BinInterface!" << std::endl;
		pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out.str());

		delete bio;
	}

	/* clean up serialiser */
	if (rsSerialiser)
		delete rsSerialiser;

	// clean up outgoing. (cntrl packets)
	while(out_pkt.size() > 0)
	{
		void *pkt = out_pkt.front();
		out_pkt.pop_front();
		free(pkt);
	}

	// clean up outgoing (data packets)
	while(out_data.size() > 0)
	{
		void *pkt = out_data.front();
		out_data.pop_front();
		free(pkt);
	}

	if (pkt_wpending)
	{
		free(pkt_wpending);
		pkt_wpending = NULL;
	}

	free(pkt_rpending);

	// clean up outgoing.
	while(incoming.size() > 0)
	{
		RsItem *i = incoming.front();
		incoming.pop_front();
		delete i;
	}
	return;
}


// Get/Send Items.
int	pqistreamer::SendItem(RsItem *si)
{
#ifdef RSITEM_DEBUG 
        {
	  std::ostringstream out;
	  out << "pqistreamer::SendItem():" << std::endl;
	  si -> print(out);
	  pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out.str());
	    std::cerr << out.str();
	}
#endif

	return queue_outpqi(si);
}

RsItem *pqistreamer::GetItem()
{
        {
	  std::ostringstream out;
	  out << "pqistreamer::GetItem()";
	  pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out.str());
	}

	std::list<RsItem *>::iterator it;

	it = incoming.begin();
	if (it == incoming.end()) { return NULL; }

	RsItem *osr = (*it);
	incoming.erase(it);
	return osr;
}

// // PQInterface
int	pqistreamer::tick()
{
        {
	  std::ostringstream out;
	  out << "pqistreamer::tick()";
	  out << std::endl;
	  out << PeerId() << ": currRead/Sent: " << currRead << "/" << currSent;
	  out << std::endl;

	  pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out.str());
	}

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

	/* give details of the packets */
	{
        	std::list<void *>::iterator it;

		std::ostringstream out;
		out << "pqistreamer::tick() Queued Data:";
	    	out << " for " << PeerId();

		if (bio->isactive())
		{
			out << " (active)";
		}
		else
		{
			out << " (waiting)";
		}
		out << std::endl;

		{
			RsStackMutex stack(streamerMtx) ;		// lock out_pkt and out_data
			int total = 0;

			for(it = out_pkt.begin(); it != out_pkt.end(); it++)
			{
				total += getRsItemSize(*it);
			}

			out << "\t Out Packets [" << out_pkt.size() << "] => " << total;
			out << " bytes" << std::endl;

			total = 0;
			for(it = out_data.begin(); it != out_data.end(); it++)
			{
				total += getRsItemSize(*it);
			}

			out << "\t Out Data    [" << out_data.size() << "] => " << total;
			out << " bytes" << std::endl;

			out << "\t Incoming    [" << incoming.size() << "]";
			out << std::endl;
		}

	  	pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, out.str());
	}

	/* if there is more stuff in the queues */
	if ((incoming.size() > 0) || (out_pkt.size() > 0) || (out_data.size() > 0))
	{
		return 1;
	}
	return 0;
}

int	pqistreamer::status()
{
        {
	  std::ostringstream out;
	  out << "pqistreamer::status()";
	  pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out.str());
	}

	if (bio->isactive())
	{
		std::ostringstream out;
		out << "Data in:" << totalRead << " out:" << totalSent;
	  	pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, out.str());
	}

	return 0;
}

//
/**************** HANDLE OUTGOING TRANSLATION + TRANSMISSION ******/

int	pqistreamer::queue_outpqi(RsItem *pqi)
{
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
		*(int*)0x0=1 ;
	}
#endif

        {
	  std::ostringstream out;
	  out << "pqistreamer::queue_outpqi()";
	  pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out.str());
	}

	/* decide which type of packet it is */
	RsFileData *dta = dynamic_cast<RsFileData *>(pqi);
	bool isCntrl = (dta == NULL);

        uint32_t pktsize = rsSerialiser->size(pqi);
	void *ptr = malloc(pktsize);

#ifdef DEBUG_PQISTREAMER
        std::cerr << "pqistreamer::queue_outpqi() serializing packet with packet size : " << pktsize << std::endl;
#endif
        if (rsSerialiser->serialise(pqi, ptr, &pktsize))
	{
		if (isCntrl)
		{
			out_pkt.push_back(ptr);
		}
		else
		{
			out_data.push_back(ptr);
		}
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

	std::ostringstream out;
	out << "pqistreamer::queue_outpqi() Null Pkt generated!";
	out << std::endl;
	out << "Caused By: " << std::endl;
	pqi -> print(out);
	pqioutput(PQL_ALERT, pqistreamerzone, out.str());

	if (!(bio_flags & BIN_FLAGS_NO_DELETE))
	{
		delete pqi;
	}
	return 1; // keep error internal.
}

int 	pqistreamer::handleincomingitem(RsItem *pqi)
{
        {
	  std::ostringstream out;
	  out << "pqistreamer::handleincomingitem()";
	  pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out.str());
	}

	// Use overloaded Contact function 
	pqi -> PeerId(PeerId());
	incoming.push_back(pqi);
	return 1;
}

int	pqistreamer::handleoutgoing()
{
	RsStackMutex stack(streamerMtx) ;		// lock out_pkt and out_data

	{
		std::ostringstream out;
		out << "pqistreamer::handleoutgoing()";
		pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out.str());
	}

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
		for(it = out_pkt.begin(); it != out_pkt.end(); )
		{
			free(*it);
			it = out_pkt.erase(it);

			std::ostringstream out;
			out << "pqistreamer::handleoutgoing() Not active -> Clearing Pkt!";
			//			std::cerr << out.str() ;
			pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, out.str());
		}
		for(it = out_data.begin(); it != out_data.end(); )
		{
			free(*it);
			it = out_data.erase(it);

			std::ostringstream out;
			out << "pqistreamer::handleoutgoing() Not active -> Clearing DPkt!";
			//			std::cerr << out.str() ;
			pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, out.str());
		}

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
			outSentBytes(sentbytes);
			return 0;
		}

		// send a out_pkt., else send out_data. unless
		// there is a pending packet.
		if (!pkt_wpending)
		{
			if (out_pkt.size() > 0)
			{
				pkt_wpending = *(out_pkt.begin()); 
				out_pkt.pop_front();
			}
			else if (out_data.size() > 0)
			{
				pkt_wpending = *(out_data.begin()); 
				out_data.pop_front();
			}
		}

		if (pkt_wpending)
		{
			std::ostringstream out;
			// write packet.
			len = getRsItemSize(pkt_wpending);

#ifdef DEBUG_PQISTREAMER
                        std::cout << "Sending Out Pkt of size " << len << " !" << std::endl;
#endif

			if (len != (ss = bio->senddata(pkt_wpending, len)))
			{
				out << "Problems with Send Data! (only " << ss << " bytes sent" << ", total pkt size=" << len ;
				out << std::endl;
//				std::cerr << out.str() ;
				pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, out.str());

				outSentBytes(sentbytes);
				// pkt_wpending will kept til next time.
				// ensuring exactly the same data is written (openSSL requirement).
				return -1;
			}

//			out << " Success!" << ", sent " << len << " bytes" << std::endl;
			//			std::cerr << out.str() ;
			pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, out.str());

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

	{
		std::ostringstream out;
		out << "pqistreamer::handleincoming()";
		pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out.str());
	}

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
				// most likely blocked!
				pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, "pqistreamer::handleincoming() read blocked");
#ifdef DEBUG_PQISTREAMER
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
				std::ostringstream out;
				out << "pqistreamer::handleincoming() Incomplete ";
				out << "(Strange) read of " << tmplen << " bytes";
				pqioutput(PQL_ALERT, pqistreamerzone, out.str());
#ifdef DEBUG_PQISTREAMER
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

				std::ostringstream msgout;
				msgout <<  "               **** WARNING ****     \n";
				msgout <<  "Retroshare has caught a BAD Packet Read";
				msgout <<  "\n";
				msgout <<  "This is normally caused by connecting to an";
				msgout <<  " OLD version of Retroshare";
				msgout <<  "\n";
				msgout <<  "(M:" << maxlen << " B:" << blen << " E:" << extralen << ")\n";
				msgout <<  "\n";
				msgout << "block = " 
							<< (int)(((unsigned char*)block)[0]) << " " 
							<< (int)(((unsigned char*)block)[1]) << " " 
							<< (int)(((unsigned char*)block)[2]) << " " 
							<< (int)(((unsigned char*)block)[3])  << "\n" ;
				msgout <<  "\n";
				msgout <<  "Please get your friends to upgrade to the latest version";
				msgout <<  "\n";
				msgout <<  "\n";
				msgout <<  "If you are sure the error was not caused by an old version";
				msgout <<  "\n";
				msgout <<  "Please report the problem to Retroshare's developers";
				msgout <<  "\n";

				std::string msg = msgout.str();
				notify->AddLogMessage(0, RS_SYS_WARNING, title, msg);
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
//			memset((void*)( &(((unsigned char *)block)[blen])),0,extralen) ;	// reset the block, to avoid uninitialized memory reads.
			memset( extradata,0,extralen ) ;	// for checking later

			if (extralen != (tmplen = bio->readdata(extradata, extralen)))
			{
#ifdef DEBUG_PQISTREAMER
				if(tmplen > 0)
					std::cerr << "[" << (void*)pthread_self() << "] " << "Incomplete packet read ! This is a real problem ;-)" << std::endl ;
#endif

				if(++failed_read_attempts > max_failed_read_attempts)
				{
					std::ostringstream out;
					out << "Error Completing Read (read ";
					out << tmplen << "/" << extralen << ")" << std::endl;
					std::cerr << out.str() ;
					pqioutput(PQL_ALERT, pqistreamerzone, out.str());

					pqiNotify *notify = getPqiNotify();
					if (notify)
					{
						std::string title = "Warning: Error Completing Read";

						std::ostringstream msgout;
						msgout <<  "               **** WARNING ****     \n";
						msgout <<  "Retroshare has experienced an unexpected Read ERROR";
						msgout <<  "\n";
						msgout <<  "(M:" << maxlen << " B:" << blen;
						msgout <<  " E:" << extralen << " R:" << tmplen << ")\n";
						msgout <<  "\n";
						msgout <<  "Please contact the developers.";
						msgout <<  "\n";

						std::string msg = msgout.str();
						std::cerr << msg << std::endl ;
						std::cerr << "block = " 
							<< (int)(((unsigned char*)block)[0]) << " " 
							<< (int)(((unsigned char*)block)[1]) << " " 
							<< (int)(((unsigned char*)block)[2]) << " " 
							<< (int)(((unsigned char*)block)[3]) << " "
							<< (int)(((unsigned char*)block)[4]) << " " 
							<< (int)(((unsigned char*)block)[5]) << " " 
							<< (int)(((unsigned char*)block)[6]) << " " 
							<< (int)(((unsigned char*)block)[7]) << " "
							<< std::endl ;
											notify->AddSysMessage(0, RS_SYS_WARNING, title, msg);
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
		{
                        std::ostringstream out;
                        out << "Read Data Block -> Incoming Pkt(";
                        out << blen + extralen << ")" << std::endl;
                        //std::cerr << out.str() ;
			pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, out.str());
		}

                //		std::cerr << "Deserializing packet of size " << pktlen <<std::endl ;

		uint32_t pktlen = blen+extralen ;
#ifdef DEBUG_PQISTREAMER
		std::cerr << "[" << (void*)pthread_self() << "] " << "deserializing. Size=" << pktlen << std::endl ;
#endif

//		if(pktlen == 17306)
//		{
//			FILE *f = fopen("dbug.packet.bin","w");
//			fwrite(block,pktlen,1,f) ;
//			fclose(f) ;
//			exit(-1) ;
//		}
		RsItem *pkt = rsSerialiser->deserialise(block, &pktlen);

		if ((pkt != NULL) && (0  < handleincomingitem(pkt)))
			pqioutput(PQL_DEBUG_BASIC, pqistreamerzone, "Successfully Read a Packet!");
		else
			pqioutput(PQL_ALERT, pqistreamerzone, "Failed to handle Packet!");

		reading_state = reading_state_initial ;	// restart at state 1.
		failed_read_attempts = 0 ;						// reset failed read, as the packet has been totally read.
	}

	if(maxin > readbytes && bio->moretoread())
		goto start_packet_read ;

	inReadBytes(readbytes);
	return 0;
}


/* BandWidth Management Assistance */

float   pqistreamer::outTimeSlice()
{
        {
	  std::ostringstream out;
	  out << "pqistreamer::outTimeSlice()";
	  pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out.str());
	}

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

        {
	  std::ostringstream out;
	  out << "pqistreamer::outAllowedBytes() is ";
	  out << maxout - currSent << "/";
	  out << maxout;
	  pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out.str());
	}


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

        {
	  std::ostringstream out;
	  out << "pqistreamer::inAllowedBytes() is ";
	  out << maxin - currRead << "/";
	  out << maxin;
	  pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out.str());
	}


	return maxin - currRead;
}


static const float AVG_PERIOD = 5; // sec
static const float AVG_FRAC = 0.8; // for low pass filter.

void    pqistreamer::outSentBytes(int outb)
{
        {
	  std::ostringstream out;
	  out << "pqistreamer::outSentBytes(): ";
	  out << outb << "@" << getRate(false) << "kB/s" << std::endl;
	  pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out.str());
	}


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
        {
	  std::ostringstream out;
	  out << "pqistreamer::inReadBytes(): ";
	  out << inb << "@" << getRate(true) << "kB/s" << std::endl;
	  pqioutput(PQL_DEBUG_ALL, pqistreamerzone, out.str());
	}

	totalRead += inb;
	currRead += inb;
	avgReadCount += inb;

	return;
}

