/*******************************************************************************
 * libretroshare/src/pqi: pqihandler.cc                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2004-2006  Robert Fernie                                      *
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
#include "pqi/pqihandler.h"

#include <stdlib.h>               // for NULL
#include "util/rstime.h"                 // for time, rstime_t
#include <algorithm>              // for sort
#include <iostream>               // for dec
#include <string>                 // for string, char_traits, operator+, bas...
#include <utility>                // for pair

#include "pqi/pqi_base.h"         // for PQInterface, RsBwRates
#include "retroshare/rsconfig.h"  // for RSTrafficClue
#include "retroshare/rsids.h"     // for t_RsGenericIdType
#include "retroshare/rspeers.h"   // for RsPeers, rsPeers
#include "serialiser/rsserial.h"  // for RsItem, RsRawItem
#include "util/rsdebug.h"         // for pqioutput, PQL_DEBUG_BASIC, PQL_ALERT
#include "util/rsstring.h"        // for rs_sprintf_append

using std::dec;

#ifdef WINDOWS_SYS
#include <sys/timeb.h>
#endif

struct RsLog::logInfo pqihandlerzoneInfo = {RsLog::Default, "pqihandler"};
#define pqihandlerzone &pqihandlerzoneInfo

//static const int PQI_HANDLER_NB_PRIORITY_LEVELS = 10 ;
//static const float PQI_HANDLER_NB_PRIORITY_RATIO = 2 ;

//#define UPDATE_RATES_DEBUG 1
// #define DEBUG_TICK 1
// #define RSITEM_DEBUG 1

pqihandler::pqihandler() : coreMtx("pqihandler")
{
    RS_STACK_MUTEX(coreMtx); /**************** LOCKED MUTEX ****************/

    // setup minimal total+individual rates.
    rateIndiv_out = 0.01;
    rateIndiv_in = 0.01;
    rateMax_out = 0.01;
    rateMax_in = 0.01;
    rateTotal_in = 0.0 ;
    rateTotal_out = 0.0 ;
	traffInSum = 0;
	traffOutSum = 0;
    last_m = time(NULL) ;
    nb_ticks = 0 ;
    mLastRateCapUpdate = 0 ;
    ticks_per_sec = 5 ; // initial guess
    return;
}

int	pqihandler::tick()
{
	int moreToTick = 0;

	{
		RS_STACK_MUTEX(coreMtx); /**************** LOCKED MUTEX ****************/

		// tick all interfaces...
		std::map<RsPeerId, SearchModule *>::iterator it;
		for(it = mods.begin(); it != mods.end(); ++it)
		{
			if (0 < ((it -> second) -> pqi) -> tick())
			{
#ifdef DEBUG_TICK
				std::cerr << "pqihandler::tick() moreToTick from mod()" << std::endl;
#endif
				moreToTick = 1;
			}
		}
#ifdef TO_BE_REMOVED
		// get the items, and queue them correctly
		if (0 < locked_GetItems())
		{
#ifdef DEBUG_TICK
			std::cerr << "pqihandler::tick() moreToTick from GetItems()" << std::endl;
#endif
			moreToTick = 1;
		}
#endif
	}

	rstime_t now = time(NULL) ;

	if(now > mLastRateCapUpdate + 5)
	{
        std::map<RsPeerId, RsConfigDataRates> rateMap;
        std::map<RsPeerId, RsConfigDataRates>::iterator it;


		// every 5 secs, update the max rates for all modules

		RS_STACK_MUTEX(coreMtx); /**************** LOCKED MUTEX ****************/
		for(std::map<RsPeerId, SearchModule *>::iterator it = mods.begin(); it != mods.end(); ++it)
		{
			// This is rather inelegant, but pqihandler has searchModules that are dynamically allocated, so the max rates
			// need to be updated from inside.
			uint32_t maxUp = 0,maxDn =0 ;
			if (rsPeers->getPeerMaximumRates(it->first,maxUp,maxDn) )
				it->second->pqi->setRateCap(maxDn,maxUp);// mind the order! Dn first, than Up.
		}

		mLastRateCapUpdate = now ;
	}

	UpdateRates();
	return moreToTick;
}


bool pqihandler::queueOutRsItem(RsItem *item)
{
	RS_STACK_MUTEX(coreMtx); /**************** LOCKED MUTEX ****************/

	uint32_t size ;
    locked_HandleRsItem(item, size);

#ifdef DEBUG_QOS
	if(item->priority_level() == QOS_PRIORITY_UNKNOWN)
		std::cerr << "Caught an unprioritized item !" << std::endl;

	print() ;
#endif
	return true ;
}

int	pqihandler::status()
{
	std::map<RsPeerId, SearchModule *>::iterator it;
	RS_STACK_MUTEX(coreMtx); /**************** LOCKED MUTEX ****************/

	{ // for output
		std::string out = "pqihandler::status() Active Modules:\n";

		// display all interfaces...
		for(it = mods.begin(); it != mods.end(); ++it)
		{
			rs_sprintf_append(out, "\tModule [%s] Pointer <%p>", it -> first.toStdString().c_str(), (void *) ((it -> second) -> pqi));
		}

		pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, out);

	} // end of output.


	// status all interfaces...
	for(it = mods.begin(); it != mods.end(); ++it)
	{
		((it -> second) -> pqi) -> status();
	}
	return 1;
}

bool	pqihandler::AddSearchModule(SearchModule *mod)
{
	RS_STACK_MUTEX(coreMtx); /**************** LOCKED MUTEX ****************/
	// if peerid used -> error.
	//std::map<RsPeerId, SearchModule *>::iterator it;
	if (mod->peerid != mod->pqi->PeerId())
	{
		// ERROR!
		pqioutput(PQL_ALERT, pqihandlerzone, "ERROR peerid != PeerId!");
		return false;
	}

	if (mod->peerid.isNull())
	{
		// ERROR!
		pqioutput(PQL_ALERT, pqihandlerzone, "ERROR peerid == NULL");
		return false;
	}

	if (mods.find(mod->peerid) != mods.end())
	{
		// ERROR!
		pqioutput(PQL_ALERT, pqihandlerzone, "ERROR PeerId Module already exists!");
		return false;
	}

	// store.
	mods[mod->peerid] = mod;
	return true;
}

bool	pqihandler::RemoveSearchModule(SearchModule *mod)
{
	RS_STACK_MUTEX(coreMtx); /**************** LOCKED MUTEX ****************/
	std::map<RsPeerId, SearchModule *>::iterator it;
	for(it = mods.begin(); it != mods.end(); ++it)
	{
		if (mod == it -> second)
		{
			mods.erase(it);
			return true;
		}
	}
	return false;
}

// generalised output
int	pqihandler::locked_HandleRsItem(RsItem *item, uint32_t& computed_size)
{
	computed_size = 0 ;
	std::map<RsPeerId, SearchModule *>::iterator it;
	pqioutput(PQL_DEBUG_BASIC, pqihandlerzone,
	  "pqihandler::HandleRsItem()");

	pqioutput(PQL_DEBUG_BASIC, pqihandlerzone,
	  "pqihandler::HandleRsItem() Sending to One Channel");
#ifdef DEBUG_TICK
        std::cerr << "pqihandler::HandleRsItem() Sending to One Channel" << std::endl;
#endif


	// find module.
	if ((it = mods.find(item->PeerId())) == mods.end())
	{
		std::string out = "pqihandler::HandleRsItem() Invalid chan!";
		pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, out);
#ifdef DEBUG_TICK
		std::cerr << out << std::endl;
#endif

		delete item;
		return -1;
	}

		std::string out = "pqihandler::HandleRsItem() sending to chan: " + it -> first.toStdString();
		pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, out);
#ifdef DEBUG_TICK
		std::cerr << out << std::endl;
#endif

		// if yes send on item.
		((it -> second) -> pqi) -> SendItem(item,computed_size);
		return 1;
}

int     pqihandler::SendRsRawItem(RsRawItem *ns)
{
	pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, "pqihandler::SendRsRawItem()");

	// directly send item to streamers

	return queueOutRsItem(ns) ;
}

int     pqihandler::ExtractTrafficInfo(std::list<RSTrafficClue>& out_lst,std::list<RSTrafficClue>& in_lst)
{
    in_lst.clear() ;
    out_lst.clear() ;

    for( std::map<RsPeerId, SearchModule *>::iterator it = mods.begin(); it != mods.end(); ++it)
    {
        std::list<RSTrafficClue> ilst,olst ;

        (it -> second)->pqi->gatherStatistics(olst,ilst) ;

        for(std::list<RSTrafficClue>::const_iterator it(ilst.begin());it!=ilst.end();++it) in_lst.push_back(*it) ;
        for(std::list<RSTrafficClue>::const_iterator it(olst.begin());it!=olst.end();++it) out_lst.push_back(*it) ;
    }

    return 1 ;
}

// NEW extern fn to extract rates.
int     pqihandler::ExtractRates(std::map<RsPeerId, RsBwRates> &ratemap, RsBwRates &total)
{
	total.mMaxRateIn = getMaxRate(true);
	total.mMaxRateOut = getMaxRate(false);
	total.mRateIn = 0;
	total.mRateOut = 0;
	total.mQueueIn = 0;
	total.mQueueOut = 0;

	/* Lock once rates have been retrieved */
	RS_STACK_MUTEX(coreMtx); /**************** LOCKED MUTEX ****************/

	std::map<RsPeerId, SearchModule *>::iterator it;
	for(it = mods.begin(); it != mods.end(); ++it)
	{
		SearchModule *mod = (it -> second);

		RsBwRates peerRates;
		mod -> pqi -> getRates(peerRates);

		total.mRateIn  += peerRates.mRateIn;
		total.mRateOut += peerRates.mRateOut;
		total.mQueueIn  += peerRates.mQueueIn;
		total.mQueueOut += peerRates.mQueueOut;

		ratemap[it->first] = peerRates;

	}

	return 1;
}



// internal fn to send updates
int     pqihandler::UpdateRates()
{
	std::map<RsPeerId, SearchModule *>::iterator it;

	float avail_in = getMaxRate(true);
	float avail_out = getMaxRate(false);

	float used_bw_in = 0;
	float used_bw_out = 0;

	/* Lock once rates have been retrieved */
	RS_STACK_MUTEX(coreMtx); /**************** LOCKED MUTEX ****************/

	int num_sm = mods.size();
	float used_bw_in_table[num_sm];         /* table of in bandwidth currently used by each module */
	float used_bw_out_table[num_sm];        /* table of out bandwidth currently used by each module */

	// loop through modules to get the used bandwidth
#ifdef UPDATE_RATES_DEBUG
		RsDbg() << "UPDATE_RATES pqihandler::UpdateRates Looping through modules" << std::endl;
#endif

	int index = 0;

	for(it = mods.begin(); it != mods.end(); ++it)
	{
		SearchModule *mod = (it -> second);

		traffInSum += mod -> pqi -> getTraffic(true);
		traffOutSum += mod -> pqi -> getTraffic(false);
        
                float crate_in = mod -> pqi -> getRate(true);
		float crate_out = mod -> pqi -> getRate(false);

		used_bw_in += crate_in;
		used_bw_out += crate_out;

		/* fill the table of used bandwidths */
		used_bw_in_table[index] = crate_in;
		used_bw_out_table[index] = crate_out;

		++index;
	}

#ifdef UPDATE_RATES_DEBUG
	RsDbg() << "UPDATE_RATES pqihandler::UpdateRates Sorting used_bw_out_table: " << num_sm << " entries" << std::endl;
#endif

	/* Sort the used bw in/out table in ascending order */
	std::sort(used_bw_in_table, used_bw_in_table + num_sm);
	std::sort(used_bw_out_table, used_bw_out_table + num_sm);

#ifdef UPDATE_RATES_DEBUG
	RsDbg() << "UPDATE_RATES pqihandler::UpdateRates used_bw_out " << used_bw_out << std::endl;
#endif

	/* Calculate the optimal out_max value, taking into account avail_out and the out bw requested by modules */

	float out_remaining_bw = avail_out;
	float out_max_bw = 0;
	bool keep_going = true;
	int mod_index = 0;

	while (keep_going && (mod_index < num_sm)) {
		float result = (num_sm - mod_index) * (used_bw_out_table[mod_index] - out_max_bw);
		if (result > out_remaining_bw) {
			/* There is not enough remaining out bw to satisfy all modules,
			   distribute the remaining out bw among modules, then exit */
			out_max_bw += out_remaining_bw / (num_sm - mod_index);
			out_remaining_bw = 0;
			keep_going = false;
		} else {
			/* Grant the requested out bandwidth to all modules,
			   then recalculate the remaining out bandwidth */
			out_remaining_bw -= result;
			out_max_bw = used_bw_out_table[mod_index];
			++mod_index;
		}
	}

#ifdef UPDATE_RATES_DEBUG
	RsDbg() << "UPDATE_RATES pqihandler::UpdateRates mod_index " << mod_index << " out_max_bw " << out_max_bw << " remaining out bw " << out_remaining_bw << std::endl;
#endif

	/* Allocate only 50 pct the remaining out bw, if any, to make the transition more smooth */
	out_max_bw = out_max_bw + 0.5 * out_remaining_bw;

	/* Calculate the optimal in_max value, taking into account avail_in and the in bw requested by modules */

	float in_remaining_bw = avail_in;
	float in_max_bw = 0;
	keep_going = true;
	mod_index = 0;

	while (keep_going && mod_index < num_sm) {
		float result = (num_sm - mod_index) * (used_bw_in_table[mod_index] - in_max_bw);
		if (result > in_remaining_bw) {
			/* There is not enough remaining in bw to satisfy all modules,
			   distribute the remaining in bw among modules, then exit */
			in_max_bw += in_remaining_bw / (num_sm - mod_index);
			in_remaining_bw = 0;
			keep_going = false;
		} else {
			/* Grant the requested in bandwidth to all modules,
			   then recalculate the remaining in bandwidth */
			in_remaining_bw -= result;
			in_max_bw = used_bw_in_table[mod_index];
			++mod_index;
		}
	}

#ifdef UPDATE_RATES_DEBUG
	RsDbg() << "UPDATE_RATES pqihandler::UpdateRates mod_index " << mod_index << " in_max_bw " << in_max_bw << " remaining in bw " << in_remaining_bw << std::endl;
#endif

	// allocate only 75 pct of the remaining in bw, to make the transition more smooth
	in_max_bw = in_max_bw + 0.75 * in_remaining_bw;

	// store current total in and out used bw 
	locked_StoreCurrentRates(used_bw_in, used_bw_out);

#ifdef UPDATE_RATES_DEBUG
	RsDbg() << "UPDATE_RATES pqihandler::UpdateRates setting new out_max " << out_max_bw << " in_max " << in_max_bw << std::endl;
#endif

	// retrieve the bandwidth limits provided by peers via BwCtrl
        std::map<RsPeerId, RsConfigDataRates> rateMap;
        rsConfig->getAllBandwidthRates(rateMap);
        std::map<RsPeerId, RsConfigDataRates>::iterator rateMap_it;

#ifdef UPDATE_RATES_DEBUG
	// dump RsConfigurationDataRates
	RsDbg() << "UPDATE_RATES pqihandler::UpdateRates RsConfigDataRates dump" << std::endl;
	for (rateMap_it = rateMap.begin(); rateMap_it != rateMap.end(); rateMap_it++)
		RsDbg () << "UPDATE_RATES pqihandler::UpdateRates PeerId " << rateMap_it->first.toStdString() << " mAllowedOut " << rateMap_it->second.mAllowedOut << std::endl;
#endif

        // update max rates
	for(it = mods.begin(); it != mods.end(); ++it)
	{
		SearchModule *mod = (it -> second);

		// for our down bandwidth we use the calculated value without taking into account the max up provided by peers via BwCtrl
		// this is harmless as they will control their up bw on their side
		mod -> pqi -> setMaxRate(true, in_max_bw);

		// for our up bandwidth we take into account the max down provided by peers via BwCtrl
		// because we don't want to clog our outqueues, the TCP buffers, and the peers inbound queues
		if ((rateMap_it = rateMap.find(mod->pqi->PeerId())) != rateMap.end())
		{
			if (rateMap_it->second.mAllowedOut > 0)
			{	
				if (out_max_bw > rateMap_it->second.mAllowedOut)
        	                        mod -> pqi -> setMaxRate(false, rateMap_it->second.mAllowedOut);
				else
					mod -> pqi -> setMaxRate(false, out_max_bw);
			}
			else
				mod -> pqi -> setMaxRate(false, out_max_bw);
		}
	}

#ifdef UPDATE_RATES_DEBUG
	// dump maxRates
	for(it = mods.begin(); it != mods.end(); ++it)
	{
		SearchModule *mod = (it -> second);
		RsDbg() << "UPDATE_RATES pqihandler::UpdateRates PeerID " << (mod ->pqi -> PeerId()).toStdString() << " new bandwidth limits up " << mod -> pqi -> getMaxRate(false) << " down " << mod -> pqi -> getMaxRate(true) << std::endl;
	}
#endif

	return 1;
}

void    pqihandler::getCurrentRates(float &in, float &out)
{
	RS_STACK_MUTEX(coreMtx); /**************** LOCKED MUTEX ****************/

	in = rateTotal_in;
	out = rateTotal_out;
}

void    pqihandler::locked_StoreCurrentRates(float in, float out)
{
	rateTotal_in = in;
	rateTotal_out = out;
}

void pqihandler::GetTraffic(uint64_t &in, uint64_t &out)
{
	in = traffInSum;
	out = traffOutSum;
}
