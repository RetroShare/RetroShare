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

#include "rpc/rpcserver.h"

#include "rpc/rpc.h"

#include <iostream>

RpcServer::RpcServer(RpcMediator *med)
 :mMediator(med), mRpcMtx("RpcMtx")
{

}

void RpcServer::reset()
{
	std::cerr << "RpcServer::reset()" << std::endl;
	{
	        RsStackMutex stack(mRpcMtx); /********** LOCKED MUTEX ***************/

		std::list<RpcService *>::iterator it;
		for(it = mAllServices.begin(); it != mAllServices.end(); it++)
		{
			/* in mutex, but should be okay */
			(*it)->reset(); 
		}

		// clear existing queue.
		mRpcQueue.clear();
	}

	return;
}


int RpcServer::addService(RpcService *service)
{
	RsStackMutex stack(mRpcMtx); /********** LOCKED MUTEX ***************/

	mAllServices.push_back(service);

	return 1;
}


int RpcServer::processMsg(uint32_t msg_id, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcServer::processMsg(" << msg_id << "," << req_id;
	std::cerr << ", len(" << msg.size() << "))" << std::endl;
	{
	        RsStackMutex stack(mRpcMtx); /********** LOCKED MUTEX ***************/

		std::list<RpcService *>::iterator it;
		for(it = mAllServices.begin(); it != mAllServices.end(); it++)
		{
			int rt = (*it)->processMsg(msg_id, req_id, msg);
			if (!rt)
				continue;
	
			/* remember request */
			queueRequest_locked(msg_id, req_id, (*it));
			return 1;
		}
	}

	std::cerr << "RpcServer::processMsg() No service to accepted it - discard";
	std::cerr << std::endl;
	return 0;
}

int RpcServer::queueRequest_locked(uint32_t /* msgId */, uint32_t req_id, RpcService *service)
{
	std::cerr << "RpcServer::queueRequest_locked() req_id: " << req_id;
	std::cerr << std::endl;

	RpcQueuedObj obj;
	obj.mReqId = req_id;
	obj.mService = service;

	mRpcQueue.push_back(obj);

	return 1;
}


bool RpcServer::checkPending()
{
	std::list<RpcQueuedMsg> msgsToSend;
	bool someRemaining = false;
	bool someToSend = false;

	{
	        RsStackMutex stack(mRpcMtx); /********** LOCKED MUTEX ***************/
	
		std::list<RpcQueuedObj>::iterator it;
		for(it = mRpcQueue.begin(); it != mRpcQueue.end();)
		{
			uint32_t out_msg_id = 0;
			uint32_t out_req_id = it->mReqId;
			std::string out_msg;
			if (it->mService->getResponse(out_msg_id, out_req_id, out_msg))
			{
				std::cerr << "RpcServer::checkPending() Response: (";
				std::cerr << out_msg_id << "," << out_req_id;
				std::cerr << ", len(" << out_msg.size() << "))";
				std::cerr << std::endl;

				/* store and send after queue is processed */
				RpcQueuedMsg msg;
				msg.mMsgId = out_msg_id;
				msg.mReqId = out_req_id;
				msg.mMsg = out_msg;

				msgsToSend.push_back(msg);

				it = mRpcQueue.erase(it);
				someToSend = true;
			}
			else
			{
				it++;
				someRemaining = true;
			}
		}
	}

	if (someToSend)
	{
		sendQueuedMsgs(msgsToSend);
	}	

	return someRemaining;
}

bool RpcServer::sendQueuedMsgs(std::list<RpcQueuedMsg> &msgs)
{
	/* No need for lock, as mOut is the only accessed item - and that has own protection */

	std::cerr << "RpcServer::sendQueuedMsg() " << msgs.size() << " to send";
	std::cerr << std::endl;

	std::list<RpcQueuedMsg>::iterator it;
	for (it = msgs.begin(); it != msgs.end(); it++)
	{
			mMediator->send(it->mMsgId, it->mReqId, it->mMsg);
	}
	return true;
}







RpcQueueService::RpcQueueService(uint32_t serviceId)
:RpcService(serviceId), mQueueMtx("RpcQueueService")
{
	return;
}


void RpcQueueService::reset()
{
	RsStackMutex stack(mQueueMtx); /********** LOCKED MUTEX ***************/

	// clear existing queue.
	mResponses.clear();
	return;
}

int RpcQueueService::getResponse(uint32_t &msg_id, uint32_t &req_id, std::string &msg)
{
	RsStackMutex stack(mQueueMtx); /********** LOCKED MUTEX ***************/

        std::map<uint32_t, RpcQueuedMsg>::iterator it;

	it = mResponses.find(req_id);
	if (it == mResponses.end())
	{
		return 0;
	}

	msg_id = it->second.mMsgId;
	msg = it->second.mMsg;

	mResponses.erase(it);

	return 1;
}

int RpcQueueService::queueResponse(uint32_t msg_id, uint32_t req_id, const std::string &msg)
{
	RsStackMutex stack(mQueueMtx); /********** LOCKED MUTEX ***************/

	RpcQueuedMsg qmsg;
	qmsg.mMsgId = msg_id;
	qmsg.mReqId = req_id;
	qmsg.mMsg = msg;

	mResponses[req_id] = qmsg;

	return 1;
}


// Lower 8 bits.
uint8_t  getRpcMsgIdSubMsg(uint32_t msg_id)
{
        return msg_id & 0xFF;
}

// Middle 16 bits.
uint16_t  getRpcMsgIdService(uint32_t msg_id)
{
        return (msg_id >> 8) & 0xFFFF;
}

// Top 8 bits.
uint8_t  getRpcMsgIdExtension(uint32_t msg_id)
{
        return (msg_id >> 24) & 0xFE; // Bottom Bit is for Request / Response
}

bool    isRpcMsgIdResponse(uint32_t msg_id)
{
        return (msg_id >> 24) & 0x01;
}


uint32_t constructMsgId(uint8_t ext, uint16_t service, uint8_t submsg, bool is_response)
{
	if (is_response)
		ext |= 0x01; // Set Bottom Bit.
	else
		ext &= 0xFE; // Clear Bottom Bit.

	uint32_t msg_id = (ext << 24) + (service << 8) + (submsg);
	return msg_id;
}





