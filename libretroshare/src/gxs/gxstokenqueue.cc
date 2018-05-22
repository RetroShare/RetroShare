/*******************************************************************************
 * libretroshare/src/gxs: gxstokenqueue.cc                                     *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2012 by Robert Fernie        <retroshare@lunamutt.com>       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#include "gxs/gxstokenqueue.h"

/*******
 * #define GXS_DEBUG 	1
 ******/

bool GxsTokenQueue::queueRequest(uint32_t token, uint32_t req_type)
{
	RS_STACK_MUTEX(mQueueMtx);
	mQueue.push_back(GxsTokenQueueItem(token, req_type));
	return true;
}


void GxsTokenQueue::checkRequests()
{
	{
		RsStackMutex stack(mQueueMtx); /********** STACK LOCKED MTX ******/
		if (mQueue.empty())
		{
			return;
		}
	}

	// Must check all, and move to a different list - for reentrant / good mutex behaviour.
	std::list<GxsTokenQueueItem> toload;
	std::list<GxsTokenQueueItem>::iterator it;

	bool stuffToLoad = false;
	{
		RsStackMutex stack(mQueueMtx); /********** STACK LOCKED MTX ******/
		for(it = mQueue.begin(); it != mQueue.end();)
		{
			uint32_t token = it->mToken;
			uint32_t status = mGenExchange->getTokenService()->requestStatus(token);
	
			if (status == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
			{
				toload.push_back(*it);
				it = mQueue.erase(it);
				stuffToLoad = true;

#ifdef GXS_DEBUG
				std::cerr << "GxsTokenQueue::checkRequests() token: " << token << " Complete";
				std::cerr << std::endl;
#endif
				++it;
			}
			else if (status == RsTokenService::GXS_REQUEST_V2_STATUS_FAILED)
			{
				// maybe we should do alternative callback?
				std::cerr << "GxsTokenQueue::checkRequests() ERROR Request Failed: " << token;
				std::cerr << std::endl;

				it = mQueue.erase(it);
			}
			else
			{
#ifdef GXS_DEBUG
				std::cerr << "GxsTokenQueue::checkRequests() token: " << token << " is unfinished, status: " << status;
				std::cerr << std::endl;
#endif
				++it;
			}
		}
	}

	if (stuffToLoad)
	{
		for(it = toload.begin(); it != toload.end(); ++it)
		{
			handleResponse(it->mToken, it->mReqType);
		}
	}
}

	// This must be overloaded to complete the functionality.
void GxsTokenQueue::handleResponse(uint32_t token, uint32_t req_type)
{
	std::cerr << "GxsTokenQueue::handleResponse(" << token << "," << req_type << ") ERROR: NOT HANDLED";
	std::cerr << std::endl;
}


