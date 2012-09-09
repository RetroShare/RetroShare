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

#ifndef RPC_MEDIATOR_H
#define RPC_MEDIATOR_H

/* 
 * Interface between RpcServer and RpcComms.
 */

#include <string>
#include <inttypes.h>

#include "rpcsystem.h"

class RpcServer;

class RpcMediator: public RpcSystem
{
public:

	RpcMediator(RpcComms *c);
	void setRpcServer(RpcServer *s) { mServer = s; } /* Must only be called during setup */

	// Overloaded from RpcSystem.
virtual void reset(uint32_t chan_id);
virtual int tick();

	int recv();
	int recv_msg(uint32_t chan_id);
	int send(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg);

        int error(uint32_t chan_id, std::string msg); // pass an error up to comms level.
private:
	RpcComms *mComms;
	RpcServer *mServer;

};


/* Msg Packing */
class MsgPacker
{
public:
        static int headersize();
	static bool serialiseHeader(uint32_t msg_id, uint32_t req_id, uint32_t msg_size, uint8_t *buffer, uint32_t bufsize);
	static bool deserialiseHeader(uint32_t &msg_id, uint32_t &req_id, uint32_t &msg_size, uint8_t *buffer, uint32_t bufsize);
};



#endif /* RPC_MEDIATOR_H */
