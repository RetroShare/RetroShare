/*
 * "$Id: pqistore.cc,v 1.5 2007-03-21 18:45:41 rmf24 Exp $"
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
 * pqistore provides an store stream.
 * This stores RsItem + Person Reference + Timestamp,
 *
 * and allows Objects to be replayed or restored, 
 * independently of the rest of the pqi system.
 *
 */

#include "pqi/pqistore.h"
#include "serialiser/rsserial.h"
#include <iostream>
#include <fstream>

#include <sstream>
#include "util/rsdebug.h"

const int pqistorezone = 9511;

pqistore::pqistore(RsSerialiser *rss, BinInterface *bio_in, int bio_flags_in)
	:PQInterface(""), rsSerialiser(rss), bio(bio_in), bio_flags(bio_flags_in),
        nextPkt(NULL)
{
        {
	  std::ostringstream out;
	  out << "pqistore::pqistore()";
          out << " Initialisation!" << std::endl;
	  pqioutput(PQL_DEBUG_ALL, pqistorezone, out.str());
	}

	if (!bio_in)
        {
	  std::ostringstream out;
	  out << "pqistore::pqistore()";
          out << " NULL bio, FATAL ERROR!" << std::endl;
	  pqioutput(PQL_ALERT, pqistorezone, out.str());
	  exit(1);
	}

	return;
}

pqistore::~pqistore()
{
        {
	  std::ostringstream out;
	  out << "pqistore::~pqistore()";
          out << " Destruction!" << std::endl;
	  pqioutput(PQL_DEBUG_ALL, pqistorezone, out.str());
	}

	if (bio_flags & BIN_FLAGS_NO_CLOSE)
	{
	  std::ostringstream out;
	  out << "pqistore::~pqistore()";
          out << " Not Closing BinInterface!" << std::endl;
	  pqioutput(PQL_DEBUG_ALL, pqistorezone, out.str());
	}
	else if (bio)
	{
	  std::ostringstream out;
	  out << "pqistore::~pqistore()";
          out << " Deleting BinInterface!" << std::endl;
	  pqioutput(PQL_DEBUG_ALL, pqistorezone, out.str());

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
int	pqistore::SendItem(RsItem *si)
{
        {
	  std::ostringstream out;
	  out << "pqistore::SendItem()" << std::endl;
	  si -> print(out);
	  pqioutput(PQL_DEBUG_BASIC, pqistorezone, out.str());
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

RsItem *pqistore::GetItem()
{
        {
	  std::ostringstream out;
	  out << "pqistore::GetItem()";
	  pqioutput(PQL_DEBUG_ALL, pqistorezone, out.str());
	}

	// check if this is a reading bio.
	if (!(bio_flags & BIN_FLAGS_READABLE))
	{
		std::ostringstream out;
		out << "pqistore::GetItem()";
        	out << "Error Not Readable" << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqistorezone, out.str());
		return NULL;
	}

	// load if we dont have a packet.
	if (!nextPkt)
	{
		if (!readPkt(&nextPkt))
		{
			std::ostringstream out;
			out << "pqistore::GetItem()";
        		out << "Failed to ReadPkt" << std::endl;
			pqioutput(PQL_DEBUG_BASIC, pqistorezone, out.str());
			return NULL;
		}
	}

	if (!nextPkt) return NULL;

	RsItem *outPkt = nextPkt;
	nextPkt = NULL;

	if (outPkt != NULL)
        {
	  std::ostringstream out;
	  out << "pqistore::GetItem() Returning:" << std::endl;
	  outPkt -> print(out);
	  pqioutput(PQL_DEBUG_BASIC, pqistorezone, out.str());
	}
	return outPkt;
}

// // PQInterface
int	pqistore::tick()
{
        {
	  std::ostringstream out;
	  out << "pqistore::tick()";
	  out << std::endl;
	}
	return 0;
}

int	pqistore::status()
{
        {
	  std::ostringstream out;
	  out << "pqistore::status()";
	  pqioutput(PQL_DEBUG_ALL, pqistorezone, out.str());
	}
	return 0;
}

//
/**************** HANDLE OUTGOING TRANSLATION + TRANSMISSION ******/

int	pqistore::writePkt(RsItem *pqi)
{
//	std::cerr << "writePkt, pqi->peerId()=" << pqi->PeerId() << std::endl ;
        {
	  std::ostringstream out;
	  out << "pqistore::writePkt()";
	  pqioutput(PQL_DEBUG_ALL, pqistorezone, out.str());
	}

	uint32_t pktsize = rsSerialiser->size(pqi);
	void *ptr = malloc(pktsize);
	if (!(rsSerialiser->serialise(pqi, ptr, &pktsize)))
	{
		std::ostringstream out;
		out << "pqistore::writePkt() Null Pkt generated!";
		out << std::endl;
		out << "Caused By: " << std::endl;
		pqi -> print(out);
		pqioutput(PQL_ALERT, pqistorezone, out.str());

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
		out << "pqistore::writePkt() Length MisMatch: len: " << len;
		out << " != pktsize: " << pktsize;
		out << std::endl;
		out << "Caused By: " << std::endl;
		pqi -> print(out);
		pqioutput(PQL_ALERT, pqistorezone, out.str());

		free(ptr);
		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete pqi;
		return 0;
	}


	if (!(bio->cansend()))
	{
		std::ostringstream out;
		out << "pqistore::writePkt() BIO cannot write!";
		out << std::endl;
		out << "Discarding: " << std::endl;
		pqi -> print(out);
		pqioutput(PQL_ALERT, pqistorezone, out.str());

		free(ptr);
		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete pqi;

		return 0;
	}

	std::ostringstream out;
	out << "Writing Pkt Body" << std::endl;
	// write packet.
	if (len != bio->senddata(ptr, len))
	{
		out << "Problems with Send Data!";
		out << std::endl;
	  	pqioutput(PQL_ALERT, pqistorezone, out.str());

		free(ptr);
		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete pqi;

		return 0;
	}

	out << " Success!" << std::endl;
	pqioutput(PQL_DEBUG_BASIC, pqistorezone, out.str());

	free(ptr);
	if (!(bio_flags & BIN_FLAGS_NO_DELETE))
		delete pqi;

	return 1;
}

/* Reads a single packet from the input stream
 * gets the timestamp as well.
 */

int     pqistore::readPkt(RsItem **item_out)
{
        {
	  std::ostringstream out;
	  out << "pqistore::readPkt()";
	  pqioutput(PQL_DEBUG_ALL, pqistorezone, out.str());
	}

	if ((!(bio->isactive())) || (!(bio->moretoread())))
	{
		return 0;
	}

	// enough space to read any packet.
	int maxlen = getRsPktMaxSize();
	void *block = malloc(maxlen);

	// initial read size: basic packet.
	int blen = getRsPktBaseSize();

	int tmplen;
	/* we have the header */

	// read the basic block (minimum packet size)
	if (blen != (tmplen = bio->readdata(block, blen)))
	{
	  	pqioutput(PQL_WARNING, pqistorezone, 
	  		"pqistore::readPkt() bad read(2)");

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
	  		out << "pqistore::readPkt() ";
			out << "Error Completing Read (read ";
			out << tmplen << "/" << extralen << ")" << std::endl;
	  		pqioutput(PQL_ALERT, pqistorezone, out.str());

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
	  	pqioutput(PQL_ALERT, pqistorezone, 
		  "pqistore::readPkt() Failed to create Item from store!");
		return 0;
	}

	*item_out = item;
	return 1;
}

/**** Hashing Functions ****/
std::string pqistore::gethash()
{
	return bio->gethash();
}


	
