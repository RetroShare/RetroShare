/*
 * RetroShare External Interface.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#include "rpc/rpc.h"
#include "rpc/rpcserver.h"

// This one is inside libretroshare (BAD)!!!
#include "serialiser/rsbaseserial.h"

#include <iostream>

const uint32_t kMsgHeaderSize = 16;
const uint32_t kMsgMagicCode  = 0x137f0001; // Arbitary + 0x0001 

RpcMediator::RpcMediator(RpcComms *c)
  :mComms(c), mServer(NULL) 
{ 
	return; 
}

void RpcMediator::reset(uint32_t chan_id)
{
	mServer->reset(chan_id);
}

int RpcMediator::error(uint32_t chan_id, std::string msg)
{
	return mComms->error(chan_id, msg);
}


int RpcMediator::tick()
{
	bool worked = false;
	if (recv())
	{
		worked = true;
	}

	if (mServer->checkPending())
	{
		worked = true;
	}

	if (mServer->checkEvents())
	{
		worked = true;
	}

	if (worked)
		return 1;
	else
		return 0;
	return 0;
}


int RpcMediator::recv()
{
	int recvd = 0;

	std::list<uint32_t> chan_ids;
	std::list<uint32_t>::iterator it;
        mComms->active_channels(chan_ids);
	for(it = chan_ids.begin(); it != chan_ids.end(); it++)
	{
		while(recv_msg(*it))
		{
			recvd = 1;
		}
	}
	return recvd;
}


int RpcMediator::recv_msg(uint32_t chan_id)
{
	/* nothing in here needs a Mutex... */

	if (!mComms->recv_ready(chan_id))
	{
		return 0;
	}

        std::cerr << "RpcMediator::recv_msg() Data Ready";
        std::cerr << std::endl;

	/* read in data */
	uint8_t buffer[kMsgHeaderSize];
	uint32_t bufsize = kMsgHeaderSize;
	uint32_t msg_id;
	uint32_t req_id;
	uint32_t msg_size;
	std::string msg_body;

        std::cerr << "RpcMediator::recv_msg() get Header: " << bufsize;
        std::cerr << " bytes" << std::endl;

	int read = mComms->recv_blocking(chan_id, buffer, bufsize);
	if (read != bufsize)
	{
		/* error */
        	std::cerr << "RpcMediator::recv_msg() Error Reading Header: " << bufsize;
        	std::cerr << " bytes" << std::endl;

		mComms->error(chan_id, "Failed to Recv Header");
		return 0;
	}

	if (!MsgPacker::deserialiseHeader(msg_id, req_id, msg_size, buffer, bufsize))
	{
		/* error */
        	std::cerr << "RpcMediator::recv_msg() Error Deserialising Header";
        	std::cerr << std::endl;

		mComms->error(chan_id, "Failed to Deserialise Header");
		return 0;
	}

       	std::cerr << "RpcMediator::recv_msg() ChanId: " << chan_id;
	std::cerr << " MsgId: " << msg_id;
       	std::cerr << " ReqId: " << req_id;
       	std::cerr << std::endl;

       	std::cerr << "RpcMediator::recv_msg() get Body: " << msg_size;
       	std::cerr << " bytes" << std::endl;

	/* grab real size */
	read = mComms->recv_blocking(chan_id, msg_body, msg_size);
	if (read != msg_size)
	{
		/* error */
       		std::cerr << "RpcMediator::recv_msg() Error Reading Body: " << bufsize;
       		std::cerr << " bytes" << std::endl;

		mComms->error(chan_id, "Failed to Recv MsgBody");
		return 0;
	}
	mServer->processMsg(chan_id, msg_id, req_id, msg_body);

	return 1;
}


int RpcMediator::send(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg)

{
	std::cerr << "RpcMediator::send(" << msg_id << "," << req_id << ", len(";
	std::cerr << msg.size() << ")) on chan_id: " << chan_id;
	std::cerr << std::endl;

	uint8_t buffer[kMsgHeaderSize];
	uint32_t bufsize = kMsgHeaderSize;
	uint32_t msg_size = msg.size();
	
	bool okay = MsgPacker::serialiseHeader(msg_id, req_id, msg_size, buffer, bufsize);
	if (!okay)
	{
		std::cerr << "RpcMediator::send() SerialiseHeader Failed";
		std::cerr << std::endl;
		/* error */
		return 0;
	}

	if (!mComms->send(chan_id, buffer, bufsize))
	{
		std::cerr << "RpcMediator::send() Send Header Failed";
		std::cerr << std::endl;
		/* error */
		mComms->error(chan_id, "Failed to Send Header");
		return 0;
	}

	/* now send the body */
	if (!mComms->send(chan_id, msg))
	{
		std::cerr << "RpcMediator::send() Send Body Failed";
		std::cerr << std::endl;
		/* error */
		mComms->error(chan_id, "Failed to Send Msg");
		return 0;
	}
	return 1;
}




/* Msg Packing */
int MsgPacker::headersize()
{
	return kMsgHeaderSize;
}

#if 0
int MsgPacker::msgsize(Message *msg)
{
	/* */
	return 0;
}

int MsgPacker::pktsize(Message *msg)
{
	/* */
	return headersize() + msgsize();
}
#endif

bool MsgPacker::serialiseHeader(uint32_t msg_id, uint32_t req_id, uint32_t msg_size, uint8_t *buffer, uint32_t bufsize)
{
	/* check size */
	if (bufsize < kMsgHeaderSize)
	{
		return false;
	}

	/* pack the data (using libretroshare serialiser for now */
	void *data = buffer;
	uint32_t offset = 0;
	uint32_t size = bufsize;

	bool ok = true;

	/* 4 x uint32_t for header */
	ok &= setRawUInt32(data, size, &offset, kMsgMagicCode);
	ok &= setRawUInt32(data, size, &offset, msg_id);
	ok &= setRawUInt32(data, size, &offset, req_id);
	ok &= setRawUInt32(data, size, &offset, msg_size);

	return ok;
}


bool MsgPacker::deserialiseHeader(uint32_t &msg_id, uint32_t &req_id, uint32_t &msg_size, uint8_t *buffer, uint32_t bufsize)
{
	/* check size */
	if (bufsize < kMsgHeaderSize)
	{
		return false;
	}

	/* pack the data (using libretroshare serialiser for now */
	void *data = buffer;
	uint32_t offset = 0;
	uint32_t size = bufsize;
	uint32_t magic_code;

	bool ok = true;

	/* 4 x uint32_t for header */
	ok &= getRawUInt32(data, size, &offset, &magic_code);
	if (!ok)
	{
		std::cerr << "Failed to deserialise uint32_t(0)";
		std::cerr << std::endl;
	}
	ok &= getRawUInt32(data, size, &offset, &msg_id);
	if (!ok)
	{
		std::cerr << "Failed to deserialise uint32_t(1)";
		std::cerr << std::endl;
	}
	ok &= getRawUInt32(data, size, &offset, &req_id);
	if (!ok)
	{
		std::cerr << "Failed to deserialise uint32_t(2)";
		std::cerr << std::endl;
	}
	ok &= getRawUInt32(data, size, &offset, &msg_size);
	if (!ok)
	{
		std::cerr << "Failed to deserialise uint32_t(3)";
		std::cerr << std::endl;
	}

	ok &= (magic_code == kMsgMagicCode);
	if (!ok)
	{
		std::cerr << "Failed to Match MagicCode";
		std::cerr << std::endl;
	}

	return ok;
}


	
