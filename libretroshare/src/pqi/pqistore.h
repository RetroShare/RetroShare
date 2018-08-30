/*******************************************************************************
 * libretroshare/src/pqi: pqistore.h                                           *
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
	pqistore(RsSerialiser *rss, const RsPeerId&srcId, BinInterface *bio_in, int bio_flagsin);
virtual ~pqistore();

// PQInterface
virtual int     SendItem(RsItem *);
virtual RsItem *GetItem();

virtual int     tick();
virtual int     status();

RsFileHash     gethash();

bool bStopReading;

protected:

// Serialiser
RsSerialiser *rsSerialiser;
unsigned int  bio_flags; // only BIN_NO_CLOSE at the moment.

// Temp Storage for transient data.....
RsItem *nextPkt;
RsPeerId mSrcId;

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

	pqiSSLstore(RsSerialiser *rss, const RsPeerId& srcId, BinEncryptedFileInterface *bio_in, int bio_flagsin);

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
