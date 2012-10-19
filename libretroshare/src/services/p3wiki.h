/*
 * libretroshare/src/services: p3wikiservice.h
 *
 * Wiki interface for RetroShare.
 *
 * Copyright 2012-2012 by Robert Fernie.
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

#ifndef P3_WIKI_SERVICE_HEADER
#define P3_WIKI_SERVICE_HEADER

#include "retroshare/rswiki.h"
#include "gxs/rsgenexchange.h"

#include <map>
#include <string>

/* 
 * Wiki Service
 *
 *
 */

class p3Wiki: public RsGenExchange, public RsWiki
{
public:
    p3Wiki(RsGeneralDataService* gds, RsNetworkExchangeService* nes);

protected:

virtual void notifyChanges(std::vector<RsGxsNotify*>& changes) ;

public:

        /* Specific Service Data */
virtual bool getCollections(const uint32_t &token, std::vector<RsWikiCollection> &collections);
virtual bool getSnapshots(const uint32_t &token, std::vector<RsWikiSnapshot> &snapshots);
virtual bool getComments(const uint32_t &token, std::vector<RsWikiComment> &comments);

virtual bool submitCollection(uint32_t &token, RsWikiCollection &collection);
virtual bool submitSnapshot(uint32_t &token, RsWikiSnapshot &snapshot);
virtual bool submitComment(uint32_t &token, RsWikiComment &comment);

	private:

//std::string genRandomId();
//	RsMutex mWikiMtx;


};

#endif 
