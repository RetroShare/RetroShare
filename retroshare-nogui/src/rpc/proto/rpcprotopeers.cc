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

#include "rpc/proto/rpcprotopeers.h"
#include "rpc/proto/gencc/peers.pb.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsdisc.h>

#include <iostream>
#include <algorithm>

RpcProtoPeers::RpcProtoPeers(uint32_t serviceId)
	:RpcQueueService(serviceId)
{ 
	return; 
}

//RpcProtoPeers::msgsAccepted(std::list<uint32_t> &msgIds); /* not used at the moment */

int RpcProtoPeers::processMsg(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg)
{
	/* check the msgId */
	uint8_t topbyte = getRpcMsgIdExtension(msg_id); 
	uint16_t service = getRpcMsgIdService(msg_id); 
	uint8_t submsg  = getRpcMsgIdSubMsg(msg_id);
	bool isResponse = isRpcMsgIdResponse(msg_id);


	std::cerr << "RpcProtoPeers::processMsg() topbyte: " << (int32_t) topbyte;
	std::cerr << " service: " << (int32_t) service << " submsg: " << (int32_t) submsg;
	std::cerr << std::endl;

	if (isResponse)
	{
		std::cerr << "RpcProtoPeers::processMsg() isResponse() - not processing";
		std::cerr << std::endl;
		return 0;
	}


	if (topbyte != (uint8_t) rsctrl::core::CORE)
	{
		std::cerr << "RpcProtoPeers::processMsg() Extension Mismatch - not processing";
		std::cerr << std::endl;
		return 0;
	}

	if (service != (uint16_t) rsctrl::core::PEERS)
	{
		std::cerr << "RpcProtoPeers::processMsg() Service Mismatch - not processing";
		std::cerr << std::endl;
		return 0;
	}

	if (!rsctrl::peers::RequestMsgIds_IsValid(submsg))
	{
		std::cerr << "RpcProtoPeers::processMsg() SubMsg Mismatch - not processing";
		std::cerr << std::endl;
		return 0;
	}

	switch(submsg)
	{
		case rsctrl::peers::MsgId_RequestPeers:
			processRequestPeers(chan_id, msg_id, req_id, msg);
			break;
		case rsctrl::peers::MsgId_RequestAddPeer:
			processAddPeer(chan_id, msg_id, req_id, msg);
			break;
		case rsctrl::peers::MsgId_RequestModifyPeer:
			processModifyPeer(chan_id, msg_id, req_id, msg);
			break;
		default:
			std::cerr << "RpcProtoPeers::processMsg() ERROR should never get here";
			std::cerr << std::endl;
			return 0;
	}

	/* must have matched id to get here */
	return 1;
}


int RpcProtoPeers::processAddPeer(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoPeers::processAddPeer() NOT FINISHED";
	std::cerr << std::endl;

	// response.
	rsctrl::peers::ResponseAddPeer resp;
	bool success = false;

        if (success)
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::SUCCESS);
	}
	else
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::NO_IMPL_YET);
	}


	std::string outmsg;
	if (!resp.SerializeToString(&outmsg))
	{
		std::cerr << "RpcProtoPeers::processAddPeer() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = constructMsgId(rsctrl::core::CORE, rsctrl::core::PEERS, 
				rsctrl::peers::MsgId_ResponseAddPeer, true);

	// queue it.
	queueResponse(chan_id, out_msg_id, req_id, outmsg);

	return 1;
}


int RpcProtoPeers::processModifyPeer(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoPeers::processModifyPeer() NOT FINISHED";
	std::cerr << std::endl;


	// response.
	rsctrl::peers::ResponseModifyPeer resp;
	bool success = false;

        if (success)
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::SUCCESS);
	}
	else
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::NO_IMPL_YET);
	}


	std::string outmsg;
	if (!resp.SerializeToString(&outmsg))
	{
		std::cerr << "RpcProtoPeers::processModifyPeer() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = constructMsgId(rsctrl::core::CORE, rsctrl::core::PEERS, 
				rsctrl::peers::MsgId_ResponseModifyPeer, true);

	// queue it.
	queueResponse(chan_id, out_msg_id, req_id, outmsg);

	return 1;
}



int RpcProtoPeers::processRequestPeers(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoPeers::processRequestPeers()";
	std::cerr << std::endl;

	// parse msg.
	rsctrl::peers::RequestPeers reqp;
	if (!reqp.ParseFromString(msg))
	{
		std::cerr << "RpcProtoPeers::processRequestPeers() ERROR ParseFromString()";
		std::cerr << std::endl;
		return 0;
	}

	// Get the list of gpg_id to generate data for.
	std::list<std::string> ids;
	bool onlyConnected = false;
        bool success = true;
	switch(reqp.set())
	{
		case rsctrl::peers::RequestPeers::OWNID:
		{
			std::cerr << "RpcProtoPeers::processRequestPeers() OWNID";
			std::cerr << std::endl;
			std::string own_id = rsPeers->getGPGOwnId();
			ids.push_back(own_id);
			break;
		}
		case rsctrl::peers::RequestPeers::LISTED: 
		{
			std::cerr << "RpcProtoPeers::processRequestPeers() LISTED";
			std::cerr << std::endl;
			int no_gpg_ids = reqp.gpg_ids_size();
			for (int i = 0; i < no_gpg_ids; i++)
			{
				std::string listed_id = reqp.gpg_ids(i);
				std::cerr << "RpcProtoPeers::processRequestPeers() Adding Id: " << listed_id;
				std::cerr << std::endl;
				ids.push_back(listed_id);
			}
			break;

		}
		case rsctrl::peers::RequestPeers::ALL:
			std::cerr << "RpcProtoPeers::processRequestPeers() ALL";
			std::cerr << std::endl;
			rsPeers->getGPGAllList(ids);
			break;
		case rsctrl::peers::RequestPeers::CONNECTED:
		{
			std::cerr << "RpcProtoPeers::processRequestPeers() CONNECTED";
			std::cerr << std::endl;
			 /* this ones a bit hard too */
			onlyConnected = true;
			std::list<std::string> ssl_ids;
			std::list<std::string>::const_iterator sit;
			rsPeers->getOnlineList(ssl_ids);
			for(sit = ssl_ids.begin(); sit != ssl_ids.end(); sit++)
			{
				std::string gpg_id = rsPeers->getGPGId(*sit);
				if (gpg_id.size() > 0)
				{
					if (std::find(ids.begin(), ids.end(),gpg_id) == ids.end())
					{
						ids.push_back(gpg_id);
					}
				}
			}
			break;
		}
		case rsctrl::peers::RequestPeers::FRIENDS:
			std::cerr << "RpcProtoPeers::processRequestPeers() FRIENDS";
			std::cerr << std::endl;
			rsPeers->getGPGAcceptedList(ids);
			break;
		case rsctrl::peers::RequestPeers::SIGNED:
			std::cerr << "RpcProtoPeers::processRequestPeers() SIGNED";
			std::cerr << std::endl;
			rsPeers->getGPGSignedList(ids);
			break;
		case rsctrl::peers::RequestPeers::VALID:
			std::cerr << "RpcProtoPeers::processRequestPeers() VALID";
			std::cerr << std::endl;
			rsPeers->getGPGSignedList(ids);
			break;
	}


	// work out what data we need to request.
	bool getLocations = false;
	switch(reqp.info())
	{
		default:
		case rsctrl::peers::RequestPeers::NAMEONLY:
		case rsctrl::peers::RequestPeers::BASIC:
			break;
		case rsctrl::peers::RequestPeers::LOCATION:
		case rsctrl::peers::RequestPeers::ALLINFO:
			getLocations = true;
			break;
	}

	// response.
	rsctrl::peers::ResponsePeerList respp;

	/* now iterate through the peers and fill in the response. */
	std::list<std::string>::const_iterator git;
	for(git = ids.begin(); git != ids.end(); git++)
	{

		RsPeerDetails details;
		if (!rsPeers->getGPGDetails(*git, details))
		{
			std::cerr << "RpcProtoPeers::processRequestPeers() ERROR Finding GPGID: ";
			std::cerr << *git;
			std::cerr << std::endl;
			continue; /* uhm.. */
		}

		rsctrl::core::Person *person = respp.add_peers();

		/* fill in key gpg details */
		person->set_gpg_id(*git);
		person->set_name(details.name);

		std::cerr << "RpcProtoPeers::processRequestPeers() Adding GPGID: ";
		std::cerr << *git << " name: " << details.name;
		std::cerr << std::endl;

		//if (details.state & RS_PEER_STATE_FRIEND)
		if (*git == rsPeers->getGPGOwnId())
		{
			std::cerr << "RpcProtoPeers::processRequestPeers() Relation YOURSELF";
			std::cerr << std::endl;
			person->set_relation(rsctrl::core::Person::YOURSELF);
		}
		else if (rsPeers->isGPGAccepted(*git))
		{
			std::cerr << "RpcProtoPeers::processRequestPeers() Relation FRIEND";
			std::cerr << std::endl;
			person->set_relation(rsctrl::core::Person::FRIEND);
		}
		else 
		{
			std::list<std::string> common_friends;
			rsDisc->getDiscGPGFriends(*git, common_friends);
			int size = common_friends.size();
			if (size)
			{
				if (size > 2)
				{
					std::cerr << "RpcProtoPeers::processRequestPeers() Relation FRIEND_OF_MANY_FRIENDS";
					std::cerr << std::endl;
					person->set_relation(rsctrl::core::Person::FRIEND_OF_MANY_FRIENDS);
				}
				else
				{
					std::cerr << "RpcProtoPeers::processRequestPeers() Relation FRIEND_OF_FRIENDS";
					std::cerr << std::endl;
					person->set_relation(rsctrl::core::Person::FRIEND_OF_FRIENDS);
				}
			}
			else
			{
				std::cerr << "RpcProtoPeers::processRequestPeers() Relation UNKNOWN";
				std::cerr << std::endl;
				person->set_relation(rsctrl::core::Person::UNKNOWN);
			}
		}
		
		if (getLocations)
		{
			std::list<std::string> ssl_ids;
			std::list<std::string>::const_iterator sit;

			if (!rsPeers->getAssociatedSSLIds(*git, ssl_ids))
			{
				std::cerr << "RpcProtoPeers::processRequestPeers() No Locations";
				std::cerr << std::endl;
				continue; /* end of this peer */
			}

			for(sit = ssl_ids.begin(); sit != ssl_ids.end(); sit++)
			{
				RsPeerDetails ssldetails;
				if (!rsPeers->getPeerDetails(*sit, ssldetails))
				{
					continue; /* uhm.. */
				}
				if ((onlyConnected) && 
					(!(ssldetails.state & RS_PEER_STATE_CONNECTED)))
				{
					continue;
				}

				rsctrl::core::Location *loc = person->add_locations();

				std::cerr << "RpcProtoPeers::processRequestPeers() \t Adding Location: ";
				std::cerr << *sit << " loc: " << ssldetails.location;
				std::cerr << std::endl;

				/* fill in ssl details */
				loc->set_ssl_id(*sit);
				loc->set_location(ssldetails.location);

				/* set addresses */
				rsctrl::core::IpAddr *laddr = loc->mutable_localaddr();
				laddr->set_addr(ssldetails.localAddr);
				laddr->set_port(ssldetails.localPort);

				rsctrl::core::IpAddr *eaddr = loc->mutable_extaddr();
				eaddr->set_addr(ssldetails.extAddr);
				eaddr->set_port(ssldetails.extPort);

				/* translate status */
				uint32_t loc_state = 0;
				//dont think this state should be here.
				//if (ssldetails.state & RS_PEER_STATE_FRIEND)
				if (ssldetails.state & RS_PEER_STATE_ONLINE)
				{
					loc_state |= (uint32_t) rsctrl::core::Location::ONLINE;
				}
				if (ssldetails.state & RS_PEER_STATE_CONNECTED)
				{
					loc_state |= (uint32_t) rsctrl::core::Location::CONNECTED;
				}
				if (ssldetails.state & RS_PEER_STATE_UNREACHABLE)
				{
					loc_state |= (uint32_t) rsctrl::core::Location::UNREACHABLE;
				}

				loc->set_state(loc_state);
			}
		}
	}

        if (success)
	{
		rsctrl::core::Status *status = respp.mutable_status();
		status->set_code(rsctrl::core::Status::SUCCESS);
	}
	else
	{
		rsctrl::core::Status *status = respp.mutable_status();
		status->set_code(rsctrl::core::Status::FAILED);
		status->set_msg("Unknown ERROR");
	}


	std::string outmsg;
	if (!respp.SerializeToString(&outmsg))
	{
		std::cerr << "RpcProtoPeers::processRequestPeers() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = constructMsgId(rsctrl::core::CORE, rsctrl::core::PEERS, 
				rsctrl::peers::MsgId_ResponsePeerList, true);

	// queue it.
	queueResponse(chan_id, out_msg_id, req_id, outmsg);

	return 1;
}


