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

#include "rpc/proto/rpcprotostream.h"
#include "rpc/proto/rpcprotoutils.h"

#include "rpc/proto/gencc/stream.pb.h"
#include "rpc/proto/gencc/core.pb.h"

#include <retroshare/rsexpr.h>
#include <retroshare/rsfiles.h>

// from libretroshare
#include "util/rsdir.h"

//#include <retroshare/rsmsgs.h>
//#include <retroshare/rspeers.h>
//#include <retroshare/rshistory.h>

#include "util/rsstring.h"

#include <stdio.h>

#include <iostream>
#include <algorithm>

#include <set>

#define MAX_DESIRED_RATE 1000.0 // 1Mb/s

#define MIN_STREAM_CHUNK_SIZE	10
#define MAX_STREAM_CHUNK_SIZE	100000

#define STREAM_STANDARD_MIN_DT		0.1
#define STREAM_BACKGROUND_MIN_DT	0.5


bool fill_stream_details(rsctrl::stream::ResponseStreamDetail &resp, 
			const std::list<RpcStream> &streams);
bool fill_stream_desc(rsctrl::stream::StreamDesc &desc, 
			const RpcStream &stream);

bool fill_stream_data(rsctrl::stream::StreamData &data, const RpcStream &stream);

bool createQueuedStreamMsg(const RpcStream &stream, rsctrl::stream::ResponseStreamData &resp, RpcQueuedMsg &qmsg);


RpcProtoStream::RpcProtoStream(uint32_t serviceId)
	:RpcQueueService(serviceId)
{ 
	mNextStreamId = 1;

	return; 
}


void RpcProtoStream::reset(uint32_t chan_id)
{
	// We should be using a mutex for all stream operations!!!!
	// TODO
        //RsStackMutex stack(mQueueMtx); /********** LOCKED MUTEX ***************/

	std::list<uint32_t> toRemove;
	std::map<uint32_t, RpcStream>::iterator it;
	for(it = mStreams.begin(); it != mStreams.end(); it++)
	{
		if (it->second.chan_id == chan_id)
		{
			toRemove.push_back(it->first);
		}
	}

	std::list<uint32_t>::iterator rit;
	for(rit = toRemove.begin(); rit != toRemove.end(); rit++)
	{
		it = mStreams.find(*rit);
		if (it != mStreams.end())
		{
			mStreams.erase(it);
		}
	}

	// Call the rest of reset.
	RpcQueueService::reset(chan_id);
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

		case rsctrl::stream::MsgId_RequestControlStream:
			processReqControlStream(chan_id, msg_id, req_id, msg);
			break;

		case rsctrl::stream::MsgId_RequestListStreams:
			processReqListStreams(chan_id, msg_id, req_id, msg);
			break;

//		case rsctrl::stream::MsgId_RequestRegisterStreams:
//			processReqRegisterStreams(chan_id, msg_id, req_id, msg);
//			break;

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
        std::list<std::string> hashes;
	hashes.push_back(req.file().hash());

        //HashExpression  exp(StringOperator::EqualsString, hashes);
        HashExpression  exp(EqualsString, hashes);
        std::list<DirDetails> results;

        FileSearchFlags flags = RS_FILE_HINTS_LOCAL;
        int ans = rsFiles->SearchBoolExp(&exp, results, flags);

	// CREATE A STREAM OBJECT.
	if (results.size() < 1)
	{
		success = false;
		errorMsg = "No Matching File";
	}
	else
	{
		DirDetails &dirdetail = results.front();

		RpcStream stream;
		stream.chan_id = chan_id;
		stream.req_id = req_id;  
		stream.stream_id = getNextStreamId();
		stream.state = RpcStream::RUNNING;

		// Convert to Full local path.
		std::string virtual_path = RsDirUtil::makePath(dirdetail.path, dirdetail.name);
                if (!rsFiles->ConvertSharedFilePath(virtual_path, stream.path))
		{
			success = false;
			errorMsg = "Cannot Match to Shared Directory";
		}

		stream.length = dirdetail.count;
		stream.hash = dirdetail.hash;
		stream.name = dirdetail.name;

		stream.offset = 0;
		stream.start_byte = 0;
		stream.end_byte = stream.length;
		stream.desired_rate = req.rate_kbs();

		if (stream.desired_rate > MAX_DESIRED_RATE)
		{
			stream.desired_rate = MAX_DESIRED_RATE;
		}

		// make response
		rsctrl::stream::StreamDesc *desc = resp.add_streams();
		if (!fill_stream_desc(*desc, stream))
		{
			success = false;
			errorMsg = "Failed to Invalid Action";
		}
		else
		{
			// insert.
			mStreams[stream.stream_id] = stream;

			// register the stream too.
			std::cerr << "RpcProtoStream::processReqStartFileStream() Registering the stream event.";
			std::cerr << std::endl;
        		registerForEvents(chan_id, req_id, REGISTRATION_STREAMS);
		}

		std::cerr << "RpcProtoStream::processReqStartFileStream() List of Registered Streams:";
		std::cerr << std::endl;
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
	it = mStreams.find(req.stream_id());
	if (it != mStreams.end())
	{
		// TWEAK

		if (it->second.state == RpcStream::STREAMERR)
		{
			if (req.action() == rsctrl::stream::RequestControlStream::STREAM_STOP)
			{
				it->second.state = RpcStream::FINISHED;
			}
			else
			{
				success = false;
				errorMsg = "Stream Error";
			}
		}
		else
		{
			switch(req.action())
			{
				case rsctrl::stream::RequestControlStream::STREAM_START:
					if (it->second.state == RpcStream::PAUSED)
					{
						it->second.state = RpcStream::RUNNING;
					}
					break;
	
				case rsctrl::stream::RequestControlStream::STREAM_STOP:
					it->second.state = RpcStream::FINISHED;
					break;
	
				case rsctrl::stream::RequestControlStream::STREAM_PAUSE:
					if (it->second.state == RpcStream::RUNNING)
					{
						it->second.state = RpcStream::PAUSED;
						it->second.transfer_time = 0;	// reset timings.
					}
					break;
	
				case rsctrl::stream::RequestControlStream::STREAM_CHANGE_RATE:
					it->second.desired_rate = req.rate_kbs();
					break;
	
				case rsctrl::stream::RequestControlStream::STREAM_SEEK:
					if (req.seek_byte() < it->second.end_byte)
					{
						it->second.offset = req.seek_byte();
					}
					break;
				default:
					success = false;
					errorMsg = "Invalid Action";
			}
		}

		// FILL IN REPLY.
		if (success)
		{
			rsctrl::stream::StreamDesc *desc = resp.add_streams();
			if (!fill_stream_desc(*desc, it->second))
			{
				success = false;
				errorMsg = "Invalid Action";
			}
		}

		// Cleanup - TODO, this is explicit at the moment. - should be automatic after finish.
		if (it->second.state == RpcStream::FINISHED)
		{
        		deregisterForEvents(it->second.chan_id, it->second.req_id, REGISTRATION_STREAMS);
			mStreams.erase(it);			
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



	std::map<uint32_t, RpcStream>::iterator it;
	for(it = mStreams.begin(); it != mStreams.end(); it++)
	{
		bool match = true;

		// TODO fill in match!
		/* check that it matches */
		if (! match)
		{
			continue;
		}

		rsctrl::stream::StreamDesc *desc = resp.add_streams();
		if (!fill_stream_desc(*desc, it->second))
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


        // EVENTS. (STREAMS)
int RpcProtoStream::locked_checkForEvents(uint32_t event, const std::list<RpcEventRegister> & /* registered */, std::list<RpcQueuedMsg> &stream_msgs)
{
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

	double ts = getTimeStamp();
	double dt = ts - mStreamRates.last_ts;
	uint32_t data_sent = 0;

#define FILTER_K (0.75)

	if (dt < 5.0) //mStreamRates.last_ts != 0)
	{
		mStreamRates.avg_dt = FILTER_K * mStreamRates.avg_dt 
				+ (1.0 - FILTER_K) * dt;
	}
	else
	{
		std::cerr << "RpcProtoStream::locked_checkForEvents() Large dT - resetting avg";
		std::cerr << std::endl;
		mStreamRates.avg_dt = 0.0;
	}

	mStreamRates.last_ts = ts;


	std::map<uint32_t, RpcStream>::iterator it;
	for(it = mStreams.begin(); it != mStreams.end(); it++)
	{
		RpcStream &stream = it->second;

		if (!(stream.state == RpcStream::RUNNING))
		{
			continue;
		}

		double stream_dt = ts - stream.transfer_time;

		switch(stream.transfer_type)
		{
			case RpcStream::REALTIME:
				// let it go through always.
				break;

			case RpcStream::STANDARD:
				if (stream_dt < STREAM_STANDARD_MIN_DT)
				{
					continue;
				}
				break;


			case RpcStream::BACKGROUND:
				if (stream_dt < STREAM_BACKGROUND_MIN_DT)
				{
					continue;
				}
				break;
		}

		if (!stream.transfer_time)
		{
			std::cerr << "RpcProtoStream::locked_checkForEvents() Null stream.transfer_time .. resetting";
			std::cerr << std::endl;
			stream.transfer_avg_dt = STREAM_STANDARD_MIN_DT;
		}
		else
		{
			std::cerr << "RpcProtoStream::locked_checkForEvents() stream.transfer_avg_dt: " << stream.transfer_avg_dt;
			std::cerr << " stream_dt: " << stream_dt;
			std::cerr << std::endl;
			stream.transfer_avg_dt = FILTER_K * stream.transfer_avg_dt
				+ (1.0 - FILTER_K) * stream_dt;

			std::cerr << "RpcProtoStream::locked_checkForEvents() ==> stream.transfer_avg_dt: " << stream.transfer_avg_dt;
			std::cerr << std::endl;
		}

		uint32_t size = stream.desired_rate * 1000.0 * stream.transfer_avg_dt;
		stream.transfer_time = ts;


		if (size < MIN_STREAM_CHUNK_SIZE)
		{
			size = MIN_STREAM_CHUNK_SIZE;
		}
		if (size > MAX_STREAM_CHUNK_SIZE)
		{
			size = MAX_STREAM_CHUNK_SIZE;
		}


		/* get data */
		uint64_t remaining = stream.end_byte - stream.offset;
		if (remaining < size)
		{
			size = remaining;
			stream.state = RpcStream::FINISHED;
			std::cerr << "RpcProtoStream::locked_checkForEvents() Sending Remaining: " << size;
			std::cerr << std::endl;
		}

		std::cerr << "RpcProtoStream::locked_checkForEvents() Handling Stream: " << stream.stream_id << " state: " << stream.state;
		std::cerr << std::endl;
		std::cerr << "path: " << stream.path;
		std::cerr << std::endl;
		std::cerr << "offset: " << stream.offset;
		std::cerr << " avg_dt: " << stream.transfer_avg_dt;
		std::cerr << " x  desired_rate: " << stream.desired_rate;
		std::cerr << " => chunk_size: " << size;
		std::cerr << std::endl;

		/* fill in the answer */

		rsctrl::stream::ResponseStreamData resp;
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::SUCCESS);

		rsctrl::stream::StreamData *data = resp.mutable_data();
		data->set_stream_id(stream.stream_id);

		// convert state.
		switch(stream.state)
		{
			case RpcStream::RUNNING:
				data->set_stream_state(rsctrl::stream::STREAM_STATE_RUN);
				break;
			// This case cannot happen.
			case RpcStream::PAUSED:
				data->set_stream_state(rsctrl::stream::STREAM_STATE_PAUSED);
				break;
			// This can only happen at last chunk.
			default:
			case RpcStream::FINISHED:
				data->set_stream_state(rsctrl::stream::STREAM_STATE_FINISHED);
				break;
		}


		rsctrl::core::Timestamp *ts = data->mutable_send_time();
		setTimeStamp(ts);

		data->set_offset(stream.offset);
		data->set_size(size);

		if (fill_stream_data(*data, stream))
		{

			/* increment seek_location - for next request */
			stream.offset += size;
	
			RpcQueuedMsg qmsg;
			if (createQueuedStreamMsg(stream, resp, qmsg))
			{
				std::cerr << "Created Stream Msg.";
				std::cerr << std::endl;
	
				stream_msgs.push_back(qmsg);
			}
			else
			{
				std::cerr << "ERROR Creating Stream Msg";
				std::cerr << std::endl;
			}
		}
		else
		{
			stream.state = RpcStream::STREAMERR;
			std::cerr << "ERROR Filling Stream Data";
			std::cerr << std::endl;
		}
		
	}
	return 1;
}



// TODO
int RpcProtoStream::cleanup_checkForEvents(uint32_t /* event */, const std::list<RpcEventRegister> & /* registered */)
{
	std::list<uint32_t> to_remove;
	std::list<uint32_t>::iterator rit;
	for(rit = to_remove.begin(); rit != to_remove.end(); rit++)
	{
		/* kill the stream! */
		std::map<uint32_t, RpcStream>::iterator it;
		it = mStreams.find(*rit);
		if (it != mStreams.end())
		{
			mStreams.erase(it);
		}
	}
	return 1;
}


/***** HELPER FUNCTIONS *****/

bool createQueuedStreamMsg(const RpcStream &stream, rsctrl::stream::ResponseStreamData &resp, RpcQueuedMsg &qmsg)
{
	std::string outmsg;
	if (!resp.SerializeToString(&outmsg))
	{
		std::cerr << "RpcProtoStream::createQueuedEventSendMsg() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return false;
	}
	
	// Correctly Name Message.
	qmsg.mMsgId = constructMsgId(rsctrl::core::CORE, rsctrl::core::STREAM, 
				rsctrl::stream::MsgId_ResponseStreamData, true);

	qmsg.mChanId = stream.chan_id;
	qmsg.mReqId = stream.req_id;
	qmsg.mMsg = outmsg;

	return true;
}




/****************** NEW HELPER FNS ******************/

bool fill_stream_details(rsctrl::stream::ResponseStreamDetail &resp, 
			const std::list<RpcStream> &streams)
{
	std::cerr << "fill_stream_details()";
	std::cerr << std::endl;

	bool val = true;
	std::list<RpcStream>::const_iterator it;
	for (it = streams.begin(); it != streams.end(); it++)
	{
		rsctrl::stream::StreamDesc *desc = resp.add_streams();
		val &= fill_stream_desc(*desc, *it);
	}

	return val;
}

bool fill_stream_desc(rsctrl::stream::StreamDesc &desc, const RpcStream &stream)
{
	std::cerr << "fill_stream_desc()";
	std::cerr << std::endl;

	return true;
}

bool fill_stream_data(rsctrl::stream::StreamData &data, const RpcStream &stream)
{
	/* fill the StreamData from stream */

	/* open file */
	FILE *fd = RsDirUtil::rs_fopen(stream.path.c_str(), "rb");
	if (!fd)
	{
		std::cerr << "fill_stream_data() Failed to open file: " << stream.path;
		std::cerr << std::endl;
		return false;
	}

	uint32_t data_size    = data.size();
	uint64_t base_loc     = data.offset();
	void *buffer = malloc(data_size);

	/* seek to correct spot */
	fseeko64(fd, base_loc, SEEK_SET);

	/* copy data into bytes */
	if (1 != fread(buffer, data_size, 1, fd))
	{
		std::cerr << "fill_stream_data() Failed to get data. data_size=" << data_size << ", base_loc=" << base_loc << " !";
		std::cerr << std::endl;
		return false;
	}

	data.set_stream_data(buffer, data_size);
	free(buffer);

	fclose(fd);

	return true;
}




uint32_t RpcProtoStream::getNextStreamId()
{
	return mNextStreamId++;
}


