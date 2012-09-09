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


#ifndef RS_RPC_SERVER_H
#define RS_RPC_SERVER_H

#include <map>
#include <list>
#include <string>
#include <inttypes.h>

#include "util/rsthreads.h"

// Dividing up MsgIds into components:

uint8_t  getRpcMsgIdSubMsg(uint32_t msg_id);
uint16_t getRpcMsgIdService(uint32_t msg_id); // Middle 16 bits.
uint8_t  getRpcMsgIdExtension(uint32_t msg_id); // Top 7 of 8 bits. Bottom Bit is for Request / Response
bool     isRpcMsgIdResponse(uint32_t msg_id);

uint32_t constructMsgId(uint8_t ext, uint16_t service, uint8_t submsg, bool is_response);

/*** This can be overloaded for plugins
 * Also allows a natural seperation of the full interface into sections.
 */

// The Combination of ChanId & ReqId must be unique for each RPC call.
// This is used as an map index, so failure to make it unique, will lead to lost entries.
class RpcUniqueId
{
	public:
	RpcUniqueId():mChanId(0), mReqId(0) {return;}
	RpcUniqueId(uint32_t chan_id, uint32_t req_id):mChanId(chan_id), mReqId(req_id) {return;}
	uint32_t mChanId;
	uint32_t mReqId;
};

bool operator<(const RpcUniqueId &a, const RpcUniqueId &b);

class RpcQueuedMsg
{
public:
	uint32_t mChanId;
	uint32_t mMsgId;
	uint32_t mReqId;
	std::string mMsg;
};

class RpcServer;

class RpcService
{
public:
	RpcService(uint32_t /* serviceId */ ):mRpcServer(NULL) { return; }
	virtual void reset(uint32_t /* chan_id */) { return; } 
	virtual int msgsAccepted(std::list<uint32_t> & /* msgIds */) { return 0; } /* not used at the moment */
	virtual int processMsg(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg) = 0;     /* returns 0 - not handled, > 0, accepted */
	virtual int getResponse(uint32_t &chan_id, uint32_t &msgId, uint32_t &req_id, std::string &msg) = 0;  /* 0 - not ready, > 0 heres the response */

	virtual int getEvents(std::list<RpcQueuedMsg> & /* events */) { return 0; } /* 0 = none, optional feature */
	
	void setRpcServer(RpcServer *server) {mRpcServer = server; }
	RpcServer *getRpcServer() { return mRpcServer; }
private:
	RpcServer *mRpcServer;
};


class RpcEventRegister
{
	public:
	uint32_t mChanId;
	uint32_t mReqId;
	uint32_t mEventId; // THIS IS A LOCAL PARAMETER, Service Specific
};

/* Implements a Queue for quick implementation of Instant Response Services */
class RpcQueueService: public RpcService
{
public:
	RpcQueueService(uint32_t serviceId);
virtual void reset(uint32_t chan_id);
virtual int getResponse(uint32_t &chan_id, uint32_t &msg_id, uint32_t &req_id, std::string &msg);
virtual int getEvents(std::list<RpcQueuedMsg> &events);

protected:
	int queueResponse(uint32_t chan_id, uint32_t msgId, uint32_t req_id, const std::string &msg);

virtual int locked_checkForEvents(uint32_t event, const std::list<RpcEventRegister> &registered, std::list<RpcQueuedMsg> &events); // Overload for functionality.

	int registerForEvents(uint32_t chan_id, uint32_t req_id, uint32_t event_id);
	int deregisterForEvents(uint32_t chan_id, uint32_t req_id, uint32_t event_id);

	int clearEventsForChannel(uint32_t chan_id);
	int printEventRegister(std::ostream &out);

private:
        RsMutex mQueueMtx;

	std::map<RpcUniqueId, RpcQueuedMsg> mResponses;
	std::map<uint32_t, std::list<RpcEventRegister> > mEventRegister;
};


/* For Tracking responses */
class RpcQueuedObj
{
public:
	uint32_t mChanId;
	uint32_t mReqId;
	RpcService *mService;
};


class RpcMediator;

class RpcServer
{

public:
	RpcServer(RpcMediator *med);
	int addService(RpcService *service);
	int processMsg(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg);
	bool checkPending();
	bool checkEvents();

	void reset(uint32_t chan_id);
        int error(uint32_t chan_id, std::string msg); // pass an error to mediator.

private:
	bool sendQueuedMsgs(std::list<RpcQueuedMsg> &msgs);
	int queueRequest_locked(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, RpcService *service);

	RpcMediator *mMediator;

        RsMutex mRpcMtx;

	std::list<RpcQueuedObj> mRpcQueue;
	std::list<RpcService *> mAllServices;
};


#endif /* RS_RPC_SERVER_H */
