/*
 * libretroshare/src/services/p3dsdv.h
 *
 * Network-Wide Routing Service.
 *
 * Copyright 2011 by Robert Fernie.
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

#include <list>
#include <string>

//#include "serialiser/rsdsdvitems.h"
#include "services/p3dsdv.h"
#include "pqi/p3linkmgr.h"


/****
 * #define DEBUG_DSDV		1
 ****/

/* DEFINE INTERFACE POINTER! */
RsDsdv *rsDsdv = NULL;

/*****
 * Routing Services are provided for any service or peer that wants to access / receive info 
 * over the internal network.
 *
 * This is going to be based loosely on Ben's Algorithm...
 * Each Service / Peer is identified by a HASH.
 * This HASH is disguised by a temporary ANON-CHUNK.
 * DSDVID = Sha1(ANONCHUNK + HASH).
 * 
 * Each peer can advertise as many Services as they want.
 * The Anon-chunk should be rotated regularly to hide it well.
 * period to be defined.
 * 
 *
 * Once this Routing table has been established, Routes can be created - in a similar manner to turtle paths.
 * (Send Path request (Path Id + Origin + Destination))... path is established.
 *
 * Then we can do Onion Routing etc, on top of this.
 *
 ****/

p3Dsdv::p3Dsdv(p3LinkMgr *lm)
	:p3Service(RS_SERVICE_TYPE_DSDV), /* p3Config(CONFIG_TYPE_DSDV), */ mDsdvMtx("p3Dsdv"), mLinkMgr(lm) 
{
	addSerialType(new RsDsdvSerialiser());

	mSentTablesTime = 0;
	mSentIncrementTime = 0;
}

int	p3Dsdv::tick()
{
	processIncoming();
	sendTables();

	return 0;
}

int	p3Dsdv::status()
{
	return 1;
}

#define DSDV_BROADCAST_PERIOD		60
#define DSDV_MIN_INCREMENT_PERIOD	5

int	p3Dsdv::sendTables()
{
	time_t now = time(NULL);
	time_t tt, it;
	bool   updateRequired = false;
	{
		RsStackMutex stack(mDsdvMtx); /****** LOCKED MUTEX *******/
		tt = mSentTablesTime;
		it = mSentIncrementTime;
		updateRequired = mSignificantChanges;
	}

	if (now - tt > DSDV_BROADCAST_PERIOD)
	{
		generateRoutingTables(false);

		RsStackMutex stack(mDsdvMtx); /****** LOCKED MUTEX *******/
		mSentTablesTime = now;
		mSentIncrementTime = now;

		return true ;
	}

	/* otherwise send incremental changes */
	if ((updateRequired) && (now - it > DSDV_MIN_INCREMENT_PERIOD))
	{
		generateRoutingTables(true);

		RsStackMutex stack(mDsdvMtx); /****** LOCKED MUTEX *******/
		mSentIncrementTime = now;
	}

	return true;
}

#define RSDSDV_SEQ_INCREMENT	2

void 	p3Dsdv::advanceLocalSequenceNumbers()
{
	RsStackMutex stack(mDsdvMtx); /****** LOCKED MUTEX *******/

	std::map<std::string, RsDsdvTableEntry>::iterator it;
	for(it = mTable.begin(); it != mTable.end(); it++)
	{
		RsDsdvTableEntry &v = (it->second);
		if (v.mOwnSource)
		{
			v.mStableRoute.mSequence += RSDSDV_SEQ_INCREMENT;
			v.mBestRoute.mSequence = v.mStableRoute.mSequence;
		}
	}
}

void 	p3Dsdv::clearSignificantChangesFlags()
{
	RsStackMutex stack(mDsdvMtx); /****** LOCKED MUTEX *******/
	
	std::map<std::string, RsDsdvTableEntry>::iterator it;
	for(it = mTable.begin(); it != mTable.end(); it++)
	{
		RsDsdvTableEntry &v = (it->second);
		if (v.mFlags & RSDSDV_FLAGS_SIGNIFICANT_CHANGE)
		{
			v.mFlags &= ~RSDSDV_FLAGS_SIGNIFICANT_CHANGE;
		}
	}
	mSignificantChanges = false;
}


int 	p3Dsdv::generateRoutingTables(bool incremental)
{
	/* we ping our peers */
	/* who is online? */
	std::list<std::string> idList;
	mLinkMgr->getOnlineList(idList);

#ifdef DEBUG_DSDV
	std::cerr << "p3Dsdv::generateRoutingTables(" << incremental << ")";
	std::cerr << std::endl;
#endif

	if (!incremental)
	{
		/* now clear significant flag */
		advanceLocalSequenceNumbers();
	}

	/* prepare packets */
	std::list<std::string>::iterator it;
	for(it = idList.begin(); it != idList.end(); it++)
	{
#ifdef DEBUG_DSDV
		std::cerr << "p3Dsdv::generateRoutingTables() For: " << *it;
		std::cerr << std::endl;
#endif

		generateRoutingTable(*it, incremental);
	}


	/* now clear significant flag */
	clearSignificantChangesFlags();
	return 1;
}


int p3Dsdv::generateRoutingTable(const std::string &peerId, bool incremental)
{
	RsDsdvRouteItem *dsdv = new RsDsdvRouteItem();
	dsdv->PeerId(peerId);

	RsStackMutex stack(mDsdvMtx); /****** LOCKED MUTEX *******/

	std::map<std::string, RsDsdvTableEntry>::iterator it;
	for(it = mTable.begin(); it != mTable.end(); it++)
	{
		RsDsdvTableEntry &v = (it->second);

		/* discard/ignore criterion */
		if (v.mStableRoute.mDistance > RSDSDV_MAX_DISTANCE) 
		{
			continue;
		}

		if (incremental)
		{
			if (v.mFlags & RSDSDV_FLAGS_SIGNIFICANT_CHANGE)
			{
				// Done elsewhere.
				//v.mFlags &= ~SIGNIFICANT_CHANGE;
			}
			else
			{
				/* ignore non-significant changes */
				continue;
			}
		}

		RsTlvDsdvEntry entry;

		entry.endPoint.idType = v.mDest.mIdType;
		entry.endPoint.anonChunk = v.mDest.mAnonChunk;
		entry.endPoint.serviceId = v.mDest.mHash;
		entry.sequence = v.mStableRoute.mSequence;
		entry.distance = v.mStableRoute.mDistance;

		dsdv->routes.entries.push_back(entry);

		if (dsdv->routes.entries.size() > RSDSDV_MAX_ROUTE_TABLE)
		{
			sendItem(dsdv);
			dsdv = new RsDsdvRouteItem();
			dsdv->PeerId(peerId);
		}
	}
	sendItem(dsdv);
	return 1;
}


/****************************************************************************
 ****************************************************************************/

int	p3Dsdv::processIncoming()
{
	/* for each packet - pass to specific handler */
	RsItem *item = NULL;
	while(NULL != (item = recvItem()))
	{
		switch(item->PacketSubType())
		{
			default:
				break;
			case RS_PKT_SUBTYPE_DSDV_ROUTE:
			{
				handleDSDV((RsDsdvRouteItem *) item);
			}
				break;
		}

		/* clean up */
		delete item;
	}
	return true ;
} 


int p3Dsdv::handleDSDV(RsDsdvRouteItem *dsdv)
{
	/* iterate over the entries */
	RsStackMutex stack(mDsdvMtx); /****** LOCKED MUTEX *******/

	time_t now = time(NULL);
	
	std::list<RsTlvDsdvEntry>::iterator it;
	for(it = dsdv->routes.entries.begin(); it != dsdv->routes.entries.end(); it++)
	{
		/* check for existing */
		RsTlvDsdvEntry &entry = *it;
		uint32_t realDistance = entry.distance + 1; // metric.

		/* find the entry */
		std::map<std::string, RsDsdvTableEntry>::iterator tit;
		tit = mTable.find(entry.endPoint.serviceId);
		if (tit == mTable.end())
		{
			/* new entry! */
			RsDsdvTableEntry v;
			v.mDest.mIdType = entry.endPoint.idType;
			v.mDest.mAnonChunk = entry.endPoint.anonChunk;
			v.mDest.mHash = entry.endPoint.serviceId;

			v.mBestRoute.mNextHop = dsdv->PeerId();
			v.mBestRoute.mReceived = now;
			v.mBestRoute.mSequence = entry.sequence;
			v.mBestRoute.mDistance = realDistance;

			v.mStableRoute = v.mBestRoute; // same as new.

			v.mFlags = RSDSDV_FLAGS_NEW_ROUTE;
			v.mOwnSource = false;
			v.mMatched = false;

			// store in table.
			mTable[v.mDest.mHash] = v;
		}
		else
		{
			RsDsdvTableEntry &v = tit->second;

			/* update best entry */
			if ((entry.sequence >= v.mBestRoute.mSequence) || (realDistance < v.mBestRoute.mDistance))
			{
				v.mBestRoute.mNextHop = dsdv->PeerId();
				v.mBestRoute.mReceived = now;
				v.mBestRoute.mSequence = entry.sequence;
				v.mBestRoute.mDistance = realDistance;

				/* if consistent route... maintain */
				if (v.mBestRoute.mNextHop == v.mStableRoute.mNextHop)
				{
					v.mStableRoute = v.mBestRoute;
					v.mFlags |= RSDSDV_FLAGS_STABLE_ROUTE;
				}
				else
				{
					/* otherwise we need to wait - see if we get new update */
				}
			}
		}
	}
	return 1;
}


/*************** pqiMonitor callback ***********************/
void p3Dsdv::statusChange(const std::list<pqipeer> &plist)
{
	std::list<pqipeer>::const_iterator it;
	for(it = plist.begin(); it != plist.end(); it++)
	{
		/* only care about disconnected / not friends cases */
		if (	1	)
		{
			RsStackMutex stack(mDsdvMtx); /****** LOCKED MUTEX *******/
	




		}
	}
}


int p3Dsdv::addDsdvId(RsDsdvId *id, std::string realHash)
{
	RsStackMutex stack(mDsdvMtx); /****** LOCKED MUTEX *******/

	time_t now = time(NULL);
	
	/* check for duplicate */
	std::map<std::string, RsDsdvTableEntry>::iterator it;
	it = mTable.find(id->mHash);
	if (it != mTable.end())
	{
		/* error */
		std::cerr << "p3Dsdv::addDsdvId() ERROR Duplicate ID";
		std::cerr << std::endl;

		return 0;
	}

	/* new entry! */
	RsDsdvTableEntry v;
	v.mDest = *id;

	v.mBestRoute.mNextHop = mLinkMgr->getOwnId();
	v.mBestRoute.mReceived = now;
	v.mBestRoute.mSequence = 0;
	v.mBestRoute.mDistance = 0;

	v.mStableRoute = v.mBestRoute; // same as new.

	v.mFlags = RSDSDV_FLAGS_OWN_SERVICE;
	v.mOwnSource = true;
	v.mMatched = true;
	v.mMatchedHash = realHash;

	// store in table.
	mTable[v.mDest.mHash] = v;

	return 1;
}




int p3Dsdv::dropDsdvId(RsDsdvId *id)
{
	RsStackMutex stack(mDsdvMtx); /****** LOCKED MUTEX *******/

	/* This should send out an infinity packet... and flag for deletion */

	std::map<std::string, RsDsdvTableEntry>::iterator it;
	it = mTable.find(id->mHash);
	if (it == mTable.end())
	{
		/* error */
		std::cerr << "p3Dsdv::addDsdvId() ERROR Unknown ID";
		std::cerr << std::endl;

		return 0;
	}

	mTable.erase(it);

	return 1;
}


int p3Dsdv::printDsdvTable(std::ostream &out)
{
	RsStackMutex stack(mDsdvMtx); /****** LOCKED MUTEX *******/

	/* iterate over the entries */
	std::map<std::string, RsDsdvTableEntry>::iterator it;
	for(it = mTable.begin(); it != mTable.end(); it++)
	{
		RsDsdvTableEntry &v = it->second;
		out << v.mDest;
		out << " BR: " << v.mBestRoute;
		out << " SR: " << v.mBestRoute;
		out << " Flags: " << v.mFlags;
		out << " Own: " << v.mOwnSource;
		if (v.mMatched)
		{
			out << " RH: " << v.mMatchedHash;
		}
		else
		{
			out << " Unknown";
		}
		out << std::endl;
	}
	return 1;
}

/*****************************************/

uint32_t p3Dsdv::getLocalServices(std::list<std::string> &hashes)
{
	RsStackMutex stack(mDsdvMtx); /****** LOCKED MUTEX *******/

	/* iterate over the entries */
	std::map<std::string, RsDsdvTableEntry>::iterator it;
	for(it = mTable.begin(); it != mTable.end(); it++)
	{
		if (it->second.mOwnSource)
		{
			hashes.push_back(it->first);
		}
	}
	return 1;
}


uint32_t p3Dsdv::getAllServices(std::list<std::string> &hashes)
{
	RsStackMutex stack(mDsdvMtx); /****** LOCKED MUTEX *******/

	/* iterate over the entries */
	std::map<std::string, RsDsdvTableEntry>::iterator it;
	for(it = mTable.begin(); it != mTable.end(); it++)
	{
		hashes.push_back(it->first);
	}
	return 1;
}


int p3Dsdv::getDsdvEntry(const std::string &hash, RsDsdvTableEntry &entry)
{
	RsStackMutex stack(mDsdvMtx); /****** LOCKED MUTEX *******/

	/* iterate over the entries */
	std::map<std::string, RsDsdvTableEntry>::iterator it;
	it = mTable.find(hash);
	if (it == mTable.end())
	{
		return 0;
	}

	entry = it->second;

	return 1;
}

std::ostream &operator<<(std::ostream &out, const RsDsdvId &id)
{
	out << "[Type: " << id.mIdType << " AMZ: " << id.mAnonChunk << " THASH: " << id.mHash;
	out << "]";

	return out;
}


std::ostream &operator<<(std::ostream &out, const RsDsdvRoute &route)
{
	out << "< Seq: " << route.mSequence << " Dist: " << route.mDistance;
	out << " NextHop: " << route.mNextHop << " recvd: " << route.mReceived;
	out << " >";

	return out;
}

std::ostream &operator<<(std::ostream &out, const RsDsdvTableEntry &entry)
{
	out << "DSDV Route for: " << entry.mDest << std::endl;
	out << "Stable: " << entry.mStableRoute << std::endl;
	out << "Best: " << entry.mBestRoute << std::endl;
	out << "OwnSource: " << entry.mOwnSource;
	out << " Flags: " << entry.mFlags << std::endl;
	if (entry.mMatched)
	{
		out << "Matched: " << entry.mMatchedHash;
	}
	else
	{
		out << "Non Matched";
	}
	out  << std::endl;
	return out;
}


