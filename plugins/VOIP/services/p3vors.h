/*
 * libretroshare/src/services/p3vors.h
 *
 * Tests for VoIP for RetroShare.
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


#ifndef SERVICE_RSVOIP_HEADER
#define SERVICE_RSVOIP_HEADER

#include <list>
#include <string>

#include "services/rsvoipitems.h"
#include "services/p3service.h"
#include "plugins/rspqiservice.h"
#include <interface/rsvoip.h>

class p3LinkMgr;
class PluginNotifier ;

class VorsPeerInfo
{
	public:

	bool initialisePeerInfo(std::string id);

	std::string mId;
	double mCurrentPingTS;
	double mCurrentPingCounter;
	bool   mCurrentPongRecvd;

	uint32_t mLostPongs;
	uint32_t mSentPings;

	std::list<RsVoipPongResult> mPongResults;
	std::list<RsVoipDataItem*> incoming_queue ;
};


//!The RS VoIP Test service.
 /**
  *
  * This is only used to test Latency for the moment.
  */

class p3VoRS: public RsPQIService, public RsVoip
// Maybe we inherit from these later - but not needed for now.
//, public p3Config, public pqiMonitor
{
	public:
		p3VoRS(RsPluginHandler *cm,PluginNotifier *);

		/***** overloaded from rsVoip *****/

		virtual uint32_t getPongResults(std::string id, int n, std::list<RsVoipPongResult> &results);

		// Call stuff.
		//

		// Sending data. The client keeps the memory ownership and must delete it after calling this.
		virtual int sendVoipData(const std::string& peer_id,const RsVoipDataChunk& chunk) ;

		// The server fill in the data and gives up memory ownership. The client must delete the memory
		// in each chunk once it has been used.
		//
		virtual bool getIncomingData(const std::string& peer_id,std::vector<RsVoipDataChunk>& chunks) ;

		virtual int sendVoipHangUpCall(const std::string& peer_id) ;
		virtual int sendVoipRinging(const std::string& peer_id) ;
		virtual int sendVoipAcceptCall(const std::string& peer_id) ;

		/***** overloaded from p3Service *****/
		/*!
		 * This retrieves all chat msg items and also (important!)
		 * processes chat-status items that are in service item queue. chat msg item requests are also processed and not returned
		 * (important! also) notifications sent to notify base  on receipt avatar, immediate status and custom status
		 * : notifyCustomState, notifyChatStatus, notifyPeerHasNewAvatar
		 * @see NotifyBase
		 */
		virtual int   tick();
		virtual int   status();
		virtual bool  recvItem(RsItem *item);

		/*************** pqiMonitor callback ***********************/
		//virtual void statusChange(const std::list<pqipeer> &plist);

		virtual int  getVoipATransmit() const  { return _atransmit ; }
		virtual void setVoipATransmit(int) ;
		virtual int  getVoipVoiceHold() const  { return _voice_hold ; }
		virtual void setVoipVoiceHold(int) ;
		virtual int  getVoipfVADmin() const    { return _vadmin ; }
		virtual void setVoipfVADmin(int) ;
		virtual int  getVoipfVADmax() const    { return _vadmax ; } 
		virtual void setVoipfVADmax(int) ;
		virtual int  getVoipiNoiseSuppress() const { return _noise_suppress ; }
		virtual void setVoipiNoiseSuppress(int) ;
		virtual int  getVoipiMinLoudness() const   { return _min_loudness ; }
		virtual void setVoipiMinLoudness(int) ;
		virtual bool getVoipEchoCancel() const 	 { return _echo_cancel ; }
		virtual void setVoipEchoCancel(bool) ;

		/************* from p3Config *******************/
		virtual RsSerialiser *setupSerialiser() ;

		/*!
		 * chat msg items and custom status are saved
		 */
		virtual bool saveList(bool& cleanup, std::list<RsItem*>&) ;
		virtual bool loadList(std::list<RsItem*>& load) ;

	private:
		int   sendPackets();
		void 	sendPingMeasurements();
		//int 	processIncoming();

		int 	handlePing(RsVoipPingItem *item);
		int 	handlePong(RsVoipPongItem *item);

		int 	storePingAttempt(std::string id, double ts, uint32_t mCounter);
		int 	storePongResult(std::string id, uint32_t counter, double ts, double rtt, double offset);

		void handleProtocol(RsVoipProtocolItem*) ;
		void handleData(RsVoipDataItem*) ;

		RsMutex mVorsMtx;

		VorsPeerInfo *locked_GetPeerInfo(std::string id);

		static RsTlvKeyValue push_int_value(const std::string& key,int value) ;
		static int pop_int_value(const std::string& s) ;

		std::map<std::string, VorsPeerInfo> mPeerInfo;
		time_t mSentPingTime;
		uint32_t mCounter;

		p3LinkMgr *mLinkMgr;
		PluginNotifier *mNotify ;

		int _atransmit ;
		int _voice_hold ;
		int _vadmin ;
		int _vadmax ;
		int _min_loudness ;
		int _noise_suppress ;
		bool _echo_cancel ;
};

#endif // SERVICE_RSVOIP_HEADER

