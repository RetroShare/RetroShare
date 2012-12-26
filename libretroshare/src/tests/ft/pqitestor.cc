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
#include "pqi/p3linkmgr.h"

/******
 *#define HUB_DEBUG 1
 *****/

P3Hub::P3Hub(uint32_t flags, RsSerialiser *rss)
	:mSerialiser(rss), mUseSerialiser(false)
{
	if (rss)
	{
		mUseSerialiser = true;
	}
	return;
}

void	P3Hub::addP3Pipe(std::string id, P3Pipe *pqi, p3LinkMgr *mgr)
{
	hubItem item(id, pqi, mgr);

	std::map<std::string, hubItem>::iterator it;
	for(it = mPeers.begin(); it != mPeers.end(); it++)
	{
		sockaddr_in remote_addr ;
		(it->second).mLinkMgr->connectResult(id, true, 0,remote_addr);
		mgr->connectResult(it->first, true, 0,remote_addr);
	}

	mPeers[id] = item;

	/* tell all the other peers we are connected */

#ifdef HUB_DEBUG
	std::cerr << "P3Hub::addPQIPipe()";
	std::cerr << std::endl;
#endif

}

RsItem	*P3Hub::SerialiserPass(RsItem *inItem)
{
	/* pass through serialiser */

	RsItem *item = NULL;

        uint32_t pktsize = mSerialiser->size(inItem);
        void *ptr = malloc(pktsize);

#ifdef HUB_DEBUG
	std::cerr << "P3Hub::SerialiserPass() Expected Size: " << pktsize;
	std::cerr << std::endl;
#endif

        if (!mSerialiser->serialise(inItem, ptr, &pktsize))
	{

#ifdef HUB_DEBUG
		std::cerr << "P3Hub::SerialiserPass() serialise Failed";
		std::cerr << std::endl;
#endif

	}
	else
	{

#ifdef HUB_DEBUG
		std::cerr << "P3Hub::SerialiserPass() serialise success, size: " << pktsize;
		std::cerr << std::endl;
#endif

        	item = mSerialiser->deserialise(ptr, &pktsize);
		item->PeerId(inItem->PeerId());

		if (!item)
		{
#ifdef HUB_DEBUG
			std::cerr << "P3Hub::SerialiserPass() deSerialise Failed";
			std::cerr << std::endl;
#endif
		}
	}

	delete inItem;
	free(ptr);
	return item;
}


void 	P3Hub::run()
{
#ifdef HUB_DEBUG
	std::cerr << "P3Hub::run()";
	std::cerr << std::endl;
#endif

	RsItem *item;
	std::list<std::pair<std::string, RsItem *> > recvdQ;
	std::list<std::pair<std::string, RsItem *> >::iterator lit;
	while(1)
	{
#ifdef HUB_DEBUG
		std::cerr << "P3Hub::run()";
		std::cerr << std::endl;
#endif

		std::map<std::string, hubItem>::iterator it;
		for(it = mPeers.begin(); it != mPeers.end(); it++)
		{
			while (NULL != (item = it->second.mPQI->PopSentItem()))
			{
#ifdef HUB_DEBUG
				std::cerr << "P3Hub::run() recvd msg from: ";
				std::cerr << it->first;
				std::cerr << " for " << item->PeerId();
				std::cerr << std::endl;
				item->print(std::cerr, 10);
				std::cerr << std::endl;
#endif

				if (mUseSerialiser)
				{
					item = SerialiserPass(item);
				}

				/* serialiser might hav munched it. */
				if (item)
				{
					recvdQ.push_back(make_pair(it->first, item));
				}
			}
		}

		/* now send out */
		for(lit = recvdQ.begin(); lit != recvdQ.end(); lit++)
		{
			std::string srcId = lit->first;
			std::string destId = (lit->second)->PeerId();
			if (mPeers.end() == (it = mPeers.find(destId)))
			{
#ifdef HUB_DEBUG
				std::cerr << "Failed to Find destination: " << destId;
				std::cerr << std::endl;
				std::cerr << "Deleting Packet";
				std::cerr << std::endl;
#endif

				delete (lit->second);

			}
			else
			{
				/* now we have dest, set source Id */
				(lit->second)->PeerId(srcId);
#ifdef HUB_DEBUG
				std::cerr << "P3Hub::run() sending msg from: ";
				std::cerr << srcId << "to: ";
				std::cerr << destId;
				std::cerr << std::endl;
				(lit->second)->print(std::cerr, 10);
				std::cerr << std::endl;
#endif

				(it->second).mPQI->PushRecvdItem(lit->second);
			}
		}

		recvdQ.clear();



		/* Tick the Connection Managers (normally done by rsserver)
		 */

		/* sleep a bit */
#ifdef WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
	}
}

		
		
	


PQIPipe::PQIPipe(std::string peerId)
	:PQInterface(peerId),pipeMtx("Pipe mutex")
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




/***** P3Pipe here *****/




int P3Pipe::SendAllItem(RsItem *item)
{
	RsStackMutex stack(pipeMtx); /***** LOCK MUTEX ****/

	mSentItems.push_back(item);

	return 1;
}

RsItem *P3Pipe::PopSentItem()
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

int P3Pipe::PushRecvdItem(RsItem *item)
{
	RsStackMutex stack(pipeMtx); /***** LOCK MUTEX ****/

	RsCacheRequest *rcr;
	RsCacheItem *rci;
	RsFileRequest *rfr;
	RsFileData *rfd;
	RsRawItem *rri;

	if (NULL != (rcr = dynamic_cast<RsCacheRequest *>(item)))
	{
		mRecvdRsCacheRequests.push_back(rcr);
	}
	else if (NULL != (rci = dynamic_cast<RsCacheItem *>(item)))
	{
		mRecvdRsCacheItems.push_back(rci);
	}
	else if (NULL != (rfr = dynamic_cast<RsFileRequest *>(item)))
	{
		mRecvdRsFileRequests.push_back(rfr);
	}
	else if (NULL != (rfd = dynamic_cast<RsFileData *>(item)))
	{
		mRecvdRsFileDatas.push_back(rfd);
	}
	else if (NULL != (rri = dynamic_cast<RsRawItem *>(item)))
	{
		mRecvdRsRawItems.push_back(rri);
	}

	return 1;
}

