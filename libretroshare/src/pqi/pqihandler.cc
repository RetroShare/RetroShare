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
	std::map<int, SearchModule *>::iterator it;
	int moreToTick = 0;
	for(it = mods.begin(); it != mods.end(); it++)
	{
		if (0 < ((it -> second) -> pqi) -> tick())
		{
			moreToTick = 1;
		}
	}
	// get the items, and queue them correctly
	if (0 < GetItems())
	{
		moreToTick = 1;
	}

	UpdateRates();
	return moreToTick;
}


int	pqihandler::status()
{
	std::map<int, SearchModule *>::iterator it;

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
	


int	pqihandler::AddSearchModule(SearchModule *mod, int chanid)
{
	// if chan id set, then use....
	// otherwise find empty channel.

	int realchanid = -1;
	std::map<int, SearchModule *>::iterator it;


	if ((chanid > 0) && (mods.find(chanid) == mods.end()))
	{
		// okay id.
		realchanid = chanid;
		
	}
	else
	{
		// find empty chan.
		for(int i = 1; (i < 1000) && (realchanid == -1); i++)
		{
			if (mods.find(i) == mods.end())
			{
				realchanid = i;
			}
		}
		if (realchanid > 0)
		{
			std::ostringstream out;
			out << "Allocated Chan Id: " << realchanid << std::endl;
			pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, out.str());
		}
		else
		{
			std::ostringstream out;
			out << "Unable to Allocate Channel!" << std::endl;
			pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, out.str());

			return -1;
		}
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
	mods[realchanid] = mod;
	mod -> smi = realchanid;

	// experimental ... pushcid.
	Person *p = (mod -> pqi) -> getContact();
	if (p != NULL)
	{
		p -> cidpush(realchanid);
	}

	return realchanid;
}

int	pqihandler::RemoveSearchModule(SearchModule *mod)
{
	std::map<int, SearchModule *>::iterator it;
	for(it = mods.begin(); it != mods.end(); it++)
	{
		if (mod == it -> second)
		{
			mods.erase(it);
			return 1;
		}
	}
	return -1;
}

// dummy output check
int	pqihandler::checkOutgoingPQItem(PQItem *item, int global)
{
	pqioutput(PQL_WARNING, pqihandlerzone, 
	  "pqihandler::checkOutgoingPQItem() NULL fn");
	return 1;
}



// generalised output
int	pqihandler::HandlePQItem(PQItem *item, int allowglobal)
{
	std::map<int, SearchModule *>::iterator it;
	pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, 
	  "pqihandler::HandlePQItem()");

	
	if ((!allowglobal) && (!checkOutgoingPQItem(item, allowglobal)))
	{
		std::ostringstream out;
	  	out <<	"pqihandler::HandlePQItem() checkOutgoingPQItem";
		out << " Failed on item: " << std::endl;
		item -> print(out);

		pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, out.str());
		delete item;
		return -1;
	}

	// need to get channel id. (and remove from stack)
	int chan = item -> cidpop();
	if (chan > 0)
	{
		pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, 
		  "pqihandler::HandlePQItem() Sending to One Channel");


		// find module.
		if ((it = mods.find(chan)) == mods.end())
		{
			std::ostringstream out;
			out << "pqihandler::HandlePQItem() Invalid chan!";
			pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, out.str());

			delete item;
			return -1;
		}

		// check security... is output allowed.
		if(0 < secpolicy_check((it -> second) -> sp, 0, PQI_OUTGOING))
		{
			std::ostringstream out;
			out << "pqihandler::HandlePQItem() sending to chan:";
			out << it -> first << std::endl;
			pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, out.str());

			// if yes send on item.
			((it -> second) -> pqi) -> SendItem(item);
			return 1;
		}
		else
		{
			std::ostringstream out;
			out << "pqihandler::HandlePQItem()";
			out << " Sec not approved";
			pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, out.str());

			delete item;
			return -1;
		}
	}
	else
	{
		if (allowglobal <= 0)
		{
			std::ostringstream out;
			out << "pqihandler::HandleSearchItem()";
			out << " invalid routing for non-global searchitem";
			pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, out.str());

			return -1;
		}

		pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, 
		  "pqihandler::HandlePQItem() Sending to All Channels");

		// loop through all channels.
		// for each one.
		
			// find module
			// if yes send.
		for(it = mods.begin(); it != mods.end(); it++)
		{

			// check security... is output allowed.
			if(0 < secpolicy_check((it -> second) -> sp, 0, PQI_OUTGOING))
			{
				std::ostringstream out;
				out << "pqihandler::HandlePQItem()";
				out << "Sending to chan:" << it -> first;
			pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, out.str());

				// if yes send on item.
				((it -> second) -> pqi) -> SendItem(item -> clone());
			}
			else
			{
				std::ostringstream out;
				out << "pqihandler::HandlePQItem()";
				out << " List Send - Not in this channel!";
			pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, out.str());

			}
		}

		// now we can clean up!
		delete item;
		return 1;
	}

	// if successfully sent to at least one.
	return 1;
}

	

// 4 very similar outputs.....
int	pqihandler::Search(SearchItem *ns)
{
	return HandlePQItem(ns, 1);
}

/* This is called when we only want it to go to 1 person.... */
int	pqihandler::SearchSpecific(SearchItem *ns) 
{
	return HandlePQItem(ns, 0);
}

int	pqihandler::CancelSearch(SearchItem *ns)
{
	return HandlePQItem(ns, 1);
}

int     pqihandler::SendFileItem(PQFileItem *ns)
{
	return HandlePQItem(ns, 0);
}

int	pqihandler::SendSearchResult(PQFileItem *ns)
{
	return HandlePQItem(ns, 0);
}

int	pqihandler::SendMsg(ChatItem *ns)
{
	/* switch from 1 -> 0 for specific directed Msg */
	return HandlePQItem(ns, 0);
}

int	pqihandler::SendGlobalMsg(ChatItem *ns)
{
	return HandlePQItem(ns, 1);
}

int     pqihandler::SendOtherPQItem(PQItem *ns)
{
	pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, 
	  	"pqihandler::SendOtherPQItem()");
	return HandlePQItem(ns, 1);
}

// inputs. This is a very basic
// system that is completely biased and slow...
// someone please fix.

int pqihandler::GetItems()
{
	std::map<int, SearchModule *>::iterator it;

	PQItem *item;
	int count = 0;

	// loop through modules....
	for(it = mods.begin(); it != mods.end(); it++)
	{
		SearchModule *mod = (it -> second);

		// check security... is output allowed.
		if(0 < secpolicy_check((it -> second) -> sp, 
					PQI_ITEM_TYPE_ITEM, PQI_INCOMING))
		{
			// if yes... attempt to read.
			while((item = (mod -> pqi) -> GetItem()) != NULL)
			{
				std::ostringstream out;
				out << "pqihandler::GetItems() Incoming Item ";
				out << " from: " << mod -> pqi << std::endl;
				item -> print(out);

				pqioutput(PQL_DEBUG_BASIC, 
						pqihandlerzone, out.str());

				// got one!....
				// update routing.
				item -> cidpush(mod -> smi);
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




void pqihandler::SortnStoreItem(PQItem *item)
{
	// some template comparors for sorting incoming.
	const static PQItem_MatchType match_fileitem(PQI_ITEM_TYPE_FILEITEM, 
				0);
	const static PQItem_MatchType match_result(PQI_ITEM_TYPE_FILEITEM, 
				PQI_FI_SUBTYPE_GENERAL);

	const static PQItem_MatchType match_endsrch(PQI_ITEM_TYPE_SEARCHITEM, 
				PQI_SI_SUBTYPE_CANCEL);
	const static PQItem_MatchType match_reqsrch(PQI_ITEM_TYPE_SEARCHITEM, 
				PQI_SI_SUBTYPE_SEARCH);

	const static PQItem_MatchType match_info(PQI_ITEM_TYPE_INFOITEM, 0);
	const static PQItem_MatchType match_chat(PQI_ITEM_TYPE_CHATITEM, 0);

	if (match_fileitem(item))
	{
		if (match_result(item))
		{
			pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, 
				"SortnStore -> Result");
				
			in_result.push_back(item);
		}
		else 
		{
			pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, 
				"SortnStore -> FileItem");
			in_file.push_back(item);
		}
	}
	else if (match_endsrch(item))
	{
		pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, 
			"SortnStore -> Cancel Search");

		in_endsrch.push_back(item);
	}
	else if (match_reqsrch(item))
	{
		pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, 
			"SortnStore -> Search Request");
		in_reqsrch.push_back(item);
	}
	else if (match_chat(item))
	{
		pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, 
			"SortnStore -> Chat");
		in_chat.push_back(item);
	}
	else if (match_info(item))
	{
		pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, 
			"SortnStore -> Info");
		in_info.push_back(item);
	}
	else
	{
		pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, 
			"SortnStore -> Other");
		in_other.push_back(item);
	}
	return;
}


// much like the input stuff.
PQFileItem *pqihandler::GetSearchResult()
{
	if (in_result.size() != 0)
	{
		PQFileItem *fi = (PQFileItem *) in_result.front();
		in_result.pop_front();
		return fi;
	}
	return NULL;
}

SearchItem *pqihandler::RequestedSearch()
{
	if (in_reqsrch.size() != 0)
	{
		SearchItem *si = (SearchItem *) in_reqsrch.front();
		in_reqsrch.pop_front();
		return si;
	}
	return NULL;
}

SearchItem *	pqihandler::CancelledSearch()
{
	if (in_endsrch.size() != 0)
	{
		SearchItem *si = (SearchItem *) in_endsrch.front();
		in_endsrch.pop_front();
		return si;
	}
	return NULL;
}

PQFileItem *pqihandler::GetFileItem()
{
	if (in_file.size() != 0)
	{
		PQFileItem *fi = (PQFileItem *) in_file.front();
		in_file.pop_front();
		return fi;
	}
	return NULL;
}

// Chat Interface
ChatItem *pqihandler::GetMsg()
{
	if (in_chat.size() != 0)
	{
		ChatItem *ci = (ChatItem *) in_chat.front();
		in_chat.pop_front();
		return ci;
	}
	return NULL;
}

PQItem *pqihandler::GetOtherPQItem()
{
	if (in_other.size() != 0)
	{
		PQItem *pqi = in_other.front();
		in_other.pop_front();
		return pqi;
	}
	return NULL;
}

PQItem *pqihandler::SelectOtherPQItem(bool (*tst)(PQItem *))
{
	std::list<PQItem *>::iterator it;
	it = find_if(in_other.begin(), in_other.end(), *tst);
	if (it != in_other.end())
	{
		PQItem *pqi = (*it);
		in_other.erase(it);
		return pqi;
	}
	return NULL;
}

static const float MIN_RATE = 0.01; // 10 B/s

// internal fn to send updates 
int     pqihandler::UpdateRates()
{
	std::map<int, SearchModule *>::iterator it;
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
//		std::cerr << "\tSM(" << mod -> smi << ")";
//		std::cerr << "In A: " << mod -> pqi -> getMaxRate(true);
//		std::cerr << " C: " << crate_in;
//		std::cerr << " && Out A: " << mod -> pqi -> getMaxRate(false);
//		std::cerr << " C: " << crate_out << std::endl;
	}
//	std::cerr << "Totals (In) Used B/W " << used_bw_in;
//	std::cerr << " Excess B/W " << extra_bw_in;
//	std::cerr << " Available B/W " << avail_in << std::endl;
//	std::cerr << "Totals (Out) Used B/W " << used_bw_out;
//	std::cerr << " Excess B/W " << extra_bw_out;
//	std::cerr << " Available B/W " << avail_out << std::endl;

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


