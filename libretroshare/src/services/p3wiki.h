/*******************************************************************************
 * libretroshare/src/services: p3wiki.h                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2012 by Robert Fernie <retroshare@lunamutt.com>              *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
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
virtual RsServiceInfo getServiceInfo();
static uint32_t wikiAuthenPolicy();

protected:

virtual void notifyChanges(std::vector<RsGxsNotify*>& changes) ;

        // Overloaded from RsTickEvent.
virtual void handle_event(uint32_t event_type, const std::string &elabel);

public:

virtual void service_tick();

        /* Specific Service Data */
virtual bool getCollections(const uint32_t &token, std::vector<RsWikiCollection> &collections) override;
virtual bool getSnapshots(const uint32_t &token, std::vector<RsWikiSnapshot> &snapshots) override;
virtual bool getComments(const uint32_t &token, std::vector<RsWikiComment> &comments) override;

virtual bool getRelatedSnapshots(const uint32_t &token, std::vector<RsWikiSnapshot> &snapshots) override;

virtual bool submitCollection(uint32_t &token, RsWikiCollection &collection) override;
virtual bool submitSnapshot(uint32_t &token, RsWikiSnapshot &snapshot) override;
virtual bool submitComment(uint32_t &token, RsWikiComment &comment) override;

virtual bool updateCollection(uint32_t &token, RsWikiCollection &collection) override;

// Blocking Interfaces.
virtual bool createCollection(RsWikiCollection &collection) override;
virtual bool updateCollection(const RsWikiCollection &collection) override;
virtual bool getCollections(const std::list<RsGxsGroupId> groupIds, std::vector<RsWikiCollection> &groups) override;

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
