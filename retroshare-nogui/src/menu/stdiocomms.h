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



#ifndef RS_STDIO_COMMS_H
#define RS_STDIO_COMMS_H

#include "rpcsystem.h"



class StdioComms: public RpcComms
{
public:
	StdioComms(int infd, int outfd);

        virtual int isOkay();
        virtual int error(uint32_t chan_id, std::string msg);

        virtual int active_channels(std::list<uint32_t> &chan_ids);

        virtual int recv_ready(uint32_t chan_id);
        virtual int recv(uint32_t chan_id, uint8_t *buffer, int bytes);
        virtual int recv(uint32_t chan_id, std::string &buffer, int bytes);

	// these make it easier...
        virtual int recv_blocking(uint32_t chan_id, uint8_t *buffer, int bytes);
        virtual int recv_blocking(uint32_t chan_id, std::string &buffer, int bytes);

        virtual int send(uint32_t chan_id, uint8_t *buffer, int bytes);
        virtual int send(uint32_t chan_id, const std::string &buffer);

private:
	int mIn, mOut;
};

#endif

