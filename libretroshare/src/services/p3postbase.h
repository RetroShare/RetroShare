/*******************************************************************************
 * libretroshare/src/services: p3postbase.h                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2008-2012 Robert Fernie <retroshare@lunamutt.com>                 *
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
#ifndef P3_POSTBASE_SERVICE_HEADER
#define P3_POSTBASE_SERVICE_HEADER


#include "services/p3gxscommon.h"
#include "gxs/rsgenexchange.h"

#include "util/rstickevent.h"

#include <retroshare/rsidentity.h>

#include <map>
#include <string>
#include <list>

/* 
 *
 */


class PostStats
{
	public:
	PostStats() :up_votes(0), down_votes(0), comments(0) { return; }
	PostStats(int up, int down, int c) :up_votes(up), down_votes(down), comments(c) { return; }

	void increment(const PostStats &s) 
	{ 
		up_votes += s.up_votes;
		down_votes += s.down_votes;
		comments += s.comments;
		return;
	}

	int up_votes;
	int down_votes;
	int comments;
	std::list<RsGxsId> voters;
};

bool encodePostCache(std::string &str, const PostStats &s);
bool extractPostCache(const std::string &str, PostStats &s);


class p3PostBase: public RsGenExchange, public GxsTokenQueue, public RsTickEvent, public p3Config
{
public:

	p3PostBase(RsGeneralDataService *gds, RsNetworkExchangeService *nes, RsGixs* gixs,
	           RsSerialType* serviceSerialiser, uint16_t serviceType);

    virtual void service_tick() override;

protected:

    virtual void notifyChanges(std::vector<RsGxsNotify*>& changes) override;

	// Overloaded from GxsTokenQueue for Request callbacks.
    virtual void handleResponse(uint32_t token, uint32_t req_type) override;

	// Overloaded from RsTickEvent.
    virtual void handle_event(uint32_t event_type, const std::string &elabel) override;

	// overloads p3Config
	virtual RsSerialiser* setupSerialiser() override;                            // @see p3Config::setupSerialiser()
	virtual bool saveList(bool &cleanup, std::list<RsItem *>&saveList) override; // @see p3Config::saveList(bool &cleanup, std::list<RsItem *>&)
	virtual bool loadList(std::list<RsItem *>& loadList) override;               // @see p3Config::loadList(std::list<RsItem *>&)

    virtual bool service_checkIfGroupIsStillUsed(const RsGxsGrpMetaData& meta) override;
public:

	//////////////////////////////////////////////////////////////////////////////

	virtual void setMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read);


protected:

	p3GxsCommentService *mCommentService;

private:

	static uint32_t postBaseAuthenPolicy();

	// Background processing.
	void background_tick();

	bool background_requestAllGroups();
	void background_loadGroups(const uint32_t &token);

	void addGroupForProcessing(RsGxsGroupId grpId);
	void background_requestUnprocessedGroup();

	void background_requestGroupMsgs(const RsGxsGroupId &grpId, bool unprocessedOnly);
	void background_loadUnprocessedMsgs(const uint32_t &token);
	void background_loadAllMsgs(const uint32_t &token);
	void background_loadMsgs(const uint32_t &token, bool unprocessed);


	void background_updateVoteCounts(const uint32_t &token);
	bool background_cleanup();


	RsMutex mPostBaseMtx;
    RsMutex mKnownPostedMutex;

	bool mBgProcessing;
	bool mBgIncremental;
	std::set<RsGxsGroupId> mBgGroupList;
	std::map<RsGxsMessageId, PostStats> mBgStatsMap;

	std::map<RsGxsGroupId,rstime_t> mKnownPosted;
};

#endif 
