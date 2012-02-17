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

// 
// #define PQISTORE_DEBUG
// 

const int pqistorezone = 9511;

pqistore::pqistore(RsSerialiser *rss, const std::string &srcId, BinInterface *bio_in, int bio_flags_in)
	:PQInterface(""), rsSerialiser(rss), bio_flags(bio_flags_in),
        nextPkt(NULL), mSrcId(srcId), bio(bio_in)
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
#ifdef PQISTORE_DEBUG
        {
	  std::ostringstream out;
	  out << "pqistore::SendItem()" << std::endl;
	  si -> print(out);
	  pqioutput(PQL_DEBUG_BASIC, pqistorezone, out.str());
	}
#endif

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
#ifdef PQISTORE_DEBUG
        {
	  std::ostringstream out;
	  out << "pqistore::GetItem()";
	  pqioutput(PQL_DEBUG_ALL, pqistorezone, out.str());
	}
#endif

	// check if this is a reading bio.
	if (!(bio_flags & BIN_FLAGS_READABLE))
	{
#ifdef PQISTORE_DEBUG
		std::ostringstream out;
		out << "pqistore::GetItem()";
        	out << "Error Not Readable" << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqistorezone, out.str());
#endif
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

#ifdef PQISTORE_DEBUG
	if (outPkt != NULL)
	{
		std::ostringstream out;
		out << "pqistore::GetItem() Returning:" << std::endl;
		outPkt -> print(out);
		pqioutput(PQL_DEBUG_BASIC, pqistorezone, out.str());
	}
#endif
	return outPkt;
}




// // PQInterface
int	pqistore::tick()
{
#ifdef PQISTORE_DEBUG
        {
	  std::ostringstream out;
	  out << "pqistore::tick()";
	  out << std::endl;
	}
#endif
	return 0;
}

int	pqistore::status()
{
#ifdef PQISTORE_DEBUG
        {
	  std::ostringstream out;
	  out << "pqistore::status()";
	  pqioutput(PQL_DEBUG_ALL, pqistorezone, out.str());
	}
#endif
	return 0;
}

//
/**************** HANDLE OUTGOING TRANSLATION + TRANSMISSION ******/

int	pqistore::writePkt(RsItem *pqi)
{
//	std::cerr << "writePkt, pqi->peerId()=" << pqi->PeerId() << std::endl ;
#ifdef PQISTORE_DEBUG
        {
	  std::ostringstream out;
	  out << "pqistore::writePkt()";
	  pqioutput(PQL_DEBUG_ALL, pqistorezone, out.str());
	}
#endif

	uint32_t pktsize = rsSerialiser->size(pqi);
	void *ptr = malloc(pktsize);
	if (!(rsSerialiser->serialise(pqi, ptr, &pktsize)))
	{
#ifdef PQISTORE_DEBUG
		std::ostringstream out;
		out << "pqistore::writePkt() Null Pkt generated!";
		out << std::endl;
		out << "Caused By: " << std::endl;
		pqi -> print(out);
		pqioutput(PQL_ALERT, pqistorezone, out.str());
#endif

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

#ifdef PQISTORE_DEBUG
	std::ostringstream out;
	out << "Writing Pkt Body" << std::endl;
#endif
	// write packet.
	if (len != (uint32_t) bio->senddata(ptr, len))
	{
#ifdef PQISTORE_DEBUG
		out << "Problems with Send Data!";
		out << std::endl;
	  	pqioutput(PQL_ALERT, pqistorezone, out.str());
#endif

		free(ptr);
		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete pqi;

		return 0;
	}

#ifdef PQISTORE_DEBUG
	out << " Success!" << std::endl;
	pqioutput(PQL_DEBUG_BASIC, pqistorezone, out.str());
#endif

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
#ifdef PQISTORE_DEBUG
        {
	  std::ostringstream out;
	  out << "pqistore::readPkt()";
	  pqioutput(PQL_DEBUG_ALL, pqistorezone, out.str());
	}
#endif

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

	if(extralen+blen > maxlen)
		std::cerr << "***** ERROR: trying to read a packet of length " << extralen+blen << ", while the maximum length is " << maxlen << std::endl ;

	if (extralen > 0)
	{
	   if(extralen > blen + maxlen)
	   {
		  std::cerr << "pqistore: ERROR: Inconsistency in packet format (extralen=" << extralen << ", maxlen=" << maxlen << "). Wasting the whole file." << std::endl ;
		  free(block) ;
		  return 0 ;
	   }

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
        //std::cerr << "Read Data Block -> Incoming Pkt(";
        //std::cerr << blen + extralen << ")" << std::endl;
	uint32_t readbytes = extralen + blen;

	RsItem *item = rsSerialiser->deserialise(block, &readbytes);
	free(block);

	if (item == NULL)
	{
	  	pqioutput(PQL_ALERT, pqistorezone, 
		  "pqistore::readPkt() Failed to create Item from store!");
		return 0;
	}

	item->PeerId(mSrcId);
	*item_out = item;
	return 1;
}

/**** Hashing Functions ****/
std::string pqistore::gethash()
{
	return bio->gethash();
}

pqiSSLstore::pqiSSLstore(RsSerialiser *rss, std::string srcId, BinEncryptedFileInterface* bio_in, int bio_flagsin)
: pqistore(rss, srcId, bio_in, bio_flagsin), enc_bio(bio_in)
{
	return;
}

pqiSSLstore::~pqiSSLstore()
{
	// no need to delete member enc_bio, as it is deleted by the parent class.
	return;
}

bool pqiSSLstore::encryptedSendItems(const std::list<RsItem*>& rsItemList)
{

	std::list<RsItem*>::const_iterator it;
	uint32_t sizeItems = 0, sizeItem = 0;
	uint32_t offset = 0;
	char* data = NULL;

	for(it = rsItemList.begin(); it != rsItemList.end(); it++)
		sizeItems += rsSerialiser->size(*it);

	data = new char[sizeItems];

	for(it = rsItemList.begin(); it != rsItemList.end(); it++)
	{
		sizeItem = rsSerialiser->size(*it);
		if(!rsSerialiser->serialise(*it, (data+offset),&sizeItem))
			return false;
		offset += sizeItem;

		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete *it;
	}

	if(sizeItems == offset)
		enc_bio->senddata(data, sizeItems);
	else
		return false;

	if(data != NULL)
		delete[] data;

	return true;
}
	
bool pqiSSLstore::getEncryptedItems(std::list<RsItem* >& rsItemList)
{
	RsItem* item;

	while(NULL != (item = GetItem()))
	{
		rsItemList.push_back(item);
	}

	return true;
}


RsItem *pqiSSLstore::GetItem()
{
#ifdef PQISTORE_DEBUG
        {
	  std::ostringstream out;
	  out << "pqistore::GetItem()";
	  pqioutput(PQL_DEBUG_ALL, pqistorezone, out.str());
	}
#endif

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

#ifdef PQISTORE_DEBUG
	if (outPkt != NULL)
        {
	  std::ostringstream out;
	  out << "pqistore::GetItem() Returning:" << std::endl;
	  outPkt -> print(out);
	  pqioutput(PQL_DEBUG_BASIC, pqistorezone, out.str());
	}
#endif
	return outPkt;
}


int     pqiSSLstore::readPkt(RsItem **item_out)
{
#ifdef PQISTORE_DEBUG
        {
	  std::ostringstream out;
	  out << "pqistore::readPkt()";
	  pqioutput(PQL_DEBUG_ALL, pqistorezone, out.str());
	}
#endif

	if ((!(enc_bio->isactive())) || (!(enc_bio->moretoread())))
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
	if (blen != (tmplen = enc_bio->readdata(block, blen)))
	{
	  	pqioutput(PQL_WARNING, pqistorezone,
	  		"pqistore::readPkt() bad read(2)");

		free(block);
		return 0;
	}

	// workout how much more to read.
	int extralen = getRsItemSize(block) - blen;

	if(extralen+blen > maxlen)
		std::cerr << "***** ERROR: trying to read a packet of length " << extralen+blen << ", while the maximum length is " << maxlen << std::endl ;

	if (extralen > 0)
	{
	   if(extralen > blen + maxlen)
	   {
		  std::cerr << "pqistore: ERROR: Inconsistency in packet format (extralen=" << extralen << ", maxlen=" << maxlen << "). Wasting the whole file." << std::endl ;
		  free(block) ;
		  return 0 ;
	   }

		void *extradata = (void *) (((char *) block) + blen);

		if (extralen != (tmplen = enc_bio->readdata(extradata, extralen)))
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
        //std::cerr << "Read Data Block -> Incoming Pkt(";
        //std::cerr << blen + extralen << ")" << std::endl;
	uint32_t readbytes = extralen + blen;

	RsItem *item = rsSerialiser->deserialise(block, &readbytes);
	free(block);

	if (item == NULL)
	{
#ifdef PQISTORE_DEBUG
		pqioutput(PQL_ALERT, pqistorezone,
		  "pqistore::readPkt() Failed to create Item from store!");
#endif
		return 0;
	}

	item->PeerId(mSrcId);
	*item_out = item;
	return 1;
}
