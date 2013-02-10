/*
 * libretroshare/src/services: p3wikiservice.h
 *
 * Wiki interface for RetroShare.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#include "util/rstickevent.h"

#include <map>
#include <string>

/* 
 * Wiki Service
 *
 *
 */

class p3Wiki: public RsGenExchange, public RsWiki, 
	public RsTickEvent
{
public:
    p3Wiki(RsGeneralDataService* gds, RsNetworkExchangeService* nes, RsGixs *gixs);
static uint32_t wikiAuthenPolicy();

protected:

virtual void notifyChanges(std::vector<RsGxsNotify*>& changes) ;

        // Overloaded from RsTickEvent.
virtual void handle_event(uint32_t event_type, const std::string &elabel);

public:

virtual void service_tick();

        /* Specific Service Data */
virtual bool getCollections(const uint32_t &token, std::vector<RsWikiCollection> &collections);
virtual bool getSnapshots(const uint32_t &token, std::vector<RsWikiSnapshot> &snapshots);
virtual bool getComments(const uint32_t &token, std::vector<RsWikiComment> &comments);

virtual bool getRelatedSnapshots(const uint32_t &token, std::vector<RsWikiSnapshot> &snapshots);

virtual bool submitCollection(uint32_t &token, RsWikiCollection &collection);
virtual bool submitSnapshot(uint32_t &token, RsWikiSnapshot &snapshot);
virtual bool submitComment(uint32_t &token, RsWikiComment &comment);


	private:

std::string genRandomId();
//	RsMutex mWikiMtx;


virtual void generateDummyData();

	// Dummy Stuff.
	void dummyTick();

	bool mAboutActive;
	uint32_t mAboutToken;
	int  mAboutLines;
	RsGxsMessageId mAboutThreadId;

	bool mImprovActive;
	uint32_t mImprovToken;
	int  mImprovLines;
	RsGxsMessageId mImprovThreadId;

	bool mMarkdownActive;
	uint32_t mMarkdownToken;
	int  mMarkdownLines;
	RsGxsMessageId mMarkdownThreadId;
};

#endif 
