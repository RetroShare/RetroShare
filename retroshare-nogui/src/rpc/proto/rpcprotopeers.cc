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
#include <iostream>
#include <algorithm>

RpcProtoPeers::RpcProtoPeers(uint32_t serviceId)
	:RpcQueueService(serviceId)
{ 
	return; 
}

//RpcProtoPeers::msgsAccepted(std::list<uint32_t> &msgIds); /* not used at the moment */

int RpcProtoPeers::processMsg(uint32_t msg_id, uint32_t req_id, const std::string &msg)
{
	/* check the msgId */
	uint8_t topbyte = 0;
	uint8_t service = 0;
	uint8_t submsg  = 0;

	std::cerr << "RpcProtoPeers::processMsg() topbyte: " << topbyte;
	std::cerr << " service: " << service << " submsg: " << submsg;
	std::cerr << std::endl;

	if (service != (uint8_t) rsctrl::peers::BASE)
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
			processRequestPeers(msg_id, req_id, msg);
			break;
		case rsctrl::peers::MsgId_RequestAddPeer:
			processAddPeer(msg_id, req_id, msg);
			break;
		case rsctrl::peers::MsgId_RequestModifyPeer:
			processModifyPeer(msg_id, req_id, msg);
			break;
		default:
			std::cerr << "RpcProtoPeers::processMsg() ERROR should never get here";
			std::cerr << std::endl;
			return 0;
	}

	/* must have matched id to get here */
	return 1;
}


int RpcProtoPeers::processAddPeer(uint32_t msg_id, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoPeers::processAddPeer() NOT FINISHED";
	std::cerr << std::endl;

	return 0;
}


int RpcProtoPeers::processModifyPeer(uint32_t msg_id, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoPeers::processModifyPeer() NOT FINISHED";
	std::cerr << std::endl;

	return 0;
}



int RpcProtoPeers::processRequestPeers(uint32_t msg_id, uint32_t req_id, const std::string &msg)
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
	bool onlyOnline = false;
	switch(reqp.set())
	{
		case rsctrl::peers::RequestPeers::OWNID:
		{
			std::string own_id = rsPeers->getGPGOwnId();
			ids.push_back(own_id);
			break;
		}
		case rsctrl::peers::RequestPeers::LISTED: 
		{
			/* extract ids from request (TODO) */
			std::string own_id = rsPeers->getGPGOwnId();
			ids.push_back(own_id);
			break;

		}
		case rsctrl::peers::RequestPeers::ALL:
			rsPeers->getGPGAllList(ids);
			break;
		case rsctrl::peers::RequestPeers::ONLINE:
		{
			 /* this ones a bit hard too */
			onlyOnline = true;
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
			rsPeers->getGPGAcceptedList(ids);
			break;
		case rsctrl::peers::RequestPeers::SIGNED:
			rsPeers->getGPGSignedList(ids);
			break;
		case rsctrl::peers::RequestPeers::VALID:
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
			continue; /* uhm.. */
		}

		rsctrl::base::Person *person = respp.add_peers();


		/* fill in key gpg details */



		
		if (getLocations)
		{
			std::list<std::string> ssl_ids;
			std::list<std::string>::const_iterator sit;

			if (!rsPeers->getAssociatedSSLIds(*git, ssl_ids))
			{
				continue; /* end of this peer */
			}

			for(sit = ssl_ids.begin(); sit != ssl_ids.end(); sit++)
			{
				RsPeerDetails ssldetails;
				if (!rsPeers->getPeerDetails(*sit, ssldetails))
				{
					continue; /* uhm.. */
				}

				rsctrl::base::Location *loc = person->add_locations();

				/* fill in ssl details */
			}
		}
	}


	std::string outmsg;
	if (!respp.SerializeToString(&outmsg))
	{
		std::cerr << "RpcProtoPeers::processRequestPeers() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = create_msg_id(
			rsctrl::peers::BASE, 
			rsctrl::peers::MsgId_ResponsePeerList);

	// queue it.
	queueResponse(out_msg_id, req_id, outmsg);
}





