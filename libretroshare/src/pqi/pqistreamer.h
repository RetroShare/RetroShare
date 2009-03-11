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

#include <list>

// Fully implements the PQInterface.
// and communicates with peer etc via the BinInterface.
//
// The interface does not handle connection, just communication.
// possible bioflags: BIN_FLAGS_NO_CLOSE | BIN_FLAGS_NO_DELETE

class pqistreamer: public PQInterface
{
public:
	pqistreamer(RsSerialiser *rss, std::string peerid, BinInterface *bio_in, int bio_flagsin);
virtual ~pqistreamer();

// PQInterface
virtual int     SendItem(RsItem *);
virtual RsItem *GetItem();

virtual int     tick();
virtual int     status();

	private:
	/* Implementation */

	// to filter functions - detect filecancel/data and act!
int	queue_outpqi(      RsItem *i);
int 	handleincomingitem(RsItem *i);

	// ticked regularly (manages out queues and sending
	// via above interfaces.
int	handleoutgoing();
int	handleincoming();

	// Bandwidth/Streaming Management.
float	outTimeSlice();

int	outAllowedBytes();
void	outSentBytes(int );

int	inAllowedBytes();
void	inReadBytes(int );

	// RsSerialiser - determines which packets can be serialised.
	RsSerialiser *rsSerialiser;
	// Binary Interface for IO, initialisated at startup.
	BinInterface *bio;
	unsigned int  bio_flags; // BIN_FLAGS_NO_CLOSE | BIN_FLAGS_NO_DELETE

	void *pkt_wpending; // storage for pending packet to write.
	int   pkt_rpend_size; // size of pkt_rpending.
	void *pkt_rpending; // storage for read in pending packets.

	enum {reading_state_packet_started=1,
			reading_state_initial=0 } ;

	int   reading_state ;
	int   failed_read_attempts ;

	// Temp Storage for transient data.....
	std::list<void *> out_pkt; // Cntrl / Search / Results queue
	std::list<void *> out_data; // FileData - secondary queue.
	std::list<RsItem *> incoming;

	// data for network stats.
	int totalRead;
	int totalSent;

	// these are representative (but not exact)
	int currRead;
	int currSent;
	int currReadTS; // TS from which these are measured.
	int currSentTS;

	int avgLastUpdate; // TS from which these are measured.
	float avgReadCount;
	float avgSentCount;

	RsMutex streamerMtx ;
//	pthread_t thread_id;
};


#endif //MRK_PQI_STREAMER_HEADER
