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
#include "pqi/pqistore.h"
#include "pqi/p3linkmgr.h"
#include <serialiser/rsserial.h>
#include <serialiser/rsconfigitems.h>

#include <sstream> // for std::istringstream

#include "services/p3vors.h"
#include "services/rsvoipitems.h"
#include "gui/PluginNotifier.h"

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

p3VoRS::p3VoRS(RsPluginHandler *handler,PluginNotifier *notifier)
	 : RsPQIService(RS_SERVICE_TYPE_VOIP_PLUGIN,CONFIG_TYPE_VOIP_PLUGIN,0,handler), mVorsMtx("p3VoRS"), mLinkMgr(handler->getLinkMgr()) , mNotify(notifier)
{
	addSerialType(new RsVoipSerialiser());

	mSentPingTime = 0;
	mCounter = 0;

        //plugin default configuration
        _atransmit = 0;
        _voice_hold = 75;
        _vadmin = 16018;
        _vadmax = 23661;
        _min_loudness = 4702;
        _noise_suppress = -45;
        _echo_cancel = true;

}

int	p3VoRS::tick()
{
#ifdef DEBUG_VORS
	std::cerr << "ticking p3VoRS" << std::endl;
#endif

	//processIncoming();
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
int p3VoRS::sendVoipHangUpCall(const std::string& peer_id)
{
	RsVoipProtocolItem *item = new RsVoipProtocolItem ;

	item->protocol = RsVoipProtocolItem::VoipProtocol_Close;
	item->flags = 0 ;
	item->PeerId(peer_id) ;

	sendItem(item) ;

	return true ;
}
int p3VoRS::sendVoipAcceptCall(const std::string& peer_id)
{
	RsVoipProtocolItem *item = new RsVoipProtocolItem ;

	item->protocol = RsVoipProtocolItem::VoipProtocol_Ackn ;
	item->flags = 0 ;
	item->PeerId(peer_id) ;

	sendItem(item) ;

	return true ;
}
int p3VoRS::sendVoipRinging(const std::string& peer_id)
{
	RsVoipProtocolItem *item = new RsVoipProtocolItem ;

	item->protocol = RsVoipProtocolItem::VoipProtocol_Ring ;
	item->flags = 0 ;
	item->PeerId(peer_id) ;

	sendItem(item) ;

	return true ;
}

int p3VoRS::sendVoipData(const std::string& peer_id,const RsVoipDataChunk& chunk)
{
#ifdef DEBUG_VORS
	std::cerr << "Sending " << chunk.size << " bytes of voip data." << std::endl;
#endif

	RsVoipDataItem *item = new RsVoipDataItem ;

	if(!item)
	{
		std::cerr << "Cannot allocate RsVoipDataItem !" << std::endl;
		return false ;
	}
	item->voip_data = malloc(chunk.size) ;

	if(item->voip_data == NULL)
	{
		std::cerr << "Cannot allocate RsVoipDataItem.voip_data of size " << chunk.size << " !" << std::endl;
		return false ;
	}
	memcpy(item->voip_data,chunk.data,chunk.size) ;
	item->flags = 0 ;
        item->PeerId(peer_id) ;
        item->data_size = chunk.size;

	sendItem(item) ;

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


void p3VoRS::handleProtocol(RsVoipProtocolItem *item)
{
	// should we keep a list of received requests?

	switch(item->protocol)
	{
		case RsVoipProtocolItem::VoipProtocol_Ring: mNotify->notifyReceivedVoipInvite(item->PeerId());
#ifdef DEBUG_VORS
																  std::cerr << "p3VoRS::handleProtocol(): Received protocol ring item." << std::endl;
#endif
																  break ;

		case RsVoipProtocolItem::VoipProtocol_Ackn: mNotify->notifyReceivedVoipAccept(item->PeerId());
#ifdef DEBUG_VORS
																  std::cerr << "p3VoRS::handleProtocol(): Received protocol accept call" << std::endl;
#endif
																  break ;

		case RsVoipProtocolItem::VoipProtocol_Close: mNotify->notifyReceivedVoipHangUp(item->PeerId());
#ifdef DEBUG_VORS
																  std::cerr << "p3VoRS::handleProtocol(): Received protocol Close call." << std::endl;
#endif
																  break ;
		default:
#ifdef DEBUG_VORS
																  std::cerr << "p3VoRS::handleProtocol(): Received protocol item # " << item->protocol << ": not handled yet ! Sorry" << std::endl;
#endif
																  break ;
	}

}

void p3VoRS::handleData(RsVoipDataItem *item)
{
	RsStackMutex stack(mVorsMtx); /****** LOCKED MUTEX *******/

	// store the data in a queue.

	std::map<std::string,VorsPeerInfo>::iterator it = mPeerInfo.find(item->PeerId()) ;

	if(it == mPeerInfo.end())
	{
		std::cerr << "Peer unknown to VOIP process. Dropping data" << std::endl;
		delete item ;
	}
	else
	{
		it->second.incoming_queue.push_back(item) ;	// be careful with the delete action!

		mNotify->notifyReceivedVoipData(item->PeerId());
	}
}

bool p3VoRS::getIncomingData(const std::string& peer_id,std::vector<RsVoipDataChunk>& incoming_data_chunks)
{
	RsStackMutex stack(mVorsMtx); /****** LOCKED MUTEX *******/

	incoming_data_chunks.clear() ;

	std::map<std::string,VorsPeerInfo>::iterator it = mPeerInfo.find(peer_id) ;

	if(it == mPeerInfo.end())
	{
		std::cerr << "Peer unknown to VOIP process. No data returned. Probably a bug !" << std::endl;
		return false ;
	}
	for(std::list<RsVoipDataItem*>::const_iterator it2(it->second.incoming_queue.begin());it2!=it->second.incoming_queue.end();++it2)
	{
		RsVoipDataChunk chunk ;
		chunk.size = (*it2)->data_size ;
		chunk.data = malloc((*it2)->data_size) ;
		memcpy(chunk.data,(*it2)->voip_data,(*it2)->data_size) ;

		incoming_data_chunks.push_back(chunk) ;

		delete *it2 ;
	}

	it->second.incoming_queue.clear() ;

	return true ;
}

bool	p3VoRS::recvItem(RsItem *item)
{
	/* pass to specific handler */
	bool keep = false ;

	switch(item->PacketSubType())
	{
		case RS_PKT_SUBTYPE_VOIP_PING: 
			handlePing(dynamic_cast<RsVoipPingItem*>(item));
			break;

		case RS_PKT_SUBTYPE_VOIP_PONG: 
			handlePong(dynamic_cast<RsVoipPongItem*>(item));
			break;

		case RS_PKT_SUBTYPE_VOIP_PROTOCOL: 
			handleProtocol(dynamic_cast<RsVoipProtocolItem*>(item)) ;
			break ;

		case RS_PKT_SUBTYPE_VOIP_DATA: 
			handleData(dynamic_cast<RsVoipDataItem*>(item));
			keep = true ;
			break;
#if 0
													 /* THESE ARE ALL FUTURISTIC DATA TYPES */
		case RS_BANDWIDTH_PING_ITEM:	 
			handleBandwidthPing(item);
			break;

		case RS_BANDWIDTH_PONG_ITEM:
			handleBandwidthPong(item);
			 break;
#endif
		default:
			break;
	}

	/* clean up */
	if(!keep)
		delete item;
	return true ;
} 

int p3VoRS::handlePing(RsVoipPingItem *ping)
{
	/* cast to right type */

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


int p3VoRS::handlePong(RsVoipPongItem *pong)
{
	/* cast to right type */

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

void p3VoRS::setVoipATransmit(int t) 
{
	_atransmit = t ;
	IndicateConfigChanged() ;
}
void p3VoRS::setVoipVoiceHold(int vh) 
{
	_voice_hold = vh ;
	IndicateConfigChanged() ;
}
void p3VoRS::setVoipfVADmin(int vad) 
{
	_vadmin = vad ;
	IndicateConfigChanged() ;
}
void p3VoRS::setVoipfVADmax(int vad) 
{
	_vadmax = vad ;
	IndicateConfigChanged() ;
}
void p3VoRS::setVoipiNoiseSuppress(int n) 
{
	_noise_suppress = n ;
	IndicateConfigChanged() ;
}
void p3VoRS::setVoipiMinLoudness(int ml) 
{
	_min_loudness = ml ;
	IndicateConfigChanged() ;
}
void p3VoRS::setVoipEchoCancel(bool b) 
{
	_echo_cancel = b ;
	IndicateConfigChanged() ;
}

RsTlvKeyValue p3VoRS::push_int_value(const std::string& key,int value)
{
	RsTlvKeyValue kv ;
	kv.key = key ;
	rs_sprintf(kv.value, "%d", value);

	return kv ;
}
int p3VoRS::pop_int_value(const std::string& s)
{
	std::istringstream is(s) ;

	int val ;
	is >> val ;

	return val ;
}

bool p3VoRS::saveList(bool& cleanup, std::list<RsItem*>& lst) 
{
	cleanup = true ;

	RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet ;

	vitem->tlvkvs.pairs.push_back(push_int_value("P3VOIP_CONFIG_ATRANSMIT",_atransmit)) ;
	vitem->tlvkvs.pairs.push_back(push_int_value("P3VOIP_CONFIG_VOICEHOLD",_voice_hold)) ;
	vitem->tlvkvs.pairs.push_back(push_int_value("P3VOIP_CONFIG_VADMIN"   ,_vadmin)) ;
	vitem->tlvkvs.pairs.push_back(push_int_value("P3VOIP_CONFIG_VADMAX"   ,_vadmax)) ;
	vitem->tlvkvs.pairs.push_back(push_int_value("P3VOIP_CONFIG_NOISE_SUP",_noise_suppress)) ;
	vitem->tlvkvs.pairs.push_back(push_int_value("P3VOIP_CONFIG_MIN_LOUDN",_min_loudness)) ;
	vitem->tlvkvs.pairs.push_back(push_int_value("P3VOIP_CONFIG_ECHO_CNCL",_echo_cancel)) ;

	lst.push_back(vitem) ;

	return true ;
}
bool p3VoRS::loadList(std::list<RsItem*>& load) 
{
	for(std::list<RsItem*>::const_iterator it(load.begin());it!=load.end();++it)
	{
#ifdef P3TURTLE_DEBUG
		assert(item!=NULL) ;
#endif
		RsConfigKeyValueSet *vitem = dynamic_cast<RsConfigKeyValueSet*>(*it) ;

		if(vitem != NULL)
			for(std::list<RsTlvKeyValue>::const_iterator kit = vitem->tlvkvs.pairs.begin(); kit != vitem->tlvkvs.pairs.end(); ++kit) 
				if(kit->key == "P3VOIP_CONFIG_ATRANSMIT")
					_atransmit = pop_int_value(kit->value) ;
				else if(kit->key == "P3VOIP_CONFIG_VOICEHOLD")
					_voice_hold = pop_int_value(kit->value) ;
				else if(kit->key == "P3VOIP_CONFIG_VADMIN")
					_vadmin = pop_int_value(kit->value) ;
				else if(kit->key == "P3VOIP_CONFIG_VADMAX")
					_vadmax = pop_int_value(kit->value) ;
				else if(kit->key == "P3VOIP_CONFIG_NOISE_SUP")
					_noise_suppress = pop_int_value(kit->value) ;
				else if(kit->key == "P3VOIP_CONFIG_MIN_LOUDN")
					_min_loudness = pop_int_value(kit->value) ;
				else if(kit->key == "P3VOIP_CONFIG_ECHO_CNCL")
					_echo_cancel = pop_int_value(kit->value) ;

		delete vitem ;
	}

	return true ;
}

RsSerialiser *p3VoRS::setupSerialiser() 
{
	RsSerialiser *rsSerialiser = new RsSerialiser();
	rsSerialiser->addSerialType(new RsVoipSerialiser());
	rsSerialiser->addSerialType(new RsGeneralConfigSerialiser());

	return rsSerialiser ;
}










