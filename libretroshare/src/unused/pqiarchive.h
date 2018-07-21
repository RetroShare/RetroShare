/*******************************************************************************
 * libretroshare/src/pqi: pqiarchive.h                                         *
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
#ifdef SUSPENDED_UNUSED_CODE

#ifndef MRK_PQI_ARCHIVE_STREAMER_HEADER
#define MRK_PQI_ARCHIVE_STREAMER_HEADER

#include "pqi/pqi.h"

#include <list>

/*******************************************************************
 * pqiarchive provides an archive stream.
 * This stores RsItem + Person Reference + Timestamp,
 *
 * and allows Objects to be replayed or restored, 
 * independently of the rest of the pqi system.
 *
 */

class pqiarchive: public PQInterface
{
public:
	pqiarchive(RsSerialiser *rss, BinInterface *bio_in, int bio_flagsin);
virtual ~pqiarchive();

// PQInterface
virtual int     SendItem(RsItem *);
virtual RsItem *GetItem();

virtual int     tick();
virtual int     status();

virtual void	setRealTime(bool r) { realTime = r; }

std::string     gethash();

	private:
int     writePkt(RsItem *item);
int     readPkt(RsItem **item_out, long *ts);

	// Serialiser
	RsSerialiser *rsSerialiser;
	// Binary Interface for IO, initialisated at startup.
	BinInterface *bio;
	unsigned int  bio_flags; // only BIN_NO_CLOSE at the moment.

	// Temp Storage for transient data.....
	RsItem *nextPkt;
	long  nextPktTS; /* timestamp associated with nextPkt */
	long  firstPktTS; /* timestamp associated with first read Pkt */
	long  initTS;    /* clock timestamp at first read */

	bool realTime;
};


#endif //MRK_PQI_ARCHIVE_STREAMER_HEADER
#endif //SUSPENDED_UNUSED_CODE
