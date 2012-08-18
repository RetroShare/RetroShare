/*
 * Token Queue.
 *
 * Copyright 2012-2012 by Robert Fernie, Christopher Evi-Parker
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

#ifndef MRK_TOKEN_QUEUE_V2_H
#define MRK_TOKEN_QUEUE_V2_H

#include <QWidget>
#include <QTimer>
#include <list>
#include <string>
#include <sys/time.h>

#include <gxs/rstokenservice.h>



#define COMPLETED_REQUEST	4

#define TOKENREQ_GROUPINFO	1
#define TOKENREQ_MSGINFO	2
#define TOKENREQ_MSGRELATEDINFO	3


class TokenQueueV2;

class TokenRequestV2
{
	public:
	uint32_t mToken;
	uint32_t mType;
	uint32_t mAnsType;
	uint32_t mUserType;
	struct timeval mRequestTs;
	struct timeval mPollTs;
};

class TokenResponseV2
{
	public:
	//virtual ~TokenResponse() { return; }
	// These Functions are overloaded to get results out.
        virtual void loadRequest(const TokenQueueV2 *queue, const TokenRequestV2 &req) = 0;
};


class TokenQueueV2: public QWidget
{
  Q_OBJECT

public:
        TokenQueueV2(RsTokenServiceV2 *service, TokenResponseV2 *resp);

	/* generic handling of token / response update behaviour */
	bool requestGroupInfo(uint32_t &token, uint32_t anstype, const RsTokReqOptionsV2 &opts,
                                                        std::list<RsGxsGroupId>& ids, uint32_t usertype);
	bool requestMsgInfo(uint32_t &token, uint32_t anstype, const RsTokReqOptionsV2 &opts,
							const GxsMsgReq& ids, uint32_t usertype);

	bool cancelRequest(const uint32_t token);

	void queueRequest(uint32_t token, uint32_t basictype, uint32_t anstype, uint32_t usertype);
	bool checkForRequest(uint32_t token);
	void loadRequest(const TokenRequestV2 &req);

protected:
	void doPoll(float dt);

private slots:
	void pollRequests();

private:
	/* Info for Data Requests */
	std::list<TokenRequestV2> mRequests;

	RsTokenServiceV2 *mService;
        TokenResponseV2 *mResponder;

	QTimer *mTrigger;
};

#endif


