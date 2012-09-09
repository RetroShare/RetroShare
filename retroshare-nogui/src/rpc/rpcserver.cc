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



bool operator<(const RpcUniqueId &a, const RpcUniqueId &b)
{
	if (a.mChanId == b.mChanId)
		return (a.mReqId < b.mReqId);
	return (a.mChanId < b.mChanId);
}


RpcServer::RpcServer(RpcMediator *med)
 :mMediator(med), mRpcMtx("RpcMtx")
{

}

void RpcServer::reset(uint32_t chan_id)
{
	std::cerr << "RpcServer::reset(" << chan_id << ")" << std::endl;
	{
	        RsStackMutex stack(mRpcMtx); /********** LOCKED MUTEX ***************/

		std::list<RpcService *>::iterator it;
		for(it = mAllServices.begin(); it != mAllServices.end(); it++)
		{
			/* in mutex, but should be okay */
			(*it)->reset(chan_id); 
		}

		// clear existing queue.
		mRpcQueue.clear();
	}

	return;
}

int RpcServer::error(uint32_t chan_id, std::string msg)
{
	return mMediator->error(chan_id, msg);
}

int RpcServer::addService(RpcService *service)
{
	RsStackMutex stack(mRpcMtx); /********** LOCKED MUTEX ***************/

	service->setRpcServer(this);
	mAllServices.push_back(service);

	return 1;
}


int RpcServer::processMsg(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcServer::processMsg(" << msg_id << "," << req_id;
	std::cerr << ", len(" << msg.size() << ")) from channel: " << chan_id;
	std::cerr << std::endl;

	{
	        RsStackMutex stack(mRpcMtx); /********** LOCKED MUTEX ***************/

		std::list<RpcService *>::iterator it;
		for(it = mAllServices.begin(); it != mAllServices.end(); it++)
		{
			int rt = (*it)->processMsg(chan_id, msg_id, req_id, msg);
			if (!rt)
				continue;
	
			/* remember request */
			queueRequest_locked(chan_id, msg_id, req_id, (*it));
			return 1;
		}
	}

	std::cerr << "RpcServer::processMsg() No service to accepted it - discard";
	std::cerr << std::endl;
	return 0;
}

int RpcServer::queueRequest_locked(uint32_t chan_id, uint32_t /* msgId */, uint32_t req_id, RpcService *service)
{
	std::cerr << "RpcServer::queueRequest_locked() req_id: " << req_id;
	std::cerr << std::endl;

	RpcQueuedObj obj;
	obj.mChanId = chan_id;
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
			uint32_t out_chan_id = it->mChanId;
			uint32_t out_msg_id = 0;
			uint32_t out_req_id = it->mReqId;
			std::string out_msg;
			if (it->mService->getResponse(out_chan_id, out_msg_id, out_req_id, out_msg))
			{
				std::cerr << "RpcServer::checkPending() Response: (";
				std::cerr << out_msg_id << "," << out_req_id;
				std::cerr << ", len(" << out_msg.size() << "))";
				std::cerr << " for chan_id: " << out_chan_id;
				std::cerr << std::endl;

				/* store and send after queue is processed */
				RpcQueuedMsg msg;
				msg.mChanId = out_chan_id;
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


bool RpcServer::checkEvents()
{
	std::list<RpcQueuedMsg> msgsToSend;
	bool someToSend = false;

	{
	        RsStackMutex stack(mRpcMtx); /********** LOCKED MUTEX ***************/
	
		std::list<RpcService *>::iterator it;
		for(it = mAllServices.begin(); it != mAllServices.end(); it++)
		{
			if ((*it)->getEvents(msgsToSend))
				someToSend = true;
		}
	}

	if (someToSend)
	{
		sendQueuedMsgs(msgsToSend);
	}	
	return someToSend;
}

bool RpcServer::sendQueuedMsgs(std::list<RpcQueuedMsg> &msgs)
{
	/* No need for lock, as mOut is the only accessed item - and that has own protection */

	std::cerr << "RpcServer::sendQueuedMsg() " << msgs.size() << " to send";
	std::cerr << std::endl;

	std::list<RpcQueuedMsg>::iterator it;
	for (it = msgs.begin(); it != msgs.end(); it++)
	{
			mMediator->send(it->mChanId, it->mMsgId, it->mReqId, it->mMsg);
	}
	return true;
}







RpcQueueService::RpcQueueService(uint32_t serviceId)
:RpcService(serviceId), mQueueMtx("RpcQueueService")
{
	return;
}


void RpcQueueService::reset(uint32_t chan_id)
{
        clearEventsForChannel(chan_id);

	RsStackMutex stack(mQueueMtx); /********** LOCKED MUTEX ***************/

	std::list<RpcUniqueId> toRemove;

	// iterate through and remove only chan_id items.
        std::map<RpcUniqueId, RpcQueuedMsg>::iterator mit;
	for(mit = mResponses.begin(); mit != mResponses.end(); mit++)
	{
		if (mit->second.mChanId == chan_id)
			toRemove.push_back(mit->first);
	}

	/* remove items */
	std::list<RpcUniqueId>::iterator rit;
	for(rit = toRemove.begin(); rit != toRemove.end(); rit++)
	{
		mit = mResponses.find(*rit);
		mResponses.erase(mit);
	}


	return;
}

int RpcQueueService::getResponse(uint32_t &chan_id, uint32_t &msg_id, uint32_t &req_id, std::string &msg)
{
	RsStackMutex stack(mQueueMtx); /********** LOCKED MUTEX ***************/

        std::map<RpcUniqueId, RpcQueuedMsg>::iterator it;

	RpcUniqueId uid(chan_id, req_id);
	it = mResponses.find(uid);
	if (it == mResponses.end())
	{
		return 0;
	}

	// chan_id & req_id are already set.
	msg_id = it->second.mMsgId;
	msg = it->second.mMsg;

	mResponses.erase(it);

	return 1;
}

int RpcQueueService::queueResponse(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg)
{
	RsStackMutex stack(mQueueMtx); /********** LOCKED MUTEX ***************/

	RpcQueuedMsg qmsg;
	qmsg.mChanId = chan_id;
	qmsg.mMsgId = msg_id;
	qmsg.mReqId = req_id;
	qmsg.mMsg = msg;

	RpcUniqueId uid(chan_id, req_id);
	mResponses[uid] = qmsg;

	return 1;
}




/********* Events & Registration ******/


int RpcQueueService::getEvents(std::list<RpcQueuedMsg> &events)
{
	RsStackMutex stack(mQueueMtx); /********** LOCKED MUTEX ***************/

	std::map<uint32_t, std::list<RpcEventRegister> >::iterator it;
	for(it = mEventRegister.begin(); it != mEventRegister.end(); it++)
	{
		locked_checkForEvents(it->first, it->second, events);
	}

	if (events.empty())
	{
		return 0;
	}
	return 1;
}

int RpcQueueService::locked_checkForEvents(uint32_t event, const std::list<RpcEventRegister> &registered, std::list<RpcQueuedMsg> & /* events */)
{
	std::cerr << "RpcQueueService::locked_checkForEvents() NOT IMPLEMENTED";
	std::cerr << std::endl;
	std::cerr << "\tRegistered for Event Type: " << event;
	std::cerr << std::endl;

	std::list<RpcEventRegister>::const_iterator it;
	for(it = registered.begin(); it != registered.end(); it++)
	{
		std::cerr << "\t\t Channel ID: " << it->mChanId;
		std::cerr << " Request ID: " << it->mReqId;
		std::cerr << std::endl;
	}

	return 1;
}

int RpcQueueService::registerForEvents(uint32_t chan_id, uint32_t req_id, uint32_t event_id)
{
	std::cerr << "RpcQueueService::registerForEvents(ChanId: " << chan_id;
	std::cerr << ", ReqId: " << req_id;
	std::cerr << ", EventId: " << event_id;
	std::cerr << ")";
	std::cerr << std::endl;

	RsStackMutex stack(mQueueMtx); /********** LOCKED MUTEX ***************/

	std::map<uint32_t, std::list<RpcEventRegister> >::iterator mit;
	mit = mEventRegister.find(event_id);
	if (mit == mEventRegister.end())
	{
		std::list<RpcEventRegister> emptyList;
		mEventRegister[event_id] = emptyList;

		mit = mEventRegister.find(event_id);
	}

	RpcEventRegister reg;
	reg.mChanId = chan_id;
	reg.mReqId  = req_id;
	reg.mEventId = event_id;

	mit->second.push_back(reg);

	return 1;
}

int RpcQueueService::deregisterForEvents(uint32_t chan_id, uint32_t req_id, uint32_t event_id)
{
	std::cerr << "RpcQueueService::deregisterForEvents(ChanId: " << chan_id;
	std::cerr << ", ReqId: " << req_id;
	std::cerr << ", EventId: " << event_id;
	std::cerr << ")";
	std::cerr << std::endl;

	RsStackMutex stack(mQueueMtx); /********** LOCKED MUTEX ***************/

	std::map<uint32_t, std::list<RpcEventRegister> >::iterator mit;
	mit = mEventRegister.find(event_id);
	if (mit == mEventRegister.end())
	{
		std::cerr << "RpcQueueService::deregisterForEvents() ";
		std::cerr << "ERROR no EventId: " << event_id;
		std::cerr << std::endl;

		return 0;
	}

	bool removed = false;
	std::list<RpcEventRegister>::iterator lit;
	for(lit = mit->second.begin(); lit != mit->second.end();)
	{
		/* remove all matches */
		if ((lit->mReqId == req_id) && (lit->mChanId == chan_id))
		{
			lit = mit->second.erase(lit);
			if (removed == true)
			{
				std::cerr << "RpcQueueService::deregisterForEvents() ";
				std::cerr << "WARNING REMOVING MULTIPLE MATCHES";
				std::cerr << std::endl;
			}
			removed = true;
		}
		else
		{
			lit++;
		}
	}

	if (mit->second.empty())
	{
		std::cerr << "RpcQueueService::deregisterForEvents() ";
		std::cerr << " Last Registrant for Event, removing List from Map";
		std::cerr << std::endl;

		mEventRegister.erase(mit);
	}

	return 1;
}


int RpcQueueService::clearEventsForChannel(uint32_t chan_id)
{
	std::cerr << "RpcQueueService::clearEventsForChannel(" << chan_id;
	std::cerr << ")";
	std::cerr << std::endl;

	RsStackMutex stack(mQueueMtx); /********** LOCKED MUTEX ***************/

	std::list<uint32_t> toMapRemove;

	std::map<uint32_t, std::list<RpcEventRegister> >::iterator mit;
	for(mit = mEventRegister.begin(); mit != mEventRegister.end(); mit++)
	{
		std::list<RpcEventRegister>::iterator lit;
		for(lit = mit->second.begin(); lit != mit->second.end();)
		{
			/* remove all matches */
			if (lit->mChanId == chan_id)
			{
				std::cerr << "RpcQueueService::clearEventsForChannel() ";
				std::cerr << " Removing ReqId: " << lit->mReqId;
				std::cerr << " for EventId: " << lit->mEventId;
				std::cerr << std::endl;

				lit = mit->second.erase(lit);
			}
			else
			{
				lit++;
			}
		}
		if (mit->second.empty())
		{
			toMapRemove.push_back(mit->first);
		}
	}

	/* remove any empty lists now */
	std::list<uint32_t>::iterator rit;
	for(rit = toMapRemove.begin(); rit != toMapRemove.end(); rit++)
	{
		mit = mEventRegister.find(*rit);
		mEventRegister.erase(mit);
	}

	return 1;
}


int RpcQueueService::printEventRegister(std::ostream &out)
{
	out << "RpcQueueService::printEventRegister()";
	out << std::endl;

	RsStackMutex stack(mQueueMtx); /********** LOCKED MUTEX ***************/

	std::list<uint32_t> toMapRemove;

	std::map<uint32_t, std::list<RpcEventRegister> >::iterator mit;
	for(mit = mEventRegister.begin(); mit != mEventRegister.end(); mit++)
	{
		out << "Event: " << mit->first;
		out << std::endl;

		std::list<RpcEventRegister>::iterator lit;
		for(lit = mit->second.begin(); lit != mit->second.end(); lit++)
		{
			out << "\tRegistrant: ReqId: " << lit->mReqId;
			out << "\t ChanId: " << lit->mChanId;
			out << std::endl;
		}
	}
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





