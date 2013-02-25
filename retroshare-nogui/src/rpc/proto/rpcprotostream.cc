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

#include "rpc/proto/rpcprotochat.h"
#include "rpc/proto/gencc/chat.pb.h"

#include <retroshare/rsmsgs.h>
#include <retroshare/rspeers.h>
#include <retroshare/rshistory.h>

#include "util/rsstring.h"

#include <stdio.h>

#include <iostream>
#include <algorithm>

#include <set>


bool fill_stream_details(rsctrl::stream::ResponseStreamDetail &resp, 
			const std::list<RpcStream> &streams);
bool fill_stream_detail(rsctrl::stream::StreamDetail &detail, 
			const RpcStream &stream);

// Helper Functions -> maybe move to libretroshare/utils ??
bool convertUTF8toWString(const std::string &msg_utf8, std::wstring &msg_wstr);
bool convertWStringToUTF8(const std::wstring &msg_wstr, std::string &msg_utf8);

bool convertStringToLobbyId(const std::string &chat_id, ChatLobbyId &lobby_id);
bool convertLobbyIdToString(const ChatLobbyId &lobby_id, std::string &chat_id);

bool fillLobbyInfoFromChatLobbyInfo(const ChatLobbyInfo &cfi, rsctrl::stream::ChatLobbyInfo *lobby);
bool fillLobbyInfoFromVisibleChatLobbyRecord(const VisibleChatLobbyRecord &pclr, rsctrl::stream::ChatLobbyInfo *lobby);
bool fillLobbyInfoFromChatLobbyInvite(const ChatLobbyInvite &cli, rsctrl::stream::ChatLobbyInfo *lobby);

bool fillChatMessageFromHistoryMsg(const HistoryMsg &histmsg, rsctrl::stream::ChatMessage *rpcmsg);

bool createQueuedEventSendMsg(const ChatInfo &chatinfo, rsctrl::stream::ChatType ctype, 
			std::string chat_id, const RpcEventRegister &ereg, RpcQueuedMsg &qmsg);


RpcProtoStream::RpcProtoStream(uint32_t serviceId)
	:RpcQueueService(serviceId)
{ 
	return; 
}

//RpcProtoStream::msgsAccepted(std::list<uint32_t> &msgIds); /* not used at the moment */

int RpcProtoStream::processMsg(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg)
{
	/* check the msgId */
	uint8_t topbyte = getRpcMsgIdExtension(msg_id); 
	uint16_t service = getRpcMsgIdService(msg_id); 
	uint8_t submsg  = getRpcMsgIdSubMsg(msg_id);
	bool isResponse = isRpcMsgIdResponse(msg_id);


	std::cerr << "RpcProtoStream::processMsg() topbyte: " << (int32_t) topbyte;
	std::cerr << " service: " << (int32_t) service << " submsg: " << (int32_t) submsg;
	std::cerr << std::endl;

	if (isResponse)
	{
		std::cerr << "RpcProtoStream::processMsg() isResponse() - not processing";
		std::cerr << std::endl;
		return 0;
	}

	if (topbyte != (uint8_t) rsctrl::core::CORE)
	{
		std::cerr << "RpcProtoStream::processMsg() Extension Mismatch - not processing";
		std::cerr << std::endl;
		return 0;
	}

	if (service != (uint16_t) rsctrl::core::STREAM)
	{
		std::cerr << "RpcProtoStream::processMsg() Service Mismatch - not processing";
		std::cerr << std::endl;
		return 0;
	}

	if (!rsctrl::stream::RequestMsgIds_IsValid(submsg))
	{
		std::cerr << "RpcProtoStream::processMsg() SubMsg Mismatch - not processing";
		std::cerr << std::endl;
		return 0;
	}

	switch(submsg)
	{

		case rsctrl::stream::MsgId_RequestStartFileStream:
        		processReqStartFileStream(chan_id, msg_id, req_id, msg);
			break;

		case rsctrl::stream::MsgId_RequestCreateLobby:
			processReqControlStream(chan_id, msg_id, req_id, msg);
			break;

		case rsctrl::stream::MsgId_RequestJoinOrLeaveLobby:
			processReqListStreams(chan_id, msg_id, req_id, msg);
			break;

		case rsctrl::stream::MsgId_RequestRegisterStreams:
			processReqRegisterStreams(chan_id, msg_id, req_id, msg);
			break;

		default:
			std::cerr << "RpcProtoStream::processMsg() ERROR should never get here";
			std::cerr << std::endl;
			return 0;
	}

	/* must have matched id to get here */
	return 1;
}



int RpcProtoStream::processReqStartFileStream(uint32_t chan_id, uint32_t /*msg_id*/, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoStream::processReqStartFileStream()";
	std::cerr << std::endl;

	// parse msg.
	rsctrl::stream::RequestStartFileStream req;
	if (!req.ParseFromString(msg))
	{
		std::cerr << "RpcProtoStream::processReqStartFileStream() ERROR ParseFromString()";
		std::cerr << std::endl;
		return 0;
	}

	// response.
	rsctrl::stream::ResponseStreamDetail resp;
	bool success = true;
	std::string errorMsg;


	// SETUP STREAM.

	// FIND the FILE.
        Expression * exp;
        std::list<DirDetails> results;
        FileSearchFlags flags;
        int ans = rsFiles->SearchBoolExp(exp, results, flags);

	// CREATE A STREAM OBJECT.
	if (results.size() < 1)
	{
		success = false;
		errorMsg = "No Matching File";
	}
	else
	{
		RpcStream stream;

		stream.chan_id = chan_id;
		stream.stream_id = getNextStreamId();
		stream.state = RUNNING;

		stream.path = ... // from results.
		stream.offset = 0;
		stream.length = file.size;
		stream.start_byte = 0;
		stream.end_byte = stream.length;

		stream.desired_rate = req.rate_kBs;

		// make response
		rsctrl::stream::StreamDetail &detail = resp.streams_add();
		if (!fill_stream_detail(detail, stream))
		{
			success = false;
			errorMsg = "Failed to Invalid Action";
		}
		else
		{
			// insert.
			mStreams[stream.stream_id] = stream;
		}

	}

	/* DONE - Generate Reply */
        if (success)
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::SUCCESS);
	}
	else
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::FAILED);
		status->set_msg(errorMsg);
	}

	std::string outmsg;
	if (!resp.SerializeToString(&outmsg))
	{
		std::cerr << "RpcProtoStream::processReqStartFileStream() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = constructMsgId(rsctrl::core::CORE, rsctrl::core::STREAM, 
				rsctrl::stream::MsgId_ResponseStreamDetail, true);

	// queue it.
	queueResponse(chan_id, out_msg_id, req_id, outmsg);
	return 1;
}



int RpcProtoStream::processReqControlStream(uint32_t chan_id, uint32_t /*msg_id*/, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoStream::processReqControlStream()";
	std::cerr << std::endl;

	// parse msg.
	rsctrl::stream::RequestControlStream req;
	if (!req.ParseFromString(msg))
	{
		std::cerr << "RpcProtoStream::processReqControlStream() ERROR ParseFromString()";
		std::cerr << std::endl;
		return 0;
	}

	// response.
	rsctrl::stream::ResponseStreamDetail resp;
	bool success = true;
	std::string errorMsg;

	// FIND MATCHING STREAM.
	std::map<uint32_t, RpcStream>::iterator it;
	it = mStreams.find(resp.stream_id);
	if (it != mStreams.end())
	{
		// TWEAK
		// FILL IN REPLY.
		// DONE.

		switch(resp.action)
		{
			case STREAM_START:
				if (it->state == PAUSED)
				{
					it->state = RUNNING;
				}
				break;
			case STREAM_STOP:
				it->state = FINISHED;
				break;
			case STREAM_PAUSE:
				if (it->state == RUNNING)
				{
					it->state = PAUSED;
				}
				break;
			case STREAM_CHANGE_RATE:
				it->desired_rate = req.rate_kBs;
				break;
			case STREAM_SEEK:
				if (req.seek_byte < it-> endByte)
				{
					it->offset = req.seek_byte;
				}
				break;
			default:
				success = false;
				errorMsg = "Invalid Action";
		}

		if (success)
		{
			rsctrl::stream::StreamDetail &detail = resp.streams_add();
			if (!fill_stream_detail(detail, it->second))
			{
				success = false;
				errorMsg = "Invalid Action";
			}
		}
	}
	else
	{
		success = false;
		errorMsg = "No Matching Stream";
	}

	/* DONE - Generate Reply */
        if (success)
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::SUCCESS);
	}
	else
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::FAILED);
		status->set_msg(errorMsg);
	}

	std::string outmsg;
	if (!resp.SerializeToString(&outmsg))
	{
		std::cerr << "RpcProtoStream::processReqControlStream() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = constructMsgId(rsctrl::core::CORE, rsctrl::core::STREAM, 
				rsctrl::stream::MsgId_ResponseStreamDetail, true);

	// queue it.
	queueResponse(chan_id, out_msg_id, req_id, outmsg);

	return 1;
}



int RpcProtoStream::processReqListStreams(uint32_t chan_id, uint32_t /*msg_id*/, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoStream::processReqListStreams()";
	std::cerr << std::endl;

	// parse msg.
	rsctrl::stream::RequestListStreams req;
	if (!req.ParseFromString(msg))
	{
		std::cerr << "RpcProtoStream::processReqListStreams() ERROR ParseFromString()";
		std::cerr << std::endl;
		return 0;
	}

	// response.
	rsctrl::stream::ResponseStreamDetail resp;
	bool success = false;
	std::string errorMsg;



	for(it = mStreams.begin(); it != mStreams.end(); it++)
	{
		/* check that it matches */
		if (! match)
		{
			continue;
		}

		rsctrl::stream::StreamDetail &detail = resp.streams_add();
		if (!fill_stream_detail(detail, it->second))
		{
			success = false;
			errorMsg = "Some Details Failed to Fill";
		}
	}

	/* DONE - Generate Reply */
        if (success)
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::SUCCESS);
	}
	else
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::FAILED);
		status->set_msg(errorMsg);
	}

	std::string outmsg;
	if (!resp.SerializeToString(&outmsg))
	{
		std::cerr << "RpcProtoStream::processReqListStreams() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = constructMsgId(rsctrl::core::CORE, rsctrl::core::STREAM, 
				rsctrl::stream::MsgId_ResponseStreamDetail, true);

	// queue it.
	queueResponse(chan_id, out_msg_id, req_id, outmsg);
	return 1;
}





int RpcProtoStream::processReqRegisterStreams(uint32_t chan_id, uint32_t /*msg_id*/, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoStream::processReqRegisterStreams()";
	std::cerr << std::endl;

	// parse msg.
	rsctrl::stream::RequestRegisterStreams req;
	if (!req.ParseFromString(msg))
	{
		std::cerr << "RpcProtoStream::processReqRegisterStreams() ERROR ParseFromString()";
		std::cerr << std::endl;
		return 0;
	}

	// response.
	rsctrl::stream::ResponseRegisterStreams resp;
	bool success = true;
	bool doregister = false;
	std::string errorMsg;

	switch(req.action())
	{
		case rsctrl::stream::RequestRegisterStreams::REGISTER:
			doregister = true;
			break;
		case rsctrl::stream::RequestRegisterStreams::DEREGISTER:
			doregister = false;
			break;
		default:
			std::cerr << "ERROR action is invalid";
			std::cerr << std::endl;

			success = false;
			errorMsg = "RegisterStreams.Action is invalid";
			break;
	}

        if (success)
	{
		if (doregister)
		{
			std::cerr << "Registering for Streams";
			std::cerr << std::endl;

        		registerForEvents(chan_id, req_id, REGISTRATION_STREAMS);
		}
		else
		{
			std::cerr << "Deregistering for Streams";
			std::cerr << std::endl;

        		deregisterForEvents(chan_id, req_id, REGISTRATION_STREAMS);
		}
        	printEventRegister(std::cerr);
	}


	/* DONE - Generate Reply */
        if (success)
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::SUCCESS);
	}
	else
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::FAILED);
		status->set_msg(errorMsg);
	}

	std::string outmsg;
	if (!resp.SerializeToString(&outmsg))
	{
		std::cerr << "RpcProtoStream::processReqCreateLobbies() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = constructMsgId(rsctrl::core::CORE, rsctrl::core::STREAM, 
				rsctrl::stream::MsgId_ResponseRegisterStreams, true);

	// queue it.
	queueResponse(chan_id, out_msg_id, req_id, outmsg);
	return 1;
}




        // EVENTS. (STREAMS)
int RpcProtoStream::locked_checkForEvents(uint32_t event, const std::list<RpcEventRegister> &registered, std::list<RpcQueuedMsg> &stream_msgs)
{
	/* Wow - here already! */
	std::cerr << "locked_checkForEvents()";
	std::cerr << std::endl;

	/* only one event type for now */
	if (event !=  REGISTRATION_STREAMS)
	{
		std::cerr << "ERROR Invalid Stream Event Type";
		std::cerr << std::endl;

		/* error */
		return 0;
	}

	/* iterate through streams, and get next chunk of data.
	 * package up and send it.
         * NOTE we'll have to something more complex for VoIP!
	 */

	float ts = getTimeStamp();
	float dt = ts - mStreamRates.last_ts;
	uint32_t data_sent = 0;

#define FILTER_K (0.75)

	if (mStreamRates.last_ts != 0)
	{
		mStreamRates.avg_dt = FILTER_K * mStreamRates.avg_dt 
				+ (1.0 - FILTER_K) * dt;
	}
	else
	{
		mStreamRates.avg_dt = dt;
	}
	mStreamRates.last_ts = ts;


	std::list<uint32_t> to_remove;
	std::map<uint32_t, RpcStream>::iterator it;
	for(it = mStreams.begin(); it != mStreams.end(); it++)
	{
		RpcStream &stream = it->second;

		if (stream.state == PAUSED)
		{
			continue;
		}

		/* we ignore the Events Register... just use stream info, */
		int channel_id = stream.chan_id;
		uint32_t size = stream.desired_rate * mStreamRates.avg_dt;

		/* get data */
		uint64_t remaining = stream.end_byte - stream.offset;
		if (remaining < size)
		{
			size = remaining;
			stream.state = FINISHED;
			to_remove.push_back(it->first);
		}


		/* fill in the answer */

		StreamData data;
		data.stream_id = stream.stream_id;
		data.stream_state = stream.state;
		data.send_time = getTimeStamp();
		data.offset = stream.offset;
		data.size = size;

		fill_stream_data(stream, data);




		RpcQueuedMsg qmsg;
		rsctrl::stream::ChatType ctype = rsctrl::stream::TYPE_GROUP;
		std::string chat_id = ""; // No ID for group.

		if (createQueuedEventSendMsg(*it, ctype, chat_id, *rit, qmsg))
		{
			std::cerr << "Created MsgEvent";
			std::cerr << std::endl;

			events.push_back(qmsg);
		}
		else
		{
			std::cerr << "ERROR Creating MsgEvent";
			std::cerr << std::endl;
		}
	}

	std::list<uint32_t>::iterator rit;
	for(rit = to_remove.begin(); rit != to_remove.end(); rit++)
	{
		/* kill the stream! */
		it = mStreams.find(*rit);
		if (it != mStreams.end())
		{
			mStreams.erase(it);
		}
	}

	return 1;
}


/***** HELPER FUNCTIONS *****/


bool createQueuedEventSendMsg(const ChatInfo &chatinfo, rsctrl::stream::ChatType ctype, 
			std::string chat_id, const RpcEventRegister &ereg, RpcQueuedMsg &qmsg)
{

	rsctrl::stream::EventChatMessage event;
	rsctrl::stream::ChatMessage *msg = event.mutable_msg();
	rsctrl::stream::ChatId *id = msg->mutable_id();

	id->set_chat_type(ctype);
	id->set_chat_id(chat_id);

  	msg->set_peer_nickname(chatinfo.peer_nickname);
  	msg->set_chat_flags(chatinfo.chatflags);
  	msg->set_send_time(chatinfo.sendTime);
  	msg->set_recv_time(chatinfo.recvTime);

	std::string msg_utf8;
	if (!convertWStringToUTF8(chatinfo.msg, msg_utf8))
	{
		std::cerr << "RpcProtoStream::createQueuedEventSendMsg() ERROR Converting Msg";
		std::cerr << std::endl;
		return false;
	}

  	msg->set_msg(msg_utf8);

	/* DONE - Generate Reply */
	std::string outmsg;
	if (!event.SerializeToString(&outmsg))
	{
		std::cerr << "RpcProtoStream::createQueuedEventSendMsg() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return false;
	}
	
	// Correctly Name Message.
	qmsg.mMsgId = constructMsgId(rsctrl::core::CORE, rsctrl::core::CHAT, 
				rsctrl::stream::MsgId_EventChatMessage, true);

	qmsg.mChanId = ereg.mChanId;
	qmsg.mReqId = ereg.mReqId;
	qmsg.mMsg = outmsg;

	return true;
}


/****************** NEW HELPER FNS ******************/

bool fill_stream_details(rsctrl::stream::ResponseStreamDetail &resp, 
			const std::list<RpcStream> &streams)
{
	std::list<RpcStream>::const_iterator it;
	for (it = streams.begin(); it != streams.end(); it++)
	{
		rsctrl::stream::StreamDetail &detail = resp.streams_add();
		fill_stream_detail(detail, *it);
	}

	return val;
}

bool fill_stream_detail(rsctrl::stream::StreamDetail &detail, 
			const RpcStream &stream)
{



}

