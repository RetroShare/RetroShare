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

#include "util/rsdebug.h"
#include "util/rsstring.h"

// 
// #define PQISTORE_DEBUG
// 

const int pqistorezone = 9511;

pqistore::pqistore(RsSerialiser *rss, const std::string &srcId, BinInterface *bio_in, int bio_flags_in)
	:PQInterface(""), rsSerialiser(rss), bio_flags(bio_flags_in),
        nextPkt(NULL), mSrcId(srcId), bio(bio_in)
{
	pqioutput(PQL_DEBUG_ALL, pqistorezone, "pqistore::pqistore() Initialisation!");

	if (!bio_in)
	{
		pqioutput(PQL_ALERT, pqistorezone, "pqistore::pqistore() NULL bio, FATAL ERROR!");
		exit(1);
	}

	return;
}

pqistore::~pqistore()
{
	pqioutput(PQL_DEBUG_ALL, pqistorezone, "pqistore::~pqistore() Destruction!");

	if (bio_flags & BIN_FLAGS_NO_CLOSE)
	{
		pqioutput(PQL_DEBUG_ALL, pqistorezone, "pqistore::~pqistore() Not Closing BinInterface!;");
	}
	else if (bio)
	{
		pqioutput(PQL_DEBUG_ALL, pqistorezone, "pqistore::~pqistore() Deleting BinInterface!");

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
      std::string out = "pqistore::SendItem()\n";
	  si -> print(out);
	  pqioutput(PQL_DEBUG_BASIC, pqistorezone, out);
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
	pqioutput(PQL_DEBUG_ALL, pqistorezone, "pqistore::GetItem()");
#endif

	// check if this is a reading bio.
	if (!(bio_flags & BIN_FLAGS_READABLE))
	{
#ifdef PQISTORE_DEBUG
		pqioutput(PQL_DEBUG_BASIC, pqistorezone, "pqistore::GetItem() Error Not Readable");
#endif
		return NULL;
	}

	// load if we dont have a packet.
	if (!nextPkt)
	{
		if (!readPkt(&nextPkt))
		{
			pqioutput(PQL_DEBUG_BASIC, pqistorezone, "pqistore::GetItem() Failed to ReadPkt");
			return NULL;
		}
	}

	if (!nextPkt) return NULL;

	RsItem *outPkt = nextPkt;
	nextPkt = NULL;

#ifdef PQISTORE_DEBUG
	if (outPkt != NULL)
	{
		std::string out = "pqistore::GetItem() Returning:\n";
		outPkt -> print(out);
		pqioutput(PQL_DEBUG_BASIC, pqistorezone, out);
	}
#endif
	return outPkt;
}




// // PQInterface
int	pqistore::tick()
{
#ifdef PQISTORE_DEBUG
	std::cerr << "pqistore::tick()" << std::endl;
#endif
	return 0;
}

int	pqistore::status()
{
#ifdef PQISTORE_DEBUG
	pqioutput(PQL_DEBUG_ALL, pqistorezone, "pqistore::status()");
#endif
	return 0;
}

//
/**************** HANDLE OUTGOING TRANSLATION + TRANSMISSION ******/

int	pqistore::writePkt(RsItem *pqi)
{
//	std::cerr << "writePkt, pqi->peerId()=" << pqi->PeerId() << std::endl ;
#ifdef PQISTORE_DEBUG
	pqioutput(PQL_DEBUG_ALL, pqistorezone, "pqistore::writePkt()");
#endif

	uint32_t pktsize = rsSerialiser->size(pqi);
	void *ptr = malloc(pktsize);
	if (!(rsSerialiser->serialise(pqi, ptr, &pktsize)))
	{
#ifdef PQISTORE_DEBUG
		std::string out = "pqistore::writePkt() Null Pkt generated!\nCaused By:\n";
		pqi -> print(out);
		pqioutput(PQL_ALERT, pqistorezone, out);
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
		std::string out;
		rs_sprintf(out, "pqistore::writePkt() Length MisMatch: len: %u!= pktsize: %u\nCaused By:\n", len, pktsize);
		pqi -> print_string(out);
		pqioutput(PQL_ALERT, pqistorezone, out);

		free(ptr);
		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete pqi;
		return 0;
	}


	if (!(bio->cansend()))
	{
		std::string out;
		rs_sprintf(out, "pqistore::writePkt() BIO cannot write!\niscarding:\n");
		pqi -> print_string(out);
		pqioutput(PQL_ALERT, pqistorezone, out);

		free(ptr);
		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete pqi;

		return 0;
	}

#ifdef PQISTORE_DEBUG
	std::string out = "Writing Pkt Body";
#endif
	// write packet.
	if (len != (uint32_t) bio->senddata(ptr, len))
	{
#ifdef PQISTORE_DEBUG
		out += " Problems with Send Data!";
		pqioutput(PQL_ALERT, pqistorezone, out);
#endif

		free(ptr);
		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete pqi;

		return 0;
	}

#ifdef PQISTORE_DEBUG
	out += " Success!";
	pqioutput(PQL_DEBUG_BASIC, pqistorezone, out);
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
	pqioutput(PQL_DEBUG_ALL, pqistorezone, "pqistore::readPkt()");
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
		pqioutput(PQL_WARNING, pqistorezone, "pqistore::readPkt() bad read(2)");

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
			std::string out;
			rs_sprintf(out, "pqistore::readPkt() Error Completing Read (read %d/%d)", tmplen, extralen);
			pqioutput(PQL_ALERT, pqistorezone, out);

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
		pqioutput(PQL_ALERT, pqistorezone, "pqistore::readPkt() Failed to create Item from store!");
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
		{
			std::cerr << "(EE) pqiSSLstore::encryptedSendItems(): One item did not serialize. sizeItem=" << sizeItem << ". Dropping the entire file. " << std::endl;
			return false;
		}
		offset += sizeItem;

		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete *it;
	}

	bool result = true;

	if(sizeItems == offset)
		enc_bio->senddata(data, sizeItems);
	else
		result = false;

	if(data != NULL)
		delete[] data;

	return result;
}
	
bool pqiSSLstore::getEncryptedItems(std::list<RsItem* >& rsItemList)
{
	RsItem* item;

	do
	{
		if (NULL != (item = GetItem()))
			rsItemList.push_back(item);
	} while (enc_bio->isactive() && enc_bio->moretoread());

	return true;
}


RsItem *pqiSSLstore::GetItem()
{
#ifdef PQISTORE_DEBUG
	pqioutput(PQL_DEBUG_ALL, pqistorezone, "pqistore::GetItem()");
#endif

	// check if this is a reading bio.
	if (!(bio_flags & BIN_FLAGS_READABLE))
	{
		pqioutput(PQL_DEBUG_BASIC, pqistorezone, "pqistore::GetItem() Error Not Readable");
		return NULL;
	}

	// load if we dont have a packet.
	if (!nextPkt)
	{
		if (!readPkt(&nextPkt))
		{
			pqioutput(PQL_DEBUG_BASIC, pqistorezone, "pqistore::GetItem() Failed to ReadPkt");
			return NULL;
		}
	}

	if (!nextPkt) return NULL;

	RsItem *outPkt = nextPkt;
	nextPkt = NULL;

#ifdef PQISTORE_DEBUG
	if (outPkt != NULL)
	{
		std::string out;
		rs_sprintf(out, "pqistore::GetItem() Returning:\n");
		outPkt -> print(out);
		pqioutput(PQL_DEBUG_BASIC, pqistorezone, out);
	}
#endif
	return outPkt;
}


int     pqiSSLstore::readPkt(RsItem **item_out)
{
#ifdef PQISTORE_DEBUG
	pqioutput(PQL_DEBUG_ALL, pqistorezone, "pqistore::readPkt()");
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
			std::string out;
			rs_sprintf(out, "pqistore::readPkt() Error Completing Read (read %d/%d)", tmplen, extralen);
			pqioutput(PQL_ALERT, pqistorezone, out);

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
