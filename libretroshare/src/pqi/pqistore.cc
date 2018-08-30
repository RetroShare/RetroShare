/*******************************************************************************
 * libretroshare/src/pqi: pqistore.cc                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2006 by Robert Fernie.                                       *
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
 * pqistore provides an store stream.
 * This stores RsItem + Person Reference + Timestamp,
 *
 * and allows Objects to be replayed or restored, 
 * independently of the rest of the pqi system.
 *
 */

#include "rsitems/rsitem.h"
#include "pqi/pqistore.h"
#include "serialiser/rsserial.h"
#include <iostream>
#include <fstream>

#include "util/rsdebug.h"
#include "util/rsmemory.h"
#include "util/rsstring.h"

// 
// #define PQISTORE_DEBUG
// 

static struct RsLog::logInfo pqistorezoneInfo = {RsLog::Default, "pqistore"};
#define pqistorezone &pqistorezoneInfo

pqistore::pqistore(RsSerialiser *rss, const RsPeerId& srcId, BinInterface *bio_in, int bio_flags_in)
	:PQInterface(RsPeerId()), rsSerialiser(rss), bio_flags(bio_flags_in),
        nextPkt(NULL), mSrcId(srcId), bio(bio_in)
{
	pqioutput(PQL_DEBUG_ALL, pqistorezone, "pqistore::pqistore() Initialisation!");

	if (!bio_in)
	{
		pqioutput(PQL_ALERT, pqistorezone, "pqistore::pqistore() NULL bio, FATAL ERROR!");
		exit(1);
	}

	bStopReading=false;
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

	bStopReading=false;
	// check if this is a reading bio.
	if (!(bio_flags & BIN_FLAGS_READABLE))
	{
		pqioutput(PQL_DEBUG_BASIC, pqistorezone, "pqistore::GetItem() Error Not Readable");
		bStopReading=true;
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
    
    	RsTemporaryMemory ptr(pktsize) ;
        
    	if(ptr == NULL)
            return 0 ;
        
	if (!(rsSerialiser->serialise(pqi, ptr, &pktsize)))
	{
#ifdef PQISTORE_DEBUG
		std::string out = "pqistore::writePkt() Null Pkt generated!\nCaused By:\n";
		pqi -> print(out);
		pqioutput(PQL_ALERT, pqistorezone, out);
#endif

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

		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete pqi;
		return 0;
	}


	if (!(bio->cansend(0)))
	{
		std::string out;
		rs_sprintf(out, "pqistore::writePkt() BIO cannot write!\niscarding:\n");
		pqi -> print_string(out);
		pqioutput(PQL_ALERT, pqistorezone, out);

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

		if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			delete pqi;

		return 0;
	}

#ifdef PQISTORE_DEBUG
	out += " Success!";
	pqioutput(PQL_DEBUG_BASIC, pqistorezone, out);
#endif

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
	 bStopReading = false ;

	if ((!(bio->isactive())) || (!(bio->moretoread(0))))
	{
		bStopReading = true ;
		return 0;
	}

	// initial read size: basic packet.
	int blen = getRsPktBaseSize();
	void *block = rs_malloc(blen);
    
    	if(block == NULL)
            return false ;

	int tmplen;
	/* we have the header */

	// read the basic block (minimum packet size)
	if (blen != (tmplen = bio->readdata(block, blen)))
	{
		pqioutput(PQL_WARNING, pqistorezone, "pqistore::readPkt() bad read(2)");

		free(block);
		bStopReading=true;
		return 0;
	}

	// workout how much more to read.
    int blocklength = getRsItemSize(block);

	// make sure that blocklength is not a crazy number. If so, we drop the entire stream that might be corrupted.

    if(blocklength < blen || blocklength > 1024*1024*10)
	{
		std::cerr << "pqistore: ERROR: trying to realloc memory for packet of length" << blocklength <<", which is either too small, or exceeds the safety limit (10 MB)" << std::endl ;
		free(block) ;
		bStopReading=true;
		return 0 ;
	}
	int extralen = blocklength - blen;

	void *tmp = realloc(block, blocklength);

	if (tmp == NULL) 
	{
		free(block);
		std::cerr << "pqistore: ERROR: trying to realloc memory for packet of length" << blocklength << std::endl ;
		std::cerr << "Have you got enought memory?" << std::endl ;
		bStopReading=true;
		return 0 ;
	} 
	else 
		block = tmp;

	if (extralen > 0)
	{
		void *extradata = (void *) (((char *) block) + blen);

		if (extralen != (tmplen = bio->readdata(extradata, extralen)))
		{
			std::string out;
			rs_sprintf(out, "pqistore::readPkt() Error Completing Read (read %d/%d)", tmplen, extralen);
			pqioutput(PQL_ALERT, pqistorezone, out);
			bStopReading=true;

			free(block);
			return 0;
		}
	}

	// create packet, based on header.
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
RsFileHash pqistore::gethash()
{
	return bio->gethash();
}

pqiSSLstore::pqiSSLstore(RsSerialiser *rss, const RsPeerId& srcId, BinEncryptedFileInterface* bio_in, int bio_flagsin)
: pqistore(rss, srcId, bio_in, bio_flagsin), enc_bio(bio_in)
{
	bStopReading=false;
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

	for(it = rsItemList.begin(); it != rsItemList.end(); ++it)
		if(*it != NULL) sizeItems += rsSerialiser->size(*it);

	RsTemporaryMemory data( sizeItems ? sizeItems : 1 );

	for(it = rsItemList.begin(); it != rsItemList.end(); ++it)
	    if(*it != NULL)
	    {
		    sizeItem = rsSerialiser->size(*it);

		    if(rsSerialiser->serialise(*it, &data[offset],&sizeItem))
			    offset += sizeItem;
		    else
	    	{
		    std::cerr << "(EE) pqiSSLstore::encryptedSendItems(): One item did not serialize. The item is probably unknown from the serializer. Dropping the item. " << std::endl;
		    std::cerr << "Item content: " << std::endl;
		    (*it)->print(std::cerr) ;
	    	}

		    if (!(bio_flags & BIN_FLAGS_NO_DELETE))
			    delete *it;
	    }

	bool result = true;

	if(sizeItems == offset)
		enc_bio->senddata(data, sizeItems);
	else
		result = false;

	return result;
}
	
bool pqiSSLstore::getEncryptedItems(std::list<RsItem* >& rsItemList)
{
	RsItem* item;
	bStopReading=false;

	do
	{
		if (NULL != (item = GetItem()))
			rsItemList.push_back(item);

	} while (enc_bio->isactive() && enc_bio->moretoread(0) && !bStopReading);

	return true;
}


RsItem *pqiSSLstore::GetItem()
{
#ifdef PQISTORE_DEBUG
	pqioutput(PQL_DEBUG_ALL, pqistorezone, "pqiSSLstore::GetItem()");
#endif

	bStopReading=false;
	// check if this is a reading bio.
	if (!(bio_flags & BIN_FLAGS_READABLE))
	{
		pqioutput(PQL_DEBUG_BASIC, pqistorezone, "pqiSSLstore::GetItem() Error Not Readable");
		bStopReading=true;
		return NULL;
	}

	// load if we dont have a packet.
	if (!nextPkt)
	{
		if (!readPkt(&nextPkt))
		{
			pqioutput(PQL_DEBUG_BASIC, pqistorezone, "pqiSSLstore::GetItem() Failed to ReadPkt");
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
		rs_sprintf(out, "pqiSSLstore::GetItem() Returning:\n");
		outPkt -> print(out);
		pqioutput(PQL_DEBUG_BASIC, pqistorezone, out);
	}
#endif
	return outPkt;
}


int     pqiSSLstore::readPkt(RsItem **item_out)
{
	*item_out = NULL ;
#ifdef PQISTORE_DEBUG
	pqioutput(PQL_DEBUG_ALL, pqistorezone, "pqistore::readPkt()");
#endif

	bStopReading=false ;
	if ((!(enc_bio->isactive())) || (!(enc_bio->moretoread(0))))
	{
		bStopReading=true ;
		return 0;
	}

	// initial read size: basic packet.
	int blen = getRsPktBaseSize();
	void *block = rs_malloc(blen);
    
    	if(block == NULL)
            return false ;

	int tmplen;
	/* we have the header */

	// read the basic block (minimum packet size)
	if (blen != (tmplen = enc_bio->readdata(block, blen)))
	{
	  	pqioutput(PQL_WARNING, pqistorezone, "pqiSSLstore::readPkt() bad read(2)");

		free(block);
		bStopReading=true;
		return 0;
	}

	// workout how much more to read.
    int blocklength = getRsItemSize(block);

    if(blocklength < blen || blocklength > 1024*1024*10)
	{
		free(block);
		std::cerr << "pqiSSLstore: ERROR: block length has invalid value " << blocklength << " (either too small, or exceeds the safety limit of 10 MB)" << std::endl ;
		bStopReading=true;
		return 0 ;
	}
	int extralen = blocklength - blen;

	void *tmp = realloc(block, blocklength);

	if (tmp == NULL) 
	{
		free(block);
		std::cerr << "pqiSSLstore: ERROR: trying to realloc memory for packet of length" << extralen+blen << std::endl ;
		std::cerr << "Have you got enought memory?" << std::endl ;
		bStopReading=true;
		return 0 ;
	} 
	else 
		block = tmp;

	if (extralen > 0)
	{
		void *extradata = (void *) (((char *) block) + blen);

		if (extralen != (tmplen = enc_bio->readdata(extradata, extralen)))
		{
			std::string out;
			rs_sprintf(out, "pqiSSLstore::readPkt() Error Completing Read (read %d/%d)", tmplen, extralen);
			pqioutput(PQL_ALERT, pqistorezone, out);

			free(block);
			bStopReading=true;
			return 0;
		}
	}

	// create packet, based on header.
	uint32_t readbytes = extralen + blen;

	RsItem *item = rsSerialiser->deserialise(block, &readbytes);
	free(block);

	if (item == NULL)
	{
		pqioutput(PQL_ALERT, pqistorezone, "pqiSSLstore::readPkt() Failed to create Item from store!");
		return 0;
	}

	item->PeerId(mSrcId);
	*item_out = item;
	return 1;
}

