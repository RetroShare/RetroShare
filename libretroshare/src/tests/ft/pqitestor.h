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
#include "serialiser/rsbaseitems.h"

#include <string>

class hubItem;
class PQIPipe;
class PQIHub;
class P3Pipe;
class P3Hub;
class p3LinkMgr;

class hubItem
{
	public:
	hubItem()
	:mPQI(NULL), mLinkMgr(NULL) { return; }

	hubItem(std::string id, P3Pipe *pqi, p3LinkMgr *mgr)
	:mPeerId(id), mPQI(pqi), mLinkMgr(mgr) { return; }

	std::string mPeerId;
	P3Pipe *mPQI;
	p3LinkMgr *mLinkMgr;
};


class P3Hub: public RsThread
{
	public:

	P3Hub(uint32_t flags, RsSerialiser *rss);
void 	addP3Pipe(std::string id, P3Pipe *, p3LinkMgr *mgr);

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

		virtual RsCacheRequest *RequestedSearch()										{ return GetSpecificItem(mRecvdRsCacheRequests) ; }
		virtual RsCacheItem *GetSearchResult() 										{ return GetSpecificItem(mRecvdRsCacheItems) ; }
		virtual RsFileRequest *GetFileRequest() 										{ return GetSpecificItem(mRecvdRsFileRequests) ; }
		virtual RsFileData *GetFileData() 												{ return GetSpecificItem(mRecvdRsFileDatas) ; }
		virtual RsRawItem *GetRsRawItem() 												{ return GetSpecificItem(mRecvdRsRawItems) ; }
		virtual RsFileChunkMapRequest* GetFileChunkMapRequest() 					{ return GetSpecificItem(mRecvdRsChunkMapRequests) ; }
		virtual RsFileChunkMap* GetFileChunkMap()  									{ return GetSpecificItem(mRecvdRsChunkMaps) ; }
		virtual RsFileCRC32MapRequest* GetFileCRC32MapRequest()  				{ return GetSpecificItem(mRecvdRsCRC32MapRequests) ; }
		virtual RsFileCRC32Map* GetFileCRC32Map()  									{ return GetSpecificItem(mRecvdRsCRC32Maps) ; }
		virtual RsFileSingleChunkCrcRequest* GetFileSingleChunkCrcRequest() 	{ return GetSpecificItem(mRecvdRsSingleChunkCRCRequests) ; }
		virtual RsFileSingleChunkCrc* GetFileSingleChunkCrc()						{ return GetSpecificItem(mRecvdRsSingleChunkCRCs) ; }

		virtual int	SearchSpecific(RsCacheRequest *item)								{ SendAllItem(item); return 1 ; }
		virtual int SendSearchResult(RsCacheItem *item)									{ SendAllItem(item); return 1 ; }
		virtual int SendFileRequest(RsFileRequest *item)								{ SendAllItem(item); return 1 ; }
		virtual int SendFileData(RsFileData *item)										{ SendAllItem(item); return 1 ; }
		virtual int	SendRsRawItem(RsRawItem *item)										{ SendAllItem(item); return 1 ; }
		virtual int SendFileChunkMapRequest(RsFileChunkMapRequest*item) 				{ SendAllItem(item); return 1 ; }
		virtual int SendFileChunkMap(RsFileChunkMap*item) 									{ SendAllItem(item); return 1 ; }
		virtual int SendFileCRC32MapRequest(RsFileCRC32MapRequest*item) 				{ SendAllItem(item); return 1 ; }
		virtual int SendFileCRC32Map(RsFileCRC32Map*item) 									{ SendAllItem(item); return 1 ; }
		virtual int SendFileSingleChunkCrcRequest(RsFileSingleChunkCrcRequest*item)	{ SendAllItem(item); return 1 ; }
		virtual int SendFileSingleChunkCrc(RsFileSingleChunkCrc*item) 					{ SendAllItem(item); return 1 ; }

		/* Lower Interface for PQIHub */

		RsItem *PopSentItem();
		int 	PushRecvdItem(RsItem *item);

	private:
		template<class T> T *GetSpecificItem(std::list<T*>& item_list)
		{
			RsStackMutex stack(pipeMtx); /***** LOCK MUTEX ****/

			if (item_list.size() == 0)
				return NULL;

			T *item = item_list.front();
			item_list.pop_front();

			return item;
		}

		int 	SendAllItem(RsItem *item);

		RsMutex pipeMtx;

		std::list<RsItem *> mSentItems; 

		std::list<RsCacheRequest *> 				mRecvdRsCacheRequests;
		std::list<RsCacheItem *> 					mRecvdRsCacheItems;
		std::list<RsFileRequest *> 				mRecvdRsFileRequests;
		std::list<RsFileData *> 					mRecvdRsFileDatas;
		std::list<RsRawItem *> 						mRecvdRsRawItems;
		std::list<RsFileChunkMapRequest *> 		mRecvdRsChunkMapRequests;
		std::list<RsFileChunkMap *> 				mRecvdRsChunkMaps;
		std::list<RsFileCRC32MapRequest *> 			mRecvdRsCRC32MapRequests;
		std::list<RsFileCRC32Map *> 					mRecvdRsCRC32Maps;
		std::list<RsFileSingleChunkCrcRequest *>	mRecvdRsSingleChunkCRCRequests;
		std::list<RsFileSingleChunkCrc *> 			mRecvdRsSingleChunkCRCs;
};


#endif
