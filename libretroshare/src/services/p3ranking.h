/*
 * libretroshare/src/services: p3ranking.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2007-2008 by Robert Fernie.
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

#ifndef P3_GENERIC_RANKING_HEADER
#define P3_GENERIC_RANKING_HEADER

#include "dbase/cachestrapper.h"
#include "pqi/pqiservice.h"
#include "pqi/pqistreamer.h"
#include "pqi/p3connmgr.h"
#include "pqi/p3cfgmgr.h"

#include "serialiser/rsserial.h"

#include "rsiface/rsrank.h"

/* 
 * A Generic Ranking system.
 * Each User provides one cache...
 *
 * can be overloaded for specific types
 * (links, shares, photos etc)
 *
 * This is not generic yet!!!
 */

class RsRankMsg;
class RsRankLinkMsg;

class RankGroup
{
	public:

	std::string rid; /* Random Id */
	std::wstring link; 
	std::wstring title; 
	float rank;
	bool ownTag;
	std::map<std::string, RsRankLinkMsg *>  comments;
};


class p3Ranking: public CacheSource, public CacheStore, public p3Config
{
	public:

	p3Ranking(p3ConnectMgr *connMgr, 
		uint16_t type, CacheStrapper *cs, CacheTransfer *cft,
		std::string sourcedir, std::string storedir, 
		uint32_t storePeriod);

/******************************* CACHE SOURCE / STORE Interface *********************/

	/* overloaded functions from Cache Source */
virtual bool    loadLocalCache(const CacheData &data);

	/* overloaded functions from Cache Store */
virtual int    loadCache(const CacheData &data);

/******************************* CACHE SOURCE / STORE Interface *********************/

	public:

/************* Extern Interface *******/

	/* changed */
virtual bool updated();

        /* Set Sort Methods */
virtual bool setSortPeriod(uint32_t period);
virtual bool setSortMethod(uint32_t type);
virtual bool clearPeerFilter();
virtual bool setPeerFilter(std::list<std::string> peers);

        /* get Ids */
virtual uint32_t getRankingsCount();
virtual float   getMaxRank();
virtual bool    getRankings(uint32_t first, uint32_t count, std::list<std::string> &rids);
virtual bool    getRankDetails(std::string rid, RsRankDetails &details);

        /* Add New Comment / Msg */
virtual std::string newRankMsg(std::wstring link, std::wstring title, std::wstring comment, int32_t score);
virtual bool updateComment(std::string rid, std::wstring comment, int32_t score);
virtual std::string anonRankMsg(std::string rid, std::wstring link, std::wstring title);


void	tick();

void	loadRankFile(std::string filename, std::string src);
void 	addRankMsg(RsRankLinkMsg *msg);
void 	publishMsgs(bool own);

float 	locked_calcRank(RankGroup &grp); /* returns 0->100 */
void	locked_reSortGroup(RankGroup &grp);

void	sortAllMsgs();
pqistreamer *createStreamer(std::string file, std::string src, bool reading);


	/****************** p3Config STUFF *******************/
        protected:
bool    addAnonToList(RsRankLinkMsg *msg);

virtual RsSerialiser *setupSerialiser();
virtual std::list<RsItem *> saveList(bool &cleanup);
virtual bool loadList(std::list<RsItem *> load);
virtual void    saveDone();

	private:	

void	createDummyData();

	p3ConnectMgr *mConnMgr;

	RsMutex mRankMtx;

	/***** below here is locked *****/

	bool mRepublish;
	bool mRepublishFriends;
	time_t mRepublishFriendTS;

	uint32_t mStorePeriod;

	std::string mOwnId;
	bool mUpdated;

	std::map<std::string, RankGroup> mData;
	std::multimap<float, std::string> mRankings;

	/* Filter/Sort params */
	std::list<std::string> mPeerFilter;
	uint32_t mViewPeriod;
	uint32_t mSortType;

	/* Anonymous Link List */
	std::list<RsRankLinkMsg *> mAnon;

};

#endif 



