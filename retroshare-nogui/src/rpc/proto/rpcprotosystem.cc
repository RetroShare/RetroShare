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

#include "rpc/proto/rpcprotosystem.h"
#include "rpc/proto/gencc/system.pb.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsconfig.h>
#include <retroshare/rsiface.h>
#include <retroshare/rsdht.h>

#include <iostream>
#include <algorithm>

// NASTY GLOBAL VARIABLE HACK - NEED TO THINK OF A BETTER SYSTEM.
uint16_t RpcProtoSystem::mExtPort = 0;

RpcProtoSystem::RpcProtoSystem(uint32_t serviceId)
	:RpcQueueService(serviceId)
{ 
	return; 
}

//RpcProtoSystem::msgsAccepted(std::list<uint32_t> &msgIds); /* not used at the moment */

int RpcProtoSystem::processMsg(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg)
{
	/* check the msgId */
	uint8_t topbyte = getRpcMsgIdExtension(msg_id); 
	uint16_t service = getRpcMsgIdService(msg_id); 
	uint8_t submsg  = getRpcMsgIdSubMsg(msg_id);
	bool isResponse = isRpcMsgIdResponse(msg_id);


	std::cerr << "RpcProtoSystem::processMsg() topbyte: " << (int32_t) topbyte;
	std::cerr << " service: " << (int32_t) service << " submsg: " << (int32_t) submsg;
	std::cerr << std::endl;

	if (isResponse)
	{
		std::cerr << "RpcProtoSystem::processMsg() isResponse() - not processing";
		std::cerr << std::endl;
		return 0;
	}


	if (topbyte != (uint8_t) rsctrl::core::CORE)
	{
		std::cerr << "RpcProtoSystem::processMsg() Extension Mismatch - not processing";
		std::cerr << std::endl;
		return 0;
	}

	if (service != (uint16_t) rsctrl::core::SYSTEM)
	{
		std::cerr << "RpcProtoSystem::processMsg() Service Mismatch - not processing";
		std::cerr << std::endl;
		return 0;
	}

	if (!rsctrl::system::RequestMsgIds_IsValid(submsg))
	{
		std::cerr << "RpcProtoSystem::processMsg() SubMsg Mismatch - not processing";
		std::cerr << std::endl;
		return 0;
	}

	switch(submsg)
	{
		case rsctrl::system::MsgId_RequestSystemStatus:
			processSystemStatus(chan_id, msg_id, req_id, msg);
			break;
#if 0 
		case rsctrl::system::MsgId_RequestSystemQuit:
			processSystemQuit(chan_id, msg_id, req_id, msg);
			break;
#endif
		case rsctrl::system::MsgId_RequestSystemExternalAccess:
			processSystemExternalAccess(chan_id, msg_id, req_id, msg);
			break;
		default:
			std::cerr << "RpcProtoSystem::processMsg() ERROR should never get here";
			std::cerr << std::endl;
			return 0;
	}

	/* must have matched id to get here */
	return 1;
}


int RpcProtoSystem::processSystemStatus(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoSystem::processSystemStatus()";
	std::cerr << std::endl;

	// parse msg.
	rsctrl::system::RequestSystemStatus req;
	if (!req.ParseFromString(msg))
	{
		std::cerr << "RpcProtoSystem::processSystemStatus() ERROR ParseFromString()";
		std::cerr << std::endl;
		return 0;
	}

	// NO Options... so go straight to answer.
	// response.
	rsctrl::system::ResponseSystemStatus resp;
	bool success = true;

	unsigned int nTotal = 0;
	unsigned int nConnected = 0;
	rsPeers->getPeerCount(&nTotal, &nConnected, false);

	float downKb = 0;
	float upKb = 0;
	rsConfig->GetCurrentDataRates(downKb, upKb);

	// set the data.
	resp.set_no_peers(nTotal);
	resp.set_no_connected(nConnected);

	rsctrl::core::Bandwidth *bw = resp.mutable_bw_total();
	bw->set_up(upKb);
	bw->set_down(downKb);
	bw->set_name("Total Connection Bandwidth");

	uint32_t netState = rsConfig->getNetState();
	std::string natState("Unknown");
	rsctrl::system::ResponseSystemStatus_NetCode protoCode;

	switch(netState)
	{
		default:
		case RSNET_NETSTATE_BAD_UNKNOWN:
			protoCode = rsctrl::system::ResponseSystemStatus::BAD_UNKNOWN;
			break;

		case RSNET_NETSTATE_BAD_OFFLINE:
			protoCode = rsctrl::system::ResponseSystemStatus::BAD_OFFLINE;
			break;

		case RSNET_NETSTATE_BAD_NATSYM:
			protoCode = rsctrl::system::ResponseSystemStatus::BAD_NATSYM;
			break;

		case RSNET_NETSTATE_BAD_NODHT_NAT:
			protoCode = rsctrl::system::ResponseSystemStatus::BAD_NODHT_NAT;
			break;

		case RSNET_NETSTATE_WARNING_RESTART:
			protoCode = rsctrl::system::ResponseSystemStatus::WARNING_RESTART;
			break;

		case RSNET_NETSTATE_WARNING_NATTED:
			protoCode = rsctrl::system::ResponseSystemStatus::WARNING_NATTED;
			break;

		case RSNET_NETSTATE_WARNING_NODHT:
			protoCode = rsctrl::system::ResponseSystemStatus::WARNING_NODHT;
			break;

		case RSNET_NETSTATE_GOOD:
			protoCode = rsctrl::system::ResponseSystemStatus::GOOD;
			break;

		case RSNET_NETSTATE_ADV_FORWARD:
			protoCode = rsctrl::system::ResponseSystemStatus::ADV_FORWARD;
			break;
	}

	resp.set_net_status(protoCode);


        if (success)
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::SUCCESS);
	}
	else
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::FAILED);
		status->set_msg("Unknown ERROR");
	}

	std::string outmsg;
	if (!resp.SerializeToString(&outmsg))
	{
		std::cerr << "RpcProtoSystem::processSystemStatus() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = constructMsgId(rsctrl::core::CORE, rsctrl::core::SYSTEM, 
				rsctrl::system::MsgId_ResponseSystemStatus, true);

	// queue it.
	queueResponse(chan_id, out_msg_id, req_id, outmsg);

	return 1;
}



int RpcProtoSystem::processSystemQuit(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoSystem::processSystemQuit()";
	std::cerr << std::endl;

	// parse msg.
	rsctrl::system::RequestSystemQuit req;
	if (!req.ParseFromString(msg))
	{
		std::cerr << "RpcProtoSystem::processSystemQuit() ERROR ParseFromString()";
		std::cerr << std::endl;
		return 0;
	}

	// NO Options... so go straight to answer.
	// response.
	rsctrl::system::ResponseSystemQuit resp;
	bool success = true;

	switch(req.quit_code())
	{
		default:
		case rsctrl::system::RequestSystemQuit::CLOSE_CHANNEL:
		{
				RpcServer *server = getRpcServer();
				server->error(chan_id, "CLOSE_CHANNEL");

			break;
		}
		case rsctrl::system::RequestSystemQuit::SHUTDOWN_RS:
		{
				rsicontrol->rsGlobalShutDown();
			break;
		}
	}

        if (success)
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::SUCCESS);
	}
	else
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::FAILED);
		status->set_msg("Unknown ERROR");
	}

	std::string outmsg;
	if (!resp.SerializeToString(&outmsg))
	{
		std::cerr << "RpcProtoSystem::processSystemQuit() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = constructMsgId(rsctrl::core::CORE, rsctrl::core::SYSTEM, 
				rsctrl::system::MsgId_ResponseSystemQuit, true);

	// queue it.
	queueResponse(chan_id, out_msg_id, req_id, outmsg);

	return 1;
}


int RpcProtoSystem::processSystemExternalAccess(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoSystem::processSystemExternalAccess()";
	std::cerr << std::endl;

	// parse msg.
	rsctrl::system::RequestSystemExternalAccess req;
	if (!req.ParseFromString(msg))
	{
		std::cerr << "RpcProtoSystem::processSystemExternalAccess() ERROR ParseFromString()";
		std::cerr << std::endl;
		return 0;
	}

	// NO Options... so go straight to answer.
	// response.
	rsctrl::system::ResponseSystemExternalAccess resp;
	bool success = true;


	std::string dhtKey;
	if (!rsDht->getOwnDhtId(dhtKey))
	{
		success = false;
	}
	// have to set something anyway!.
	resp.set_dht_key(dhtKey);
        // NASTY GLOBAL VARIABLE HACK - NEED TO THINK OF A BETTER SYSTEM.
        resp.set_ext_port(mExtPort);

        if (success)
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::SUCCESS);
	}
	else
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::FAILED);
		status->set_msg("Unknown ERROR");
	}

	std::string outmsg;
	if (!resp.SerializeToString(&outmsg))
	{
		std::cerr << "RpcProtoSystem::processSystemExternalAccess() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = constructMsgId(rsctrl::core::CORE, rsctrl::core::SYSTEM, 
				rsctrl::system::MsgId_ResponseSystemExternalAccess, true);

	// queue it.
	queueResponse(chan_id, out_msg_id, req_id, outmsg);

	return 1;
}


