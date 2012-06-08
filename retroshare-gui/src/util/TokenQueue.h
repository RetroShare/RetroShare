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

#ifndef MRK_TOKEN_QUEUE_H
#define MRK_TOKEN_QUEUE_H

#include <QWidget>
#include <QTimer>
#include <list>
#include <string>
#include <sys/time.h>

#include <retroshare/rsidentity.h>


#define COMPLETED_REQUEST	4


#define TOKENREQ_GROUPLIST	1
#define TOKENREQ_GROUPDATA	2
#define TOKENREQ_MSGLIST	3
#define TOKENREQ_MSGDATA	4


class TokenQueue;

class TokenRequest
{
	public:
	uint32_t mToken;
	uint32_t mType;
	uint32_t mUserType;
	struct timeval mRequestTs;
	struct timeval mPollTs;
};

class TokenResponse
{
	public:
	//virtual ~TokenResponse() { return; }
	// These Functions are overloaded to get results out.
	virtual void loadRequest(const TokenQueue *queue, const TokenRequest &req) = 0;
};


class TokenQueue: public QWidget
{
  Q_OBJECT

public:
	TokenQueue(RsTokenService *service, TokenResponse *resp);

	/* generic handling of token / response update behaviour */
	bool genericRequest(uint32_t basictype, std::list<std::string> ids, uint32_t usertype);
	void queueRequest(uint32_t token, uint32_t basictype, uint32_t usertype);
	bool checkForRequest(uint32_t token);
	void loadRequest(const TokenRequest &req);

protected:
	void doPoll(float dt);

private slots:
	void pollRequests();

private:
	/* Info for Data Requests */
	std::list<TokenRequest> mRequests;

	RsTokenService *mService;
	TokenResponse *mResponder;

	QTimer *mTrigger;
};

#endif

