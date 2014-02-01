/*
 * "$Id: pqihandler.cc,v 1.12 2007-03-31 09:41:32 rmf24 Exp $"
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
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

#include "pqi/pqihandler.h"

#include "util/rsdebug.h"
#include "util/rsstring.h"
#include <stdlib.h>
#include <time.h>
const int pqihandlerzone = 34283;

static const int PQI_HANDLER_NB_PRIORITY_LEVELS = 10 ;
static const float PQI_HANDLER_NB_PRIORITY_RATIO = 2 ;

/****
#define DEBUG_TICK 1
#define RSITEM_DEBUG 1
****/

pqihandler::pqihandler(SecurityPolicy *Global) : coreMtx("pqihandler")
{
	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

	// The global security....
	// if something is disabled here...
	// cannot be enabled by module.
	globsec = Global;

	pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, "New pqihandler()\nSecurity Policy: " + secpolicy_print(globsec));

	// setup minimal total+individual rates.
	rateIndiv_out = 0.01;
	rateIndiv_in = 0.01;
	rateMax_out = 0.01;
	rateMax_in = 0.01;
	last_m = time(NULL) ;
	nb_ticks = 0 ;
	ticks_per_sec = 5 ; // initial guess
	return;
}

int	pqihandler::tick()
{
	int moreToTick = 0;

	{ 
		RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

		// tick all interfaces...
		std::map<std::string, SearchModule *>::iterator it;
		for(it = mods.begin(); it != mods.end(); it++)
		{
			if (0 < ((it -> second) -> pqi) -> tick())
			{
#ifdef DEBUG_TICK
				std::cerr << "pqihandler::tick() moreToTick from mod()" << std::endl;
#endif
				moreToTick = 1;
			}
		}
		// get the items, and queue them correctly
		if (0 < locked_GetItems())
		{
#ifdef DEBUG_TICK
			std::cerr << "pqihandler::tick() moreToTick from GetItems()" << std::endl;
#endif
			moreToTick = 1;
		}
	}

	UpdateRates();
	return moreToTick;
}


bool pqihandler::queueOutRsItem(RsItem *item)
{
	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

	uint32_t size ;
	locked_HandleRsItem(item, 0, size);

#ifdef DEBUG_QOS
	if(item->priority_level() == QOS_PRIORITY_UNKNOWN)
		std::cerr << "Caught an unprioritized item !" << std::endl;

	print() ;
#endif
	return true ;
}

int	pqihandler::status()
{
	std::map<std::string, SearchModule *>::iterator it;
	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

	{ // for output
		std::string out = "pqihandler::status() Active Modules:\n";

		// display all interfaces...
		for(it = mods.begin(); it != mods.end(); it++)
		{
			rs_sprintf_append(out, "\tModule [%s] Pointer <%p>", it -> first.c_str(), (void *) ((it -> second) -> pqi));
		}

		pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, out);

	} // end of output.


	// status all interfaces...
	for(it = mods.begin(); it != mods.end(); it++)
	{
		((it -> second) -> pqi) -> status();
	}
	return 1;
}

bool	pqihandler::AddSearchModule(SearchModule *mod)
{
	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/
	// if peerid used -> error.
	std::map<std::string, SearchModule *>::iterator it;
	if (mod->peerid != mod->pqi->PeerId())
	{
		// ERROR!
		pqioutput(PQL_ALERT, pqihandlerzone, "ERROR peerid != PeerId!");
		return false;
	}

	if (mod->peerid == "")
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

	// check security.
	if (mod -> sp == NULL)
	{
		// create policy.
		mod -> sp = secpolicy_create();
	}

	// limit to what global security allows.
	secpolicy_limit(globsec, mod -> sp);

	// store.
	mods[mod->peerid] = mod;
	return true;
}

bool	pqihandler::RemoveSearchModule(SearchModule *mod)
{
	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/
	std::map<std::string, SearchModule *>::iterator it;
	for(it = mods.begin(); it != mods.end(); it++)
	{
		if (mod == it -> second)
		{
			mods.erase(it);
			return true;
		}
	}
	return false;
}

// dummy output check
int	pqihandler::locked_checkOutgoingRsItem(RsItem * /*item*/, int /*global*/)
{
	//pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, "pqihandler::checkOutgoingPQItem() NULL fn");

	return 1;
}



// generalised output
int	pqihandler::locked_HandleRsItem(RsItem *item, int allowglobal,uint32_t& computed_size)
{
	computed_size = 0 ;
	std::map<std::string, SearchModule *>::iterator it;
	pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, 
	  "pqihandler::HandleRsItem()");

	/* simplified to no global! */
	if (allowglobal)
	{
		/* error */
		std::string out = "pqihandler::HandleSearchItem() Cannot send out Global RsItem";
		pqioutput(PQL_ALERT, pqihandlerzone, out);
#ifdef DEBUG_TICK
		std::cerr << out << std::endl;
#endif
		delete item;
		return -1;
	}

	if (!locked_checkOutgoingRsItem(item, allowglobal))
	{
		std::string out = "pqihandler::HandleRsItem() checkOutgoingPQItem  Failed on item: \n";
#ifdef DEBUG_TICK
		std::cerr << out;
#endif
		item -> print_string(out);

		pqioutput(PQL_ALERT, pqihandlerzone, out);
		delete item;
		return -1;
	}

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

	// check security... is output allowed.
	if(0 < secpolicy_check((it -> second) -> sp, 0, PQI_OUTGOING))
	{
		std::string out = "pqihandler::HandleRsItem() sending to chan: " + it -> first;
		pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, out);
#ifdef DEBUG_TICK
		std::cerr << out << std::endl;
#endif

		// if yes send on item.
		((it -> second) -> pqi) -> SendItem(item,computed_size);
		return 1;
	}
	else
	{
		std::string out = "pqihandler::HandleRsItem() Sec not approved";
		pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, out);
#ifdef DEBUG_TICK
		std::cerr << out << std::endl;
#endif

		delete item;
		return -1;
	}

	// if successfully sent to at least one.
	return 1;
}

int     pqihandler::SendRsRawItem(RsRawItem *ns)
{
	pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, "pqihandler::SendRsRawItem()");

	// directly send item to streamers
	
	return queueOutRsItem(ns) ;
}

// inputs. This is a very basic
// system that is completely biased and slow...
// someone please fix.

int pqihandler::locked_GetItems()
{
	std::map<std::string, SearchModule *>::iterator it;

	RsItem *item;
	int count = 0;

	// loop through modules....
	for(it = mods.begin(); it != mods.end(); it++)
	{
		SearchModule *mod = (it -> second);

		// check security... is output allowed.
		if(0 < secpolicy_check((it -> second) -> sp, 
					0, PQI_INCOMING)) // PQI_ITEM_TYPE_ITEM, PQI_INCOMING))
		{
			// if yes... attempt to read.
			while((item = (mod -> pqi) -> GetItem()) != NULL)
			{
#ifdef RSITEM_DEBUG 
				std::string out;
				rs_sprintf(out, "pqihandler::GetItems() Incoming Item from: %p\n", mod -> pqi);
				item -> print_string(out);

				pqioutput(PQL_DEBUG_BASIC, 
						pqihandlerzone, out);
#endif

				if (item->PeerId() != (mod->pqi)->PeerId())
				{
					/* ERROR */
					pqioutput(PQL_ALERT, 
						pqihandlerzone, "ERROR PeerIds dont match!");
					item->PeerId(mod->pqi->PeerId());
				}

				locked_SortnStoreItem(item);
				count++;
			}
		}
		else
		{
			// not allowed to recieve from here....
			while((item = (mod -> pqi) -> GetItem()) != NULL)
			{
				std::string out;
				rs_sprintf(out, "pqihandler::GetItems() Incoming Item from: %p\n", mod -> pqi);
				item -> print_string(out);
				out += "\nItem Not Allowed (Sec Pol). deleting!";

				pqioutput(PQL_DEBUG_BASIC,
						pqihandlerzone, out);

				delete item;
			}
		}
	}
	return count;
}

void pqihandler::locked_SortnStoreItem(RsItem *item)
{
	/* get class type / subtype out of the item */
	uint8_t vers    = item -> PacketVersion();

	/* whole Version reserved for SERVICES/CACHES */
	if (vers == RS_PKT_VERSION_SERVICE)
	{
	    pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, 
	      "SortnStore -> Service");
	    in_service.push_back(item);
	    item = NULL;
	    return;
	}
	std::cerr << "pqihandler::locked_SortnStoreItem() : unhandled item! Will be deleted. This is certainly a bug." << std::endl;

	if (vers != RS_PKT_VERSION1)
	{
		pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, "SortnStore -> Invalid VERSION! Deleting!");
		delete item;
		item = NULL;
		return;
	}

	if (item)
	{
	    pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, "SortnStore -> Deleting Unsorted Item");
	    delete item;
	}

	return;
}

RsRawItem *pqihandler::GetRsRawItem()
{
	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

	if (in_service.size() != 0)
	{
		RsRawItem *fi = dynamic_cast<RsRawItem *>(in_service.front());
		if (!fi) { delete in_service.front(); }
		in_service.pop_front();
		return fi;
	}
	return NULL;
}

static const float MIN_RATE = 0.01; // 10 B/s

// NEW extern fn to extract rates.
int     pqihandler::ExtractRates(std::map<std::string, RsBwRates> &ratemap, RsBwRates &total)
{
	total.mMaxRateIn = getMaxRate(true);
	total.mMaxRateOut = getMaxRate(false);
	total.mRateIn = 0;
	total.mRateOut = 0;
	total.mQueueIn = 0;
	total.mQueueOut = 0;

	/* Lock once rates have been retrieved */
	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

	std::map<std::string, SearchModule *>::iterator it;
	for(it = mods.begin(); it != mods.end(); it++)
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
	std::map<std::string, SearchModule *>::iterator it;
	int num_sm = mods.size();

	float avail_in = getMaxRate(true);
	float avail_out = getMaxRate(false);

	float used_bw_in = 0;
	float used_bw_out = 0;

	/* Lock once rates have been retrieved */
	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

	int effectiveUploadsSm = 0;
	int effectiveDownloadsSm = 0;
	// loop through modules to get the used bandwith and the number of modules that are affectively transfering
	//std::cerr << " Looping through modules" << std::endl;
	for(it = mods.begin(); it != mods.end(); it++)
	{
		SearchModule *mod = (it -> second);
		float crate_in = mod -> pqi -> getRate(true);
		if (crate_in > 0.01 * avail_in || crate_in > 0.1)
		{
		    effectiveDownloadsSm ++;
		}

		float crate_out = mod -> pqi -> getRate(false);
		if (crate_out > 0.01 * avail_out || crate_out > 0.1)
		{
		    effectiveUploadsSm ++;
		}

		used_bw_in += crate_in;
		used_bw_out += crate_out;
	}
#ifdef DEBUG_QOS
//	std::cerr << "Totals (In) Used B/W " << used_bw_in;
//	std::cerr << " Available B/W " << avail_in;
//	std::cerr << " Effective transfers " << effectiveDownloadsSm << std::endl;
//	std::cerr << "Totals (Out) Used B/W " << used_bw_out;
//	std::cerr << " Available B/W " << avail_out;
//	std::cerr << " Effective transfers " << effectiveUploadsSm << std::endl;
#endif

	locked_StoreCurrentRates(used_bw_in, used_bw_out);

	//computing average rates for effective transfers
	float max_in_effective = avail_in / num_sm;
	if (effectiveDownloadsSm != 0) {
	    max_in_effective = avail_in / effectiveDownloadsSm;
	}
	float max_out_effective = avail_out / num_sm;
	if (effectiveUploadsSm != 0) {
	    max_out_effective = avail_out / effectiveUploadsSm;
	}

	//modify the outgoing rates if bandwith is not used well
	float rate_out_modifier = 0;
	if (used_bw_out / avail_out < 0.95) {
	    rate_out_modifier = 0.001 * avail_out;
	} else 	if (used_bw_out / avail_out > 1.05) {
	    rate_out_modifier = - 0.001 * avail_out;
	}
	if (rate_out_modifier != 0) {
	    for(it = mods.begin(); it != mods.end(); it++)
	    {
		    SearchModule *mod = (it -> second);
			mod -> pqi -> setMaxRate(false, mod -> pqi -> getMaxRate(false) + rate_out_modifier);
	    }
	}

	//modify the incoming rates if bandwith is not used well
	float rate_in_modifier = 0;
	if (used_bw_in / avail_in < 0.95) {
	    rate_in_modifier = 0.001 * avail_in;
	} else 	if (used_bw_in / avail_in > 1.05) {
	    rate_in_modifier = - 0.001 * avail_in;
	}
	if (rate_in_modifier != 0) {
	    for(it = mods.begin(); it != mods.end(); it++)
	    {
		    SearchModule *mod = (it -> second);
			mod -> pqi -> setMaxRate(true, mod -> pqi -> getMaxRate(true) + rate_in_modifier);
	    }
	}

	//cap the rates
	for(it = mods.begin(); it != mods.end(); it++)
	{
		SearchModule *mod = (it -> second);
		if (mod -> pqi -> getMaxRate(false) < max_out_effective) {
		    mod -> pqi -> setMaxRate(false, max_out_effective);
		}
		if (mod -> pqi -> getMaxRate(false) > avail_out) {
		    mod -> pqi -> setMaxRate(false, avail_out);
		}
		if (mod -> pqi -> getMaxRate(true) < max_in_effective) {
		    mod -> pqi -> setMaxRate(true, max_in_effective);
		}
		if (mod -> pqi -> getMaxRate(true) > avail_in) {
		    mod -> pqi -> setMaxRate(true, avail_in);
		}
	}

	return 1;
}

void    pqihandler::getCurrentRates(float &in, float &out)
{
	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

	in = rateTotal_in;
	out = rateTotal_out;
}

void    pqihandler::locked_StoreCurrentRates(float in, float out)
{
	rateTotal_in = in;
	rateTotal_out = out;
}


