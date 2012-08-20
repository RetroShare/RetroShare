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


#ifndef RS_RPC_SYSTEM_H
#define RS_RPC_SYSTEM_H

#include <string>
#include <inttypes.h>

class RpcComms
{
public:
        virtual int isOkay() = 0;
        virtual int error(std::string msg) = 0;

        virtual int recv_ready() = 0;
        virtual int recv(uint8_t *buffer, int bytes) = 0;
        virtual int recv(std::string &buffer, int bytes) = 0;

	// these make it easier...
        virtual int recv_blocking(uint8_t *buffer, int bytes) = 0;
        virtual int recv_blocking(std::string &buffer, int bytes) = 0;

        virtual int send(uint8_t *buffer, int bytes) = 0;
        virtual int send(const std::string &buffer) = 0;

        virtual int setSleepPeriods(float /* busy */, float /* idle */) { return 0; }
};


class RpcSystem
{
public:
	/* this must be regularly ticked to update the display */
	virtual void reset() = 0; 
	virtual int tick() = 0;
};


#endif // RS_RPC_SERVER_H
