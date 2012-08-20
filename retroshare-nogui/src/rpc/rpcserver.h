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

/*** This can be overloaded for plugins
 * Also allows a natural seperation of the full interface into sections.
 */

class RpcQueuedMsg
{
public:
	uint32_t mMsgId;
	uint32_t mReqId;
	std::string mMsg;
};


class RpcService
{
public:
	RpcService(uint32_t /* serviceId */ ) { return; }
	virtual void reset() { return; } 
	virtual int msgsAccepted(std::list<uint32_t> & /* msgIds */) { return 0; } /* not used at the moment */
	virtual int processMsg(uint32_t msgId, uint32_t req_id, const std::string &msg) = 0;     /* returns 0 - not handled, > 0, accepted */
	virtual int getResponse(uint32_t &msgId, uint32_t &req_id, std::string &msg) = 0;  /* 0 - not ready, > 0 heres the response */
};


/* Implements a Queue for quick implementation of Instant Response Services */
class RpcQueueService: public RpcService
{
public:
	RpcQueueService(uint32_t serviceId);
virtual void reset();
virtual int getResponse(uint32_t &msgId, uint32_t &req_id, std::string &msg);

protected:
	int queueResponse(uint32_t msgId, uint32_t req_id, const std::string &msg);
private:
        RsMutex mQueueMtx;
	std::map<uint32_t, RpcQueuedMsg> mResponses;
};


/* For Tracking responses */
class RpcQueuedObj
{
public:
	uint32_t mReqId;
	RpcService *mService;
};


class RpcMediator;

class RpcServer
{

public:
	RpcServer(RpcMediator *med);
	int addService(RpcService *service);
	int processMsg(uint32_t msgId, uint32_t req_id, const std::string &msg);
	bool checkPending();

	void reset();

private:
	bool sendQueuedMsgs(std::list<RpcQueuedMsg> &msgs);
	int queueRequest_locked(uint32_t msgId, uint32_t req_id, RpcService *service);

	RpcMediator *mMediator;

        RsMutex mRpcMtx;

	std::list<RpcQueuedObj> mRpcQueue;
	std::list<RpcService *> mAllServices;
};


#endif /* RS_RPC_SERVER_H */
