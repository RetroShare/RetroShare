/*******************************************************************************
 * libretroshare/src/pqi: pqiarchive.cc                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2004-2006  Robert Fernie                                      *
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

/* This is dependent on the sslroot at the moment.
 * -> as we need to create/restore references to the Person.
 * -> and store the signatures to do this.
 */

/*******************************************************************
 * pqiarchive provides an archive stream.
 * This stores RsItem + Person Reference + Timestamp,
 *
 * and allows Objects to be replayed or restored, 
 * independently of the rest of the pqi system.
 *
 */

#ifdef SUSPENDED_UNUSED_CODE

#include "pqi/pqiarchive.h"
#include "serialiser/rsserial.h"
#include <iostream>
#include <fstream>

#include "util/rsdebug.h"
#include "util/rsstring.h"
#include "util/rstime.h"

const int pqiarchivezone = 9326;

struct pqiarchive_header
{
	uint32_t	type;
	uint32_t	length;
	uint32_t	ts;
	uint8_t		personSig[PQI_PEERID_LENGTH];
};

const int PQIARCHIVE_TYPE_PQITEM = 0x0001;

/* PeerId of PQInterface is not important ... as we are archiving
 * packets from other people...
 */

pqiarchive::pqiarchive(RsSerialiser *rss, BinInterface *bio_in, int bio_flags_in)
	:PQInterface(""), rsSerialiser(rss), bio(bio_in), bio_flags(bio_flags_in),
        nextPkt(NULL), nextPktTS(0), firstPktTS(0), initTS(0),realTime(false)
{
    pqioutput(PQL_DEBUG_ALL, pqiarchivezone, "pqiarchive::pqiarchive() Initialisation!\n");

	if (!bio_in)
	{
	  pqioutput(PQL_ALERT, pqiarchivezone, "pqiarchive::pqiarchive() NULL bio, FATAL ERROR!\n");
	  exit(1);
	}

	return;
}

pqiarchive::~pqiarchive()
{
	pqioutput(PQL_DEBUG_ALL, pqiarchivezone, "pqiarchive::~pqiarchive() Destruction!\n");

	if (bio_flags & BIN_FLAGS_NO_CLOSE)
	{
		pqioutput(PQL_DEBUG_ALL, pqiarchivezone, "pqiarchive::~pqiarchive() Not Closing BinInterface!\n");
	}
	else if (bio)
	{
		pqioutput(PQL_DEBUG_ALL, pqiarchivezone, "pqiarchive::~pqiarchive() Deleting BinInterface!\n");

		delete bio;
	}

	if (rsSerialiser)
		delete rsSerialiser;

	if (nextPkt)
	{
		delete nextPkt;
	}
	return;
}


// Get/Send Items.
int	pqiarchive::SendItem(RsItem *si)
{
	{
		std::string out = "pqiarchive::SendItem()\n";
		si -> print_string(out);
		pqioutput(PQL_DEBUG_BASIC, pqiarchivezone, out);
	}

	// check if this is a writing bio.
	
	if (!(bio_flags & BIN_FLAGS_WRITEABLE))
	{
		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete si;
		return -1;
	}

//	std::cerr << "SendItem: si->PeerId()=" << si->PeerId() << std::endl ;

	int ret = writePkt(si);
	return ret; /* 0 - failure, 1 - success*/
}

RsItem *pqiarchive::GetItem()
{
	pqioutput(PQL_DEBUG_ALL, pqiarchivezone, "pqiarchive::GetItem()");

	// check if this is a reading bio.
	if (!(bio_flags & BIN_FLAGS_READABLE))
	{
		pqioutput(PQL_DEBUG_BASIC, pqiarchivezone, "pqiarchive::GetItem() Error Not Readable");
		return NULL;
	}

	// load if we dont have a packet.
	if (!nextPkt)
	{
		if (!readPkt(&nextPkt, &nextPktTS))
		{
			pqioutput(PQL_DEBUG_BASIC, pqiarchivezone, "pqiarchive::GetItem() Failed to ReadPkt");
			return NULL;
		}
	}

	if (!nextPkt) return NULL;


	/* if realtime - check delta */
	bool rtnPkt = true;
	if ((realTime) && (initTS))
	{
		int delta = time(NULL) - initTS;
		int delta2 = nextPktTS - firstPktTS;
		if (delta >= delta2)
		{
			rtnPkt = true;
		}
		else
		{
			rtnPkt = false;
		}
	}

	if (rtnPkt)
	{
		if (!initTS)
		{
			/* first read! */
			initTS = time(NULL);
			firstPktTS = nextPktTS;
		}
		RsItem *outPkt = nextPkt;
		nextPkt = NULL;

		if (outPkt != NULL)
		{
			std::string out = "pqiarchive::GetItem() Returning:\n";
			outPkt -> print_string(out);
			pqioutput(PQL_DEBUG_BASIC, pqiarchivezone, out);
		}
		return outPkt;
	}
	return NULL;
}

// // PQInterface
int	pqiarchive::tick()
{
	return 0;
}

int	pqiarchive::status()
{
	pqioutput(PQL_DEBUG_ALL, pqiarchivezone, "pqiarchive::status()");
	return 0;
}

//
/**************** HANDLE OUTGOING TRANSLATION + TRANSMISSION ******/

int	pqiarchive::writePkt(RsItem *pqi)
{
//	std::cerr << "writePkt, pqi->peerId()=" << pqi->PeerId() << std::endl ;
	pqioutput(PQL_DEBUG_ALL, pqiarchivezone, "pqiarchive::writePkt()");

	uint32_t pktsize = rsSerialiser->size(pqi);
	void *ptr = malloc(pktsize);
	if (!(rsSerialiser->serialise(pqi, ptr, &pktsize)))
	{
		std::string out = "pqiarchive::writePkt() Null Pkt generated!\nCaused By:\n";
		pqi -> print_string(out);
		pqioutput(PQL_ALERT, pqiarchivezone, out);

		free(ptr);
		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete pqi;
		return 0;
	}

	/* extract the extra details */
	uint32_t len = getRsItemSize(ptr);
	if (len != pktsize)
	{
		std::string out;
		rs_sprintf(out, "pqiarchive::writePkt() Length MisMatch: len: %lu != pktsize: %lu\nCaused By:\n", len, pktsize);
		pqi -> print_string(out);
		pqioutput(PQL_ALERT, pqiarchivezone, out);

		free(ptr);
		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete pqi;
		return 0;
	}


	if (!(bio->cansend(0)))
	{
		std::string out = "pqiarchive::writePkt() BIO cannot write!\nDiscarding:\n";
		pqi -> print_string(out);
		pqioutput(PQL_ALERT, pqiarchivezone, out);

		free(ptr);
		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete pqi;

		return 0;
	}

	// using the peerid from the item.
	if (pqi->PeerId().length() != PQI_PEERID_LENGTH)
	{
		std::string out = "pqiarchive::writePkt() Invalid peerId Length!\n";
		rs_sprintf_append(out, "Found %ld instead of %ld\n", pqi->PeerId().length(), PQI_PEERID_LENGTH);
		out += "pqi->PeerId() = " + pqi->PeerId() + "\nCaused By:\n";
		pqi -> print_string(out);
		pqioutput(PQL_ALERT, pqiarchivezone, out);

		free(ptr);
		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete pqi;
		return 0;
	}

	std::string out = "Writing Pkt Header\n";
	struct pqiarchive_header hdr;
	hdr.type 	= PQIARCHIVE_TYPE_PQITEM;
	hdr.length 	= len;
	hdr.ts 		= time(NULL);
	memcpy(hdr.personSig, pqi->PeerId().c_str(), PQI_PEERID_LENGTH);

	// write packet header.
	if (sizeof(hdr) != bio->senddata(&hdr, sizeof(hdr)))
	{
		out += "Trouble writing header!\n";
		pqioutput(PQL_ALERT, pqiarchivezone, out);

		free(ptr);
		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete pqi;

		return 0;
	}

	out += "Writing Pkt Body\n";

	// write packet.
	if ((int) len != bio->senddata(ptr, len))
	{
		out += "Problems with Send Data!\n";
		pqioutput(PQL_ALERT, pqiarchivezone, out);

		free(ptr);
		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete pqi;

		return 0;
	}

	out += " Success!";
	pqioutput(PQL_DEBUG_BASIC, pqiarchivezone, out);

	free(ptr);
	if (!(bio_flags & BIN_FLAGS_NO_DELETE))
		delete pqi;

	return 1;
}

/* Reads a single packet from the input stream
 * gets the timestamp as well.
 *
 */

int	pqiarchive::readPkt(RsItem **item_out, long *ts_out)
{
	pqioutput(PQL_DEBUG_ALL, pqiarchivezone, "pqiarchive::readPkt()");

	if ((!(bio->isactive())) || (!(bio->moretoread(0))))
	{
		return 0;
	}

	// header
	struct pqiarchive_header hdr;

	// enough space to read any packet.
	int maxlen = getRsPktMaxSize();
	void *block = malloc(maxlen);

	// initial read size: basic packet.
	int blen = getRsPktBaseSize();

	int tmplen;

	/* read in the header */
	if (sizeof(hdr) != bio->readdata(&hdr, sizeof(hdr)))
	{
		/* error */
	  	pqioutput(PQL_WARNING, pqiarchivezone, 
	  		"pqiarchive::readPkt() bad read(1)");

		free(block);
		return 0;
	}

	/* we have the header */

	// read the basic block (minimum packet size)
	if (blen != (tmplen = bio->readdata(block, blen)))
	{
	  	pqioutput(PQL_WARNING, pqiarchivezone, 
	  		"pqiarchive::readPkt() bad read(2)");

		free(block);
		return 0;
	}

	// workout how much more to read.
	int extralen = getRsItemSize(block) - blen;
	if (extralen > 0)
	{
		void *extradata = (void *) (((char *) block) + blen);
		if (extralen != (tmplen = bio->readdata(extradata, extralen)))
		{

			std::string out;
			rs_sprintf(out, "pqiarchive::readPkt() Error Completing Read (read %d/%d)\n", tmplen, extralen);
			pqioutput(PQL_ALERT, pqiarchivezone, out);

			free(block);
			return 0;
		}
	}

	// create packet, based on header.
        //std::cerr << "Read Data Block -> Incoming Pkt(";
        //std::cerr << blen + extralen << ")" << std::endl;
	uint32_t readbytes = extralen + blen;

	RsItem *item = rsSerialiser->deserialise(block, &readbytes);
	free(block);

	if (item == NULL)
	{
	  	pqioutput(PQL_ALERT, pqiarchivezone, 
		  "pqiarchive::readPkt() Failed to create Item from archive!");
		return 0;
	}

	/* Cannot detect bad ids here anymore.... 
	 * but removed dependancy on the sslroot!
	 */

	std::string peerId = "";
	for(int i = 0; i < PQI_PEERID_LENGTH; i++)
	{
		peerId += hdr.personSig[i];
	}
	item->PeerId(peerId);

	*item_out = item;
	*ts_out   = hdr.ts;

	return 1;
}

/**** Hashing Functions ****/
std::string pqiarchive::gethash()
{
	return bio->gethash();
}
#endif

	
