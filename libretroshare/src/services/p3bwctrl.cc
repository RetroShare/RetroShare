/*
 * libretroshare/src/services p3bwctrl.cc
 *
 * Bandwidth Control Service  for RetroShare.
 *
 * Copyright 2011-2011 by Robert Fernie.
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

#include "pqi/p3linkmgr.h"
#include "pqi/p3netmgr.h"

#include "util/rsnet.h"

#include "services/p3bwctrl.h"
#include "serialiser/rsbwctrlitems.h"

#include <sys/time.h>

/****
 * #define DEBUG_BWCTRL		1
 ****/


/************ IMPLEMENTATION NOTES *********************************
 * 
 */

p3BandwidthControl *rsBandwidthControl;


p3BandwidthControl::p3BandwidthControl(pqipersongrp *pg)
	:p3Service(RS_SERVICE_TYPE_BWCTRL), mPg(pg), mBwMtx("p3BwCtrl")
{
	addSerialType(new RsBwCtrlSerialiser());

	mLastCheck = 0;
}


int	p3BandwidthControl::tick()
{
	processIncoming();

	bool doCheck = false;
	{
		RsStackMutex stack(mBwMtx); /****** LOCKED MUTEX *******/

#define CHECK_PERIOD 5

		time_t now = time(NULL);
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
	std::map<std::string, RsBwRates> rateMap;
	RsBwRates total;

	mPg->ExtractRates(rateMap, total);
	std::map<std::string, RsBwRates>::iterator it;
	std::map<std::string, BwCtrlData>::iterator bit;

	/* have to merge with existing list,
	 * erasing as we go ... then any left have to deal with
	 */
	RsStackMutex stack(mBwMtx); /****** LOCKED MUTEX *******/

	mTotalRates = total;

	time_t now = time(NULL);
	std::list<std::string> oldIds; // unused for now!

	for(bit = mBwMap.begin(); bit != mBwMap.end(); bit++)
	{
		/* check alloc rate */
		//time_t age = now - bit->second.mLastSend;

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
#define ALLOC_FACTOR (0.9)
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

	printRateInfo_locked(std::cerr);

	/* any left over rateMaps ... are bad! (or not active - more likely) */
	return true;
}



bool p3BandwidthControl::processIncoming()
{
	RsItem *item = NULL;
	time_t now = time(NULL);

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
		std::map<std::string, BwCtrlData>::iterator bit;

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

	return 1;
}

int p3BandwidthControl::getAllBandwidthRates(std::map<std::string, RsConfigDataRates> &ratemap)
{
	RsStackMutex stack(mBwMtx); /****** LOCKED MUTEX *******/

	std::map<std::string, BwCtrlData>::iterator bit;
	for(bit = mBwMap.begin(); bit != mBwMap.end(); bit++)
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

		ratemap[bit->first] = rates;
	}			
	return true ;


}






int p3BandwidthControl::printRateInfo_locked(std::ostream &out)
{
	out << "p3BandwidthControl::printRateInfo_locked()";
	out << std::endl;
	
	//time_t now = time(NULL);
	out << "Totals: ";
	out << " In: " << mTotalRates.mRateIn;
	out << " MaxIn: " << mTotalRates.mMaxRateIn;
	out << " Out: " << mTotalRates.mRateOut;
	out << " MaxOut: " << mTotalRates.mMaxRateOut;
	out << std::endl;

	std::map<std::string, BwCtrlData>::iterator bit;
	for(bit = mBwMap.begin(); bit != mBwMap.end(); bit++)
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
void p3BandwidthControl::statusChange(const std::list<pqipeer> &plist)
{
	std::list<pqipeer>::const_iterator it;
	for (it = plist.begin(); it != plist.end(); it++) 
	{
		if (it->state & RS_PEER_S_FRIEND) 
		{
			if (it->actions & RS_PEER_DISCONNECTED)
			{
				/* remove from map */
				RsStackMutex stack(mBwMtx); /****** LOCKED MUTEX *******/
				std::map<std::string, BwCtrlData>::iterator bit;
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
			else if (it->actions & RS_PEER_CONNECTED) 
			{
				/* stuff */
				BwCtrlData data;
				mBwMap[it->id] = data;
			}
		}
	}
	return;
}


