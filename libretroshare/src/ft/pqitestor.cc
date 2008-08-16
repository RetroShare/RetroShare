/*
 * libretroshare/src/ft: pqitestor.cc
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

#include "ft/pqitestor.h"
#include "pqi/p3connmgr.h"


PQIHub::PQIHub()
{
	return;
}

void	PQIHub::addPQIPipe(std::string id, PQIPipe *pqi, p3ConnectMgr *mgr)
{
	hubItem item(id, pqi, mgr);

	std::map<std::string, hubItem>::iterator it;
	for(it = mPeers.begin(); it != mPeers.end(); it++)
	{
		(it->second).mConnMgr->connectResult(id, true, 0);
		mgr->connectResult(it->first, true, 0);
	}

	mPeers[id] = item;

	/* tell all the other peers we are connected */
	std::cerr << "PQIHub::addPQIPipe()";
	std::cerr << std::endl;

}


void 	PQIHub::run()
{
	RsItem *item;
	std::list<RsItem *> recvdQ;
	std::list<RsItem *>::iterator lit;
	while(1)
	{
		std::cerr << "PQIHub::run()";
		std::cerr << std::endl;

		std::map<std::string, hubItem>::iterator it;
		for(it = mPeers.begin(); it != mPeers.end(); it++)
		{
			while (NULL != (item = it->second.mPQI->PopSentItem()))
			{
				std::cerr << "PQIHub::run() recvd msg from: ";
				std::cerr << it->first;
				std::cerr << std::endl;
				recvdQ.push_back(item);
			}
		}

		/* now send out */
		for(lit = recvdQ.begin(); lit != recvdQ.end(); lit++)
		{
			std::string pId = (*lit)->PeerId();
			if (mPeers.end() == (it = mPeers.find(pId)))
			{
				std::cerr << "Failed to Find destination: " << pId;
				std::cerr << std::endl;
			}
			std::cerr << "PQIHub::run() sending msg to: ";
			std::cerr << it->first;
			std::cerr << std::endl;

			(it->second).mPQI->PushRecvdItem(*lit);
		}


		/* Tick the Connection Managers (normally done by rsserver)
		 */

		/* sleep a bit */
		sleep(1);
	}
}

		
		
	


PQIPipe::PQIPipe(std::string peerId)
	:PQInterface(peerId)
{
	return;
}

int PQIPipe::SendItem(RsItem *item)
{
	RsStackMutex stack(pipeMtx); /***** LOCK MUTEX ****/

	mSentItems.push_back(item);

	return 1;
}

RsItem *PQIPipe::PopSentItem()
{
	RsStackMutex stack(pipeMtx); /***** LOCK MUTEX ****/

	if (mSentItems.size() == 0)
	{
		return NULL;
	}

	RsItem *item = mSentItems.front();
	mSentItems.pop_front();
	
	return item;
}

int PQIPipe::PushRecvdItem(RsItem *item)
{
	RsStackMutex stack(pipeMtx); /***** LOCK MUTEX ****/

	mRecvdItems.push_back(item);

	return 1;
}

RsItem *PQIPipe::GetItem()
{
	RsStackMutex stack(pipeMtx); /***** LOCK MUTEX ****/

	if (mRecvdItems.size() == 0)
	{
		return NULL;
	}

	RsItem *item = mRecvdItems.front();
	mRecvdItems.pop_front();
	
	return item;
}


