/*******************************************************************************
 * libretroshare/src/services: p3bwctrl.h                                      *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012 by Robert Fernie <retroshare@lunamutt.com>                   *
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
#ifndef SERVICE_RSBANDWIDTH_CONTROL_HEADER
#define SERVICE_RSBANDWIDTH_CONTROL_HEADER

#include <string>
#include <list>
#include <map>

#include "rsitems/rsbwctrlitems.h"
#include "services/p3service.h"
#include "pqi/pqiservicemonitor.h"
#include "retroshare/rsconfig.h" // for datatypes.

class pqipersongrp;

// Extern is defined here - as this is bundled with rsconfig.h
class p3BandwidthControl;
extern p3BandwidthControl *rsBandwidthControl;


class BwCtrlData
{
	public:
	BwCtrlData()
	:mRateUpdateTs(0), mAllocated(0), mLastSend(0), mAllowedOut(0), mLastRecvd(0)
	{ return; }

	/* Rates are floats in KB/s */
	RsBwRates mRates; 
	rstime_t    mRateUpdateTs;

	/* these are integers (B/s) */
	uint32_t  mAllocated;
	rstime_t    mLastSend;

	uint32_t  mAllowedOut;
	rstime_t    mLastRecvd;
};


//!The RS bandwidth Control Service.
 /**
  *
  * Exchange packets to regulate p2p bandwidth.
  * 
  * Sadly this has to be strongly integrated into pqi, with ref to pqipersongrp.
  */

class p3BandwidthControl: public p3Service, public pqiServiceMonitor
{
	public:
		explicit p3BandwidthControl(pqipersongrp *pg);
		virtual RsServiceInfo getServiceInfo();

		/***** overloaded from RsBanList *****/


		/***** overloaded from p3Service *****/
		/*!
		 * This retrieves all BwCtrl items
		 */
		virtual int   tick();
		virtual int   status();


		/***** for RsConfig (not directly overloaded) ****/

		virtual int getTotalBandwidthRates(RsConfigDataRates &rates);
		virtual int getAllBandwidthRates(std::map<RsPeerId, RsConfigDataRates> &ratemap);


        virtual int ExtractTrafficInfo(std::list<RSTrafficClue> &out_stats, std::list<RSTrafficClue> &in_stats);

		/*!
		 * Interface stuff.
		 */

		/*************** pqiMonitor callback ***********************/
		virtual void statusChange(const std::list<pqiServicePeer> &plist);


		/************* from p3Config *******************/
		//virtual RsSerialiser *setupSerialiser() ;
		//virtual bool saveList(bool& cleanup, std::list<RsItem*>&) ;
		//virtual void saveDone();
		//virtual bool loadList(std::list<RsItem*>& load) ;

private:

		bool 	checkAvailableBandwidth();
		bool 	processIncoming();

		pqipersongrp *mPg;

		RsMutex mBwMtx;

		int printRateInfo_locked(std::ostream &out);

		rstime_t mLastCheck;

		RsBwRates mTotalRates;
		std::map<RsPeerId, BwCtrlData> mBwMap;

};

#endif // SERVICE_RSBANDWIDTH_CONTROL_HEADER
