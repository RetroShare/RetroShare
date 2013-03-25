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
	RpcStream(): chan_id(0), req_id(0), stream_id(0), state(0), 
	offset(0), length(0), start_byte(0), end_byte(0), desired_rate(0),
	transfer_type(0), transfer_time(0), transfer_avg_dt(0)
	{ return; }

static const uint32_t STREAMERR	= 0x00000;
static const uint32_t RUNNING 	= 0x00001;
static const uint32_t PAUSED 	= 0x00002;
static const uint32_t FINISHED 	= 0x00003;

	uint32_t chan_id;
	uint32_t req_id;
	uint32_t stream_id;
	uint32_t state;

	std::string name;
	std::string hash;
	std::string path;

	uint64_t offset;	// where we currently are.
	uint64_t length;	// filesize.

	uint64_t start_byte;
	uint64_t end_byte;

	float desired_rate; // Kb/s


	// Transfer Type
static const uint32_t STANDARD  	= 0x00000;
static const uint32_t REALTIME  	= 0x00001;
static const uint32_t BACKGROUND	= 0x00002;

	uint32_t transfer_type;
	double   transfer_time;
	double   transfer_avg_dt;

	
};


class RpcStreamRates
{
	public:
	RpcStreamRates(): avg_data_rate(0), avg_dt(1), last_data_rate(0), last_ts(0) { return; }

	double avg_data_rate;
	double avg_dt;

	double last_data_rate;
	double last_ts;
};




class RpcProtoStream: public RpcQueueService
{
public:
	RpcProtoStream(uint32_t serviceId);
	virtual int processMsg(uint32_t chan_id, uint32_t msgId, uint32_t req_id, const std::string &msg);
	virtual void reset(uint32_t chan_id);

	uint32_t getNextStreamId();

protected:


	int processReqStartFileStream(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg);
	int processReqControlStream(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg);
	int processReqListStreams(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg);
	int processReqRegisterStreams(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg);


        uint32_t mNextStreamId;

        RpcStreamRates mStreamRates;
	std::map<uint32_t, RpcStream> mStreams;

	// EVENTS.
	virtual int locked_checkForEvents(uint32_t event, const std::list<RpcEventRegister> &registered, std::list<RpcQueuedMsg> &events); 

	// Not actually used yet.
	int cleanup_checkForEvents(uint32_t event, const std::list<RpcEventRegister> &registered);

};


#endif /* RS_PROTO_STREAM_H */
