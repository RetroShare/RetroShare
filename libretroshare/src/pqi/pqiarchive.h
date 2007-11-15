/*
 * "$Id: pqiarchive.h,v 1.3 2007-02-18 21:46:49 rmf24 Exp $"
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



#ifndef MRK_PQI_ARCHIVE_STREAMER_HEADER
#define MRK_PQI_ARCHIVE_STREAMER_HEADER

// Only dependent on the base stuff.
#include "pqi/pqi_base.h"

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
#include "pqi/xpgpcert.h"
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
#include "pqi/sslcert.h"
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

#include <list>

/*******************************************************************
 * pqiarchive provides an archive stream.
 * This stores PQItem + Person Reference + Timestamp,
 *
 * and allows Objects to be replayed or restored, 
 * independently of the rest of the pqi system.
 *
 */

class pqiarchive: PQInterface
{
public:
	pqiarchive(BinInterface *bio_in, int bio_flagsin, sslroot *);
virtual ~pqiarchive();

// PQInterface
virtual int     SendItem(PQItem *);
virtual PQItem *GetItem();

virtual int     tick();
virtual int     status();

virtual void	setRealTime(bool r) { realTime = r; }
	private:
int     writePkt(PQItem *item);
int     readPkt(PQItem **item_out, long *ts);

	// Binary Interface for IO, initialisated at startup.
	BinInterface *bio;
	unsigned int  bio_flags; // only BIN_NO_CLOSE at the moment.

	// Temp Storage for transient data.....
	PQItem *nextPkt;
	long  nextPktTS; /* timestamp associated with nextPkt */
	long  firstPktTS; /* timestamp associated with first read Pkt */
	long  initTS;    /* clock timestamp at first read */
	sslroot *sslr;

	bool realTime;
};


#endif //MRK_PQI_ARCHIVE_STREAMER_HEADER
