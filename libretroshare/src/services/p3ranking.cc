/*
 * libretroshare/src/services p3ranking.cc
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

#include "serialiser/rsrankitems.h"
#include "services/p3ranking.h"
#include <iomanip>

#include "pqi/pqibin.h"
#include "pqi/authssl.h"

const uint32_t RANK_MAX_FWD_OFFSET = (60 * 60 * 24 * 2); /* 2 Days */

const uint32_t FRIEND_RANK_REPUBLISH_PERIOD = 60; /* every minute for testing */
//const uint32_t FRIEND_RANK_REPUBLISH_PERIOD = 1800; /* every 30 minutes */

std::string generateRandomLinkId();

/*****
 * TODO
 * (1) Streaming.
 * (2) Ranking.
 *
 */

/*********
 * #define RANK_DEBUG 1
 *********/

p3Ranking::p3Ranking(p3ConnectMgr *connMgr, 
		uint16_t type, CacheStrapper *cs, CacheTransfer *cft,
		std::string sourcedir, std::string storedir, 
		uint32_t storePeriod)
	:CacheSource(type, true, cs, sourcedir), 
	CacheStore(type, true, cs, cft, storedir), 
	p3Config(CONFIG_TYPE_RANK_LINK),
	mConnMgr(connMgr), 
        mRepublish(false), mRepublishFriends(false), mRepublishFriendTS(0),
	mStorePeriod(storePeriod), mUpdated(true)
{

     { 	RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/

	mOwnId = mConnMgr->getOwnId();
	mViewPeriod = 60 * 60 * 24 * 30; /* one Month */
	mSortType = RS_RANK_ALG;

     } 	RsStackMutex stack(mRankMtx); 

//	createDummyData();
	return;
}

bool    p3Ranking::loadLocalCache(const CacheData &data)
{
	std::string filename = data.path + '/' + data.name;
	std::string hash = data.hash;
	//uint64_t size = data.size;
	std::string source = data.pid;

#ifdef RANK_DEBUG
	std::cerr << "p3Ranking::loadLocalCache()";
	std::cerr << std::endl;
	std::cerr << "\tSource: " << source;
	std::cerr << std::endl;
	std::cerr << "\tFilename: " << filename;
	std::cerr << std::endl;
	std::cerr << "\tHash: " << hash;
	std::cerr << std::endl;
	std::cerr << "\tSize: " << data.size;
	std::cerr << std::endl;
#endif

	loadRankFile(filename, source);

	{
          RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/
	  mRepublish = false;
	}

	if (data.size > 0) /* don't refresh zero sized caches */
	{
		refreshCache(data);
	}
	return true;
}

int    p3Ranking::loadCache(const CacheData &data)
{
	std::string filename = data.path + '/' + data.name;
	std::string hash = data.hash;
	//uint64_t size = data.size;
	std::string source = data.pid;

#ifdef RANK_DEBUG
	std::cerr << "p3Ranking::loadCache()";
	std::cerr << std::endl;
	std::cerr << "\tSource: " << source;
	std::cerr << std::endl;
	std::cerr << "\tFilename: " << filename;
	std::cerr << std::endl;
	std::cerr << "\tHash: " << hash;
	std::cerr << std::endl;
	std::cerr << "\tSize: " << data.size;
	std::cerr << std::endl;
#endif

	loadRankFile(filename, source);


	CacheStore::lockData();   /*****   LOCK ****/
	locked_storeCacheEntry(data); 
	CacheStore::unlockData(); /***** UNLOCK ****/

	return 1;
}


void p3Ranking::loadRankFile(std::string filename, std::string src)
{
	/* create the serialiser to load info */
	RsSerialiser *rsSerialiser = new RsSerialiser();
	rsSerialiser->addSerialType(new RsRankSerialiser());
	
	uint32_t bioflags = BIN_FLAGS_HASH_DATA | BIN_FLAGS_READABLE;
	BinInterface *bio = new BinFileInterface(filename.c_str(), bioflags);
	pqistore *store = new pqistore(rsSerialiser, src, bio, BIN_FLAGS_READABLE);
	
	time_t now = time(NULL);
	time_t min, max;

     { 	RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/

	min = now - mStorePeriod;
	max = now + RANK_MAX_FWD_OFFSET;

     } 	/********** STACK LOCKED MTX ******/

#ifdef RANK_DEBUG
	std::cerr << "p3Ranking::loadRankFile()";
	std::cerr << std::endl;
	std::cerr << "\tSource: " << src;
	std::cerr << std::endl;
	std::cerr << "\tFilename: " << filename;
	std::cerr << std::endl;
#endif

	RsItem *item;
	RsRankLinkMsg *newMsg;

	while(NULL != (item = store->GetItem()))
	{

#ifdef RANK_DEBUG
		std::cerr << "p3Ranking::loadRankFile() Got Item:";
		std::cerr << std::endl;
		item->print(std::cerr, 10);
		std::cerr << std::endl;
#endif

		if (NULL == (newMsg = dynamic_cast<RsRankLinkMsg *>(item)))
		{
#ifdef RANK_DEBUG
			std::cerr << "p3Ranking::loadRankFile() Item not LinkMsg (deleting):";
			std::cerr << std::endl;
#endif

			delete item;
		}
			/* check timestamp */
		else if (((time_t) newMsg->timestamp < min) || 
				((time_t) newMsg->timestamp > max))
		{
#ifdef RANK_DEBUG
			std::cerr << "p3Ranking::loadRankFile() Outside TimeRange (deleting):";
			std::cerr << std::endl;
#endif
			/* if outside range -> remove */
			delete newMsg;
		}
		else
		{
#ifdef RANK_DEBUG
			std::cerr << "p3Ranking::loadRankFile() Loading Item";
			std::cerr << std::endl;
#endif
			/* correct the source (if is a message from a friend) */
			newMsg->PeerId(newMsg->pid);
			addRankMsg(newMsg);
		}
	}

	delete store;
}


void p3Ranking::publishMsgs(bool own)
{

#ifdef RANK_DEBUG
	std::cerr << "p3Ranking::publishMsgs()";
	std::cerr << std::endl;
#endif

	std::string path = CacheSource::getCacheDir();
	std::ostringstream out;
	uint16_t subid;


	/* setup name / etc based on whether we're
	 * publishing own or friends...
	 */

	if (own)
	{
		/* setup to publish own messages */
		out << "rank-links-" << time(NULL) << ".rsrl";
		subid = 1;
	}
	else
	{
		/* setup to publish friend messages */
		out << "rank-friend-links-" << time(NULL) << ".rsrl";
		subid = 2;
	}

	/* determine filename */
	std::string tmpname = out.str();
	std::string fname = path + "/" + tmpname;

#ifdef RANK_DEBUG
	std::cerr << "p3Ranking::publishMsgs() Storing to: " << fname;
	std::cerr << std::endl;
#endif

	RsSerialiser *rsSerialiser = new RsSerialiser();
	rsSerialiser->addSerialType(new RsRankSerialiser());
	
	uint32_t bioflags = BIN_FLAGS_HASH_DATA | BIN_FLAGS_WRITEABLE;
	BinInterface *bio = new BinFileInterface(fname.c_str(), bioflags);
	pqistore *store = new pqistore(rsSerialiser, mOwnId, bio, BIN_FLAGS_NO_DELETE | BIN_FLAGS_WRITEABLE);
	
     { 	RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/

	/* iterate through list */
	std::map<std::string, RankGroup>::iterator it;
	std::map<std::string, RsRankLinkMsg *>::iterator cit;

	for(it = mData.begin(); it != mData.end(); it++)
	{
		if (own)
		{
		  if (it->second.ownTag)
		  {
			/* write to serialiser */
			RsItem *item = it->second.comments[mOwnId];
			if (item)
			{

#ifdef RANK_DEBUG
				std::cerr << "p3Ranking::publishMsgs() Storing Item:";
				std::cerr << std::endl;
				item->print(std::cerr, 10);
				std::cerr << std::endl;
#endif
				store->SendItem(item);
			}
		  }
		  else
		  {
#ifdef RANK_DEBUG
			std::cerr << "p3Ranking::publishMsgs() Skipping Foreign item";
			std::cerr << std::endl;
#endif
		  }
		}
		else
		{
		  /* if we have pushed it out already - don't bother */
		  if (it->second.ownTag)
		  {
#ifdef RANK_DEBUG
			std::cerr << "p3Ranking::publishMsgs() (Friends) Skipping Own Item";
			std::cerr << std::endl;
#endif
		  	continue;
		  }

		  /* if we have some comments ... then a friend has recommended it 
		   * serialise a sanitized version.
		   */
		  if (it->second.comments.size() > 0)
		  {
		  	RsRankLinkMsg *origmsg = (it->second.comments.begin())->second;
			RsRankLinkMsg *msg = new RsRankLinkMsg();

			/* copy anon data */

/*************************************************************************/
/****************************** LINK SPECIFIC ****************************/
/*************************************************************************/
			msg->clear();
			msg->PeerId("");
			msg->pid = ""; /* Anon */
			msg->rid = origmsg->rid;
			msg->link = origmsg->link;
			msg->title = origmsg->title;
			msg->timestamp = origmsg->timestamp;
			msg->score = 0;

#ifdef RANK_DEBUG
			std::cerr << "p3Ranking::publishMsgs() (Friends) Storing (Anon) Item:";
			std::cerr << std::endl;
			msg->print(std::cerr, 10);
			std::cerr << std::endl;
#endif
			store->SendItem(msg);

			/* cleanup */
			delete msg;
		  }
		}
	}


	/* now we also add our anon messages to the friends list */
	if (!own)
	{
		std::list<RsRankLinkMsg *>::iterator ait;
		for(ait=mAnon.begin(); ait != mAnon.end(); ait++)
		{
#ifdef RANK_DEBUG
			std::cerr << "p3Ranking::publishMsgs() (Friends) Adding Own Anon Item:";
			std::cerr << std::endl;
			(*ait)->print(std::cerr, 10);
			std::cerr << std::endl;
#endif
			store->SendItem(*ait);
		}
	}

     } 	/********** STACK LOCKED MTX ******/

	/* flag as new info */
	CacheData data;

     { 	RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/
	data.pid = mOwnId;
     } 	/********** STACK LOCKED MTX ******/

	data.cid = CacheId(CacheSource::getCacheType(), subid);

	data.path = path;
	data.name = tmpname;

	data.hash = bio->gethash();
	data.size = bio->bytecount();
	data.recvd = time(NULL);
	
#ifdef RANK_DEBUG
	std::cerr << "p3Ranking::publishMsgs() refreshing Cache";
	std::cerr << std::endl;
	std::cerr << "\tCache Path: " << data.path;
	std::cerr << std::endl;
	std::cerr << "\tCache Name: " << data.name;
	std::cerr << std::endl;
	std::cerr << "\tCache Hash: " << data.hash;
	std::cerr << std::endl;
	std::cerr << "\tCache Size: " << data.size;
	std::cerr << std::endl;
#endif
	if (data.size > 0) /* don't refresh zero sized caches */
	{
		refreshCache(data);
	}

	delete store;
}



void p3Ranking::addRankMsg(RsRankLinkMsg *msg)
{
	/* find msg */
	std::string id = msg->PeerId();
	std::string rid = msg->rid;

#ifdef RANK_DEBUG
	std::cerr << "p3Ranking::addRankMsg() Item:";
	std::cerr << std::endl;
	msg->print(std::cerr, 10);
	std::cerr << std::endl;
#endif

     	RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/

	std::map<std::string, RankGroup>::iterator it;
	it = mData.find(rid);
	if (it == mData.end())
	{
		/* add a new one */
		RankGroup grp;
		grp.rid = rid;
		grp.ownTag = false;
		grp.rank = 0.0f;

/*************************************************************************/
/****************************** LINK SPECIFIC ****************************/
/*************************************************************************/

		grp.link = msg->link;
		grp.title = msg->title;

/*************************************************************************/
/****************************** LINK SPECIFIC ****************************/
/*************************************************************************/

		mData[rid] = grp;
		it = mData.find(rid);

		if (id == "")
		{
#ifdef RANK_DEBUG
			std::cerr << "p3Ranking::addRankMsg() New Anon Link: mUpdated = true";
			std::cerr << std::endl;
#endif
			locked_reSortGroup(it->second);
			mUpdated = true;
		}
	}

	/**** If it is an anonymous Link (ie Friend of a Friend) Drop out now ***/
	if (id == "")
	{
		return;
	}

	/* check for old comment */
	std::map<std::string, RsRankLinkMsg *>::iterator cit;
	cit = (it->second).comments.find(id);

	/* Check that it is different! */
	bool newComment = false;
	if ((it->second).comments.end() == cit)
	{
		newComment = true;
	}
	else 
	{
		RsRankLinkMsg *old = cit->second;
		if ((msg->timestamp != old->timestamp) ||
		 	(msg->comment != old->comment))
		{
			newComment = true;
		}
	}

	if (newComment)
	{
#ifdef RANK_DEBUG
		std::cerr << "p3Ranking::addRankMsg() New Comment";
		std::cerr << std::endl;
#endif
		/* clean up old */
		if ((it->second).comments.end() != cit)
		{
			delete (cit->second);
			(it->second).comments.erase(cit);
		}

		/* add in */
		(it->second).comments[id] = msg;

		/* republish? */
		if (id == mOwnId)
		{
			it->second.ownTag = true;
			mRepublish = true;
#ifdef RANK_DEBUG
			std::cerr << "p3Ranking::addRankMsg() Own Comment: mRepublish = true";
			std::cerr << std::endl;
#endif
		}
		else
		{
			mRepublishFriends = true;
#ifdef RANK_DEBUG
			std::cerr << "p3Ranking::addRankMsg() Other Comment: mRepublishFriends = true";
			std::cerr << "p3Ranking::addRankMsg() Old Comment ignoring";
			std::cerr << std::endl;
#endif
		}

		locked_reSortGroup(it->second);

		mUpdated = true;
	}
	else
	{
		delete msg;
#ifdef RANK_DEBUG
		std::cerr << "p3Ranking::addRankMsg() Old Comment ignoring";
		std::cerr << std::endl;
#endif
	}
}


/***************** Sorting ****************/

bool p3Ranking::setSortPeriod(uint32_t period)
{
	bool reSort = false;

	{
     	  RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/
	  reSort = (mViewPeriod != period);
	  mViewPeriod = period;
	}


	if (reSort)
	{
		sortAllMsgs();
	}

	return true;
}

bool p3Ranking::setSortMethod(uint32_t type)
{
	bool reSort = false;

	{
     	  RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/
	  reSort = (mSortType != type);
	  mSortType = type;
	}

	if (reSort)
	{
		sortAllMsgs();
	}

	return true;
}

bool p3Ranking::clearPeerFilter()
{
	bool reSort = false;

	{
     	  RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/
	  reSort = (mPeerFilter.size() > 0);
	  mPeerFilter.clear();
	}


	if (reSort)
	{
		sortAllMsgs();
	}

	return true;
}

bool p3Ranking::setPeerFilter(std::list<std::string> peers)
{
	{
     	  RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/
	  mPeerFilter = peers;
	}

	sortAllMsgs();

	return true;
}

float 	p3Ranking::locked_calcRank(RankGroup &grp) 
{
	/* Ranking Calculations ..... 
	 */

	time_t now = time(NULL);
	time_t minTime = now-mViewPeriod;
	bool doFilter = (mPeerFilter.size() > 0);
	bool doScore = (mSortType & RS_RANK_SCORE);
	bool doTime  = (mSortType & RS_RANK_TIME); 

	uint32_t count = 0;
	float   algScore = 0;
	float   comboScore = 0;
	float   popScore = 0;

#ifdef RANK_DEBUG
	std::string normlink(grp.link.begin(), grp.link.end());
	std::cerr << "p3Ranking::locked_calcRank() for: " << normlink;
	std::cerr << std::endl;
	std::cerr << "Period: " << mViewPeriod;
	std::cerr << " doFilter: " << doFilter;
	std::cerr << " doScore: " << doScore;
	std::cerr << " doTime: " << doTime;
	std::cerr << std::endl;
#endif

	std::map<std::string, RsRankLinkMsg *>::iterator it;
	for(it = grp.comments.begin(); it != grp.comments.end(); it++)
	{
#ifdef RANK_DEBUG
	std::cerr << "Comment by:" << it->first << " age: " << now - it->second->timestamp;
	std::cerr << std::endl;
#endif
		if (doFilter)
		{
			if (mPeerFilter.end() == 
				std::find(mPeerFilter.begin(), mPeerFilter.end(), it->first))
			{
				continue; /* skip it */
#ifdef RANK_DEBUG
				std::cerr << "\tFiltered Out";
				std::cerr << std::endl;
#endif

			}
		}

		/* if Scoring is involved... drop old ones */
		if ((doScore) && ((time_t) it->second->timestamp < minTime))
		{
#ifdef RANK_DEBUG
			std::cerr << "\tToo Old";
			std::cerr << std::endl;
#endif
			continue;
		}

		time_t deltaT;
		if ((time_t) it->second->timestamp > now)
		{
			deltaT = it->second->timestamp - now;
		}
		else
		{
			deltaT = now - it->second->timestamp;
		}
		float timeScore = ((float) mViewPeriod - deltaT) / (mViewPeriod + 0.01);

#ifdef RANK_DEBUG
		std::cerr << "\tTimeScore: " << timeScore;
		std::cerr << std::endl;
#endif

		/* algScore is sum of (filtered) timeScores */
		/* timeScore is average of (all) timeScores */
		/* popScore is just count of valid scores */

		algScore += timeScore;
		count++;

		/* for more advanced scoring (where each peer gives a score +2 -> -2) */
		/* combo = SUM value * timeScore */
		/* time = same as before (average) */
		/* popScore = SUM value */

		float value = it->second->score;
		comboScore += value * timeScore;
		popScore += value;
	}

#ifdef RANK_DEBUG
	std::cerr << "p3Ranking::locked_calcRank() algScore: " << algScore;
	std::cerr << " Count: " << count;
	std::cerr << std::endl;
#endif

	if ((count <= 0) || (algScore <= 0))
	{
#ifdef RANK_DEBUG
		std::cerr << "Final score: 0";
		std::cerr << std::endl;
#endif
		return 0;
	}

	if ((doScore) && (doTime))
	{
#ifdef RANK_DEBUG
		std::cerr << "Old (alg) score:" << algScore;
		std::cerr << std::endl;
		std::cerr << "Final (Combo) score:" << comboScore;
		std::cerr << std::endl;
#endif

		if (comboScore < 0)
		{
#ifdef RANK_DEBUG
			std::cerr << "Combo score reset = 0";
			std::cerr << std::endl;
#endif
			comboScore = 0;
		}
		return comboScore;

	}
	else if (doScore)
	{
#ifdef RANK_DEBUG
		std::cerr << "Old (tally) score:" << count;
		std::cerr << std::endl;
		std::cerr << "Final (pop) score:" << popScore;
		std::cerr << std::endl;
#endif
		if (popScore < 0)
		{
#ifdef RANK_DEBUG
			std::cerr << "Pop score reset = 0";
			std::cerr << std::endl;
#endif
			popScore = 0;
		}
		return popScore;
	}
	else if (doTime)
	{
#ifdef RANK_DEBUG
		std::cerr << "Final (time) score:" << algScore / count;
		std::cerr << std::endl;
#endif
		return algScore / count;
	}
	return 0;
}


void	p3Ranking::locked_reSortGroup(RankGroup &grp)
{
	std::string rid = grp.rid;

	/* remove from existings rankings */
	std::multimap<float, std::string>::iterator rit;
	rit = mRankings.lower_bound(grp.rank);
	for(; (rit != mRankings.end()) && (rit->first == grp.rank); rit++)
	{
		if (rit->second == rid)
		{
			mRankings.erase(rit);
			break;
		}
	}

	/* add it back in */
	grp.rank = locked_calcRank(grp);
	mRankings.insert(
		std::pair<float, std::string>(grp.rank, rid));
}

void	p3Ranking::sortAllMsgs()
{
     	RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/

	/* iterate through list and re-score each one */
	std::map<std::string, RankGroup>::iterator it;

	mRankings.clear();

	for(it = mData.begin(); it != mData.end(); it++)
	{
		(it->second).rank = locked_calcRank(it->second);
		if (it->second.rank < 0)
		{
			it->second.rank = 0;
		}

		mRankings.insert(
			std::pair<float, std::string>
			(it->second.rank, it->first));
	}
}

/******** ACCESS *************/

        /* get Ids */
uint32_t p3Ranking::getRankingsCount()
{
     	RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/

	return mRankings.size();
}

float   p3Ranking::getMaxRank()
{
     	RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/

	if (mRankings.size() == 0)
		return 0;

	return mRankings.rbegin()->first;
}

bool    p3Ranking::getRankings(uint32_t first, uint32_t count, std::list<std::string> &rids)
{
     	RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/

#ifdef RANK_DEBUG
	std::cerr << "p3Ranking::getRankings() First: " << first << " Count: " << count;
	std::cerr << std::endl;
#endif

	uint32_t i = 0;
	std::multimap<float, std::string>::reverse_iterator rit;
	for(rit = mRankings.rbegin(); (i < first) && (rit != mRankings.rend()); rit++, i++);

	i = 0;
	for(; (i < count) && (rit != mRankings.rend()); rit++, i++)
	{
		rids.push_back(rit->second);
	}
	return true;
}


bool    p3Ranking::getRankDetails(std::string rid, RsRankDetails &details)
{
     	RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/

	/* get the details. */

	std::map<std::string, RankGroup>::iterator it;
	it = mData.find(rid);
	if (mData.end() == it)
	{
		return false;
	}

/*************************************************************************/
/****************************** LINK SPECIFIC ****************************/
/*************************************************************************/

	details.rid = it->first;
	details.link = (it->second).link;
	details.title = (it->second).title;
	details.rank = (it->second).rank;
	details.ownTag = (it->second).ownTag;

/*************************************************************************/
/****************************** LINK SPECIFIC ****************************/
/*************************************************************************/

	std::map<std::string, RsRankLinkMsg *>::iterator cit;
	for(cit = (it->second).comments.begin();
		cit != (it->second).comments.end(); cit++)
	{
		RsRankComment comm;
		comm.id = (cit->second)->PeerId();
		comm.timestamp = (cit->second)->timestamp;
		comm.comment = (cit->second)->comment;
		comm.score = (cit->second)->score;

		details.comments.push_back(comm);
	}

	return true;
}


void	p3Ranking::tick()
{
	bool repub = false;
	bool repubFriends = false;

	{
     		RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/
		repub = mRepublish;
		repubFriends = mRepublishFriends && (time(NULL) > mRepublishFriendTS);
	}

	if (repub)
	{
		publishMsgs(true);

     		RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/
		mRepublish = false;
	}


	if (repubFriends)
	{
		publishMsgs(false);

     		RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/
		mRepublishFriends = false;
		mRepublishFriendTS = time(NULL) + FRIEND_RANK_REPUBLISH_PERIOD;
	}

		
}

bool 	p3Ranking::updated()
{
     	RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/

	if (mUpdated)
	{
		mUpdated = false;
		return true;
	}
	return false;
}

/***** NEW CONTENT *****/
/*************************************************************************/
/****************************** LINK SPECIFIC ****************************/
/*************************************************************************/
std::string p3Ranking::newRankMsg(std::wstring link, std::wstring title, std::wstring comment, int32_t score)
{
	/* generate an id */
	std::string rid = generateRandomLinkId();

	RsRankLinkMsg *msg = new RsRankLinkMsg();

	time_t now = time(NULL);

	{
     		RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/
		msg->PeerId(mOwnId);
		msg->pid = mOwnId;
	}

	msg->rid = rid;
	msg->title = title;
	msg->timestamp = now;
	msg->comment = comment;
	msg->score = score;

	msg->linktype =  RS_LINK_TYPE_WEB;
	msg->link = link;


	addRankMsg(msg);

	return rid;
}


/*************************************************************************/
/****************************** LINK SPECIFIC ****************************/
/*************************************************************************/
bool p3Ranking::updateComment(std::string rid, std::wstring comment, int32_t score)
{

#ifdef RANK_DEBUG
	std::cerr << "p3Ranking::updateComment() rid:" << rid;
	std::cerr << std::endl;
#endif
	RsRankLinkMsg *msg = NULL;

     {  RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/

	std::map<std::string, RankGroup>::iterator it;
	it = mData.find(rid);
	if (it == mData.end())
	{
		/* missing group -> fail */

#ifdef RANK_DEBUG
		std::cerr << "p3Ranking::updateComment() Failed - noData";
		std::cerr << std::endl;
#endif
		return false;
	}

	msg = new RsRankLinkMsg();

	time_t now = time(NULL);

	msg->PeerId(mOwnId);
	msg->pid = mOwnId;
	msg->rid = rid;
	msg->timestamp = now;
	msg->title = (it->second).title;
	msg->comment = comment;
	msg->score = score;

	msg->linktype =  RS_LINK_TYPE_WEB;
	msg->link =  (it->second).link;

     }  /********** STACK UNLOCKED MTX ******/

#ifdef RANK_DEBUG
	std::cerr << "p3Ranking::updateComment() Item:";
	std::cerr << std::endl;
	msg->print(std::cerr, 10);
	std::cerr << std::endl;
#endif

	addRankMsg(msg);
	return true;
}

/*************************************************************************/
/****************************** LINK SPECIFIC ****************************/
/*************************************************************************/
std::string p3Ranking::anonRankMsg(std::string rid, std::wstring link, std::wstring title)
{
	bool alreadyExists = true;

	if (rid == "")
	{
		alreadyExists = false;
		/* generate an id */
		rid = generateRandomLinkId();
	}

	RsRankLinkMsg *msg1 = new RsRankLinkMsg();
	RsRankLinkMsg *msg2 = new RsRankLinkMsg();

	time_t now = time(NULL);

	{
     		RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/
		msg1->PeerId("");
		msg1->pid = "";

		msg2->PeerId("");
		msg2->pid = "";
	}

	msg1->rid = rid;
	msg1->title = title;
	msg1->timestamp = now;
	msg1->comment.clear();
	msg1->score = 0;

	msg1->linktype =  RS_LINK_TYPE_WEB;
	msg1->link = link;

	msg2->rid = rid;
	msg2->title = title;
	msg2->timestamp = now;
	msg2->comment.clear();
	msg2->score = 0;

	msg2->linktype =  RS_LINK_TYPE_WEB;
	msg2->link = link;

	if (alreadyExists)
	{
		delete msg1;
	}
	else
	{
		addRankMsg(msg1);
	}

	addAnonToList(msg2);

	return rid;
}



pqistore *createStore(std::string file, std::string src, bool reading)
{

	RsSerialiser *rsSerialiser = new RsSerialiser();
	rsSerialiser->addSerialType(new RsRankSerialiser());

	uint32_t bioflags = BIN_FLAGS_HASH_DATA;
	if (reading)
	{
		bioflags |= BIN_FLAGS_READABLE;
	}
	else
	{
		bioflags |= BIN_FLAGS_WRITEABLE;
	}

	/* bin flags: READ | WRITE | HASH_DATA */
	BinInterface *bio = new BinFileInterface(file.c_str(), bioflags);
	/* store flags: NO_DELETE (yes) | NO_CLOSE (no) */
	pqistore *store = new pqistore(rsSerialiser, src, bio, BIN_FLAGS_NO_DELETE | (bioflags & BIN_FLAGS_WRITEABLE));

	return store;
}

std::string generateRandomLinkId()
{
	std::ostringstream out;
	out << std::hex;
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
	/* 4 bytes per random number: 4 x 4 = 16 bytes */
	for(int i = 0; i < 4; i++)
	{
		out << std::setw(8) << std::setfill('0');
		uint32_t rint = random();
		out << rint;
	}
#else
	srand(time(NULL));
	/* 2 bytes per random number: 8 x 2 = 16 bytes */
	for(int i = 0; i < 8; i++)
	{
		out << std::setw(4) << std::setfill('0');
		uint16_t rint = rand(); /* only gives 16 bits */
		out << rint;
	}
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
	return out.str();
}
	

/*************************************************************************/
/****************************** LINK SPECIFIC ****************************/
/*************************************************************************/
void	p3Ranking::createDummyData()
{
	RsRankLinkMsg *msg = new RsRankLinkMsg();

	time_t now = time(NULL);

	msg->PeerId(mOwnId);
	msg->pid = mOwnId;
	msg->rid = "0001";
	msg->title = L"Original Awesome Site!";
	msg->timestamp = now - 60 * 60 * 24 * 15;
	msg->link = L"http://www.retroshare.org";
	msg->comment = L"Retroshares Website";
	msg->score = 1;

	addRankMsg(msg);

	msg = new RsRankLinkMsg();
	msg->PeerId(mOwnId);
	msg->pid = mOwnId;
	msg->rid = "0002";
	msg->title = L"Awesome Site!";
	msg->timestamp = now - 123;
	msg->link = L"http://www.lunamutt.org";
	msg->comment = L"Lunamutt's Website";
	msg->score = 1;

	addRankMsg(msg);

	msg = new RsRankLinkMsg();
	msg->PeerId("ALTID");
	msg->pid = "ALTID";
	msg->rid = "0002";
	msg->title = L"Awesome Site!";
	msg->timestamp = now - 60 * 60 * 24 * 29;
	msg->link = L"http://www.lunamutt.org";
	msg->comment = L"Lunamutt's Website (TWO) How Long can this comment be!\n";
	msg->comment += L"What happens to the second line?\n";
	msg->comment += L"And a 3rd!";
	msg->score = 1;

	addRankMsg(msg);

	msg = new RsRankLinkMsg();
	msg->PeerId("ALTID2");
	msg->pid = "ALTID2";
	msg->rid = "0002";
	msg->title = L"Awesome Site!";
	msg->timestamp = now - 60 * 60 * 7;
	msg->link = L"http://www.lunamutt.org";
	msg->comment += L"A Short Comment";
	msg->score = 1;

	addRankMsg(msg);


	/***** Third one ****/

	msg = new RsRankLinkMsg();
	msg->PeerId(mOwnId);
	msg->pid = mOwnId;
	msg->rid = "0003";
	msg->title = L"Weird Site!";
	msg->timestamp = now - 60 * 60;
	msg->link = L"http://www.lunamutt.com";
	msg->comment = L"";
	msg->score = 1;

	addRankMsg(msg);

	msg = new RsRankLinkMsg();
	msg->PeerId("ALTID");
	msg->pid = "ALTID";
	msg->rid = "0003";
	msg->title = L"Weird Site!";
	msg->timestamp = now - 60 * 60 * 24 * 2;
	msg->link = L"http://www.lunamutt.com";
	msg->comment = L"";
	msg->score = 1;

	addRankMsg(msg);

}


/***************************************************************************/
/****************************** CONFIGURATION HANDLING *********************/
/***************************************************************************/

/**** Store Anon Links: OVERLOADED FROM p3Config ****/

RsSerialiser *p3Ranking::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser();

	/* add in the types we need! */
	rss->addSerialType(new RsRankSerialiser());
	return rss;
}

bool	p3Ranking::addAnonToList(RsRankLinkMsg *msg)
{
	{
     	  RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/
	  std::list<RsRankLinkMsg *>::iterator it;
	  for(it = mAnon.begin(); it != mAnon.end(); it++)
	  {
	  	if (msg->rid == (*it)->rid)
			break;
	  }

	  if (it != mAnon.end())
	  {
	  	delete msg;
		return false;
	  }
	  
	  mAnon.push_back(msg);
	  mRepublishFriends = true;
	}

	IndicateConfigChanged(); /**** INDICATE CONFIG CHANGED! *****/
	return true;
}

std::list<RsItem *> p3Ranking::saveList(bool &cleanup)
{
	std::list<RsItem *> saveData;

	mRankMtx.lock(); /*********************** LOCK *******/

	cleanup = false;

	std::list<RsRankLinkMsg *>::iterator it;
	for(it = mAnon.begin(); it != mAnon.end(); it++)
	{
		saveData.push_back(*it);
	}

	/* list completed! */
	return saveData;
}

void    p3Ranking::saveDone()
{ 
	mRankMtx.unlock(); /*********************** UNLOCK *******/
	return; 
}

bool p3Ranking::loadList(std::list<RsItem *> load)
{
	std::list<RsItem *>::iterator it;
	RsRankLinkMsg *msg;

#ifdef SERVER_DEBUG 
	std::cerr << "p3Ranking::loadList() Item Count: " << load.size();
	std::cerr << std::endl;
#endif

	time_t now = time(NULL);
	time_t min, max;

     { 	RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/

	min = now - mStorePeriod;
	max = now + RANK_MAX_FWD_OFFSET;

     } 	/********** STACK LOCKED MTX ******/

	for(it = load.begin(); it != load.end(); it++)
	{
		/* switch on type */
		if (NULL != (msg = dynamic_cast<RsRankLinkMsg *>(*it)))
		{
			/* check date -> if old expire */
			if (((time_t) msg->timestamp < min) || 
				((time_t) msg->timestamp > max))
			{
#ifdef RANK_DEBUG
				std::cerr << "p3Ranking::loadList() Outside TimeRange (deleting Own Anon):";
				std::cerr << std::endl;
#endif
				/* if outside range -> remove */
				delete msg;
				continue;
			}

#ifdef RANK_DEBUG
			std::cerr << "p3Ranking::loadList() Anon TimeRange ok";
			std::cerr << std::endl;
#endif
			msg->PeerId("");
			msg->pid = "";

			RsRankLinkMsg *msg2 = new RsRankLinkMsg();
			msg2->clear();
			msg2->PeerId(msg->PeerId());
			msg2->pid = msg->pid;
			msg2->rid = msg->rid;
			msg2->title = msg->title;
			msg2->timestamp = msg->timestamp;
			msg2->comment.clear();
			msg2->score = 0;

			msg2->linktype =  msg->linktype;
			msg2->link = msg->link;

			/* make a copy to add into standard map */
			addRankMsg(msg);

     		 	RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/
			mAnon.push_back(msg2);
		}
		else
		{
			/* cleanup */
			delete (*it);
		}
	}

	return true;

}

