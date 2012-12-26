/*
 * libretroshare/src/ft: ftdata.h
 *
 * File Transfer for RetroShare.
 *
 * Copyright 2008 by Robert Fernie.
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

#ifndef FT_DATA_TEST_INTERFACE_HEADER
#define FT_DATA_TEST_INTERFACE_HEADER

/* 
 * ftData.
 *
 * Internal Interfaces for sending and receiving data.
 * Most likely to be implemented by ftServer.
 * Provided as an independent interface for testing purposes.
 *
 */

#include <string>
#include <inttypes.h>

#include <retroshare/rstypes.h>
#include <ft/ftdata.h>

	/*************** SEND INTERFACE *******************/

class CompressedChunkMap ;
class Sha1CheckSum ;

	/**************** FOR TESTING ***********************/

/******* Pair of Send/Recv (Only need to handle Send side) ******/
class ftDataSendPair: public ftDataSend
{
	public:

	ftDataSendPair(ftDataRecv *recv);
virtual ~ftDataSendPair() { return; }

	/* Client Send */
virtual bool    sendDataRequest(const std::string &peerId, const std::string &hash, 
			uint64_t size, uint64_t offset, uint32_t chunksize);

	/* Server Send */
virtual bool    sendData(const std::string &peerId, const std::string &hash, uint64_t size,
                        uint64_t offset, uint32_t chunksize, void *data);

	/* Send a request for a chunk map */
virtual bool 	sendChunkMapRequest(const std::string& peer_id,const std::string& hash,bool is_client);

	/* Send a chunk map */
virtual bool 	sendChunkMap(const std::string& peer_id,const std::string& hash, const CompressedChunkMap& cmap,bool is_client);

	/* Send a request for a chunk map */
virtual bool 	sendCRC32MapRequest(const std::string& peer_id,const std::string& hash);

	/* Send a chunk map */
virtual bool 	sendCRC32Map(const std::string& peer_id,const std::string& hash, const CRC32Map& cmap);
	ftDataRecv *mDataRecv;

	virtual bool sendSingleChunkCRCRequest(const std::string&, const std::string&, uint32_t);
	virtual bool sendSingleChunkCRC(const std::string&, const std::string&, uint32_t, const Sha1CheckSum&);
};


class ftDataSendDummy: public ftDataSend
{
	public:
		virtual ~ftDataSendDummy() { return; }

		/* Client Send */
		virtual bool    sendDataRequest(const std::string &peerId, const std::string &hash, 
				uint64_t size, uint64_t offset, uint32_t chunksize);

		/* Server Send */
		virtual bool    sendData(const std::string &peerId, const std::string &hash, uint64_t size,
				uint64_t offset, uint32_t chunksize, void *data);

		/* Send a request for a chunk map */
		virtual bool 	sendChunkMapRequest(const std::string& peer_id,const std::string& hash,bool is_client);

		/* Send a chunk map */
		virtual bool 	sendChunkMap(const std::string& peer_id,const std::string& hash, const CompressedChunkMap& cmap,bool is_client);

		/* Send a request for a chunk map */
		virtual bool 	sendCRC32MapRequest(const std::string& peer_id,const std::string& hash);

		/* Send a chunk map */
		virtual bool 	sendCRC32Map(const std::string& peer_id,const std::string& hash, const CRC32Map& cmap);

		virtual bool sendSingleChunkCRCRequest(const std::string&, const std::string&, uint32_t);
		virtual bool sendSingleChunkCRC(const std::string&, const std::string&, uint32_t, const Sha1CheckSum&);

};

class ftDataRecvDummy: public ftDataRecv
{
	public:

virtual ~ftDataRecvDummy() { return; }

	/* Client Recv */
virtual bool    recvData(const std::string& peerId, const std::string& hash, 
		uint64_t size, uint64_t offset, uint32_t chunksize, void *data);

	/* Server Recv */
virtual bool    recvDataRequest(const std::string& peerId, const std::string& hash, 
		uint64_t size, uint64_t offset, uint32_t chunksize);

	/* Send a request for a chunk map */
virtual bool recvChunkMapRequest(const std::string& peer_id,const std::string& hash,
		bool is_client);

	/* Send a chunk map */
virtual bool recvChunkMap(const std::string& peer_id,const std::string& hash,
		const CompressedChunkMap& cmap,bool is_client);

	/* Send a request for a chunk map */
virtual bool 	recvCRC32MapRequest(const std::string& peer_id,const std::string& hash);

	/* Send a chunk map */
virtual bool 	recvCRC32Map(const std::string& peer_id,const std::string& hash, const CompressedChunkMap& cmap);

virtual bool recvSingleChunkCrcRequest(const std::string& peer_id,const std::string& hash,uint32_t chunk_id) ;
virtual bool recvSingleChunkCrc(const std::string& peer_id,const std::string& hash,uint32_t chunk_id,const Sha1CheckSum& sum);
};

#endif
