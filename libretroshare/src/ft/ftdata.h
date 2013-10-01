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

#ifndef FT_DATA_INTERFACE_HEADER
#define FT_DATA_INTERFACE_HEADER

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

	/*************** SEND INTERFACE *******************/

class CompressedChunkMap ;
class Sha1CheckSum ;

class ftDataSend
{
	public:
		virtual ~ftDataSend() { return; }

		/* Client Send */
		virtual bool    sendDataRequest(const std::string& peerId, const std::string& hash, uint64_t size, uint64_t offset, uint32_t chunksize) = 0;

		/* Server Send */
		virtual bool    sendData(const std::string& peerId, const std::string& hash, uint64_t size, uint64_t offset, uint32_t chunksize, void *data) = 0;

		/// Send a chunkmap[request]. Because requests/chunkmaps can go both
		//directions, but for different usages, we have this "is_client" flags,
		//that gives the ultimate goal of the data. "is_client==true" means that
		//the message is for a client (download) instead of a server.
		//
		virtual bool sendChunkMapRequest(const std::string& peer_id,const std::string& hash,bool is_client) = 0;
		virtual bool sendChunkMap(const std::string& peer_id,const std::string& hash,const CompressedChunkMap& cmap,bool is_client) = 0;

		/// Send a request for a chunk crc map
		virtual bool sendSingleChunkCRCRequest(const std::string& peer_id,const std::string& hash,uint32_t chunk_number) = 0;
		/// Send a chunk crc map
		virtual bool sendSingleChunkCRC(const std::string& peer_id,const std::string& hash,uint32_t chunk_number,const Sha1CheckSum& crc) = 0;
};



	/*************** RECV INTERFACE *******************/

class ftDataRecv
{
	public:
		virtual ~ftDataRecv() { return; }

		/* Client Recv */
		virtual bool    recvData(const std::string& peerId, const std::string& hash, uint64_t size, uint64_t offset, uint32_t chunksize, void *data) = 0;

		/* Server Recv */
		virtual bool    recvDataRequest(const std::string& peerId, const std::string& hash, uint64_t size, uint64_t offset, uint32_t chunksize) = 0;

		/// Send a request for a chunk map
		virtual bool recvChunkMapRequest(const std::string& peer_id,const std::string& hash,bool is_client) = 0;

		/// Send a chunk map
		virtual bool recvChunkMap(const std::string& peer_id,const std::string& hash,const CompressedChunkMap& cmap,bool is_client) = 0;

		virtual bool recvSingleChunkCRCRequest(const std::string& peer_id,const std::string& hash,uint32_t chunk_id) = 0;
		virtual bool recvSingleChunkCRC(const std::string& peer_id,const std::string& hash,uint32_t chunk_id,const Sha1CheckSum& sum) = 0;

};

#endif
