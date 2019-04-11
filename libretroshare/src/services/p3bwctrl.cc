/*******************************************************************************
 * libretroshare/src/services: p3bwctrl.cc                                     *
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
#include "pqi/p3linkmgr.h"
#include "pqi/p3netmgr.h"
#include "pqi/pqipersongrp.h"

#include "util/rsnet.h"

#include "services/p3bwctrl.h"
#include "rsitems/rsbwctrlitems.h"

#include <sys/time.h>

/****
 * #define DEBUG_BWCTRL		1
 ****/


/************ IMPLEMENTATION NOTES *********************************
 * 
 */

p3BandwidthControl *rsBandwidthControl;


p3BandwidthControl::p3BandwidthControl(pqipersongrp *pg)
	:p3Service(), mPg(pg), mBwMtx("p3BwCtrl")
{
	addSerialType(new RsBwCtrlSerialiser());

	mLastCheck = 0;
}


const std::string BANDWIDTH_CTRL_APP_NAME = "bandwidth_ctrl";
const uint16_t BANDWIDTH_CTRL_APP_MAJOR_VERSION  =       1;
const uint16_t BANDWIDTH_CTRL_APP_MINOR_VERSION  =       0;
const uint16_t BANDWIDTH_CTRL_MIN_MAJOR_VERSION  =       1;
const uint16_t BANDWIDTH_CTRL_MIN_MINOR_VERSION  =       0;

RsServiceInfo p3BandwidthControl::getServiceInfo()
{
        return RsServiceInfo(RS_SERVICE_TYPE_BWCTRL,
                BANDWIDTH_CTRL_APP_NAME,
                BANDWIDTH_CTRL_APP_MAJOR_VERSION,
                BANDWIDTH_CTRL_APP_MINOR_VERSION,
                BANDWIDTH_CTRL_MIN_MAJOR_VERSION,
                BANDWIDTH_CTRL_MIN_MINOR_VERSION);
}



int	p3BandwidthControl::tick()
{
	processIncoming();

	bool doCheck = false;
	{
		RsStackMutex stack(mBwMtx); /****** LOCKED MUTEX *******/

#define CHECK_PERIOD 5

		rstime_t now = time(NULL);
		if (now - mLastCheck > CHECK_PERIOD)
		{
			doCheck = true;
			mLastCheck = now;
		}
	}

	if (doCheck)
	{
		checkAvailableBandwidth();
	}

	return 0;
}

int	p3BandwidthControl::status()
{
	return 1;
}


/***** Implementation ******/


bool p3BandwidthControl::checkAvailableBandwidth()
{
	/* check each connection status */
	std::map<RsPeerId, RsBwRates> rateMap;
	RsBwRates total;

	mPg->ExtractRates(rateMap, total);
	std::map<RsPeerId, RsBwRates>::iterator it;
	std::map<RsPeerId, BwCtrlData>::iterator bit;

	/* have to merge with existing list,
	 * erasing as we go ... then any left have to deal with
	 */
	RsStackMutex stack(mBwMtx); /****** LOCKED MUTEX *******/

	mTotalRates = total;

	rstime_t now = time(NULL);
	std::list<RsPeerId> oldIds; // unused for now!

	for(bit = mBwMap.begin(); bit != mBwMap.end(); ++bit)
	{
		/* check alloc rate */
		//rstime_t age = now - bit->second.mLastSend;

		/* find a matching entry */
		it = rateMap.find(bit->first);
		if (it == rateMap.end())
		{
			oldIds.push_back(bit->first);
			continue;
		}

		//float delta = bit->second.mAllocated - it->second.mMaxRateIn;
		/* if delta < 0 ... then need update (or else we get a queue) */
		/* if delta > 0 ... then need update (to allow more data) */

		/* for the moment - always send an update */
		bool updatePeer = true;

#if 0
		/* if changed significantly */
		if (sig)
		{
			updatePeer = true;
		}

		/* if changed small but old */
		if ((any change) && (timeperiod))
		{
			updatePeer = true;
		}
#endif

		/* update rates info */
		bit->second.mRates = it->second;
		bit->second.mRateUpdateTs = now;

		if (updatePeer)
		{
#define ALLOC_FACTOR (1.0)
			// save value sent,
			bit->second.mAllocated = ALLOC_FACTOR * 1000.0 * it->second.mMaxRateIn;
			bit->second.mLastSend = now;

			RsBwCtrlAllowedItem *item = new RsBwCtrlAllowedItem();
			item->PeerId(bit->first);
			item->allowedBw = bit->second.mAllocated;

			sendItem(item);
		}

		/* now cleanup */
		rateMap.erase(it);
	}

	//printRateInfo_locked(std::cerr);

	/* any left over rateMaps ... are bad! (or not active - more likely) */
	return true;
}



bool p3BandwidthControl::processIncoming()
{
	RsItem *item = NULL;
	rstime_t now = time(NULL);

	while(NULL != (item = recvItem()))
	{
		RsBwCtrlAllowedItem *bci = dynamic_cast<RsBwCtrlAllowedItem *>(item);
		if (!bci)
		{
			delete item;
			continue;
		}

		/* For each packet */
		RsStackMutex stack(mBwMtx); /****** LOCKED MUTEX *******/
		std::map<RsPeerId, BwCtrlData>::iterator bit;

		bit = mBwMap.find(bci->PeerId());
		if (bit == mBwMap.end())
		{
			// ERROR.
			delete item;
			continue;
		}

		/* update allowed bandwidth */
		bit->second.mAllowedOut = bci->allowedBw;
		bit->second.mLastRecvd = now;
		delete item;

		/* store info in data */
		//mPg->setAllowedRate(bit->first, bit->second.mAllowedOut / 1000.0);
	}
	return true;
}

int  p3BandwidthControl::getTotalBandwidthRates(RsConfigDataRates &rates)
{
	RsStackMutex stack(mBwMtx); /****** LOCKED MUTEX *******/

	rates.mRateIn = mTotalRates.mRateIn;
	rates.mRateMaxIn = mTotalRates.mMaxRateIn;
	rates.mRateOut = mTotalRates.mRateOut;
	rates.mRateMaxOut = mTotalRates.mMaxRateOut;

	rates.mAllocIn = 0;
	rates.mAllocTs = 0;

	rates.mAllowedOut = 0;
	rates.mAllowedTs = 0;

	rates.mQueueIn = mTotalRates.mQueueIn;
	rates.mQueueOut = mTotalRates.mQueueOut;

	return 1;
}

int p3BandwidthControl::getAllBandwidthRates(std::map<RsPeerId, RsConfigDataRates> &ratemap)
{
	RsStackMutex stack(mBwMtx); /****** LOCKED MUTEX *******/

	std::map<RsPeerId, BwCtrlData>::iterator bit;
	for(bit = mBwMap.begin(); bit != mBwMap.end(); ++bit)
	{
		RsConfigDataRates rates;

		rates.mRateIn = bit->second.mRates.mRateIn;
		rates.mRateMaxIn = bit->second.mRates.mMaxRateIn;
		rates.mRateOut = bit->second.mRates.mRateOut;
		rates.mRateMaxOut = bit->second.mRates.mMaxRateOut;

		rates.mAllocIn = bit->second.mAllocated / 1000.0;
		rates.mAllocTs = bit->second.mLastSend;

		rates.mAllowedOut = bit->second.mAllowedOut / 1000.0;
		rates.mAllowedTs = bit->second.mLastRecvd;

        	rates.mQueueIn = bit->second.mRates.mQueueIn;
        	rates.mQueueOut = bit->second.mRates.mQueueOut;

		ratemap[bit->first] = rates;
	}			
	return true ;


}

int     p3BandwidthControl::ExtractTrafficInfo(std::list<RSTrafficClue>& out_stats, std::list<RSTrafficClue>& in_stats)
{
    return mPg->ExtractTrafficInfo(out_stats,in_stats) ;
}






int p3BandwidthControl::printRateInfo_locked(std::ostream &out)
{
	out << "p3BandwidthControl::printRateInfo_locked()";
	out << std::endl;
	
	//rstime_t now = time(NULL);
	out << "Totals: ";
	out << " In: " << mTotalRates.mRateIn;
	out << " MaxIn: " << mTotalRates.mMaxRateIn;
	out << " Out: " << mTotalRates.mRateOut;
	out << " MaxOut: " << mTotalRates.mMaxRateOut;
	out << std::endl;

	std::map<RsPeerId, BwCtrlData>::iterator bit;
	for(bit = mBwMap.begin(); bit != mBwMap.end(); ++bit)
	{
		out << "\t" << bit->first;
		out << " In: " << bit->second.mRates.mRateIn;
		out << " MaxIn: " << bit->second.mRates.mMaxRateIn;
		out << " Out: " << bit->second.mRates.mRateOut;
		out << " MaxOut: " << bit->second.mRates.mMaxRateOut;
		out << std::endl;
	}			
	return true ;
}

	/*************** pqiMonitor callback ***********************/
void p3BandwidthControl::statusChange(const std::list<pqiServicePeer> &plist)
{
	std::list<pqiServicePeer>::const_iterator it;
	for (it = plist.begin(); it != plist.end(); ++it)
	{
		RsStackMutex stack(mBwMtx); /****** LOCKED MUTEX *******/

		if (it->actions & RS_SERVICE_PEER_DISCONNECTED)
		{
			/* remove from map */
			std::map<RsPeerId, BwCtrlData>::iterator bit;
			bit = mBwMap.find(it->id);
			if (bit == mBwMap.end())
			{
				std::cerr << "p3BandwidthControl::statusChange() ERROR";
				std::cerr << " Entry not in map";
				std::cerr << std::endl;
			}
			else
			{
				mBwMap.erase(bit);
			}
		}
		else if (it->actions & RS_SERVICE_PEER_CONNECTED) 
		{
			/* stuff */
			BwCtrlData data;
			mBwMap[it->id] = data;
		}
	}
	return;
}


