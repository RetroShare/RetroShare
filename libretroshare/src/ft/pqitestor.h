/*
 * libretroshare/src/ft: pqitestor.h
 *
 * File Transfer for RetroShare.
 *
 * Copyright 2008 by Robert Fernie.
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

#ifndef PQI_HUB_TEST_H
#define PQI_HUB_TEST_H

/*
 * pqi Test Interface.
 */

/*** 
 * Structures for ftserver simulations
 *
 ****/

#include "pqi/pqi.h"
#include "util/rsthreads.h"

#include <string>

class hubItem;
class PQIPipe;
class PQIHub;
class p3ConnectMgr;


class hubItem
{
	public:
	hubItem()
	:mPQI(NULL), mConnMgr(NULL) { return; }

	hubItem(std::string id, PQIPipe *pqi, p3ConnectMgr *mgr)
	:mPeerId(id), mPQI(pqi), mConnMgr(mgr) { return; }

	std::string mPeerId;
	PQIPipe *mPQI;
	p3ConnectMgr *mConnMgr;
};


class PQIHub: public RsThread
{
	public:

	PQIHub();
void 	addPQIPipe(std::string id, PQIPipe *, p3ConnectMgr *mgr);

virtual	void run();

	private:
	std::map<std::string, hubItem> mPeers;
};


class PQIPipe: public PQInterface
{
public:
	PQIPipe(std::string peerId);

virtual int     SendItem(RsItem *);
virtual RsItem *GetItem();

	// PQIHub Interface.
RsItem *PopSentItem();
int	PushRecvdItem(RsItem *);

	/*
	 */

private:

	RsMutex pipeMtx;

	std::list<RsItem *> mSentItems; 
	std::list<RsItem *> mRecvdItems; 

};


#endif
