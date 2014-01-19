/*
 * pqistore.h
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



#ifndef MRK_PQI_STORE_STREAMER_HEADER
#define MRK_PQI_STORE_STREAMER_HEADER

#include "pqi/pqibin.h"

#include <list>

/*******************************************************************
 * pqistore provides a stream to file.
 * objects only - like pqistreamer as opposed to pqiarchive.
 *
 */

class pqistore: public PQInterface
{
public:
	pqistore(RsSerialiser *rss, const std::string &srcId, BinInterface *bio_in, int bio_flagsin);
virtual ~pqistore();

// PQInterface
virtual int     SendItem(RsItem *);
virtual RsItem *GetItem();

virtual int     tick();
virtual int     status();

std::string     gethash();

bool bStopReading;

protected:

// Serialiser
RsSerialiser *rsSerialiser;
unsigned int  bio_flags; // only BIN_NO_CLOSE at the moment.

// Temp Storage for transient data.....
RsItem *nextPkt;
std::string mSrcId;

private:

	int     writePkt(RsItem *item);
	int     readPkt(RsItem **item_out);



	// Binary Interface for IO, initialisated at startup.
	BinInterface *bio;

};


/*!
 * provdes an ssl encrypted stream to file storage
 */
class pqiSSLstore: public pqistore
{

public:

	pqiSSLstore(RsSerialiser *rss, std::string srcId, BinEncryptedFileInterface *bio_in, int bio_flagsin);

	virtual ~pqiSSLstore();

	/*!
	 * send items encrypted to file using client's ssl key
	 */
	bool encryptedSendItems(const std::list<RsItem* >&);

	/*!
	 * retrieve encrypted file using client's ssl key
	 */
	bool getEncryptedItems(std::list<RsItem*>&);

private:

	RsItem *GetItem();
	int     readPkt(RsItem **item_out);

	BinEncryptedFileInterface* enc_bio;

	//bool bStopReading;

};


#endif //MRK_PQI_STORE_STREAMER_HEADER
