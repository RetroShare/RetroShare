/*
 * libretroshare/src/services p3vors.cc
 *
 * Voice Over Retroshare Service  for RetroShare.
 *
 * Copyright 2011-2011 by Robert Fernie.
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

#include "util/rsdir.h"
#include "retroshare/rsiface.h"
#include "pqi/pqibin.h"
#include "pqi/pqinotify.h"
#include "pqi/pqistore.h"
#include "pqi/p3linkmgr.h"

#include "services/p3vors.h"
#include "serialiser/rsvoipitems.h"

#include <sys/time.h>

/****
 * #define DEBUG_VORS		1
 ****/


/* DEFINE INTERFACE POINTER! */
RsVoip *rsVoip = NULL;


#define MAX_PONG_RESULTS	150
#define VORS_PING_PERIOD  	10

/************ IMPLEMENTATION NOTES *********************************
 * 
 * Voice over Retroshare ;)
 * 
 * This will be a simple test VoIP system aimed at testing out the possibilities.
 *
 * Important things to test:
 * 1) lag, and variability in data rate
 * 	- To do this we time tag every packet..., the destination can use this info to calculate the results.
 *	- Like imixitup. Dt = clock_diff + lag. 
 *	        we expect clock_diff to be relatively constant, but lag to vary.
 *		lag cannot be negative, so minimal Dt is ~clock_diff, and delays on this are considered +lag.
 *
 * 2) we could directly measure lag. ping back and forth with Timestamps.
 *
 * 3) we also want to measure bandwidth...
 *	- not sure the best method? 
 *		one way: send a ping, then a large amount of data (5 seconds worth), then another ping.
 *			the delta in timestamps should be a decent indication of bandwidth.
 *			say we have a 100kb/s connection... need 500kb.
 *			actually the amount of data should be based on a reasonable maximum that we require.
 *			what does decent video require?
 *			Audio we can test for 64kb/s - which seems like a decent rate: e.g. mono, 16bit 22k = 1 x 2 x 22k = 44 kilobytes/sec
 *		best to do this without a VoIP call going on ;)
 *
 *
 */



#if 0
class RsVorsLagItem: public RsItem
{
	public:

	uint32_t seqno;
	uint32_t type; 	// REQUEST, RESPONSE.
	double peerTs;

};

class RsVorsDatatem: public RsItem
{
	public:

	uint32_t seqno;
	uint32_t encoding;
	uint32_t audiolength;   // in 44.1 kbs samples.
	uint32_t datalength;   
	void *data;
};

#endif


#ifdef WINDOWS_SYS
#include <time.h>
#include <sys/timeb.h>
#endif

static double getCurrentTS()
{

#ifndef WINDOWS_SYS
        struct timeval cts_tmp;
        gettimeofday(&cts_tmp, NULL);
        double cts =  (cts_tmp.tv_sec) + ((double) cts_tmp.tv_usec) / 1000000.0;
#else
        struct _timeb timebuf;
        _ftime( &timebuf);
        double cts =  (timebuf.time) + ((double) timebuf.millitm) / 1000.0;
#endif
        return cts;
}

static uint64_t convertTsTo64bits(double ts)
{
	uint32_t secs = (uint32_t) ts;
	uint32_t usecs = (uint32_t) ((ts - (double) secs) * 1000000);
	uint64_t bits = (((uint64_t) secs) << 32) + usecs;
	return bits;
}


static double convert64bitsToTs(uint64_t bits)
{
	uint32_t usecs = (uint32_t) (bits & 0xffffffff);
	uint32_t secs = (uint32_t) ((bits >> 32) & 0xffffffff);
	double ts =  (secs) + ((double) usecs) / 1000000.0;

	return ts;
}




p3VoRS::p3VoRS(p3LinkMgr *lm)
	:p3Service(RS_SERVICE_TYPE_VOIP), /* p3Config(CONFIG_TYPE_VOIP), */ mVorsMtx("p3VoRS"), mLinkMgr(lm) 
{
	addSerialType(new RsVoipSerialiser());

	mSentPingTime = 0;
	mCounter = 0;

}


int	p3VoRS::tick()
{
	processIncoming();
	sendPackets();

	return 0;
}

int	p3VoRS::status()
{
	return 1;
}



int	p3VoRS::sendPackets()
{
	time_t now = time(NULL);
	time_t pt;
	{
		RsStackMutex stack(mVorsMtx); /****** LOCKED MUTEX *******/
		pt = mSentPingTime;
	}

	if (now - pt > VORS_PING_PERIOD)
	{
		sendPingMeasurements();

		RsStackMutex stack(mVorsMtx); /****** LOCKED MUTEX *******/
		mSentPingTime = now;
	}
	return true ;
}



void p3VoRS::sendPingMeasurements()
{


	/* we ping our peers */
	/* who is online? */
	std::list<std::string> idList;

	mLinkMgr->getOnlineList(idList);

	double ts = getCurrentTS();

#ifdef DEBUG_VORS
	std::cerr << "p3VoRS::sendPingMeasurements() @ts: " << ts;
	std::cerr << std::endl;
#endif

	/* prepare packets */
	std::list<std::string>::iterator it;
	for(it = idList.begin(); it != idList.end(); it++)
	{
#ifdef DEBUG_VORS
		std::cerr << "p3VoRS::sendPingMeasurements() Pinging: " << *it;
		std::cerr << std::endl;
#endif

		/* create the packet */
		RsVoipPingItem *pingPkt = new RsVoipPingItem();
		pingPkt->PeerId(*it);
		pingPkt->mSeqNo = mCounter;
		pingPkt->mPingTS = convertTsTo64bits(ts);

		storePingAttempt(*it, ts, mCounter);

#ifdef DEBUG_VORS
		std::cerr << "p3VoRS::sendPingMeasurements() With Packet:";
		std::cerr << std::endl;
		pingPkt->print(std::cerr, 10);
#endif

		sendItem(pingPkt);
	}

	RsStackMutex stack(mVorsMtx); /****** LOCKED MUTEX *******/
	mCounter++;
}




int	p3VoRS::processIncoming()
{
	/* for each packet - pass to specific handler */
	RsItem *item = NULL;
	while(NULL != (item = recvItem()))
	{
		switch(item->PacketSubType())
		{
			default:
				break;
			case RS_PKT_SUBTYPE_VOIP_PING:
			{
				handlePing(item);
			}
				break;
			case RS_PKT_SUBTYPE_VOIP_PONG:
			{
				handlePong(item);
			}
				break;

#if 0
			/* THESE ARE ALL FUTURISTIC DATA TYPES */
			case RS_DATA_ITEM:
			{
				handleData(item);
			}
				break;

			case RS_BANDWIDTH_PING_ITEM:
			{
				handleBandwidthPing(item);
			}
				break;

			case RS_BANDWIDTH_PONG_ITEM:
			{
				handleBandwidthPong(item);
			}
				break;
#endif
		}

		/* clean up */
		delete item;
	}
	return true ;
} 

int p3VoRS::handlePing(RsItem *item)
{
	/* cast to right type */
	RsVoipPingItem *ping = (RsVoipPingItem *) item;

#ifdef DEBUG_VORS
	std::cerr << "p3VoRS::handlePing() Recvd Packet from: " << ping->PeerId();
	std::cerr << std::endl;
#endif

	/* with a ping, we just respond as quickly as possible - they do all the analysis */
	RsVoipPongItem *pong = new RsVoipPongItem();


	pong->PeerId(ping->PeerId());
	pong->mPingTS = ping->mPingTS;
	pong->mSeqNo = ping->mSeqNo;

	// add our timestamp.
	double ts = getCurrentTS();
	pong->mPongTS = convertTsTo64bits(ts);


#ifdef DEBUG_VORS
	std::cerr << "p3VoRS::handlePing() With Packet:";
	std::cerr << std::endl;
	pong->print(std::cerr, 10);
#endif

	sendItem(pong);
	return true ;
}


int p3VoRS::handlePong(RsItem *item)
{
	/* cast to right type */
	RsVoipPongItem *pong = (RsVoipPongItem *) item;

#ifdef DEBUG_VORS
	std::cerr << "p3VoRS::handlePong() Recvd Packet from: " << pong->PeerId();
	std::cerr << std::endl;
	pong->print(std::cerr, 10);
#endif

	/* with a pong, we do the maths! */
	double recvTS = getCurrentTS();
	double pingTS = convert64bitsToTs(pong->mPingTS);
	double pongTS = convert64bitsToTs(pong->mPongTS);

	double rtt = recvTS - pingTS;
	double offset = pongTS - (recvTS - rtt / 2.0);  // so to get to their time, we go ourTS + offset.

#ifdef DEBUG_VORS
	std::cerr << "p3VoRS::handlePong() Timing:";
	std::cerr << std::endl;
	std::cerr << "\tpingTS: " << pingTS;
	std::cerr << std::endl;
	std::cerr << "\tpongTS: " << pongTS;
	std::cerr << std::endl;
	std::cerr << "\trecvTS: " << recvTS;
	std::cerr << std::endl;
	std::cerr << "\t ==> rtt: " << rtt;
	std::cerr << std::endl;
	std::cerr << "\t ==> offset: " << offset;
	std::cerr << std::endl;
#endif

	storePongResult(pong->PeerId(), pong->mSeqNo, pingTS, rtt, offset);
	return true ;
}




int	p3VoRS::storePingAttempt(std::string id, double ts, uint32_t seqno)
{
	RsStackMutex stack(mVorsMtx); /****** LOCKED MUTEX *******/

	/* find corresponding local data */
	VorsPeerInfo *peerInfo = locked_GetPeerInfo(id);

	peerInfo->mCurrentPingTS = ts;
	peerInfo->mCurrentPingCounter = seqno;

	peerInfo->mSentPings++;
	if (!peerInfo->mCurrentPongRecvd)
	{
		peerInfo->mLostPongs++;
	}

	peerInfo->mCurrentPongRecvd = true;

	return 1;
}



int	p3VoRS::storePongResult(std::string id, uint32_t counter, double ts, double rtt, double offset)
{
	RsStackMutex stack(mVorsMtx); /****** LOCKED MUTEX *******/

	/* find corresponding local data */
	VorsPeerInfo *peerInfo = locked_GetPeerInfo(id);

	if (peerInfo->mCurrentPingCounter != counter)
	{
#ifdef DEBUG_VORS
		std::cerr << "p3VoRS::storePongResult() ERROR Severly Delayed Measurements!" << std::endl;
#endif
	}
	else
	{
		peerInfo->mCurrentPongRecvd = true;
	}

	peerInfo->mPongResults.push_back(RsVoipPongResult(ts, rtt, offset));


	while(peerInfo->mPongResults.size() > MAX_PONG_RESULTS)
	{
		peerInfo->mPongResults.pop_front();
	}

	/* should do calculations */
	return 1;
}


uint32_t p3VoRS::getPongResults(std::string id, int n, std::list<RsVoipPongResult> &results)
{
	RsStackMutex stack(mVorsMtx); /****** LOCKED MUTEX *******/

	VorsPeerInfo *peer = locked_GetPeerInfo(id);

	std::list<RsVoipPongResult>::reverse_iterator it;
	int i = 0;
	for(it = peer->mPongResults.rbegin(); (it != peer->mPongResults.rend()) && (i < n); it++, i++)
	{
		/* reversing order - so its easy to trim later */
		results.push_back(*it);
	}
	return i ;
}



VorsPeerInfo *p3VoRS::locked_GetPeerInfo(std::string id)
{
	std::map<std::string, VorsPeerInfo>::iterator it;
	it = mPeerInfo.find(id);
	if (it == mPeerInfo.end())
	{
		/* add it in */
		VorsPeerInfo pinfo;

		/* initialise entry */
		pinfo.initialisePeerInfo(id);
		
		mPeerInfo[id] = pinfo;

		it = mPeerInfo.find(id);

	}

	return &(it->second);
}



bool VorsPeerInfo::initialisePeerInfo(std::string id)
{
	mId = id;

	/* reset variables */
	mCurrentPingTS = 0;
	mCurrentPingCounter = 0;
	mCurrentPongRecvd = true;

	mSentPings = 0;
	mLostPongs = 0;

	mPongResults.clear();

	return true;
}










