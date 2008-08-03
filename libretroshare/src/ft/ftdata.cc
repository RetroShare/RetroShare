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

#include "ft/ftdata.h"

/******* Pair of Send/Recv (Only need to handle Send side) ******/
ftDataSendPair::ftDataSendPair(ftDataRecv *recv)
	:mDataRecv(recv) 
{ 
	return; 
}

	/* Client Send */
bool	ftDataSendPair::sendDataRequest(std::string peerId, std::string hash, 
			uint64_t size, uint64_t offset, uint32_t chunksize)
{
	return mDataRecv->recvDataRequest(peerId,hash,size,offset,chunksize);
}

	/* Server Send */
bool	ftDataSendPair::sendData(std::string peerId, 
			std::string hash, uint64_t size,
                        uint64_t offset, uint32_t chunksize, void *data)
{
	return mDataRecv->recvData(peerId, hash,size,offset,chunksize,data);
}


	/* Client Send */
bool	ftDataSendDummy::sendDataRequest(std::string peerId, std::string hash, 
			uint64_t size, uint64_t offset, uint32_t chunksize)
{
	return true;
}

	/* Server Send */
bool	ftDataSendDummy::sendData(std::string peerId, 
			std::string hash, uint64_t size,
                        uint64_t offset, uint32_t chunksize, void *data)
{
	return true;
}


	/* Client Recv */
bool	ftDataRecvDummy::recvData(std::string peerId, 
			std::string hash, uint64_t size, 
			uint64_t offset, uint32_t chunksize, void *data)
{
	return true;
}


	/* Server Recv */
bool	ftDataRecvDummy::recvDataRequest(std::string peerId, std::string hash, 
			uint64_t size, uint64_t offset, uint32_t chunksize)
{
	return true;
}

