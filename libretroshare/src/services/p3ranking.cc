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

#include "pqi/pqibin.h"
#include "pqi/p3authmgr.h"

const uint32_t RANK_MAX_FWD_OFFSET = (60 * 60 * 24 * 2); /* 2 Days */

std::string generateRandomLinkId();

/*****
 * TODO
 * (1) Streaming.
 * (2) Ranking.
 *
 */

#define RANK_DEBUG 1

p3Ranking::p3Ranking(uint16_t type, CacheTransfer *cft,
		std::string sourcedir, std::string storedir, 
		uint32_t storePeriod)
	:CacheSource(type, true, sourcedir), 
	CacheStore(type, true, cft, storedir), 
	mStorePeriod(storePeriod)
{

     { 	RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/

	mOwnId = getAuthMgr()->OwnId();
	mViewPeriod = 60 * 60 * 24 * 30; /* one Month */
	mSortType = RS_RANK_ALG;

     } 	RsStackMutex stack(mRankMtx); 

//	createDummyData();
	return;
}

bool    p3Ranking::loadLocalCache(const CacheData &data)
{
	/* ignore Local Cache -> just use remote caches */
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

	return 1;
}


void p3Ranking::loadRankFile(std::string filename, std::string src)
{
	/* create the serialiser to load info */
	RsSerialiser *rsSerialiser = new RsSerialiser();
	rsSerialiser->addSerialType(new RsRankSerialiser());
	
	uint32_t bioflags = BIN_FLAGS_HASH_DATA | BIN_FLAGS_READABLE;
	BinInterface *bio = new BinFileInterface(filename.c_str(), bioflags);
	pqistreamer *stream = new pqistreamer(rsSerialiser, src, bio, 0);
	
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

	stream->tick(); /* Tick to read! */
	while(NULL != (item = stream->GetItem()))
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
			addRankMsg(newMsg);
		}

		stream->tick(); /* Tick to read! */
	}

	delete stream;
}


void p3Ranking::publishMsgs()
{

#ifdef RANK_DEBUG
	std::cerr << "p3Ranking::publishMsgs()";
	std::cerr << std::endl;
#endif

	/* determine filename */

	std::string path = CacheSource::getCacheDir();
	std::ostringstream out;
	out << "rank-links-" << time(NULL) << ".rsrl";
	
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
	pqistreamer *stream = new pqistreamer(rsSerialiser, mOwnId, bio,
					BIN_FLAGS_NO_DELETE);
	
     { 	RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/

	/* iterate through list */
	std::map<std::string, RankGroup>::iterator it;
	for(it = mData.begin(); it != mData.end(); it++)
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
				stream->SendItem(item);
				stream->tick(); /* Tick to write! */

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

     } 	/********** STACK LOCKED MTX ******/


	stream->tick(); /* Tick for final write! */

	/* flag as new info */
	CacheData data;

     { 	RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/
	data.pid = mOwnId;
     } 	/********** STACK LOCKED MTX ******/

	data.cid = CacheId(CacheSource::getCacheType(), 1);

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

	delete stream;
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

	/******** LINK SPECIFIC ****/
		grp.link = msg->link;
		grp.title = msg->title;

		mData[rid] = grp;
		it = mData.find(rid);
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
		}

		locked_reSortGroup(it->second);
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
	}

#ifdef RANK_DEBUG
	std::cerr << "p3Ranking::locked_calcRank() algScore: " << algScore;
	std::cerr << " Count: " << count;
	std::cerr << std::endl;
#endif

	if ((count < 0) || (algScore < 0))
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
		std::cerr << "Final (alg) score:" << algScore;
		std::cerr << std::endl;
#endif
		return algScore;
	}
	else if (doScore)
	{
#ifdef RANK_DEBUG
		std::cerr << "Final (pop) score:" << count;
		std::cerr << std::endl;
#endif
		return count;
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
		if (it->second.rank > 0)
		{
			mRankings.insert(
				std::pair<float, std::string>
				(it->second.rank, it->first));
		}
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

	uint32_t i = 0;
	std::multimap<float, std::string>::reverse_iterator rit;
	for(rit = mRankings.rbegin(); (i < first) && (rit != mRankings.rend()); rit++);

	i = 0;
	for(; (i < count) && (rit != mRankings.rend()); rit++)
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

	details.rid = it->first;
	details.link = (it->second).link;
	details.title = (it->second).title;
	details.rank = (it->second).rank;
	details.ownTag = (it->second).ownTag;

	std::map<std::string, RsRankLinkMsg *>::iterator cit;
	for(cit = (it->second).comments.begin();
		cit != (it->second).comments.end(); cit++)
	{
		RsRankComment comm;
		comm.id = (cit->second)->PeerId();
		comm.timestamp = (cit->second)->timestamp;
		comm.comment = (cit->second)->comment;

		details.comments.push_back(comm);
	}

	return true;
}


void	p3Ranking::tick()
{
	bool repub = false;
	{
     		RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/
		repub = mRepublish;
	}

	if (repub)
	{
		publishMsgs();

     		RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/
		mRepublish = false;
	}
}




/***** NEW CONTENT *****/
std::string p3Ranking::newRankMsg(std::wstring link, std::wstring title, std::wstring comment)
{
	/* generate an id */
	std::string rid = generateRandomLinkId();

	RsRankLinkMsg *msg = new RsRankLinkMsg();

	time_t now = time(NULL);

	{
     		RsStackMutex stack(mRankMtx); /********** STACK LOCKED MTX ******/
		msg->PeerId(mOwnId);
	}

	msg->rid = rid;
	msg->title = title;
	msg->timestamp = now;
	msg->comment = comment;

	msg->linktype =  RS_LINK_TYPE_WEB;
	msg->link = link;

	addRankMsg(msg);

	return rid;
}

bool p3Ranking::updateComment(std::string rid, std::wstring comment)
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
	msg->rid = rid;
	msg->timestamp = now;
	msg->title = (it->second).title;
	msg->comment = comment;

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

pqistreamer *createStreamer(std::string file, std::string src, bool reading)
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
	/* streamer flags: NO_DELETE (yes) | NO_CLOSE (no) */
	pqistreamer *streamer = new pqistreamer(rsSerialiser, src, bio, 
						BIN_FLAGS_NO_DELETE);

	return streamer;
}

std::string generateRandomLinkId()
{
	std::ostringstream out;
	out << std::hex;
	
	/* 4 bytes per random number: 4 x 4 = 16 bytes */
	for(int i = 0; i < 4; i++)
	{
		uint32_t rint = random();
		out << rint;
	}
	return out.str();
}
	

void	p3Ranking::createDummyData()
{
	RsRankLinkMsg *msg = new RsRankLinkMsg();

	time_t now = time(NULL);

	msg->PeerId(mOwnId);
	msg->rid = "0001";
	msg->title = L"Original Awesome Site!";
	msg->timestamp = now - 60 * 60 * 24 * 15;
	msg->link = L"http://www.retroshare.org";
	msg->comment = L"Retroshares Website";

	addRankMsg(msg);

	msg = new RsRankLinkMsg();
	msg->PeerId(mOwnId);
	msg->rid = "0002";
	msg->title = L"Awesome Site!";
	msg->timestamp = now - 123;
	msg->link = L"http://www.lunamutt.org";
	msg->comment = L"Lunamutt's Website";

	addRankMsg(msg);

	msg = new RsRankLinkMsg();
	msg->PeerId("ALTID");
	msg->rid = "0002";
	msg->title = L"Awesome Site!";
	msg->timestamp = now - 60 * 60 * 24 * 29;
	msg->link = L"http://www.lunamutt.org";
	msg->comment = L"Lunamutt's Website (TWO) How Long can this comment be!\n";
	msg->comment += L"What happens to the second line?\n";
	msg->comment += L"And a 3rd!";

	addRankMsg(msg);

	msg = new RsRankLinkMsg();
	msg->PeerId("ALTID2");
	msg->rid = "0002";
	msg->title = L"Awesome Site!";
	msg->timestamp = now - 60 * 60 * 7;
	msg->link = L"http://www.lunamutt.org";
	msg->comment += L"A Short Comment";

	addRankMsg(msg);


	/***** Third one ****/

	msg = new RsRankLinkMsg();
	msg->PeerId(mOwnId);
	msg->rid = "0003";
	msg->title = L"Weird Site!";
	msg->timestamp = now - 60 * 60;
	msg->link = L"http://www.lunamutt.com";
	msg->comment = L"";

	addRankMsg(msg);

	msg = new RsRankLinkMsg();
	msg->PeerId("ALTID");
	msg->rid = "0003";
	msg->title = L"Weird Site!";
	msg->timestamp = now - 60 * 60 * 24 * 2;
	msg->link = L"http://www.lunamutt.com";
	msg->comment = L"";

	addRankMsg(msg);

}

