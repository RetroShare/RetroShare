/*
 * "$Id: pqiarchive.cc,v 1.5 2007-03-21 18:45:41 rmf24 Exp $"
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

#include "pqi/pqiarchive.h"
#include "serialiser/rsserial.h"
#include <iostream>
#include <fstream>

#include <sstream>
#include "pqi/pqidebug.h"

const int pqiarchivezone = 9326;

struct pqiarchive_header
{
	uint32_t	type;
	uint32_t	length;
	uint32_t	ts;
	uint8_t		personSig[CERTSIGNLEN];
};

const int PQIARCHIVE_TYPE_PQITEM = 0x0001;

/* PeerId of PQInterface is not important ... as we are archiving
 * packets from other people...
 */

pqiarchive::pqiarchive(RsSerialiser *rss, BinInterface *bio_in, int bio_flags_in)
	:PQInterface(""), rsSerialiser(rss), bio(bio_in), bio_flags(bio_flags_in),
        nextPkt(NULL), nextPktTS(0), firstPktTS(0), initTS(0)
{
        {
	  std::ostringstream out;
	  out << "pqiarchive::pqiarchive()";
          out << " Initialisation!" << std::endl;
	  pqioutput(PQL_DEBUG_ALL, pqiarchivezone, out.str());
	}

	if (!bio_in)
        {
	  std::ostringstream out;
	  out << "pqiarchive::pqiarchive()";
          out << " NULL bio, FATAL ERROR!" << std::endl;
	  pqioutput(PQL_ALERT, pqiarchivezone, out.str());
	  exit(1);
	}

	return;
}

pqiarchive::~pqiarchive()
{
        {
	  std::ostringstream out;
	  out << "pqiarchive::~pqiarchive()";
          out << " Destruction!" << std::endl;
	  pqioutput(PQL_DEBUG_ALL, pqiarchivezone, out.str());
	}

	if (bio_flags & BIN_FLAGS_NO_CLOSE)
	{
	  std::ostringstream out;
	  out << "pqiarchive::~pqiarchive()";
          out << " Not Closing BinInterface!" << std::endl;
	  pqioutput(PQL_DEBUG_ALL, pqiarchivezone, out.str());
	}
	else if (bio)
	{
	  std::ostringstream out;
	  out << "pqiarchive::~pqiarchive()";
          out << " Deleting BinInterface!" << std::endl;
	  pqioutput(PQL_DEBUG_ALL, pqiarchivezone, out.str());

	  delete bio;
	}

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
	  std::ostringstream out;
	  out << "pqiarchive::SendItem()" << std::endl;
	  si -> print(out);
	  pqioutput(PQL_DEBUG_BASIC, pqiarchivezone, out.str());
	}

	// check if this is a writing bio.
	
	if (!(bio_flags & BIN_FLAGS_WRITEABLE))
	{
		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete si;
		return -1;
	}

	int ret = writePkt(si);
	return ret; /* 0 - failure, 1 - success*/
}

RsItem *pqiarchive::GetItem()
{
        {
	  std::ostringstream out;
	  out << "pqiarchive::GetItem()";
	  pqioutput(PQL_DEBUG_ALL, pqiarchivezone, out.str());
	}

	// check if this is a reading bio.
	if (!(bio_flags & BIN_FLAGS_READABLE))
	{
		std::ostringstream out;
		out << "pqiarchive::GetItem()";
        	out << "Error Not Readable" << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqiarchivezone, out.str());
		return NULL;
	}

	// load if we dont have a packet.
	if (!nextPkt)
	{
		if (!readPkt(&nextPkt, &nextPktTS))
		{
			std::ostringstream out;
			out << "pqiarchive::GetItem()";
        		out << "Failed to ReadPkt" << std::endl;
			pqioutput(PQL_DEBUG_BASIC, pqiarchivezone, out.str());
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
		  std::ostringstream out;
		  out << "pqiarchive::GetItem() Returning:" << std::endl;
		  outPkt -> print(out);
		  pqioutput(PQL_DEBUG_BASIC, pqiarchivezone, out.str());
		}
		return outPkt;
	}
	return NULL;
}

// // PQInterface
int	pqiarchive::tick()
{
        {
	  std::ostringstream out;
	  out << "pqiarchive::tick()";
	  out << std::endl;
	}
	return 0;
}

int	pqiarchive::status()
{
        {
	  std::ostringstream out;
	  out << "pqiarchive::status()";
	  pqioutput(PQL_DEBUG_ALL, pqiarchivezone, out.str());
	}
	return 0;
}

//
/**************** HANDLE OUTGOING TRANSLATION + TRANSMISSION ******/

int	pqiarchive::writePkt(RsItem *pqi)
{
        {
	  std::ostringstream out;
	  out << "pqiarchive::writePkt()";
	  pqioutput(PQL_DEBUG_ALL, pqiarchivezone, out.str());
	}

	uint32_t pktsize = rsSerialiser->size(pqi);
	void *ptr = malloc(pktsize);
	if (!(rsSerialiser->serialise(pqi, ptr, &pktsize)))
	{
		std::ostringstream out;
		out << "pqiarchive::writePkt() Null Pkt generated!";
		out << std::endl;
		out << "Caused By: " << std::endl;
		pqi -> print(out);
		pqioutput(PQL_ALERT, pqiarchivezone, out.str());

		free(ptr);
		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete pqi;
		return 0;
	}

	/* extract the extra details */
	uint32_t len = getRsItemSize(ptr);
	if (len != pktsize)
	{
		std::ostringstream out;
		out << "pqiarchive::writePkt() Length MisMatch: len: " << len;
		out << " != pktsize: " << pktsize;
		out << std::endl;
		out << "Caused By: " << std::endl;
		pqi -> print(out);
		pqioutput(PQL_ALERT, pqiarchivezone, out.str());

		free(ptr);
		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete pqi;
		return 0;
	}


	if (!(bio->cansend()))
	{
		std::ostringstream out;
		out << "pqiarchive::writePkt() BIO cannot write!";
		out << std::endl;
		out << "Discarding: " << std::endl;
		pqi -> print(out);
		pqioutput(PQL_ALERT, pqiarchivezone, out.str());

		free(ptr);
		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete pqi;

		return 0;
	}

	// using the peerid from the item.
	if (pqi->PeerId().length() != PQI_PEERID_LENGTH)
	{
		std::ostringstream out;
		out << "pqiarchive::writePkt() Invalid peerId Length!";
		out << std::endl;
		out << "Caused By: " << std::endl;
		pqi -> print(out);
		pqioutput(PQL_ALERT, pqiarchivezone, out.str());

		free(ptr);
		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete pqi;
		return 0;
	}

	std::ostringstream out;
	out << "Writing Pkt Header" << std::endl;
	struct pqiarchive_header hdr;
	hdr.type 	= PQIARCHIVE_TYPE_PQITEM;
	hdr.length 	= len;
	hdr.ts 		= time(NULL);
	memcpy(hdr.personSig, pqi->PeerId().c_str(), PQI_PEERID_LENGTH);

	// write packet header.
	if (sizeof(hdr) != bio->senddata(&hdr, sizeof(hdr)))
	{
		out << "Trouble writing header!";
		out << std::endl;
	  	pqioutput(PQL_ALERT, pqiarchivezone, out.str());

		free(ptr);
		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete pqi;

		return 0;
	}

	out << "Writing Pkt Body" << std::endl;

	// write packet.
	if (len != bio->senddata(ptr, len))
	{
		out << "Problems with Send Data!";
		out << std::endl;
	  	pqioutput(PQL_ALERT, pqiarchivezone, out.str());

		free(ptr);
		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete pqi;

		return 0;
	}

	out << " Success!" << std::endl;
	pqioutput(PQL_DEBUG_BASIC, pqiarchivezone, out.str());

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
        {
	  std::ostringstream out;
	  out << "pqiarchive::readPkt()";
	  pqioutput(PQL_DEBUG_ALL, pqiarchivezone, out.str());
	}

	if ((!(bio->isactive())) || (!(bio->moretoread())))
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

	  		std::ostringstream out;
	  		out << "pqiarchive::readPkt() ";
			out << "Error Completing Read (read ";
			out << tmplen << "/" << extralen << ")" << std::endl;
	  		pqioutput(PQL_ALERT, pqiarchivezone, out.str());

			free(block);
			return 0;
		}
	}

	// create packet, based on header.
	std::cerr << "Read Data Block -> Incoming Pkt(";
	std::cerr << blen + extralen << ")" << std::endl;
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


	
