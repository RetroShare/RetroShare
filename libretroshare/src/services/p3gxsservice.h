/*
 * libretroshare/src/services p3gxsservice.h
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

#ifndef P3_GXS_SERVICE_HEADER
#define P3_GXS_SERVICE_HEADER

#include "services/p3service.h"

/* 
 * This class provides useful generic support for GXS style services.
 * I expect much of this will be incorporated into the base GXS.
 *
 */

#define GXS_REQUEST_STATUS_FAILED		0
#define GXS_REQUEST_STATUS_PENDING		1
#define GXS_REQUEST_STATUS_PARTIAL		2
#define GXS_REQUEST_STATUS_FINISHED_INCOMPLETE	3
#define GXS_REQUEST_STATUS_COMPLETE		4
#define GXS_REQUEST_STATUS_DONE			5 // ONCE ALL DATA RETRIEVED.

#define GXS_REQUEST_TYPE_LIST			0x00010000
#define GXS_REQUEST_TYPE_DATA			0x00020000

#define GXS_REQUEST_TYPE_GROUPS			0x01000000
#define GXS_REQUEST_TYPE_MSGS			0x02000000

class gxsRequest
{
	public:
	uint32_t token;
	uint32_t reqTime;
	uint32_t reqType; /* specific to service */
	uint32_t status;

	std::list<std::string> IdList;
	std::map<std::string, void *> readyData;
};




class p3GxsService: public p3Service
{
	protected:

	p3GxsService(uint16_t type);

	public:

//virtual ~p3Service() { p3Service::~p3Service(); return; }

bool	generateToken(uint32_t &token);
bool	storeRequest(const uint32_t &token, const uint32_t &type, const std::list<std::string> &ids);
bool	clearRequest(const uint32_t &token);

bool	updateRequestStatus(const uint32_t &token, const uint32_t &status);
bool	updateRequestList(const uint32_t &token, std::list<std::string> ids);
bool	updateRequestData(const uint32_t &token, std::map<std::string, void *> data);
bool    checkRequestStatus(const uint32_t &token, uint32_t &status, uint32_t &reqtype, time_t &ts);

	// special ones for testing (not in final design)
bool    tokenList(std::list<uint32_t> &tokens);
bool    popRequestList(const uint32_t &token, std::string &id);
bool    fakeprocessrequests();

	private:

	RsMutex mReqMtx;

	uint32_t mNextToken;

	std::map<uint32_t, gxsRequest> mRequests;

};


#endif // P3_GXS_SERVICE_HEADER

