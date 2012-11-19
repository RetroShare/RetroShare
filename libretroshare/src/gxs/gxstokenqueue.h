#ifndef R_GXS_TOKEN_QUEUE_H
#define R_GXS_TOKEN_QUEUE_H
/*
 * libretroshare/src/gxs gxstokenqueue.h
 *
 * Gxs Support for RetroShare.
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

#include "gxs/rsgenexchange.h"
#include "util/rsthreads.h"


/*
 *
 * A little helper class, to manage callbacks from requests
 *
 */

class GxsTokenQueueItem
{
	public:
	GxsTokenQueueItem(const uint32_t token, const uint32_t req_type)
	:mToken(token),mReqType(req_type) { return; }

	GxsTokenQueueItem(): mToken(0), mReqType(0) { return; }

	uint32_t mToken;
	uint32_t mReqType;
};


class GxsTokenQueue
{
	public:

	GxsTokenQueue(RsGenExchange *gxs)
	:mGenExchange(gxs), mQueueMtx("GxsTokenQueueMtx") { return; }
bool	queueRequest(uint32_t token, uint32_t req_type);

void	checkRequests(); // must be called by

	// This must be overloaded to complete the functionality.
virtual void handleResponse(uint32_t token, uint32_t req_type);

	private:
	RsGenExchange *mGenExchange;
	RsMutex mQueueMtx;
	std::list<GxsTokenQueueItem> mQueue;
};


#endif //R_GXS_TOKEN_QUEUE_H
