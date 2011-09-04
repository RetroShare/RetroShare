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
#include "util/rsdebug.h"
#include <stdlib.h>
const int pqihandlerzone = 34283;

static const int PQI_HANDLER_NB_PRIORITY_LEVELS = 10 ;
static const float PQI_HANDLER_NB_PRIORITY_RATIO = 2 ;

/****
#define DEBUG_TICK 1
#define RSITEM_DEBUG 1
****/
//#define DEBUG_QOS 1

pqihandler::pqihandler(SecurityPolicy *Global) : pqiQoS(PQI_HANDLER_NB_PRIORITY_LEVELS,PQI_HANDLER_NB_PRIORITY_RATIO),coreMtx("pqihandler")
{
	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

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

	// send items from QoS queue

	moreToTick |= drawFromQoS_queue() ;

	UpdateRates();
	return moreToTick;
}

bool pqihandler::drawFromQoS_queue()
{
	float avail_out = getMaxRate(false) * 1024 / ticks_per_sec ;

	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

	++nb_ticks ;
	time_t now = time(NULL) ;
	if(last_m + 3 < now)
	{
		ticks_per_sec = nb_ticks / (float)(now - last_m) ;
		nb_ticks = 0 ;
		last_m = now ;
	}
#ifdef DEBUG_QOS
	std::cerr << "ticks per sec: " << ticks_per_sec << ", max rate in bytes/s = " << avail_out*ticks_per_sec << ", avail out per tick= " << avail_out << std::endl;
#endif

	uint64_t total_bytes_sent = 0 ;
	RsItem *item ;

	while( total_bytes_sent < avail_out && (item = out_rsItem()) != NULL)
	{
		//
		uint32_t size ;
		locked_HandleRsItem(item, 0, size);
		total_bytes_sent += size ;
#ifdef DEBUG_QOS
		std::cerr << "treating item " << (void*)item << ", priority " << (int)item->priority_level() << ", size=" << size << ", total = " << total_bytes_sent << ", queue size = " << qos_queue_size() << std::endl;
#endif
	}
#ifdef DEBUG_QOS
	assert(total_bytes_sent >= avail_out || qos_queue_size() == 0) ;
	std::cerr << "total bytes sent = " << total_bytes_sent << ", " ;
	if(qos_queue_size() > 0) 
		std::cerr << "Queue still has " << qos_queue_size() << " elements." << std::endl;
	else
		std::cerr << "Queue is empty." << std::endl;
#endif

	return (qos_queue_size() > 0) ;
}


bool pqihandler::queueOutRsItem(RsItem *item)
{
	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/
	in_rsItem(item) ;

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
	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/
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
int	pqihandler::locked_checkOutgoingRsItem(RsItem */*item*/, int /*global*/)
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
		std::ostringstream out;
		out << "pqihandler::HandleSearchItem()";
		out << " Cannot send out Global RsItem";
		pqioutput(PQL_ALERT, pqihandlerzone, out.str());
#ifdef DEBUG_TICK
		std::cerr << out.str();
#endif
		delete item;
		return -1;
	}

	if (!locked_checkOutgoingRsItem(item, allowglobal))
	{
		std::ostringstream out;
	  	out <<	"pqihandler::HandleRsItem() checkOutgoingPQItem";
		out << " Failed on item: " << std::endl;
#ifdef DEBUG_TICK
                std::cerr << out.str();
#endif
                item -> print(out);

		pqioutput(PQL_ALERT, pqihandlerzone, out.str());
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
		std::ostringstream out;
		out << "pqihandler::HandleRsItem() Invalid chan!";
		pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, out.str());
#ifdef DEBUG_TICK
                std::cerr << out.str();
#endif

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
#ifdef DEBUG_TICK
                std::cerr << out.str();
#endif

		// if yes send on item.
		((it -> second) -> pqi) -> SendItem(item,computed_size);
		return 1;
	}
	else
	{
		std::ostringstream out;
		out << "pqihandler::HandleRsItem()";
		out << " Sec not approved";
		pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, out.str());
#ifdef DEBUG_TICK
                std::cerr << out.str();
#endif

		delete item;
		return -1;
	}

	// if successfully sent to at least one.
	return 1;
}

int	pqihandler::SearchSpecific(RsCacheRequest *ns) 
{
	return queueOutRsItem(ns) ;
}

int	pqihandler::SendSearchResult(RsCacheItem *ns)
{
	return queueOutRsItem(ns) ;
}

int     pqihandler::SendFileRequest(RsFileRequest *ns)
{
	return queueOutRsItem(ns) ;
}

int     pqihandler::SendFileData(RsFileData *ns)
{
	return queueOutRsItem(ns) ;
}
int     pqihandler::SendFileChunkMapRequest(RsFileChunkMapRequest *ns)
{
	return queueOutRsItem(ns) ;
}
int     pqihandler::SendFileChunkMap(RsFileChunkMap *ns)
{
	return queueOutRsItem(ns) ;
}
int     pqihandler::SendFileCRC32MapRequest(RsFileCRC32MapRequest *ns)
{
	return queueOutRsItem(ns) ;
}
int     pqihandler::SendFileCRC32Map(RsFileCRC32Map *ns)
{
	return queueOutRsItem(ns) ;
}

int     pqihandler::SendRsRawItem(RsRawItem *ns)
{
	pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, "pqihandler::SendRsRawItem()");

	// queue the item into the QoS 
	
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

				locked_SortnStoreItem(item);
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




void pqihandler::locked_SortnStoreItem(RsItem *item)
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
					  pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, "SortnStore -> File Data");
					  in_data.push_back(item);
					  item = NULL;
					  break;

				  case RS_PKT_SUBTYPE_FI_CHUNK_MAP_REQUEST:
					  pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, "SortnStore -> File ChunkMap Request");
					  in_chunkmap_request.push_back(item);
					  item = NULL;
					  break;

				  case RS_PKT_SUBTYPE_FI_CHUNK_MAP:
					  pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, "SortnStore -> File ChunkMap");
					  in_chunkmap.push_back(item);
					  item = NULL;
					  break;

				  case RS_PKT_SUBTYPE_FI_CRC32_MAP_REQUEST:
					  pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, "SortnStore -> File Crc32Map Request");
					  in_crc32map_request.push_back(item);
					  item = NULL;
					  break;

				  case RS_PKT_SUBTYPE_FI_CRC32_MAP:
					  pqioutput(PQL_DEBUG_BASIC, pqihandlerzone, "SortnStore -> File CRC32Map");
					  in_crc32map.push_back(item);
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
	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

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
	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

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
	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

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
	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

	if (in_data.size() != 0)
	{
		RsFileData *fi = dynamic_cast<RsFileData *>(in_data.front());
		if (!fi) { delete in_data.front(); }
		in_data.pop_front();
		return fi;
	}
	return NULL;
}
RsFileChunkMapRequest *pqihandler::GetFileChunkMapRequest()
{
	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

	if (in_chunkmap_request.size() != 0)
	{
		RsFileChunkMapRequest *fi = dynamic_cast<RsFileChunkMapRequest *>(in_chunkmap_request.front());
		if (!fi) { delete in_chunkmap_request.front(); }
		in_chunkmap_request.pop_front();
		return fi;
	}
	return NULL;
}
RsFileChunkMap *pqihandler::GetFileChunkMap()
{
	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

	if (in_chunkmap.size() != 0)
	{
		RsFileChunkMap *fi = dynamic_cast<RsFileChunkMap *>(in_chunkmap.front());
		if (!fi) { delete in_chunkmap.front(); }
		in_chunkmap.pop_front();
		return fi;
	}
	return NULL;
}
RsFileCRC32MapRequest *pqihandler::GetFileCRC32MapRequest()
{
	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

	if (in_crc32map_request.size() != 0)
	{
		RsFileCRC32MapRequest *fi = dynamic_cast<RsFileCRC32MapRequest *>(in_crc32map_request.front());
		if (!fi) { delete in_crc32map_request.front(); }
		in_crc32map_request.pop_front();
		return fi;
	}
	return NULL;
}
RsFileCRC32Map *pqihandler::GetFileCRC32Map()
{
	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

	if (in_crc32map.size() != 0)
	{
		RsFileCRC32Map *fi = dynamic_cast<RsFileCRC32Map *>(in_crc32map.front());
		if (!fi) { delete in_crc32map.front(); }
		in_crc32map.pop_front();
		return fi;
	}
	return NULL;
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


