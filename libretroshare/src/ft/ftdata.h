/*******************************************************************************
 * libretroshare/src/ft: ftdata.h                                              *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2008 by Robert Fernie <drbob@lunamutt.com>                        *
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

class ftDataSend
{
	public:
		virtual ~ftDataSend() { return; }

		/* Client Send */
        virtual bool    sendDataRequest(const RsPeerId& peerId, const RsFileHash& hash, uint64_t size, uint64_t offset, uint32_t chunksize) = 0;

		/* Server Send */
        virtual bool    sendData(const RsPeerId& peerId, const RsFileHash& hash, uint64_t size, uint64_t offset, uint32_t chunksize, void *data) = 0;

		/// Send a chunkmap[request]. Because requests/chunkmaps can go both
		//directions, but for different usages, we have this "is_client" flags,
		//that gives the ultimate goal of the data. "is_client==true" means that
		//the message is for a client (download) instead of a server.
		//
        virtual bool sendChunkMapRequest(const RsPeerId& peer_id,const RsFileHash& hash,bool is_client) = 0;
        virtual bool sendChunkMap(const RsPeerId& peer_id,const RsFileHash& hash,const CompressedChunkMap& cmap,bool is_client) = 0;

		/// Send a request for a chunk crc map
        virtual bool sendSingleChunkCRCRequest(const RsPeerId& peer_id,const RsFileHash& hash,uint32_t chunk_number) = 0;
		/// Send a chunk crc map
        virtual bool sendSingleChunkCRC(const RsPeerId& peer_id,const RsFileHash& hash,uint32_t chunk_number,const Sha1CheckSum& crc) = 0;
};



	/*************** RECV INTERFACE *******************/

class ftDataRecv
{
	public:
		virtual ~ftDataRecv() { return; }

		/* Client Recv */
        virtual bool    recvData(const RsPeerId& peerId, const RsFileHash& hash, uint64_t size, uint64_t offset, uint32_t chunksize, void *data) = 0;

		/* Server Recv */
        virtual bool    recvDataRequest(const RsPeerId& peerId, const RsFileHash& hash, uint64_t size, uint64_t offset, uint32_t chunksize) = 0;

		/// Send a request for a chunk map
        virtual bool recvChunkMapRequest(const RsPeerId& peer_id,const RsFileHash& hash,bool is_client) = 0;

		/// Send a chunk map
        virtual bool recvChunkMap(const RsPeerId& peer_id,const RsFileHash& hash,const CompressedChunkMap& cmap,bool is_client) = 0;

        virtual bool recvSingleChunkCRCRequest(const RsPeerId& peer_id,const RsFileHash& hash,uint32_t chunk_id) = 0;
        virtual bool recvSingleChunkCRC(const RsPeerId& peer_id,const RsFileHash& hash,uint32_t chunk_id,const Sha1CheckSum& sum) = 0;

};

#endif
