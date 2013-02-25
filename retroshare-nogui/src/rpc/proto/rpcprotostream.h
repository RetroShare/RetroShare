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


#ifndef RS_RPC_PROTO_STREAM_H
#define RS_RPC_PROTO_STREAM_H

#include "rpc/rpcserver.h"

// Registrations.
#define REGISTRATION_STREAMS		1

class RpcStream
{
	public:
	uint32_t chan_id;
	uint32_t stream_id;
	uint32_t state;
	std::string path;
	uint64_t offset;	// where we currently are.
	uint64_t length;	// filesize.

	uint64_t start_byte;
	uint64_t end_byte;

	float desired_rate; // Kb/s
};


class RpcStreamRates
{
	public:

	float avg_data_rate;
	float avg_dt;

	float last_data_rate;
	float last_ts;
};




class RpcProtoStream: public RpcQueueService
{
public:
	RpcProtoStream(uint32_t serviceId);
	virtual int processMsg(uint32_t chan_id, uint32_t msgId, uint32_t req_id, const std::string &msg);

protected:

	int processReqStartFileStream(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg);
	int processReqControlStream(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg);
	int processReqListStreams(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg);
	int processReqRegisterStreams(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg);



        RpcStreamRates mStreamRates;
	std::map<uint32_t, RpcStream> mStreams;

	// EVENTS.
	virtual int locked_checkForEvents(uint32_t event, const std::list<RpcEventRegister> &registered, std::list<RpcQueuedMsg> &events); 

};


#endif /* RS_PROTO_STREAM_H */
