/*
 * libretroshare/src/services p3gxsservice.cc
 *
 * Generic Service Support Class for RetroShare.
 *
 * Copyright 2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
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

#include "services/p3gxsservice.h"

p3GxsService::p3GxsService(uint16_t type) 
	:p3Service(type),  mReqMtx("p3GxsService")
{
	mNextToken = 0;
	return; 
}

bool	p3GxsService::generateToken(uint32_t &token)
{
	RsStackMutex stack(mReqMtx); /****** LOCKED *****/

	token = mNextToken++;

	return true;
}

bool	p3GxsService::storeRequest(const uint32_t &token, const uint32_t &type, const std::list<std::string> &ids)
{
	RsStackMutex stack(mReqMtx); /****** LOCKED *****/

	gxsRequest req;
	req.token = token;
	req.reqTime = time(NULL);
	req.reqType = type;
	req.status = GXS_REQUEST_STATUS_PENDING;
	req.IdList = ids;
	
	mRequests[token] = req;

	return true;
}


bool	p3GxsService::clearRequest(const uint32_t &token)
{
	RsStackMutex stack(mReqMtx); /****** LOCKED *****/

        std::map<uint32_t, gxsRequest>::iterator it;

	it = mRequests.find(token);
	if (it == mRequests.end())
	{
		return false;
	}

	mRequests.erase(it);

	return true;
}

bool	p3GxsService::updateRequestStatus(const uint32_t &token, const uint32_t &status)
{
	RsStackMutex stack(mReqMtx); /****** LOCKED *****/

        std::map<uint32_t, gxsRequest>::iterator it;

	it = mRequests.find(token);
	if (it == mRequests.end())
	{
		return false;
	}

	it->second.status = status;

	return true;
}

bool	p3GxsService::updateRequestList(const uint32_t &token, std::list<std::string> ids)
{
	RsStackMutex stack(mReqMtx); /****** LOCKED *****/

        std::map<uint32_t, gxsRequest>::iterator it;

	it = mRequests.find(token);
	if (it == mRequests.end())
	{
		return false;
	}

	std::list<std::string>::iterator iit;
	for(iit = ids.begin(); iit != ids.end(); iit++)
	{
		it->second.IdList.push_back(*iit);
	}

	return true;
}

bool	p3GxsService::updateRequestData(const uint32_t &token, std::map<std::string, void *> data)
{
	RsStackMutex stack(mReqMtx); /****** LOCKED *****/

        std::map<uint32_t, gxsRequest>::iterator it;

	it = mRequests.find(token);
	if (it == mRequests.end())
	{
		return false;
	}

	std::map<std::string, void *>::iterator dit;
	for(dit = data.begin(); dit != data.end(); dit++)
	{
		it->second.readyData[dit->first] = dit->second;
	}

	return true;
}

bool    p3GxsService::checkRequestStatus(const uint32_t &token, uint32_t &status, uint32_t &reqtype, time_t &ts)
{
	RsStackMutex stack(mReqMtx); /****** LOCKED *****/

        std::map<uint32_t, gxsRequest>::iterator it;

	it = mRequests.find(token);
	if (it == mRequests.end())
	{
		return false;
	}
	
	status = it->second.status;
	reqtype = it->second.reqType;
	ts = it->second.reqTime;

	return true;
}


	// special ones for testing (not in final design)
bool    p3GxsService::tokenList(std::list<uint32_t> &tokens)
{
	RsStackMutex stack(mReqMtx); /****** LOCKED *****/

        std::map<uint32_t, gxsRequest>::iterator it;

	for(it = mRequests.begin(); it != mRequests.end(); it++)
	{
		tokens.push_back(it->first);
	}

	return true;
}

bool    p3GxsService::popRequestList(const uint32_t &token, std::string &id)
{
	RsStackMutex stack(mReqMtx); /****** LOCKED *****/

        std::map<uint32_t, gxsRequest>::iterator it;

	it = mRequests.find(token);
	if (it == mRequests.end())
	{
		return false;
	}

	if (it->second.IdList.size() == 0)
	{
		return false;
	}

	id = it->second.IdList.front();
	it->second.IdList.pop_front();

	return true;
}


