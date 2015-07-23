/*
 * libretroshare/src/services/p3bwctrl.h
 *
 * Bandwidth Control.
 *
 * Copyright 2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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


#ifndef SERVICE_RSBANDWIDTH_CONTROL_HEADER
#define SERVICE_RSBANDWIDTH_CONTROL_HEADER

#include <string>
#include <list>
#include <map>

#include "serialiser/rsbwctrlitems.h"
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
	time_t    mRateUpdateTs;

	/* these are integers (B/s) */
	uint32_t  mAllocated;
	time_t    mLastSend;

	uint32_t  mAllowedOut;
	time_t    mLastRecvd;
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
		p3BandwidthControl(pqipersongrp *pg);
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


        virtual int ExtractTrafficInfo(std::list<RSTrafficClue> &in_stats, std::list<RSTrafficClue> &out_stats);

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

		time_t mLastCheck;

		RsBwRates mTotalRates;
		std::map<RsPeerId, BwCtrlData> mBwMap;

};

#endif // SERVICE_RSBANDWIDTH_CONTROL_HEADER
