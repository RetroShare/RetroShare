/*******************************************************************************
 * libretroshare/src/services: p3rtt.h                                         *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011-2013 Robert Fernie <retroshare@lunamutt.com>                 *
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
#ifndef SERVICE_RSRTT_HEADER
#define SERVICE_RSRTT_HEADER

#include <list>
#include <string>

#include "rsitems/rsrttitems.h"
#include "services/p3service.h"
#include "retroshare/rsrtt.h"

class p3ServiceControl;

class RttPeerInfo
{
	public:

	bool initialisePeerInfo(const RsPeerId& id);

	RsPeerId mId;
	double mCurrentPingTS;
	double mCurrentPingCounter;
	bool   mCurrentPongRecvd;
	double mCurrentMeanOffset;

	uint32_t mLostPongs;
	uint32_t mSentPings;

	std::list<RsRttPongResult> mPongResults;
};


//!The RS Rtt Test service.
 /**
  *
  * Used to test Latency.
  */

class p3rtt: public RsRtt, public p3FastService
{
	public:
		p3rtt(p3ServiceControl *sc);
virtual RsServiceInfo getServiceInfo();

		/***** overloaded from rsRtt *****/

virtual uint32_t getPongResults(const RsPeerId& id, int n, std::list<RsRttPongResult> &results);
virtual double getMeanOffset(const RsPeerId &id);

		/***** overloaded from p3Service *****/

		virtual int   tick();
		virtual int   status();

		int     sendPackets();
		void 	sendPingMeasurements();

virtual bool recvItem(RsItem *item); // Overloaded from p3FastService.

		int 	handlePing(RsItem *item);
		int 	handlePong(RsItem *item);

		int 	storePingAttempt(const RsPeerId& id, double ts, uint32_t mCounter);
		int 	storePongResult(const RsPeerId& id, uint32_t counter, double recv_ts, double rtt, double offset);


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
		RsMutex mRttMtx;

		RttPeerInfo *locked_GetPeerInfo(const RsPeerId& id);

		std::map<RsPeerId, RttPeerInfo> mPeerInfo;
		rstime_t mSentPingTime;
		uint32_t mCounter;

		p3ServiceControl *mServiceCtrl;

};

#endif // SERVICE_RSRTT_HEADER

