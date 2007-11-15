/*
 * "$Id: pqi.h,v 1.8 2007-03-31 09:41:32 rmf24 Exp $"
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



#ifndef MRK_P3_PUBLIC_INTERFACE_HEADER
#define MRK_P3_PUBLIC_INTERFACE_HEADER

// This is the overall include, for public usage
// + extension of the p3 interface.

#include "pqi/pqi_data.h"

#include <iostream>
#include <functional>
#include <algorithm>


/********************** SEARCH INTERFACE ***************************/
// this is an interface.... so should be
// classified as virtual   = 0;

class SearchInterface
{
public:
	SearchInterface()  { return; }

virtual	~SearchInterface() { return; }

	// OutGoing Searches...

virtual int	Search(SearchItem *) = 0;  // Search All people...
virtual int	SearchSpecific(SearchItem *) = 0; // Search One person only...
virtual int	CancelSearch(SearchItem *) = 0; // no longer used?
virtual PQFileItem *GetSearchResult() = 0;


	// Incoming Searches...
virtual SearchItem *RequestedSearch() = 0;
virtual SearchItem *CancelledSearch() = 0;
virtual int     SendSearchResult(PQFileItem *item) = 0;

	// FileTransfer.
virtual PQFileItem *GetFileItem() = 0;
virtual int     SendFileItem(PQFileItem *) = 0;

};

class ChatInterface
{
public:
	ChatInterface() {return; };
virtual ~ChatInterface() {return; };

virtual int	SendMsg(ChatItem *item) = 0;
virtual int     SendGlobalMsg(ChatItem *ns) = 0; 
virtual ChatItem *GetMsg() = 0;

};

// This can potentially be moved to where the
// discovery code is!!!!

class HostItem: public PQItem { };

class DiscoveryInterface
{
	public:
virtual ~DiscoveryInterface() { return; }
	// Discovery Request.
virtual int 	DiscoveryRequest(PQItem *) = 0; // item is just for direction.
virtual HostItem *DiscoveryResult() = 0;
};


class P3Interface: public SearchInterface, 
		   public ChatInterface 
{
public:
	P3Interface() {return; }
virtual ~P3Interface() {return; }

virtual int	tick() { return 1; }
virtual int	status() { return 1; }

virtual int	SendOtherPQItem(PQItem *) = 0;
virtual PQItem *GetOtherPQItem() = 0;
virtual PQItem *SelectOtherPQItem(bool (*tst)(PQItem *)) = 0;

};

#endif //MRK_PQI_BASE_HEADER
