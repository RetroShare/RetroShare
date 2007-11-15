/*
 * "$Id: p3loopback.h,v 1.4 2007-02-18 21:46:49 rmf24 Exp $"
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



#ifndef MRK_P3_LOOPBACK_HEADER
#define MRK_P3_LOOPBACK_HEADER


// The standard data types and the search interface.
#include "pqi/pqi.h"

#include <map>
#include <list>


// Test Interface (LoopBack)
// public interface .... includes the whole p3interface.
class p3loopback: public P3Interface
{
public:
virtual ~p3loopback();

// search Interface.
virtual int	Search(SearchItem *item);
virtual int	CancelSearch(SearchItem *item);
virtual PQFileItem *GetSearchResult();

virtual int	GetFile(PQFileItem *item, std::ostream &in);
virtual int     CancelFile(PQFileItem *);

virtual SearchItem *RequestedSearch();
virtual SearchItem *CancelledSearch();
virtual int	SendSearchResult(PQFileItem *item);

virtual PQFileItem *RequestedFile();
virtual PQFileItem *CancelledFile();
virtual int	SendFile(PQFileItem *, std::istream &out);

// control interface.
//virtual int 	RequestInfo(InfoItem *);
//virtual int     doCommand(CommandItem *);

// chat interface.
virtual int     SendMsg(ChatItem *item);
virtual ChatItem *GetMsg();

// PQI interface.
virtual int	tick();
virtual int	status();

virtual int     SendOtherPQItem(PQItem *);
virtual PQItem *GetOtherPQItem();

	private:
	std::list<PQFileItem *> searchResults;
	std::map<int, SearchItem *> searches;
	std::map<int, SearchItem *> files;
	std::list<SearchItem *> cancelledSearches;
	std::list<SearchItem *> cancelledFiles;
	std::list<PQItem *> other;

	// remote mapping
	std::map<int, int> sid2rsid;
};



#endif //MRK_P3_LOOPBACK_HEADER
