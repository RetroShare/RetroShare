/*
 * "$Id: p3loopback.cc,v 1.3 2007-02-18 21:46:49 rmf24 Exp $"
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




#include "pqi/pqi.h"
#include "pqi/p3loopback.h"

// Test Interface (LoopBack)


p3loopback::~p3loopback()
{
	// should clean up lists ...
	// delete all random search items...
	// fix later..
	return;
}

int	p3loopback::Search(SearchItem *si)
{
	// no more copy - just use.

	// tag as new.
	si -> flags = 1;

	// add to requested searches.
	searches[si -> sid] = si;

	return 1;
}

int	p3loopback::CancelSearch(SearchItem *item)
{
	std::map<int, SearchItem *>::iterator it;
	std::list<PQFileItem *>::iterator it2;

	unsigned int sid = item -> sid;

	// Quick Sanity Check.
	it = searches.find(sid);
	if (it == searches.end())
	{
		std::cerr << "p3loopback::CancelSearch(" << sid;
		std::cerr << ") Search Not Found!" << std::endl;
		return -2;
	}

	// found search delete.
	SearchItem *osi = it -> second;
	searches.erase(it);

	// find anything matching in the list.
	for(it2 = searchResults.begin(); it2 != searchResults.end();)
	{
		if ((*it2) -> sid == sid) // erase
		{
			PQFileItem *osr = (*it2);
			it2 = searchResults.erase(it2);
			delete osr;
		}
		else
		{
			it2++;
		}
	}
	
	cancelledSearches.push_back(osi);
	// clean up input.
	delete item;
	return 1;
}


PQFileItem * 	p3loopback::GetSearchResult()
{
	std::list<PQFileItem *>::iterator it;

	it = searchResults.begin();
	if (it == searchResults.end()) // empty list
	{
		return NULL;
	}
	PQFileItem *osr = (*it);
	searchResults.erase(it);

	// set as local...
	osr -> flags |= PQI_ITEM_FLAG_LOCAL;
	return osr;
}

int p3loopback::GetFile(PQFileItem *item, std::ostream &in)
{
	// save a stream ....
	// send on the request.
	//fixme("p3loopback::GetFile()", 1);
	return 1;
}

int     p3loopback::CancelFile(PQFileItem *item)
{
	// close down stream (if we created it)
	// send on request.
	//fixme("p3loopback::CancelFile()", 1);
	return 1;
}


// Remote Requests.

SearchItem *p3loopback::RequestedSearch()
{
	std::map<int, SearchItem *>::iterator it;
	SearchItem *ns;

	for(it = searches.begin(); it != searches.end(); it++)
	{
		// check if new...
		if ((it -> second) -> flags == 1)
		{
			// generate new sid number.
			int rsid = getPQIsearchId();
			int sid = (it -> second) -> sid;

			// setup translation stuff. 
			sid2rsid[sid] = rsid;

			ns = new SearchItem();
			ns -> copy(it -> second);
			(it -> second) -> flags = 0;

			ns -> sid = rsid;

			return ns;
		}
	}
	return NULL;
}



SearchItem *p3loopback::CancelledSearch()
{
	std::list<SearchItem *>::iterator it;
	std::map<int, int>::iterator mit;

	it = cancelledSearches.begin();
	if (it == cancelledSearches.end()) // empty list
	{
		return NULL;
	}
	// else copy into structure.
	SearchItem *ncs = (*it);
	cancelledSearches.erase(it);

	// get from mapping.
	mit = sid2rsid.find(ncs -> sid);

	if (mit == sid2rsid.end()) // error
	{
		std::cerr << "Error No Mapping (Cancelling the Cancel)!";
		std::cerr << "But Still deleting item";
		std::cerr << std::endl;
		delete ncs;
		return NULL;
	}
	else
	{
		ncs -> sid = mit -> second;
		sid2rsid.erase(mit);
	}

	return ncs;
}

int	p3loopback::SendSearchResult(PQFileItem *item)
{
	//std::list<SearchItem *> >::iterator it;
	std::map<int, int>::iterator it;

	int rsid = item -> sid;
	int sid = 0;

	// get from mapping.
	for(it = sid2rsid.begin(); ((it != sid2rsid.end()) && (sid == 0)); it++)
	{
		if (rsid == (it -> second))
		{
			sid = it -> first;
		}
	}

	// if no mapping.
	if (sid == 0)
	{
		std::cerr << "SendSearchResult::Error No Mapping (Removing Item)!";
		std::cerr << std::endl;
		delete item;
		return 0;
	}

	// have mapping.... translate.
	item -> sid = sid;

	// stick on the queue.
	searchResults.push_back(item);

	return 1;
}

PQFileItem *p3loopback::RequestedFile()
{
	// check for searches with new tag...
	// if none return 0;
	// copy data from first.

	// mark old.
	//fixme("p3loopback::RequestedFile()", 1);
	return NULL;
}


PQFileItem *p3loopback::CancelledFile()
{
	// check for cancelled files.
	// if none return 0;
	// copy data from first.

	// remove/cleanup
	//fixme("p3loopback::CancelledFile()", 1);
	return NULL;
}

int	p3loopback::SendFile(PQFileItem *, std::istream &out)
{
	// get data...
	// attach streams.....
	//
	//fixme("p3loopback::SendFile()", 1);
	return 1;
}

	
//
//// control interface.
//int	p3loopback::RequestInfo(InfoItem *item)
//{
//        switch(item -> querytype)
//	{
//		case PQI_II_QUERYTYPE_TAG:
//			item -> answer = "<++Local++>";
//			return 1;
//			break;
//
//		case PQI_II_QUERYTYPE_CLASS:
//			item -> answer = "p3loopback";
//			return 1;
//			break;
//
//		default:
//			return 0;
//			break;
//	}
//	return -1;
//
//	return 0;
//}
//
//int 	p3loopback::doCommand(CommandItem *)
//{
//	//fixme("p3loopback::doCommand()", 1);
//	return 0;
//}
//

// // chat interface.
int 	p3loopback::SendMsg(ChatItem *item)
{
	//fixme("p3loopback::SendMsg()", 1);
	return 0;
}

ChatItem *p3loopback::GetMsg()
{
	//fixme("p3loopback::GetMsg()", 1);
	return NULL;
}

int     p3loopback::SendOtherPQItem(PQItem *item)
{
	other.push_back(item);
	return 1;
}

PQItem *p3loopback::GetOtherPQItem()
{
	if (other.size() != 0)
	{
		PQItem *i = other.front();
		other.pop_front();
		return i;
	}
	return NULL;
}


// // PQI interface.
int 	p3loopback::tick()
{
	//fixme("p3loopback::tick()", 1);
	return 0;
}

int 	p3loopback::status()
{
	//fixme("p3loopback::status()", 1);
	return 0;
}
