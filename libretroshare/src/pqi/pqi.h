/*
 * libretroshare/src/pqi pqi.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2008 by Robert Fernie.
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


#ifndef PQI_TOP_HEADER
#define PQI_TOP_HEADER

/* This just includes the standard headers required.
 */


#include "pqi/pqi_base.h"
#include "pqi/pqinetwork.h"
#include "serialiser/rsserial.h"
#include "serialiser/rsbaseitems.h"

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

	// Cache Requests
virtual int	SearchSpecific(RsCacheRequest *) = 0; 
virtual RsCacheRequest *RequestedSearch() = 0;

	// Cache Results
virtual int     SendSearchResult(RsCacheItem *item) = 0;
virtual RsCacheItem *GetSearchResult() = 0;

	// FileTransfer.
virtual RsFileRequest *GetFileRequest() = 0;
virtual int     SendFileRequest(RsFileRequest *) = 0;

virtual RsFileData *GetFileData() = 0;
virtual int     SendFileData(RsFileData *) = 0;

};

class P3Interface: public SearchInterface
{
public:
	P3Interface() {return; }
virtual ~P3Interface() {return; }

virtual int	tick() { return 1; }
virtual int	status() { return 1; }

virtual int	SendRsRawItem(RsRawItem *) = 0;
virtual RsRawItem *GetRsRawItem() = 0;

};

#endif // PQI_TOP_HEADER

