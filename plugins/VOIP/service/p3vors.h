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

#include "serialiser/rsvoipitems.h"
#include "services/p3service.h"
#include "retroshare/rsvoip.h"

class p3LinkMgr;

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
};


//!The RS VoIP Test service.
 /**
  *
  * This is only used to test Latency for the moment.
  */

class p3VoRS: public RsVoip, public p3Service
// Maybe we inherit from these later - but not needed for now.
//, public p3Config, public pqiMonitor
{
	public:
		p3VoRS(p3LinkMgr *cm);

		/***** overloaded from rsVoip *****/

virtual uint32_t getPongResults(std::string id, int n, std::list<RsVoipPongResult> &results);

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


		int     sendPackets();
		void 	sendPingMeasurements();
		int 	processIncoming();

		int 	handlePing(RsItem *item);
		int 	handlePong(RsItem *item);

		int 	storePingAttempt(std::string id, double ts, uint32_t mCounter);
		int 	storePongResult(std::string id, uint32_t counter, double ts, double rtt, double offset);


		/*!
		 * This retrieves all public chat msg items
		 */
		//bool getPublicChatQueue(std::list<ChatInfo> &chats);

		/*************** pqiMonitor callback ***********************/
		//virtual void statusChange(const std::list<pqipeer> &plist);


		/************* from p3Config *******************/
		//virtual RsSerialiser *setupSerialiser() ;
		//virtual bool saveList(bool& cleanup, std::list<RsItem*>&) ;
		//virtual void saveDone();
		//virtual bool loadList(std::list<RsItem*>& load) ;

	private:
		RsMutex mVorsMtx;

		VorsPeerInfo *locked_GetPeerInfo(std::string id);

		std::map<std::string, VorsPeerInfo> mPeerInfo;
		time_t mSentPingTime;
		uint32_t mCounter;

		p3LinkMgr *mLinkMgr;

};

#endif // SERVICE_RSVOIP_HEADER

