/*
 * libretroshare/src/ft: ftdata.cc
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

#include "ftdata_dummy.h"

/******* Pair of Send/Recv (Only need to handle Send side) ******/
ftDataSendPair::ftDataSendPair(ftDataRecv *recv)
	:mDataRecv(recv) 
{ 
	return; 
}

	/* Client Send */
bool	ftDataSendPair::sendDataRequest(const std::string &peerId, const std::string &hash, 
			uint64_t size, uint64_t offset, uint32_t chunksize)
{
	return mDataRecv->recvDataRequest(peerId,hash,size,offset,chunksize);
}

	/* Server Send */
bool	ftDataSendPair::sendData(const std::string &peerId, const std::string &hash, 
		uint64_t size, uint64_t offset, uint32_t chunksize, void *data)
{
	return mDataRecv->recvData(peerId, hash,size,offset,chunksize,data);
}

        /* Send a request for a chunk map */
bool	ftDataSendPair::sendChunkMapRequest(const std::string& peer_id,const std::string& hash,bool is_client)
{
	return 	mDataRecv->recvChunkMapRequest(peer_id,hash,is_client);
}

        /* Send a chunk map */
bool    ftDataSendPair::sendChunkMap(const std::string& peer_id,const std::string& hash, const CompressedChunkMap& cmap,bool is_client)
{
	return mDataRecv->recvChunkMap(peer_id,hash,cmap, is_client);
}
        /* Send a request for a chunk map */
bool	ftDataSendPair::sendCRC32MapRequest(const std::string& peer_id,const std::string& hash)
{
	return 	mDataRecv->recvCRC32MapRequest(peer_id,hash);
}

        /* Send a chunk map */
bool    ftDataSendPair::sendCRC32Map(const std::string& peer_id,const std::string& hash, const CRC32Map& crcmap)
{
	return mDataRecv->recvCRC32Map(peer_id,hash,crcmap) ;
}

bool ftDataSendPair::sendSingleChunkCRCRequest(const std::string& peer_id, const std::string& hash, unsigned int c_id)
{
	return mDataRecv->recvSingleChunkCRCRequest(peer_id,hash,c_id) ;
}
bool ftDataSendPair::sendSingleChunkCRC(const std::string& peer_id, const std::string& hash, uint32_t c_id, const Sha1CheckSum& crc)
{
	return mDataRecv->recvSingleChunkCRC(peer_id,hash,c_id,crc) ;
}
	/* Client Send */
bool	ftDataSendDummy::sendDataRequest(const std::string &/*peerId*/, const std::string &/*hash*/,
			uint64_t /*size*/, uint64_t /*offset*/, uint32_t /*chunksize*/)
{
	return true;
}

	/* Server Send */
bool	ftDataSendDummy::sendData(const std::string &/*peerId*/, const std::string &/*hash*/,
			uint64_t /*size*/, uint64_t /*offset*/, uint32_t /*chunksize*/, void */*data*/)
{
	return true;
}


        /* Send a request for a chunk map */
bool	ftDataSendDummy::sendChunkMapRequest(const std::string& /*peer_id*/,const std::string& /*hash*/,bool /*is_client*/)
{
	return true;
}

        /* Send a chunk map */
bool    ftDataSendDummy::sendChunkMap(const std::string& /*peer_id*/,const std::string& /*hash*/, const CompressedChunkMap& /*cmap*/,bool /*is_client*/)
{
	return true;
}
bool	ftDataSendDummy::sendCRC32MapRequest(const std::string& /*peer_id*/,const std::string& /*hash*/)
{
	return true;
}

        /* Send a chunk map */
bool    ftDataSendDummy::sendCRC32Map(const std::string& /*peer_id*/,const std::string& /*hash*/, const CRC32Map& /*cmap*/)
{
	return true;
}

	/* Client Recv */
bool	ftDataRecvDummy::recvData(const std::string &/*peerId*/, const std::string &/*hash*/,
			uint64_t /*size*/, uint64_t /*offset*/, uint32_t /*chunksize*/, void */*data*/)
{
	return true;
}


	/* Server Recv */
bool	ftDataRecvDummy::recvDataRequest(const std::string &/*peerId*/, const std::string &/*hash*/,
			uint64_t /*size*/, uint64_t /*offset*/, uint32_t /*chunksize*/)
{
	return true;
}

        /* Send a request for a chunk map */
bool 	ftDataRecvDummy::recvChunkMapRequest(const std::string& /*peer_id*/,const std::string& /*hash*/,
                bool /*is_client*/)
{
	return true;
}

        /* Send a chunk map */
bool 	ftDataRecvDummy::recvChunkMap(const std::string& /*peer_id*/,const std::string& /*hash*/,
                const CompressedChunkMap& /*cmap*/,bool /*is_client*/)
{
	return true;
}
bool ftDataRecvDummy::recvSingleChunkCrcRequest(const std::string& peer_id,const std::string& hash,uint32_t chunk_id) 
{
	return true ;
}
bool ftDataRecvDummy::recvSingleChunkCrc(const std::string& peer_id,const std::string& hash,uint32_t chunk_id,const Sha1CheckSum& sum) 
{
	return true ;
}

bool 	ftDataRecvDummy::recvCRC32MapRequest(const std::string& /*peer_id*/,const std::string& /*hash*/)
{
	return true ;
}

	/* Send a chunk map */
bool 	ftDataRecvDummy::recvCRC32Map(const std::string& /*peer_id*/,const std::string& /*hash*/, const CompressedChunkMap& /*cmap*/)
{
	return true ; 
}

bool ftDataSendDummy::sendSingleChunkCRCRequest(const std::string&, const std::string&, unsigned int)
{
	return true ;
}
bool ftDataSendDummy::sendSingleChunkCRC(const std::string&, const std::string&, uint32_t, const Sha1CheckSum&)
{
	return true ;
}
