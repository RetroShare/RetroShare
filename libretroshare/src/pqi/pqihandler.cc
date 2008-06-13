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

#include <sstream>
#include "pqi/pqidebug.h"
const int pqihandlerzone = 34283;

/****
#define DEBUG_TICK 1
#define RSITEM_DEBUG 1
****/

pqihandler::pqihandler(SecurityPolicy *Global)
{
	// The global security....
	// if something is disabled here...
	// cannot be enabled by module.
	globsec = Global;

	{
		std::ostringstream out;
		out  << "New pqihandler()" << std::endl;
		out  << "Security Policy: " << secpolicy_print(globsec);
		out  << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, out.str());
	}

	// setup minimal total+individual rates.
	rateIndiv_out = 0.01;
	rateIndiv_in = 0.01;
	rateMax_out = 0.01;
	rateMax_in = 0.01;
	return;
}

int	pqihandler::tick()
{
	// tick all interfaces...
	std::map<std::string, SearchModule *>::iterator it;
	int moreToTick = 0;
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
	if (0 < GetItems())
	{
#ifdef DEBUG_TICK
               	std::cerr << "pqihandler::tick() moreToTick from GetItems()" << std::endl;
#endif
		moreToTick = 1;
	}

	UpdateRates();
	return moreToTick;
}


int	pqihandler::status()
{
	std::map<std::string, SearchModule *>::iterator it;

	{ // for output
		std::ostringstream out;
		out  << "pqihandler::status() Active Modules:" << std::endl;

	// display all interfaces...
	for(it = mods.begin(); it != mods.end(); it++)
	{
		out << "\tModule [" << it -> first << "] Pointer <";
		out << (void *) ((it -> second) -> pqi) << ">" << std::endl;
	}

		pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, out.str());

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
	// if peerid used -> error.
	std::map<std::string, SearchModule *>::iterator it;
	if (mod->peerid != mod->pqi->PeerId())
	{
		// ERROR!
		std::ostringstream out;
		out << "ERROR peerid != PeerId!" << std::endl;
		pqioutput(PQL_ALERT, pqihandlerzone, out.str());
		return false;
	}

	if (mod->peerid == "")
	{
		// ERROR!
		std::ostringstream out;
		out << "ERROR peerid == NULL" << std::endl;
		pqioutput(PQL_ALERT, pqihandlerzone, out.str());
		return false;
	}

	if (mods.find(mod->peerid) != mods.end())
	{
		// ERROR!
		std::ostringstream out;
		out << "ERROR PeerId Module already exists!" << std::endl;
		pqioutput(PQL_ALERT, pqihandlerzone, out.str());
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
int	pqihandler::checkOutgoingRsItem(RsItem *item, int global)
{
	pqioutput(PQL_WARNING, pqihandlerzone, 
	  "pqihandler::checkOutgoingPQItem() NULL fn");
	return 1;
}



// generalised output
int	pqihandler::HandleRsItem(RsItem *item, int allowglobal)
{
	std::map<std::string, SearchModule *>::iterator it;
	pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, 
	  "pqihandler::HandleRsItem()");

	/* simplified to no global! */
	if (allowglobal)
	{
		/* error */
		std::ostringstream out;
		out << "pqihandler::HandleSearchItem()";
		out << " Cannot send out Global RsItem";
		pqioutput(PQL_ALERT, pqihandlerzone, out.str());
		delete item;
		return -1;
	}

	if (!checkOutgoingRsItem(item, allowglobal))
	{
		std::ostringstream out;
	  	out <<	"pqihandler::HandleRsItem() checkOutgoingPQItem";
		out << " Failed on item: " << std::endl;
		item -> print(out);

		pqioutput(PQL_ALERT, pqihandlerzone, out.str());
		delete item;
		return -1;
	}

	pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, 
	  "pqihandler::HandleRsItem() Sending to One Channel");


	// find module.
	if ((it = mods.find(item->PeerId())) == mods.end())
	{
		std::ostringstream out;
		out << "pqihandler::HandleRsItem() Invalid chan!";
		pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, out.str());

		delete item;
		return -1;
	}

	// check security... is output allowed.
	if(0 < secpolicy_check((it -> second) -> sp, 0, PQI_OUTGOING))
	{
		std::ostringstream out;
		out << "pqihandler::HandleRsItem() sending to chan:";
		out << it -> first << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, out.str());

		// if yes send on item.
		((it -> second) -> pqi) -> SendItem(item);
		return 1;
	}
	else
	{
		std::ostringstream out;
		out << "pqihandler::HandleRsItem()";
		out << " Sec not approved";
		pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, out.str());

		delete item;
		return -1;
	}

	// if successfully sent to at least one.
	return 1;
}

int	pqihandler::SearchSpecific(RsCacheRequest *ns) 
{
	return HandleRsItem(ns, 0);
}

int	pqihandler::SendSearchResult(RsCacheItem *ns)
{
	return HandleRsItem(ns, 0);
}

int     pqihandler::SendFileRequest(RsFileRequest *ns)
{
	return HandleRsItem(ns, 0);
}

int     pqihandler::SendFileData(RsFileData *ns)
{
	return HandleRsItem(ns, 0);
}

int     pqihandler::SendRsRawItem(RsRawItem *ns)
{
	pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, 
	  	"pqihandler::SendRsRawItem()");
	return HandleRsItem(ns, 0);
}


// inputs. This is a very basic
// system that is completely biased and slow...
// someone please fix.

int pqihandler::GetItems()
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
				std::ostringstream out;
				out << "pqihandler::GetItems() Incoming Item ";
				out << " from: " << mod -> pqi << std::endl;
				item -> print(out);

				pqioutput(PQL_DEBUG_BASIC, 
						pqihandlerzone, out.str());
#endif

				if (item->PeerId() != (mod->pqi)->PeerId())
				{
					/* ERROR */
					pqioutput(PQL_ALERT, 
						pqihandlerzone, "ERROR PeerIds dont match!");
					item->PeerId(mod->pqi->PeerId());
				}

				SortnStoreItem(item);
				count++;
			}
		}
		else
		{
			// not allowed to recieve from here....
			while((item = (mod -> pqi) -> GetItem()) != NULL)
			{
				std::ostringstream out;
				out << "pqihandler::GetItems() Incoming Item ";
				out << " from: " << mod -> pqi << std::endl;
				item -> print(out);
				out << std::endl;
				out << "Item Not Allowed (Sec Pol). deleting!";
				out << std::endl;

				pqioutput(PQL_DEBUG_BASIC, 
						pqihandlerzone, out.str());

				delete item;
			}
		}
	}
	return count;
}




void pqihandler::SortnStoreItem(RsItem *item)
{
	/* get class type / subtype out of the item */
	uint8_t vers    = item -> PacketVersion();
	uint8_t cls     = item -> PacketClass();
	uint8_t type    = item -> PacketType();
	uint8_t subtype = item -> PacketSubType();

	/* whole Version reserved for SERVICES/CACHES */
	if (vers == RS_PKT_VERSION_SERVICE)
	{
	    pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, 
	      "SortnStore -> Service");
	    in_service.push_back(item);
	    item = NULL;
	    return;
	}

	if (vers != RS_PKT_VERSION1)
	{
		pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, 
			"SortnStore -> Invalid VERSION! Deleting!");
		delete item;
		item = NULL;
		return;
	}

	switch(cls)
	{
	  case RS_PKT_CLASS_BASE:
	    switch(type)
	    {
	      case RS_PKT_TYPE_CACHE:
	        switch(subtype)
	        {
	          case RS_PKT_SUBTYPE_CACHE_REQUEST:
	            pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, 
	              "SortnStore -> Cache Request");
	            in_search.push_back(item);
		    item = NULL;
		    break;

	          case RS_PKT_SUBTYPE_CACHE_ITEM:
	            pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, 
	              "SortnStore -> Cache Result");
	            in_result.push_back(item);
		    item = NULL;
		    break;

		  default:
		    break; /* no match! */
		}
	        break;

	      case RS_PKT_TYPE_FILE:
	        switch(subtype)
	        {
	          case RS_PKT_SUBTYPE_FI_REQUEST:
	            pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, 
	              "SortnStore -> File Request");
	            in_request.push_back(item);
		    item = NULL;
		    break;

	          case RS_PKT_SUBTYPE_FI_DATA:
	            pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, 
	              "SortnStore -> File Data");
	            in_data.push_back(item);
		    item = NULL;
		    break;

		  default:
		    break; /* no match! */
		}
	        break;

	      default:
	        break;  /* no match! */
	    }
	    break;
	  
	  default:
	    pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, 
	      "SortnStore -> Unknown");
	    break;

	}
	 
	if (item)
	{
	    pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, 
	      "SortnStore -> Deleting Unsorted Item");
	    delete item;
	}

	return;
}


// much like the input stuff.
RsCacheItem *pqihandler::GetSearchResult()
{
	if (in_result.size() != 0)
	{
		RsCacheItem *fi = dynamic_cast<RsCacheItem *>(in_result.front());
		if (!fi) { delete in_result.front(); }
		in_result.pop_front();
		return fi;
	}
	return NULL;
}

RsCacheRequest *pqihandler::RequestedSearch()
{
	if (in_search.size() != 0)
	{
		RsCacheRequest *fi = dynamic_cast<RsCacheRequest *>(in_search.front());
		if (!fi) { delete in_search.front(); }
		in_search.pop_front();
		return fi;
	}
	return NULL;
}

RsFileRequest *pqihandler::GetFileRequest()
{
	if (in_request.size() != 0)
	{
		RsFileRequest *fi = dynamic_cast<RsFileRequest *>(in_request.front());
		if (!fi) { delete in_request.front(); }
		in_request.pop_front();
		return fi;
	}
	return NULL;
}

RsFileData *pqihandler::GetFileData()
{
	if (in_data.size() != 0)
	{
		RsFileData *fi = dynamic_cast<RsFileData *>(in_data.front());
		if (!fi) { delete in_data.front(); }
		in_data.pop_front();
		return fi;
	}
	return NULL;
}

RsRawItem *pqihandler::GetRsRawItem()
{
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

// internal fn to send updates 
int     pqihandler::UpdateRates()
{
	std::map<std::string, SearchModule *>::iterator it;
	int num_sm = mods.size();

	float avail_in = getMaxRate(true);
	float avail_out = getMaxRate(false);

	float avg_rate_in = avail_in/num_sm;
	float avg_rate_out = avail_out/num_sm;

	float indiv_in = getMaxIndivRate(true);
	float indiv_out = getMaxIndivRate(false);

	float used_bw_in = 0;
	float used_bw_out = 0;

	float extra_bw_in = 0;
	float extra_bw_out = 0;

	int maxxed_in = 0;
	int maxxed_out = 0;

	// loop through modules....
	for(it = mods.begin(); it != mods.end(); it++)
	{
		SearchModule *mod = (it -> second);
		float crate_in = mod -> pqi -> getRate(true);
		float crate_out = mod -> pqi -> getRate(false);

		used_bw_in += crate_in;
		used_bw_out += crate_out;

		if (crate_in > avg_rate_in)
		{
			if (mod -> pqi -> getMaxRate(true) == indiv_in)
			{
				maxxed_in++;
			}
			extra_bw_in +=  crate_in - avg_rate_in;
		}
		if (crate_out > avg_rate_out)
		{
			if (mod -> pqi -> getMaxRate(false) == indiv_out)
			{
				maxxed_out++;
			}
			extra_bw_out +=  crate_out - avg_rate_out;
		}
		//std::cerr << "\tSM(" << mod -> smi << ")";
		//std::cerr << "In A: " << mod -> pqi -> getMaxRate(true);
		//std::cerr << " C: " << crate_in;
		//std::cerr << " && Out A: " << mod -> pqi -> getMaxRate(false);
		//std::cerr << " C: " << crate_out << std::endl;
	}
	//std::cerr << "Totals (In) Used B/W " << used_bw_in;
	//std::cerr << " Excess B/W " << extra_bw_in;
	//std::cerr << " Available B/W " << avail_in << std::endl;
	//std::cerr << "Totals (Out) Used B/W " << used_bw_out;
	//std::cerr << " Excess B/W " << extra_bw_out;
	//std::cerr << " Available B/W " << avail_out << std::endl;

	StoreCurrentRates(used_bw_in, used_bw_out);

	if (used_bw_in > avail_in)
	{
		//std::cerr << "Decreasing Incoming B/W!" << std::endl;

		// drop all above the avg down!
		float fchg = (used_bw_in - avail_in) / (float) extra_bw_in;
		for(it = mods.begin(); it != mods.end(); it++)
		{
			SearchModule *mod = (it -> second);
			float crate_in = mod -> pqi -> getRate(true);
			float new_max = avg_rate_in;
			if (crate_in > avg_rate_in)
			{
				new_max = avg_rate_in + (1 - fchg) * 
						(crate_in - avg_rate_in);
			}
			if (new_max > indiv_in)
			{
				new_max = indiv_in;
			}
			mod -> pqi -> setMaxRate(true, new_max);
		}
	}
	// if not maxxed already and using less than 95%
	else if ((maxxed_in != num_sm) && (used_bw_in < 0.95 * avail_in))
	{
		//std::cerr << "Increasing Incoming B/W!" << std::endl;

		// increase.
		float fchg = (avail_in - used_bw_in) / avail_in;
		for(it = mods.begin(); it != mods.end(); it++)
		{
			SearchModule *mod = (it -> second);
			float crate_in = mod -> pqi -> getRate(true);
			float max_in = mod -> pqi -> getMaxRate(true);

			if (max_in == indiv_in)
			{
				// do nothing...
			}
			else
			{
				float new_max = max_in;
				if (max_in < avg_rate_in)
				{
					new_max = avg_rate_in * (1 + fchg);
				}
				else if (crate_in > 0.5 * max_in)
				{
					new_max =  max_in * (1 + fchg);
				}
				if (new_max > indiv_in)
				{
					new_max = indiv_in;
				}
				mod -> pqi -> setMaxRate(true, new_max);
			}
		}

	}


	if (used_bw_out > avail_out)
	{
		//std::cerr << "Decreasing Outgoing B/W!" << std::endl;
		// drop all above the avg down!
		float fchg = (used_bw_out - avail_out) / (float) extra_bw_out;
		for(it = mods.begin(); it != mods.end(); it++)
		{
			SearchModule *mod = (it -> second);
			float crate_out = mod -> pqi -> getRate(false);
			float new_max = avg_rate_out;
			if (crate_out > avg_rate_out)
			{
				new_max = avg_rate_out + (1 - fchg) * 
						(crate_out - avg_rate_out);
			}
			if (new_max > indiv_out)
			{
				new_max = indiv_out;
			}
			mod -> pqi -> setMaxRate(false, new_max);
		}
	}
	// if not maxxed already and using less than 95%
	else if ((maxxed_out != num_sm) && (used_bw_out < 0.95 * avail_out))
	{
		//std::cerr << "Increasing Outgoing B/W!" << std::endl;
		// increase.
		float fchg = (avail_out - used_bw_out) / avail_out;
		for(it = mods.begin(); it != mods.end(); it++)
		{
			SearchModule *mod = (it -> second);
			float crate_out = mod -> pqi -> getRate(false);
			float max_out = mod -> pqi -> getMaxRate(false);

			if (max_out == indiv_out)
			{
				// do nothing...
			}
			else
			{
				float new_max = max_out;
				if (max_out < avg_rate_out)
				{
					new_max = avg_rate_out * (1 + fchg);
				}
				else if (crate_out > 0.5 * max_out)
				{
					new_max =  max_out * (1 + fchg);
				}
				if (new_max > indiv_out)
				{
					new_max = indiv_out;
				}
				mod -> pqi -> setMaxRate(false, new_max);
			}
		}

	}

	return 1;
}

void    pqihandler::getCurrentRates(float &in, float &out)
{
	in = rateTotal_in;
	out = rateTotal_out;
}

void    pqihandler::StoreCurrentRates(float in, float out)
{
	rateTotal_in = in;
	rateTotal_out = out;
}


