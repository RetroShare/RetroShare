/*
 * Token Queue.
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

#include "util/TokenQueueVEG.h"
#include <iostream>

#include <QTimer>

/******
 * #define ID_DEBUG 1
 *****/

/** Constructor */
TokenQueueVEG::TokenQueueVEG(RsTokenServiceVEG *service, TokenResponseVEG *resp)
:QWidget(NULL), mService(service), mResponder(resp)
{
	return;
}

bool TokenQueueVEG::requestGroupInfo(uint32_t &token, uint32_t anstype, const RsTokReqOptionsVEG &opts, std::list<std::string> ids, uint32_t usertype)
{
	uint32_t basictype = TOKENREQ_GROUPINFO;
	mService->requestGroupInfo(token, anstype, opts, ids);
	queueRequest(token, basictype, anstype, usertype);

	return true;
}


bool TokenQueueVEG::requestMsgInfo(uint32_t &token, uint32_t anstype, const RsTokReqOptionsVEG &opts, std::list<std::string> ids, uint32_t usertype)
{
	uint32_t basictype = TOKENREQ_MSGINFO;
	mService->requestMsgInfo(token, anstype, opts, ids);
	queueRequest(token, basictype, anstype, usertype);

	return true;
}


bool TokenQueueVEG::requestMsgRelatedInfo(uint32_t &token, uint32_t anstype, const RsTokReqOptionsVEG &opts, std::list<std::string> ids, uint32_t usertype)
{
	uint32_t basictype = TOKENREQ_MSGRELATEDINFO;
	mService->requestMsgRelatedInfo(token, anstype, opts, ids);
	queueRequest(token, basictype, anstype, usertype);

	return true;
}


void TokenQueueVEG::queueRequest(uint32_t token, uint32_t basictype, uint32_t anstype, uint32_t usertype)
{
	std::cerr << "TokenQueueVEG::queueRequest() Token: " << token << " Type: " << basictype;
	std::cerr << " AnsType: " << anstype << " UserType: " << usertype;
	std::cerr << std::endl;

	TokenRequestVEG req;
	req.mToken = token;
	req.mType = basictype;
	req.mAnsType = anstype;
	req.mUserType = usertype;

	gettimeofday(&req.mRequestTs, NULL);
	req.mPollTs = req.mRequestTs;

	mRequests.push_back(req);

	if (mRequests.size() == 1)
	{
		/* start the timer */
		doPoll(0.1);
	}
}

void TokenQueueVEG::doPoll(float dt)
{
	/* single shot poll */
	//mTrigger->singlesshot(dt * 1000);
	QTimer::singleShot((int) (dt * 1000.0), this, SLOT(pollRequests()));
}


void TokenQueueVEG::pollRequests()
{
	std::list<TokenRequestVEG>::iterator it;

	double pollPeriod = 1.0; // max poll period.
	for(it = mRequests.begin(); it != mRequests.end();)
	{
		if (checkForRequest(it->mToken))
		{
			/* clean it up and handle */
			loadRequest(*it);		
			it = mRequests.erase(it);	
		}
		else
		{
			/* calculate desired poll period */

			/* if less then current poll period, adjust */	
			
			it++;
		}
	}

	if (mRequests.size() > 0)
	{
		doPoll(pollPeriod);
	}
}


bool TokenQueueVEG::checkForRequest(uint32_t token)
{
	/* check token */
	return (COMPLETED_REQUEST == mService->requestStatus(token));
}


void TokenQueueVEG::loadRequest(const TokenRequestVEG &req)
{
	std::cerr << "TokenQueueVEG::loadRequest(): ";
	std::cerr << "Token: " << req.mToken << " Type: " << req.mType;
	std::cerr << " AnsType: " << req.mAnsType << " UserType: " << req.mUserType;
	std::cerr << std::endl;

	mResponder->loadRequest(this, req);

	return;
}


bool TokenQueueVEG::cancelRequest(const uint32_t token)
{
	/* cancel at lower level first */
	mService->cancelRequest(token);

	std::list<TokenRequestVEG>::iterator it;

	for(it = mRequests.begin(); it != mRequests.end(); it++)
	{
		if (it->mToken == token)
		{
			mRequests.erase(it);

			std::cerr << "TokenQueueVEG::cancelRequest() Cleared Request: " << token;
			std::cerr << std::endl;

			return true;
		}
	}

	std::cerr << "TokenQueueVEG::cancelRequest() Failed to Find Request: " << token;
	std::cerr << std::endl;

	return false;
}






