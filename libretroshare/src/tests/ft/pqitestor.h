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
class P3Pipe;
class P3Hub;
class p3ConnectMgr;


class hubItem
{
	public:
	hubItem()
	:mPQI(NULL), mConnMgr(NULL) { return; }

	hubItem(std::string id, P3Pipe *pqi, p3ConnectMgr *mgr)
	:mPeerId(id), mPQI(pqi), mConnMgr(mgr) { return; }

	std::string mPeerId;
	P3Pipe *mPQI;
	p3ConnectMgr *mConnMgr;
};


class P3Hub: public RsThread
{
	public:

	P3Hub(uint32_t flags, RsSerialiser *rss);
void 	addP3Pipe(std::string id, P3Pipe *, p3ConnectMgr *mgr);

virtual	void run();

	private:

RsItem* SerialiserPass(RsItem *inItem);

	std::map<std::string, hubItem> mPeers;
	RsSerialiser *mSerialiser;
	bool mUseSerialiser;
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

class P3Pipe: public P3Interface
{
	public:
		P3Pipe() : pipeMtx(std::string("Pipe mutex")) {return; }
		virtual ~P3Pipe() {return; }

		virtual int	tick() { return 1; }
		virtual int	status() { return 1; }

		/* Overloaded from P3Interface */
		virtual int	SearchSpecific(RsCacheRequest *item);
		virtual int     SendSearchResult(RsCacheItem *item);
		virtual int     SendFileRequest(RsFileRequest *item);
		virtual int     SendFileData(RsFileData *item);
		virtual int	SendRsRawItem(RsRawItem *item);

		virtual RsCacheRequest *RequestedSearch();
		virtual RsCacheItem *GetSearchResult();
		virtual RsFileRequest *GetFileRequest();
		virtual RsFileData *GetFileData();
		virtual RsRawItem *GetRsRawItem();

		virtual RsFileChunkMapRequest* GetFileChunkMapRequest() ;
		virtual int SendFileChunkMapRequest(RsFileChunkMapRequest*) ;
		virtual RsFileChunkMap* GetFileChunkMap() ;
		virtual int SendFileChunkMap(RsFileChunkMap*) ;
		virtual RsFileCRC32MapRequest* GetFileCRC32MapRequest() ;
		virtual int SendFileCRC32MapRequest(RsFileCRC32MapRequest*) ;
		virtual RsFileCRC32Map* GetFileCRC32Map() ;
		virtual int SendFileCRC32Map(RsFileCRC32Map*) ;

		/* Lower Interface for PQIHub */

		RsItem *PopSentItem();
		int 	PushRecvdItem(RsItem *item);

	private:

		int 	SendAllItem(RsItem *item);

		RsMutex pipeMtx;

		std::list<RsItem *> mSentItems; 

		std::list<RsCacheRequest *> mRecvdRsCacheRequests;
		std::list<RsCacheItem *> mRecvdRsCacheItems;
		std::list<RsFileRequest *> mRecvdRsFileRequests;
		std::list<RsFileData *> mRecvdRsFileDatas;
		std::list<RsRawItem *> mRecvdRsRawItems;
};


#endif
