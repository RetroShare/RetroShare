/*******************************************************************************
 * libretroshare/src/gxs: gxstokenqueue.h                                      *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2012 by Robert Fernie        <retroshare@lunamutt.com>       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#ifndef R_GXS_TOKEN_QUEUE_H
#define R_GXS_TOKEN_QUEUE_H

#include "gxs/rsgenexchange.h"
#include "retroshare/rsservicecontrol.h"
#include "util/rsthreads.h"


struct GxsTokenQueueItem
{
public:

	GxsTokenQueueItem(const uint32_t token, const uint32_t req_type) :
	    mToken(token), mReqType(req_type), mStatus(RsTokenService::PENDING) {}

	GxsTokenQueueItem(): mToken(0), mReqType(0), mStatus(RsTokenService::PENDING) {}

	uint32_t mToken;
	uint32_t mReqType;
	RsTokenService::GxsRequestStatus mStatus;
};


/**
 * A little helper class, to manage callbacks from requests
 */
class GxsTokenQueue
{
public:
	explicit GxsTokenQueue(RsGenExchange *gxs) :
	    mGenExchange(gxs), mQueueMtx("GxsTokenQueueMtx") {}

	bool queueRequest(uint32_t token, uint32_t req_type);
	void checkRequests(); /// must be called by

protected:

	/// This must be overloaded to complete the functionality.
	virtual void handleResponse(uint32_t token, uint32_t req_type
	                            , RsTokenService::GxsRequestStatus status) = 0;

private:
	RsGenExchange *mGenExchange;
	RsMutex mQueueMtx;
	std::list<GxsTokenQueueItem> mQueue;
};


#endif //R_GXS_TOKEN_QUEUE_H
