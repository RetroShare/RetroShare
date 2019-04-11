/*******************************************************************************
 * plugins/VOIP/services/p3VOIP.h                                              *
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

#pragma once

#include <list>
#include <string>

#include "services/rsVOIPItems.h"
#include "services/p3service.h"
#include "serialiser/rstlvbase.h"
#include "rsitems/rsconfigitems.h"
#include "plugins/rspqiservice.h"
#include <interface/rsVOIP.h>

class p3LinkMgr;
class VOIPNotify ;

class VOIPPeerInfo
{
	public:

    bool initialisePeerInfo(const RsPeerId &id);

    RsPeerId mId;
	double mCurrentPingTS;
	double mCurrentPingCounter;
	bool   mCurrentPongRecvd;

	uint32_t mLostPongs;
	uint32_t mSentPings;
	uint32_t total_bytes_received ;
	uint32_t average_incoming_bandwidth ;

	std::list<RsVOIPPongResult> mPongResults;
	std::list<RsVOIPDataItem*> incoming_queue ;
};


//!The RS VoIP Test service.
 /**
  *
  * This is only used to test Latency for the moment.
  */

class p3VOIP: public RsPQIService, public RsVOIP
// Maybe we inherit from these later - but not needed for now.
//, public p3Config, public pqiMonitor
{
	public:
		p3VOIP(RsPluginHandler *cm,VOIPNotify *);

		/***** overloaded from rsVOIP *****/

		virtual uint32_t getPongResults(const RsPeerId &id, int n, std::list<RsVOIPPongResult> &results);

		// Call stuff.
		//

		// Sending data. The client keeps the memory ownership and must delete it after calling this.
		virtual int sendVoipData(const RsPeerId &peer_id,const RsVOIPDataChunk& chunk) ;

		// The server fill in the data and gives up memory ownership. The client must delete the memory
		// in each chunk once it has been used.
		//
		virtual bool getIncomingData(const RsPeerId& peer_id,std::vector<RsVOIPDataChunk>& chunks) ;

		virtual int sendVoipHangUpCall(const RsPeerId& peer_id, uint32_t flags) ;
		virtual int sendVoipRinging(const RsPeerId& peer_id, uint32_t flags) ;
		virtual int sendVoipAcceptCall(const RsPeerId &peer_id, uint32_t flags) ;

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
		virtual std::string configurationFileName() const { return "voip.cfg" ; }

		virtual RsServiceInfo getServiceInfo() ;

	private:
		int   sendPackets();
		void 	sendPingMeasurements();
		void 	sendBandwidthInfo();

		int sendVoipBandwidth(const RsPeerId &peer_id,uint32_t bytes_per_sec) ;

		int 	handlePing(RsVOIPPingItem *item);
		int 	handlePong(RsVOIPPongItem *item);

		int 	storePingAttempt(const RsPeerId &id, double ts, uint32_t mCounter);
		int 	storePongResult(const RsPeerId& id, uint32_t counter, double ts, double rtt, double offset);

		void handleProtocol(RsVOIPProtocolItem*) ;
		void handleData(RsVOIPDataItem*) ;

		RsMutex mVOIPMtx;

		VOIPPeerInfo *locked_GetPeerInfo(const RsPeerId& id);

		static RsTlvKeyValue push_int_value(const std::string& key,int value) ;
		static int pop_int_value(const std::string& s) ;

		std::map<RsPeerId, VOIPPeerInfo> mPeerInfo;
		time_t mSentPingTime;
		time_t mSentBandwidthInfoTime;
		uint32_t mCounter;

		RsServiceControl *mServiceControl;
		VOIPNotify *mNotify ;

		int _atransmit ;
		int _voice_hold ;
		int _vadmin ;
		int _vadmax ;
		int _min_loudness ;
		int _noise_suppress ;
		bool _echo_cancel ;
};
