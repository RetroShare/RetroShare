/*******************************************************************************
 * plugins/VOIP/services/p3VOIP.cc                                             *
 *                                                                             *
 * Copyright (C) 2015 by Retroshare Team <retroshare.project@gmail.com>        *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "util/rsdir.h"
#include "retroshare/rsiface.h"
#include "pqi/pqibin.h"
#include "pqi/pqistore.h"
#include "pqi/p3linkmgr.h"
#include <serialiser/rsserial.h>
#include <rsitems/rsconfigitems.h>

#include <sstream> // for std::istringstream

#include "services/p3VOIP.h"
#include "services/rsVOIPItems.h"
#include "gui/VOIPNotify.h"

#include <sys/time.h>

/****
 * #define DEBUG_VOIP		1
 ****/

/* DEFINE INTERFACE POINTER! */
RsVOIP *rsVOIP = NULL;


#define MAX_PONG_RESULTS		150
#define VOIP_PING_PERIOD  		10
#define VOIP_BANDWIDTH_PERIOD 5

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

p3VOIP::p3VOIP(RsPluginHandler *handler,VOIPNotify *notifier)
     : RsPQIService(RS_SERVICE_TYPE_VOIP_PLUGIN,0,handler), mVOIPMtx("p3VOIP"), mServiceControl(handler->getServiceControl()) , mNotify(notifier)
{
	addSerialType(new RsVOIPSerialiser());

	mSentPingTime = 0;
	mSentBandwidthInfoTime = 0;
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
RsServiceInfo p3VOIP::getServiceInfo()
{
    const std::string TURTLE_APP_NAME = "VOIP";
    const uint16_t TURTLE_APP_MAJOR_VERSION  =       1;
    const uint16_t TURTLE_APP_MINOR_VERSION  =       0;
    const uint16_t TURTLE_MIN_MAJOR_VERSION  =       1;
    const uint16_t TURTLE_MIN_MINOR_VERSION  =       0;

    return RsServiceInfo(RS_SERVICE_TYPE_VOIP_PLUGIN,
                         TURTLE_APP_NAME,
                         TURTLE_APP_MAJOR_VERSION,
                         TURTLE_APP_MINOR_VERSION,
                         TURTLE_MIN_MAJOR_VERSION,
                         TURTLE_MIN_MINOR_VERSION);
}

void RsVOIPDataChunk::clear()
{ 
    
    if(data) 
	    free(data) ; 
    data=NULL; 
    size=0 ;
}
int	p3VOIP::tick()
{
#ifdef DEBUG_VOIP
	std::cerr << "ticking p3VOIP" << std::endl;
#endif

	//processIncoming();
	sendPackets();

	return 0;
}

int	p3VOIP::status()
{
	return 1;
}

int	p3VOIP::sendPackets()
{
	time_t now = time(NULL);
	time_t pt;
	time_t pt2;
	{
		RsStackMutex stack(mVOIPMtx); /****** LOCKED MUTEX *******/
		pt = mSentPingTime;
		pt2 = mSentBandwidthInfoTime;
	}

	if (now > pt + VOIP_PING_PERIOD)
	{
		sendPingMeasurements();

		RsStackMutex stack(mVOIPMtx); /****** LOCKED MUTEX *******/
		mSentPingTime = now;
	}
	if (now > pt2 + VOIP_BANDWIDTH_PERIOD)
	{
		sendBandwidthInfo();

		RsStackMutex stack(mVOIPMtx); /****** LOCKED MUTEX *******/
		mSentBandwidthInfoTime = now;
	}
	return true ;
}
void p3VOIP::sendBandwidthInfo()
{
    std::set<RsPeerId> onlineIds;
	 mServiceControl->getPeersConnected(getServiceInfo().mServiceType, onlineIds);

	 RsStackMutex stack(mVOIPMtx); /****** LOCKED MUTEX *******/

	for(std::map<RsPeerId,VOIPPeerInfo>::iterator it(mPeerInfo.begin());it!=mPeerInfo.end();++it)
	{
		it->second.average_incoming_bandwidth = 0.75 * it->second.average_incoming_bandwidth + 0.25 * it->second.total_bytes_received / VOIP_BANDWIDTH_PERIOD ;
		it->second.total_bytes_received = 0 ;

		if(onlineIds.find(it->first) == onlineIds.end() || it->second.average_incoming_bandwidth == 0)
			continue ;

		std::cerr << "average bandwidth for peer " << it->first << ": " << it->second.average_incoming_bandwidth << " Bps" << std::endl;
		sendVoipBandwidth(it->first,it->second.average_incoming_bandwidth) ;
	}
}

int p3VOIP::sendVoipHangUpCall(const RsPeerId &peer_id, uint32_t flags)
{
	RsVOIPProtocolItem *item = new RsVOIPProtocolItem ;

	item->protocol = RsVOIPProtocolItem::VoipProtocol_Close;
	item->flags = flags ;
	item->PeerId(peer_id) ;

	sendItem(item) ;

	return true ;
}
int p3VOIP::sendVoipAcceptCall(const RsPeerId& peer_id, uint32_t flags)
{
	RsVOIPProtocolItem *item = new RsVOIPProtocolItem ;

	item->protocol = RsVOIPProtocolItem::VoipProtocol_Ackn ;
	item->flags = flags ;
	item->PeerId(peer_id) ;

	sendItem(item) ;

	return true ;
}
int p3VOIP::sendVoipRinging(const RsPeerId &peer_id, uint32_t flags)
{
	RsVOIPProtocolItem *item = new RsVOIPProtocolItem ;

	item->protocol = RsVOIPProtocolItem::VoipProtocol_Ring ;
	item->flags = flags ;
	item->PeerId(peer_id) ;

	sendItem(item) ;

	return true ;
}
int p3VOIP::sendVoipBandwidth(const RsPeerId &peer_id,uint32_t bytes_per_sec)
{
	RsVOIPProtocolItem *item = new RsVOIPProtocolItem ;

	item->protocol = RsVOIPProtocolItem::VoipProtocol_Bandwidth ;
	item->flags = bytes_per_sec ;
	item->PeerId(peer_id) ;

	sendItem(item) ;

	return true ;
}
int p3VOIP::sendVoipData(const RsPeerId& peer_id,const RsVOIPDataChunk& chunk)
{
#ifdef DEBUG_VOIP
	std::cerr << "Sending " << chunk.size << " bytes of voip data." << std::endl;
#endif

	RsVOIPDataItem *item = new RsVOIPDataItem ;

	if(!item)
	{
		std::cerr << "Cannot allocate RsVOIPDataItem !" << std::endl;
		return false ;
	}
	item->voip_data = rs_malloc(chunk.size) ;

	if(item->voip_data == NULL)
	{
		delete item ;
		return false ;
	}
	memcpy(item->voip_data,chunk.data,chunk.size) ;
	item->PeerId(peer_id) ;
	item->data_size = chunk.size;

	if(chunk.type == RsVOIPDataChunk::RS_VOIP_DATA_TYPE_AUDIO) 
		item->flags = RS_VOIP_FLAGS_AUDIO_DATA ;
	else if(chunk.type == RsVOIPDataChunk::RS_VOIP_DATA_TYPE_VIDEO) 
		item->flags = RS_VOIP_FLAGS_VIDEO_DATA ;
	else
	{
		std::cerr << "(EE) p3VOIP: cannot send chunk data. Unknown data type = " << chunk.type << std::endl;
		delete item ;
		return false ;
	}

	sendItem(item) ;

	return true ;
}

void p3VOIP::sendPingMeasurements()
{
	/* we ping our peers */
	/* who is online? */
    if(!mServiceControl)
        return ;

    std::set<RsPeerId> onlineIds;
        mServiceControl->getPeersConnected(getServiceInfo().mServiceType, onlineIds);

	double ts = getCurrentTS();

#ifdef DEBUG_VOIP
	std::cerr << "p3VOIP::sendPingMeasurements() @ts: " << ts;
	std::cerr << std::endl;
#endif

	/* prepare packets */
    std::set<RsPeerId>::iterator it;
    for(it = onlineIds.begin(); it != onlineIds.end(); it++)
	{
#ifdef DEBUG_VOIP
		std::cerr << "p3VOIP::sendPingMeasurements() Pinging: " << *it;
		std::cerr << std::endl;
#endif

		/* create the packet */
		RsVOIPPingItem *pingPkt = new RsVOIPPingItem();
		pingPkt->PeerId(*it);
		pingPkt->mSeqNo = mCounter;
		pingPkt->mPingTS = convertTsTo64bits(ts);

		storePingAttempt(*it, ts, mCounter);

#ifdef DEBUG_VOIP
		std::cerr << "p3VOIP::sendPingMeasurements() With Packet:";
		std::cerr << std::endl;
		pingPkt->print(std::cerr, 10);
#endif

		sendItem(pingPkt);
	}

	RsStackMutex stack(mVOIPMtx); /****** LOCKED MUTEX *******/
	mCounter++;
}


void p3VOIP::handleProtocol(RsVOIPProtocolItem *item)
{
	// should we keep a list of received requests?

	switch(item->protocol)
	{
		case RsVOIPProtocolItem::VoipProtocol_Ring: mNotify->notifyReceivedVoipInvite(item->PeerId(), (uint32_t)item->flags);
#ifdef DEBUG_VOIP
			std::cerr << "p3VOIP::handleProtocol(): Received protocol ring item." << std::endl;
#endif
		break ;

		case RsVOIPProtocolItem::VoipProtocol_Ackn: mNotify->notifyReceivedVoipAccept(item->PeerId(), (uint32_t)item->flags);
#ifdef DEBUG_VOIP
			std::cerr << "p3VOIP::handleProtocol(): Received protocol accept call" << std::endl;
#endif
		break ;

		case RsVOIPProtocolItem::VoipProtocol_Close: mNotify->notifyReceivedVoipHangUp(item->PeerId(), (uint32_t)item->flags);
#ifdef DEBUG_VOIP
			std::cerr << "p3VOIP::handleProtocol(): Received protocol Close call." << std::endl;
#endif
		break ;
		case RsVOIPProtocolItem::VoipProtocol_Bandwidth: mNotify->notifyReceivedVoipBandwidth(item->PeerId(),(uint32_t)item->flags);
#ifdef DEBUG_VOIP
			std::cerr << "p3VOIP::handleProtocol(): Received protocol bandwidth. Value=" << item->flags << std::endl;
#endif
		break ;
		default:
			std::cerr << "p3VOIP::handleProtocol(): Received protocol item # " << item->protocol << ": not handled yet ! Sorry" << std::endl;
		break ;
	}

}

void p3VOIP::handleData(RsVOIPDataItem *item)
{
	RsStackMutex stack(mVOIPMtx); /****** LOCKED MUTEX *******/

	// store the data in a queue.

	std::map<RsPeerId,VOIPPeerInfo>::iterator it = mPeerInfo.find(item->PeerId()) ;

	if(it == mPeerInfo.end())
	{
		std::cerr << "Peer unknown to VOIP process. Dropping data" << std::endl;
		delete item ;
		return ;
	}
	it->second.incoming_queue.push_back(item) ;	// be careful with the delete action!

	// For Video data, measure the bandwidth
	
	if(item->flags & RS_VOIP_FLAGS_VIDEO_DATA)
		it->second.total_bytes_received += item->data_size ;

	mNotify->notifyReceivedVoipData(item->PeerId());
}

bool p3VOIP::getIncomingData(const RsPeerId& peer_id,std::vector<RsVOIPDataChunk>& incoming_data_chunks)
{
	RsStackMutex stack(mVOIPMtx); /****** LOCKED MUTEX *******/

	incoming_data_chunks.clear() ;

	std::map<RsPeerId,VOIPPeerInfo>::iterator it = mPeerInfo.find(peer_id) ;

	if(it == mPeerInfo.end())
	{
		std::cerr << "Peer unknown to VOIP process. No data returned. Probably a bug !" << std::endl;
		return false ;
	}
	for(std::list<RsVOIPDataItem*>::const_iterator it2(it->second.incoming_queue.begin());it2!=it->second.incoming_queue.end();++it2)
	{
		RsVOIPDataChunk chunk ;
		chunk.size = (*it2)->data_size ;
		chunk.data = rs_malloc((*it2)->data_size) ;
        
        	if(chunk.data == NULL)
	    	{
                	delete *it2 ;
                	continue ;
            	}

		uint32_t type_flags = (*it2)->flags & (RS_VOIP_FLAGS_AUDIO_DATA | RS_VOIP_FLAGS_VIDEO_DATA) ;
		if(type_flags == RS_VOIP_FLAGS_AUDIO_DATA)
			chunk.type = RsVOIPDataChunk::RS_VOIP_DATA_TYPE_AUDIO ;
		else if(type_flags == RS_VOIP_FLAGS_VIDEO_DATA)
			chunk.type = RsVOIPDataChunk::RS_VOIP_DATA_TYPE_VIDEO ;
		else
		{
			std::cerr << "(EE) p3VOIP::getIncomingData(): error. Cannot handle item with unknown type " << type_flags << std::endl;
			delete *it2 ;
			free(chunk.data) ;
			continue ;
		}

		memcpy(chunk.data,(*it2)->voip_data,(*it2)->data_size) ;

		incoming_data_chunks.push_back(chunk) ;

		delete *it2 ;
	}

	it->second.incoming_queue.clear() ;

	return true ;
}

bool	p3VOIP::recvItem(RsItem *item)
{
	/* pass to specific handler */
	bool keep = false ;

	switch(item->PacketSubType())
	{
		case RS_PKT_SUBTYPE_VOIP_PING: 
			handlePing(dynamic_cast<RsVOIPPingItem*>(item));
			break;

		case RS_PKT_SUBTYPE_VOIP_PONG: 
			handlePong(dynamic_cast<RsVOIPPongItem*>(item));
			break;

		case RS_PKT_SUBTYPE_VOIP_PROTOCOL: 
			handleProtocol(dynamic_cast<RsVOIPProtocolItem*>(item)) ;
			break ;

		case RS_PKT_SUBTYPE_VOIP_DATA: 
			handleData(dynamic_cast<RsVOIPDataItem*>(item));
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

int p3VOIP::handlePing(RsVOIPPingItem *ping)
{
	/* cast to right type */

#ifdef DEBUG_VOIP
	std::cerr << "p3VOIP::handlePing() Recvd Packet from: " << ping->PeerId();
	std::cerr << std::endl;
#endif

	/* with a ping, we just respond as quickly as possible - they do all the analysis */
	RsVOIPPongItem *pong = new RsVOIPPongItem();


	pong->PeerId(ping->PeerId());
	pong->mPingTS = ping->mPingTS;
	pong->mSeqNo = ping->mSeqNo;

	// add our timestamp.
	double ts = getCurrentTS();
	pong->mPongTS = convertTsTo64bits(ts);


#ifdef DEBUG_VOIP
	std::cerr << "p3VOIP::handlePing() With Packet:";
	std::cerr << std::endl;
	pong->print(std::cerr, 10);
#endif

	sendItem(pong);
	return true ;
}


int p3VOIP::handlePong(RsVOIPPongItem *pong)
{
	/* cast to right type */

#ifdef DEBUG_VOIP
	std::cerr << "p3VOIP::handlePong() Recvd Packet from: " << pong->PeerId();
	std::cerr << std::endl;
	pong->print(std::cerr, 10);
#endif

	/* with a pong, we do the maths! */
	double recvTS = getCurrentTS();
	double pingTS = convert64bitsToTs(pong->mPingTS);
	double pongTS = convert64bitsToTs(pong->mPongTS);

	double rtt = recvTS - pingTS;
	double offset = pongTS - (recvTS - rtt / 2.0);  // so to get to their time, we go ourTS + offset.

#ifdef DEBUG_VOIP
	std::cerr << "p3VOIP::handlePong() Timing:";
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

int	p3VOIP::storePingAttempt(const RsPeerId& id, double ts, uint32_t seqno)
{
	RsStackMutex stack(mVOIPMtx); /****** LOCKED MUTEX *******/

	/* find corresponding local data */
	VOIPPeerInfo *peerInfo = locked_GetPeerInfo(id);

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



int	p3VOIP::storePongResult(const RsPeerId &id, uint32_t counter, double ts, double rtt, double offset)
{
	RsStackMutex stack(mVOIPMtx); /****** LOCKED MUTEX *******/

	/* find corresponding local data */
	VOIPPeerInfo *peerInfo = locked_GetPeerInfo(id);

	if (peerInfo->mCurrentPingCounter != counter)
	{
#ifdef DEBUG_VOIP
		std::cerr << "p3VOIP::storePongResult() ERROR Severly Delayed Measurements!" << std::endl;
#endif
	}
	else
	{
		peerInfo->mCurrentPongRecvd = true;
	}

	peerInfo->mPongResults.push_back(RsVOIPPongResult(ts, rtt, offset));


	while(peerInfo->mPongResults.size() > MAX_PONG_RESULTS)
	{
		peerInfo->mPongResults.pop_front();
	}

	/* should do calculations */
	return 1;
}


uint32_t p3VOIP::getPongResults(const RsPeerId& id, int n, std::list<RsVOIPPongResult> &results)
{
	RsStackMutex stack(mVOIPMtx); /****** LOCKED MUTEX *******/

	VOIPPeerInfo *peer = locked_GetPeerInfo(id);

	std::list<RsVOIPPongResult>::reverse_iterator it;
	int i = 0;
	for(it = peer->mPongResults.rbegin(); (it != peer->mPongResults.rend()) && (i < n); it++, i++)
	{
		/* reversing order - so its easy to trim later */
		results.push_back(*it);
	}
	return i ;
}



VOIPPeerInfo *p3VOIP::locked_GetPeerInfo(const RsPeerId &id)
{
    std::map<RsPeerId, VOIPPeerInfo>::iterator it;
	it = mPeerInfo.find(id);
	if (it == mPeerInfo.end())
	{
		/* add it in */
		VOIPPeerInfo pinfo;

		/* initialise entry */
		pinfo.initialisePeerInfo(id);
		
		mPeerInfo[id] = pinfo;

		it = mPeerInfo.find(id);
	}

	return &(it->second);
}

bool VOIPPeerInfo::initialisePeerInfo(const RsPeerId& id)
{
	mId = id;

	/* reset variables */
	mCurrentPingTS = 0;
	mCurrentPingCounter = 0;
	mCurrentPongRecvd = true;

	mSentPings = 0;
	mLostPongs = 0;
	average_incoming_bandwidth = 0 ;
	total_bytes_received = 0 ;

	mPongResults.clear();

	return true;
}

void p3VOIP::setVoipATransmit(int t)
{
	_atransmit = t ;
	IndicateConfigChanged() ;
}
void p3VOIP::setVoipVoiceHold(int vh)
{
	_voice_hold = vh ;
	IndicateConfigChanged() ;
}
void p3VOIP::setVoipfVADmin(int vad)
{
	_vadmin = vad ;
	IndicateConfigChanged() ;
}
void p3VOIP::setVoipfVADmax(int vad)
{
	_vadmax = vad ;
	IndicateConfigChanged() ;
}
void p3VOIP::setVoipiNoiseSuppress(int n)
{
	_noise_suppress = n ;
	IndicateConfigChanged() ;
}
void p3VOIP::setVoipiMinLoudness(int ml)
{
	_min_loudness = ml ;
	IndicateConfigChanged() ;
}
void p3VOIP::setVoipEchoCancel(bool b)
{
	_echo_cancel = b ;
	IndicateConfigChanged() ;
}

RsTlvKeyValue p3VOIP::push_int_value(const std::string& key,int value)
{
	RsTlvKeyValue kv ;
	kv.key = key ;
	rs_sprintf(kv.value, "%d", value);

	return kv ;
}
int p3VOIP::pop_int_value(const std::string& s)
{
	std::istringstream is(s) ;

	int val ;
	is >> val ;

	return val ;
}

bool p3VOIP::saveList(bool& cleanup, std::list<RsItem*>& lst)
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
bool p3VOIP::loadList(std::list<RsItem*>& load)
{
	for(std::list<RsItem*>::const_iterator it(load.begin());it!=load.end();++it)
	{
#ifdef P3TURTLE_DEBUG
		assert(item!=NULL) ;
#endif
		RsConfigKeyValueSet *vitem = dynamic_cast<RsConfigKeyValueSet*>(*it) ;

		if(vitem != NULL)
		{
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
		}
		delete vitem ;
	}
    
    load.clear() ;
	return true ;
}

RsSerialiser *p3VOIP::setupSerialiser()
{
	RsSerialiser *rsSerialiser = new RsSerialiser();
	rsSerialiser->addSerialType(new RsVOIPSerialiser());
	rsSerialiser->addSerialType(new RsGeneralConfigSerialiser());

	return rsSerialiser ;
}










