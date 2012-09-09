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

#include "rpc/proto/rpcprotosearch.h"
#include "rpc/proto/gencc/search.pb.h"

#include "notifytxt.h"

#include <retroshare/rsturtle.h>
#include <retroshare/rsexpr.h>

#include "util/rsstring.h"

#include <stdio.h>

#include <iostream>
#include <algorithm>

#include <set>


RpcProtoSearch::RpcProtoSearch(uint32_t serviceId, NotifyTxt *notify)
	:RpcQueueService(serviceId), mNotify(notify), searchMtx("RpcProtoSearch")
{ 
	return; 
}

void RpcProtoSearch::reset(uint32_t chan_id)
{
	RpcQueueService::reset(chan_id);

	/* must clear all searches */
        clear_searches(chan_id);
}


int RpcProtoSearch::processMsg(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg)
{
	/* check the msgId */
	uint8_t topbyte = getRpcMsgIdExtension(msg_id); 
	uint16_t service = getRpcMsgIdService(msg_id); 
	uint8_t submsg  = getRpcMsgIdSubMsg(msg_id);
	bool isResponse = isRpcMsgIdResponse(msg_id);


	std::cerr << "RpcProtoSearch::processMsg() topbyte: " << (int32_t) topbyte;
	std::cerr << " service: " << (int32_t) service << " submsg: " << (int32_t) submsg;
	std::cerr << std::endl;

	if (isResponse)
	{
		std::cerr << "RpcProtoSearch::processMsg() isResponse() - not processing";
		std::cerr << std::endl;
		return 0;
	}

	if (topbyte != (uint8_t) rsctrl::core::CORE)
	{
		std::cerr << "RpcProtoSearch::processMsg() Extension Mismatch - not processing";
		std::cerr << std::endl;
		return 0;
	}

	if (service != (uint16_t) rsctrl::core::SEARCH)
	{
		std::cerr << "RpcProtoSearch::processMsg() Service Mismatch - not processing";
		std::cerr << std::endl;
		return 0;
	}

	if (!rsctrl::search::RequestMsgIds_IsValid(submsg))
	{
		std::cerr << "RpcProtoSearch::processMsg() SubMsg Mismatch - not processing";
		std::cerr << std::endl;
		return 0;
	}

	switch(submsg)
	{
		case rsctrl::search::MsgId_RequestBasicSearch:
			processReqBasicSearch(chan_id, msg_id, req_id, msg);
			break;

		case rsctrl::search::MsgId_RequestCloseSearch:
			processReqCloseSearch(chan_id, msg_id, req_id, msg);
			break;

		case rsctrl::search::MsgId_RequestListSearches:
			processReqListSearches(chan_id, msg_id, req_id, msg);
			break;

		case rsctrl::search::MsgId_RequestSearchResults:
			processReqSearchResults(chan_id, msg_id, req_id, msg);
			break;

		default:
			std::cerr << "RpcProtoSearch::processMsg() ERROR should never get here";
			std::cerr << std::endl;
			return 0;
	}

	/* must have matched id to get here */
	return 1;
}



int RpcProtoSearch::processReqBasicSearch(uint32_t chan_id, uint32_t /* msg_id */, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoSearch::processReqBasicSearch()";
	std::cerr << std::endl;

	// parse msg.
	rsctrl::search::RequestBasicSearch req;
	if (!req.ParseFromString(msg))
	{
		std::cerr << "RpcProtoSearch::processReqBasicSearch() ERROR ParseFromString()";
		std::cerr << std::endl;
		return 0;
	}

	// response.
	rsctrl::search::ResponseSearchIds resp;
	bool success = true;
	std::string errorMsg;

	/* convert msg parameters into local ones */
	std::list<std::string> terms;

	int no_terms = req.terms_size();
	for(int i = 0; i < no_terms; i++)
	{
		std::string term = req.terms(i);
		/* check for valid term? */
		if (term.size() > 0)
		{
			terms.push_back(term);
		}
	}

	NameExpression nameexp(ContainsAllStrings, terms, true);
	LinearizedExpression lexpr;
	nameexp.linearize(lexpr);

        uint32_t searchId = (uint32_t) rsTurtle->turtleSearch(lexpr);
        mNotify->collectSearchResults(searchId);

	/* add into search array */
	add_search(chan_id, searchId);

	/* add to answer */
	resp.add_search_id(searchId);

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
		std::cerr << "RpcProtoSearch::processReqBasicSearch() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = constructMsgId(rsctrl::core::CORE, rsctrl::core::SEARCH, 
				rsctrl::search::MsgId_ResponseSearchIds, true);

	// queue it.
	queueResponse(chan_id, out_msg_id, req_id, outmsg);

	return 1;
}


int RpcProtoSearch::processReqCloseSearch(uint32_t chan_id, uint32_t /* msg_id */, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoSearch::processReqCloseSearch()";
	std::cerr << std::endl;

	// parse msg.
	rsctrl::search::RequestCloseSearch req;
	if (!req.ParseFromString(msg))
	{
		std::cerr << "RpcProtoSearch::processReqCloseSearch() ERROR ParseFromString()";
		std::cerr << std::endl;
		return 0;
	}

	// response.
	rsctrl::search::ResponseSearchIds resp;
	bool success = true;
	std::string errorMsg;

	/* convert msg parameters into local ones */
        uint32_t searchId = req.search_id();


	/* remove into search array */
	if (!remove_search(chan_id, searchId))
	{
		success = false;
		errorMsg = "Unknown SearchId in List";
	}
	
	/* clear search results 
	 * we cannot cancel a turtle search
	 * so we tell notify to ignore further results
         */

	if (success)
	{
        	if (!mNotify->clearSearchId(searchId))
		{
			success = false;
			errorMsg = "Unknown SearchId in Notify";
		}
	}

	/* add to answer */
	if (success)
	{
		resp.add_search_id(searchId);
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
		std::cerr << "RpcProtoSearch::processReqCloseSearch() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = constructMsgId(rsctrl::core::CORE, rsctrl::core::SEARCH, 
				rsctrl::search::MsgId_ResponseSearchIds, true);

	// queue it.
	queueResponse(chan_id, out_msg_id, req_id, outmsg);

	return 1;
}


int RpcProtoSearch::processReqListSearches(uint32_t chan_id, uint32_t /* msg_id */, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoSearch::processReqListSearches()";
	std::cerr << std::endl;

	// parse msg.
	rsctrl::search::RequestListSearches req;
	if (!req.ParseFromString(msg))
	{
		std::cerr << "RpcProtoSearch::processReqListSearches() ERROR ParseFromString()";
		std::cerr << std::endl;
		return 0;
	}

	// response.
	rsctrl::search::ResponseSearchIds resp;
	bool success = true;
	std::string errorMsg;

	/* convert msg parameters into local ones */
	// Nothing to do.

	std::list<uint32_t> reg_search_ids;
	std::list<uint32_t>::iterator it;
	if (!get_search_list(chan_id, reg_search_ids))
	{
		/* warning */
		success = false;
		errorMsg = "No Searches Active";
	}

	/* iterate through search array */
	for(it = reg_search_ids.begin(); it != reg_search_ids.end(); it++)
	{
		/* add to answer */
		resp.add_search_id(*it);
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
		std::cerr << "RpcProtoSearch::processReqListSearches() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = constructMsgId(rsctrl::core::CORE, rsctrl::core::SEARCH, 
				rsctrl::search::MsgId_ResponseSearchIds, true);

	// queue it.
	queueResponse(chan_id, out_msg_id, req_id, outmsg);

	return 1;
}


int RpcProtoSearch::processReqSearchResults(uint32_t chan_id, uint32_t /* msg_id */, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoSearch::processReqSearchResults()";
	std::cerr << std::endl;

	// parse msg.
	rsctrl::search::RequestSearchResults req;
	if (!req.ParseFromString(msg))
	{
		std::cerr << "RpcProtoSearch::processReqSearchResults() ERROR ParseFromString()";
		std::cerr << std::endl;
		return 0;
	}

	// response.
	rsctrl::search::ResponseSearchResults resp;
	bool success = true;
	std::string errorMsg;

	/* convert msg parameters into local ones */
	std::list<uint32_t> reg_search_ids;
	std::list<uint32_t> requested_search_ids;
	if (!get_search_list(chan_id, reg_search_ids))
	{
		/* warning */
	}

	int no_searches = req.search_ids_size();
	if (no_searches)
	{
		/* painful check that they are our searches */
		for(int i = 0; i < no_searches; i++)
		{
			uint32_t search_id = req.search_ids(i);

			/* check that its in reg_search_ids */
			if (reg_search_ids.end() != std::find(reg_search_ids.begin(), reg_search_ids.end(), search_id))
			{
				/* error */
				continue;
			}
			requested_search_ids.push_back(search_id);
		}
	}
	else
	{
		/* all current searches */
		requested_search_ids = reg_search_ids;
	}


	std::list<uint32_t>::iterator rit;
	for(rit = requested_search_ids.begin(); 
			rit != requested_search_ids.end(); rit++)
	{
		rsctrl::search::SearchSet *set = resp.add_searches();
		/* add to answer */
		set->set_search_id(*rit);
		/* no search details at the moment */

		/* add into search array */
	        std::list<TurtleFileInfo>::iterator it;
	        std::list<TurtleFileInfo> searchResults;
		mNotify->getSearchResults(*rit, searchResults);

		/* convert into useful list */
		for(it = searchResults.begin(); it != searchResults.end(); it++)
		{
			/* add to answer */
			rsctrl::search::SearchHit *hit = set->add_hits();
			rsctrl::core::File *file = hit->mutable_file();

			file->set_hash(it->hash);
			file->set_name(it->name);
			file->set_size(it->size);

			// Uhm not provided for now. default to NETWORK
			hit->set_loc(rsctrl::search::SearchHit::NETWORK); 
			hit->set_no_hits(1); // No aggregation yet.
        	}
	}

	/* DONE - Generate Reply */
	/* different to others - partial success possible */
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
		std::cerr << "RpcProtoSearch::processReqSearchResults() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = constructMsgId(rsctrl::core::CORE, rsctrl::core::SEARCH, 
				rsctrl::search::MsgId_ResponseSearchResults, true);

	// queue it.
	queueResponse(chan_id, out_msg_id, req_id, outmsg);

	return 1;
}



/***** HELPER FUNCTIONS *****/

        // These private functions use Mutex below and manipulate mActiveSearches.
int RpcProtoSearch::get_search_list(uint32_t chan_id, std::list<uint32_t> &search_ids)
{
	std::cerr << "RpcProtoSearch::get_search_list(" << chan_id << ")";
	std::cerr << std::endl;

	RsStackMutex stack(searchMtx); /******* LOCKED *********/

        std::map<uint32_t, std::list<uint32_t> >::iterator mit;

	mit = mActiveSearches.find(chan_id);
	if (mit == mActiveSearches.end())
	{
		return 0;	
	}

	search_ids = mit->second;

	return 1;
}

int RpcProtoSearch::add_search(uint32_t chan_id, uint32_t search_id)
{
	std::cerr << "RpcProtoSearch::add_search(" << chan_id << ", " << search_id << ")";
	std::cerr << std::endl;

	RsStackMutex stack(searchMtx); /******* LOCKED *********/

        std::map<uint32_t, std::list<uint32_t> >::iterator mit;

	mit = mActiveSearches.find(chan_id);
	if (mit == mActiveSearches.end())
	{
		std::list<uint32_t> emptyList;
		mActiveSearches[chan_id] = emptyList;

		mit = mActiveSearches.find(chan_id);
	}

	/* sanity check */
	if (mit->second.end() != std::find(mit->second.begin(), mit->second.end(), search_id))
	{
		std::cerr << "RpcProtoSearch::add_search() ERROR search_id already exists";
		std::cerr << std::endl;
		return 0;
	}

	mit->second.push_back(search_id);
	return 1;
}

int RpcProtoSearch::remove_search(uint32_t chan_id, uint32_t search_id)
{
	std::cerr << "RpcProtoSearch::remove_search(" << chan_id << ", " << search_id << ")";
	std::cerr << std::endl;

	RsStackMutex stack(searchMtx); /******* LOCKED *********/

        std::map<uint32_t, std::list<uint32_t> >::iterator mit;

	mit = mActiveSearches.find(chan_id);
	if (mit == mActiveSearches.end())
	{
		std::cerr << "RpcProtoSearch::remove_search() ERROR search set doesn't exist";
		std::cerr << std::endl;
		return 0;
	}

	bool removed = false;
        std::list<uint32_t>::iterator lit;
	for(lit = mit->second.begin(); lit != mit->second.end();)
	{
		if (*lit == search_id)
		{
			lit = mit->second.erase(lit);
			if (removed)
			{
				std::cerr << "RpcProtoSearch::remove_search() ERROR removed multiple";
				std::cerr << std::endl;
			}
			removed = true;
		}
		else
		{
			lit++;
		}
	}

	if (removed)
		return 1;

	std::cerr << "RpcProtoSearch::remove_search() ERROR search_id not found";
	std::cerr << std::endl;

	return 0;
}

int RpcProtoSearch::clear_searches(uint32_t chan_id)
{
	std::cerr << "RpcProtoSearch::clear_searches(" << chan_id << ")";
	std::cerr << std::endl;

	RsStackMutex stack(searchMtx); /******* LOCKED *********/

        std::map<uint32_t, std::list<uint32_t> >::iterator mit;

	mit = mActiveSearches.find(chan_id);
	if (mit == mActiveSearches.end())
	{
		std::cerr << "RpcProtoSearch::clear_searches() WARNING search set not found";
		std::cerr << std::endl;
		return 0;	
	}

	mActiveSearches.erase(mit);
	return 1;
}

