/*******************************************************************************
 * libretroshare/src/unused: p3dsdv.cc                                         *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011 by Robert Fernie <retroshare@lunamutt.com>                   *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#include <list>
#include <string>
#include "util/rstime.h"

//#include "serialiser/rsdsdvitems.h"
#include "services/p3dsdv.h"
#include "pqi/p3linkmgr.h"
#include "util/rsrandom.h"

#include <openssl/sha.h>

/****
 * #define DEBUG_DSDV		1
 ****/
#define DEBUG_DSDV		1

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

p3Dsdv::p3Dsdv(p3ServiceControl *sc)
	:p3Service(), /* p3Config(CONFIG_TYPE_DSDV), */ mDsdvMtx("p3Dsdv"), mServiceCtrl(sc) 
{
	addSerialType(new RsDsdvSerialiser());

	mSentTablesTime = 0;
	mSentIncrementTime = 0;
}

const std::string DSDV_APP_NAME = "dsdv";
const uint16_t DSDV_APP_MAJOR_VERSION  =       1;
const uint16_t DSDV_APP_MINOR_VERSION  =       0;
const uint16_t DSDV_MIN_MAJOR_VERSION  =       1;
const uint16_t DSDV_MIN_MINOR_VERSION  =       0;

RsServiceInfo p3Dsdv::getServiceInfo()
{
        return RsServiceInfo(RS_SERVICE_TYPE_DSDV,
                DSDV_APP_NAME,
                DSDV_APP_MAJOR_VERSION,
                DSDV_APP_MINOR_VERSION,
                DSDV_MIN_MAJOR_VERSION,
                DSDV_MIN_MINOR_VERSION);
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

#define DSDV_BROADCAST_PERIOD		(60*10) // 10 Minutes.
#define DSDV_MIN_INCREMENT_PERIOD	5
#define DSDV_DISCARD_PERIOD			(DSDV_BROADCAST_PERIOD * 2)

int	p3Dsdv::sendTables()
{
	rstime_t now = time(NULL);
	rstime_t tt, it;
	bool   updateRequired = false;
	{
		RsStackMutex stack(mDsdvMtx); /****** LOCKED MUTEX *******/
		tt = mSentTablesTime;
		it = mSentIncrementTime;
		updateRequired = mSignificantChanges;
	}

	if (now - tt > DSDV_BROADCAST_PERIOD)
	{

#ifdef DEBUG_DSDV
		std::cerr << "p3Dsdv::sendTables() Broadcast Time";
		std::cerr << std::endl;
#endif
		selectStableRoutes();
		clearOldRoutes();

		generateRoutingTables(false);

		printDsdvTable(std::cerr);

		RsStackMutex stack(mDsdvMtx); /****** LOCKED MUTEX *******/
		mSentTablesTime = now;
		mSentIncrementTime = now;

		return true ;
	}

	/* otherwise send incremental changes */
	if ((updateRequired) && (now - it > DSDV_MIN_INCREMENT_PERIOD))
	{
		selectStableRoutes();

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

	rstime_t now = time(NULL);

	std::map<std::string, RsDsdvTableEntry>::iterator it;
	for(it = mTable.begin(); it != mTable.end(); ++it)
	{
		RsDsdvTableEntry &v = (it->second);
		if (v.mOwnSource)
		{
			v.mStableRoute.mSequence += RSDSDV_SEQ_INCREMENT;
			v.mStableRoute.mReceived = now;
		}
	}
}

void 	p3Dsdv::clearSignificantChangesFlags()
{
	RsStackMutex stack(mDsdvMtx); /****** LOCKED MUTEX *******/
	
	std::map<std::string, RsDsdvTableEntry>::iterator it;
	for(it = mTable.begin(); it != mTable.end(); ++it)
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
	std::set<RsPeerId> idList;
	mServiceCtrl->getPeersConnected(getServiceInfo().mServiceType, idList);

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
	std::set<RsPeerId>::iterator it;
	for(it = idList.begin(); it != idList.end(); ++it)
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


int p3Dsdv::generateRoutingTable(const RsPeerId &peerId, bool incremental)
{
	RsDsdvRouteItem *dsdv = new RsDsdvRouteItem();
	dsdv->PeerId(peerId);

	RsStackMutex stack(mDsdvMtx); /****** LOCKED MUTEX *******/

	std::map<std::string, RsDsdvTableEntry>::iterator it;
	for(it = mTable.begin(); it != mTable.end(); ++it)
	{
		RsDsdvTableEntry &v = (it->second);

		/* discard/ignore criterion */
		if (!v.mIsStable)
		{
			continue;
		}

		if (v.mStableRoute.mDistance >= RSDSDV_MAX_DISTANCE) 
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

		//dsdv->routes.entries.push_back(entry);
		dsdv->routes.mList.push_back(entry);

		//if (dsdv->routes.entries.size() > RSDSDV_MAX_ROUTE_TABLE)
		if (dsdv->routes.mList.size() > RSDSDV_MAX_ROUTE_TABLE)
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

	rstime_t now = time(NULL);
	
#ifdef	DEBUG_DSDV
	std::cerr << "p3Dsdv::handleDSDV() Received Pkt from: " << dsdv->PeerId();
	std::cerr << std::endl;
	//dsdv->print(std::cerr);
	//std::cerr << std::endl;
#endif

	std::list<RsTlvDsdvEntry>::iterator it;
	//for(it = dsdv->routes.entries.begin(); it != dsdv->routes.entries.end(); ++it)
	for(it = dsdv->routes.mList.begin(); it != dsdv->routes.mList.end(); ++it)
	{
		/* check for existing */
		RsTlvDsdvEntry &entry = *it;
		uint32_t realDistance = entry.distance + 1; // metric.

	
		/* find the entry */
		std::map<std::string, RsDsdvTableEntry>::iterator tit;
		tit = mTable.find(entry.endPoint.serviceId);
		if (tit == mTable.end())
		{
#ifdef	DEBUG_DSDV
			std::cerr << "p3Dsdv::handleDSDV() Adding Entry for New ServiceId: ";
			std::cerr << entry.endPoint.serviceId;
			std::cerr << std::endl;
#endif
			/* new entry! */
			RsDsdvTableEntry v;
			v.mDest.mIdType = entry.endPoint.idType;
			v.mDest.mAnonChunk = entry.endPoint.anonChunk;
			v.mDest.mHash = entry.endPoint.serviceId;

			/* add as a possible route */
			RsDsdvRoute newRoute;

			newRoute.mNextHop = dsdv->PeerId();
			newRoute.mReceived = now;
			newRoute.mSequence = entry.sequence;
			newRoute.mDistance = realDistance;
			newRoute.mValidSince = now;

			v.mAllRoutes[dsdv->PeerId()] = newRoute;
			v.mIsStable = false;

			v.mFlags = RSDSDV_FLAGS_NEW_ROUTE;
			v.mOwnSource = false;
			v.mMatched = false;

			// store in table.
			mTable[v.mDest.mHash] = v;
		}
		else
		{
			RsDsdvTableEntry &v = tit->second;
			if (v.mOwnSource)
			{
#ifdef	DEBUG_DSDV
				std::cerr << "p3Dsdv::handleDSDV() Ignoring OwnSource Entry:";
				std::cerr << entry.endPoint.serviceId;
				std::cerr << std::endl;
#endif
				continue; // Ignore if we are source.
			}
			
			/* look for this in mAllRoutes */
			std::map<RsPeerId, RsDsdvRoute>::iterator rit;
			rit = v.mAllRoutes.find(dsdv->PeerId());
			if (rit == v.mAllRoutes.end())
			{
				/* add a new entry in */
				RsDsdvRoute newRoute;

				newRoute.mNextHop = dsdv->PeerId();
				newRoute.mReceived = now;
				newRoute.mSequence = entry.sequence;
				newRoute.mDistance = realDistance;
				newRoute.mValidSince = now;

				v.mAllRoutes[dsdv->PeerId()] = newRoute;

				/* if we've just added it in - can't be stable one */
#ifdef	DEBUG_DSDV
				std::cerr << "p3Dsdv::handleDSDV() Adding NewRoute Entry:";
				std::cerr << entry.endPoint.serviceId;
				std::cerr << std::endl;
#endif
			}
			else
			{
				if (rit->second.mSequence >= entry.sequence)
				{
#ifdef	DEBUG_DSDV
				std::cerr << "p3Dsdv::handleDSDV() Ignoring OLDSEQ Entry:";
				std::cerr << entry.endPoint.serviceId;
				std::cerr << std::endl;
#endif
					/* ignore same/old sequence number??? */
					continue;
				}
#ifdef	DEBUG_DSDV
				std::cerr << "p3Dsdv::handleDSDV() Updating Entry:";
				std::cerr << entry.endPoint.serviceId;
				std::cerr << std::endl;
#endif

				/* update seq,dist,etc */
				if (rit->second.mSequence + 2 < entry.sequence)
				{
					/* skipped a sequence number - reset timer */
					rit->second.mValidSince = now;
				}

				//rit->second.mNextHop; // unchanged.
				rit->second.mReceived = now;
				rit->second.mSequence = entry.sequence;
				rit->second.mDistance = realDistance;

				/* if consistent route... maintain */
				if ((v.mIsStable) &&
					(rit->second.mNextHop == v.mStableRoute.mNextHop))
				{
					v.mStableRoute = rit->second;
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


int p3Dsdv::selectStableRoutes()
{
	/* iterate over the entries */
	RsStackMutex stack(mDsdvMtx); /****** LOCKED MUTEX *******/

	rstime_t now = time(NULL);
	
#ifdef	DEBUG_DSDV
	std::cerr << "p3Dsdv::selectStableRoutes()";
	std::cerr << std::endl;
#endif

	/* find the entry */
	std::map<std::string, RsDsdvTableEntry>::iterator tit;
	for(tit = mTable.begin(); tit != mTable.end(); ++tit)
	{
	
#ifdef	DEBUG_DSDV
		std::cerr << "p3Dsdv::selectStableRoutes() For Entry: ";
		std::cerr << tit->second;
		std::cerr << std::endl;
#endif
		if (tit->second.mOwnSource)
		{
#ifdef	DEBUG_DSDV
			std::cerr << "p3Dsdv::selectStableRoutes() OwnSource... Ignoring";
			std::cerr << std::endl;
#endif
			continue; // Ignore if we are source.
		}		
		
		std::map<RsPeerId, RsDsdvRoute>::iterator rit;
		uint32_t newest = 0;
		RsPeerId newestId;
		uint32_t closest = RSDSDV_MAX_DISTANCE + 1;
		RsPeerId closestId;
		rstime_t closestAge = 0;

		/* find newest sequence number */
		for(rit = tit->second.mAllRoutes.begin(); 
			rit != tit->second.mAllRoutes.end(); ++rit)
		{
			if ((now - rit->second.mReceived <= DSDV_DISCARD_PERIOD) &&
					(rit->second.mSequence >= newest))
			{
				newest = rit->second.mSequence;
				newestId = rit->first;

				/* also becomes default for closest (later) */
				closest = rit->second.mDistance;
				closestId = rit->first;
				closestAge = now - rit->second.mValidSince;
			}
		}

		if (closest >= RSDSDV_MAX_DISTANCE + 1)
		{
#ifdef	DEBUG_DSDV
			std::cerr << "\tNo Suitable Route";
			std::cerr << std::endl;
#endif
			tit->second.mIsStable = false;
			continue;
		}

		uint32_t currseq = newest - (newest % 2); // remove 'kill'=ODD Seq.
	
#ifdef	DEBUG_DSDV
		std::cerr << "\t Newest Seq: " << newest << " from: " << newestId;
		std::cerr << std::endl;
#endif

		/* find closest distance - with valid seq & max valid time */
		for(rit = tit->second.mAllRoutes.begin(); 
			rit != tit->second.mAllRoutes.end(); ++rit)
		{
			/* Maximum difference in Sequence number is 2*DISTANCE
			 * Otherwise it must be old.
			 */
			if (rit->second.mSequence + rit->second.mDistance * 2 < currseq)
			{
#ifdef	DEBUG_DSDV
				std::cerr << "\t\tIgnoring OLD SEQ Entry: " << rit->first;
				std::cerr << std::endl;
#endif

				continue; // ignore.
			}

			/* if we haven't received an update in ages - old */
			if (now - rit->second.mReceived > DSDV_DISCARD_PERIOD)
			{
#ifdef	DEBUG_DSDV
				std::cerr << "\t\tIgnoring OLD TIME Entry: " << rit->first;
				std::cerr << std::endl;
#endif

				continue; // ignore.
			}

			if (rit->second.mDistance < closest)
			{
				closest = rit->second.mDistance;
				closestId = rit->first;
				closestAge = now - rit->second.mValidSince;
#ifdef	DEBUG_DSDV
				std::cerr << "\t\tUpdating to Closer Entry: " << rit->first;
				std::cerr << std::endl;
#endif
			}
			else if ((rit->second.mDistance == closest) &&
				(closestAge < now - rit->second.mValidSince))
			{
				/* have a more stable (older) one */
				closest = rit->second.mDistance;
				closestId = rit->first;
				closestAge = now - rit->second.mValidSince;
#ifdef	DEBUG_DSDV
				std::cerr << "\t\tUpdating to Stabler Entry: " << rit->first;
				std::cerr << std::endl;
#endif
			}
			else
			{
#ifdef	DEBUG_DSDV
				std::cerr << "\t\tIgnoring Distant Entry: " << rit->first;
				std::cerr << std::endl;
#endif
			}

		}

		tit->second.mIsStable = true;
		rit = tit->second.mAllRoutes.find(closestId);
		tit->second.mStableRoute = rit->second;
		tit->second.mFlags &= ~RSDSDV_FLAGS_NEW_ROUTE; 

#ifdef	DEBUG_DSDV
		std::cerr << "\tStable Route: " << tit->second.mStableRoute;
		std::cerr << std::endl;
#endif
	}
	return 1;
}




int p3Dsdv::clearOldRoutes()
{
	/* iterate over the entries */
	RsStackMutex stack(mDsdvMtx); /****** LOCKED MUTEX *******/

#ifdef	DEBUG_DSDV
	std::cerr << "p3Dsdv::clearOldRoutes()";
	std::cerr << std::endl;
#endif

	/* find the entry */
	std::map<std::string, RsDsdvTableEntry>::iterator it, it2;
	for(it = mTable.begin(); it != mTable.end(); ++it)
	{
		if (it->second.mOwnSource)	
		{
			continue;
		}

		if (it->second.mIsStable)
		{
			continue;
		}

		if (it->second.mFlags & RSDSDV_FLAGS_NEW_ROUTE)
		{
			continue;
		}

		/* backstep iterator for loop, and delete original */
		it2 = it;
		it--;

#ifdef	DEBUG_DSDV
		std::cerr << "p3Dsdv::clearOldRoutes() Deleting OLD ServiceId: " << it2->first;
		std::cerr << std::endl;
#endif
		mTable.erase(it2);
	}
	return 1;
}


/*************** pqiMonitor callback ***********************/
void p3Dsdv::statusChange(const std::list<pqiServicePeer> &plist)
{
	std::list<pqiServicePeer>::const_iterator it;
	for(it = plist.begin(); it != plist.end(); ++it)
	{
		/* only care about disconnected / not friends cases */
		if (	1	)
		{
			RsStackMutex stack(mDsdvMtx); /****** LOCKED MUTEX *******/
	




		}
	}
}

int p3Dsdv::addTestService()
{
	RsDsdvId testId;

	int rndhash1[SHA_DIGEST_LENGTH / 4];
	int rndhash2[SHA_DIGEST_LENGTH / 4];
	std::string realHash;
	std::string seedHash;

	for(int i = 0; i < SHA_DIGEST_LENGTH / 4; i++)
	{
		rndhash1[i] = RSRandom::random_u32();
		rndhash2[i] = RSRandom::random_u32();
	}

	for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
	{
		rs_sprintf_append(realHash, "%02x", (uint32_t) ((uint8_t *) rndhash1)[i]);
		rs_sprintf_append(seedHash, "%02x", (uint32_t) ((uint8_t *) rndhash2)[i]);
	}


	uint8_t sha_hash[SHA_DIGEST_LENGTH];
	memset(sha_hash,0,SHA_DIGEST_LENGTH*sizeof(uint8_t)) ;
	SHA_CTX *sha_ctx = new SHA_CTX;
	SHA1_Init(sha_ctx);

	SHA1_Update(sha_ctx, realHash.c_str(), realHash.length());
	SHA1_Update(sha_ctx, seedHash.c_str(), seedHash.length());
	SHA1_Final(sha_hash, sha_ctx);
	delete sha_ctx;

	for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
	{
		rs_sprintf_append(testId.mHash, "%02x", (uint32_t) (sha_hash)[i]);
	}

	testId.mIdType = RSDSDV_IDTYPE_TEST;
	testId.mAnonChunk = seedHash;

	addDsdvId(&testId, realHash);
	return 1;
}


int p3Dsdv::addDsdvId(RsDsdvId *id, std::string realHash)
{

	RsStackMutex stack(mDsdvMtx); /****** LOCKED MUTEX *******/

#ifdef	DEBUG_DSDV
	std::cerr << "p3Dsdv::addDsdvId() ID: " << *id << " RealHash: " << realHash;
	std::cerr << std::endl;
#endif

	rstime_t now = time(NULL);
	
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

	v.mStableRoute.mNextHop = mServiceCtrl->getOwnId();
	v.mStableRoute.mReceived = now;
	v.mStableRoute.mValidSince = now;
	v.mStableRoute.mSequence = 0;
	v.mStableRoute.mDistance = 0;
	v.mIsStable = true;

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

#ifdef	DEBUG_DSDV
	std::cerr << "p3Dsdv::dropDsdvId() ID: " << *id;
	std::cerr << std::endl;
#endif

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
	for(it = mTable.begin(); it != mTable.end(); ++it)
	{
		RsDsdvTableEntry &v = it->second;
		out << v;
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
	for(it = mTable.begin(); it != mTable.end(); ++it)
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
	for(it = mTable.begin(); it != mTable.end(); ++it)
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
	rstime_t now = time(NULL);
	out << "< Seq: " << route.mSequence << " Dist: " << route.mDistance;
	out << " NextHop: " << route.mNextHop;
	out << " recvd: " << now-route.mReceived;
	out << " validSince: " << now-route.mValidSince;
	out << " >";

	return out;
}

std::ostream &operator<<(std::ostream &out, const RsDsdvTableEntry &entry)
{
	out << "DSDV Route for: " << entry.mDest << std::endl;
	if (entry.mIsStable)
	{
		out << "\tStable: " << entry.mStableRoute << std::endl;
	}
	else
	{
		out << "\tNo Stable Route" << std::endl;
	}

	out << "\tOwnSource: " << entry.mOwnSource;
	out << " Flags: " << entry.mFlags; 
	if (entry.mMatched)
	{
		out << " Matched: " << entry.mMatchedHash;
	}
	else
	{
		out << " Non Matched";
	}
	out  << std::endl;
	if (entry.mAllRoutes.size() > 0)
	{
		out << "\tAll Routes:" << std::endl;
	}

	std::map<RsPeerId, RsDsdvRoute>::const_iterator it;
	for(it = entry.mAllRoutes.begin(); it != entry.mAllRoutes.end(); ++it)
	{
		out << "\t\t" << it->second << std::endl;
	}
	return out;
}


