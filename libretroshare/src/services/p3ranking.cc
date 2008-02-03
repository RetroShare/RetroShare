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


p3Ranking::p3Ranking(uint16_t subtype, CacheTransfer *cft,
		std::string sourcedir, std::string storedir, 
		uint32_t storePeriod)
	:CacheSource(subtype, true, sourcedir), 
	CacheStore(subtype, true, cft, storedir), 
	mStorePeriod(storePeriod)
{
	mOwnId = getAuthMgr()->OwnId();
	createDummyData();
	return;
}

bool    p3Ranking::loadLocalCache(const CacheData &data)
{

	return true;
}

int    p3Ranking::loadCache(const CacheData &data)
{

	return 1;
}

void p3Ranking::loadRankFile(std::string filename, std::string src)
{

#if 0
	/* create the serialiser to load info */
	pqistreamer *streamer = createStreamer(filename, src, BIN_FLAGS_READABLE);

	time_t now = time(NULL);
	time_t min = now - mStorePeriod;
	time_t max = now + RANK_MAX_FWD_OFFSET;

	RsItem *item;
	RsRankMsg *newMsg;
	while(NULL != (item = streamer->GetItem()))
	{
		newMsg = (RsRankMsg *) item;

		/* check timestamp */
		if ((newMsg->timestamp < min) || (newMsg->timestamp > max))
		{
			/* if outside range -> remove */
			delete newMsg;
		}
		else
		{
			addRankMsg(newMsg);
		}
	}
#endif
}

void p3Ranking::publishMsgs()
{

#if 0
	/* create a serialiser */
	std::string file;
	pqistreamer *stream = createStreamer(file, mOwnId, BIN_FLAGS_NO_DELETE | BIN_FLAGS_HASH_DATA);

	/* iterate through list */
	std::map<std::string, RankGroup>::iterator it;
	for(it = mData.begin(); it != mData.end(); it++)
	{
		if (it->second.ownTag)
		{
			/* write to serialiser */
			RsItem *item = it->second.comments[mOwnId];
			if (item)
				stream->SendItem(item);
		}
	}

	CacheData data;
	data.pid = mOwnId;
	data.cid = CacheId(CacheSource::getCacheType(), 0);
	data.name = file;

	refreshCache(data);
#endif
}



void p3Ranking::addRankMsg(RsRankMsg *msg)
{
	/* find msg */
	std::string id = msg->PeerId();
	std::string rid = msg->rid;

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
	std::map<std::string, RsRankMsg *>::iterator cit;
	cit = (it->second).comments.find(id);
	if ((it->second).comments.end() != cit)
	{
		(it->second).comments.erase(cit);
	}

	(it->second).comments[id] = msg;

	if (id == mOwnId)
	{
		it->second.ownTag = true;
		mRepublish = true;
	}

	reSortGroup(it->second);
}


/***************** Sorting ****************/

bool p3Ranking::setSortPeriod(uint32_t period)
{
	mViewPeriod = period;
	return true;
}

bool p3Ranking::setSortMethod(uint32_t type)
{
	mSortType = type;
	return true;
}

bool p3Ranking::clearPeerFilter()
{
	mPeerFilter.clear();
	return true;
}

bool p3Ranking::setPeerFilter(std::list<std::string> peers)
{
	mPeerFilter = peers;
	return true;
}

float 	p3Ranking::locked_calcRank(RankGroup &grp) /* returns 0->100 */
{

#if 0
	/* where all the work is done */
	bool doScore = (mSortType == 
	bool doTime  = (mSortType == 
	bool doFilter = (mPeerFilter.size() > 0);
	float rank = 0;

	for(it = grp.comments.begin(); it != grp.comments.end(); it++)
	{
		
	if (doFilter)
	{
		/* do first so we can discard */


	if (doScore)
	if (mSortType
#endif

	return 100;
}

void	p3Ranking::reSortGroup(RankGroup &grp)
{
	std::string rid = grp.rid;
	float rank = grp.rank;

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
	return mRankings.size();
}

float   p3Ranking::getMaxRank()
{
	if (mRankings.size() == 0)
		return 0;

	return mRankings.rbegin()->first;
}

bool    p3Ranking::getRankings(uint32_t first, uint32_t count, std::list<std::string> &rids)
{
	uint32_t i = 0;
	std::multimap<float, std::string>::iterator rit;
	for(rit = mRankings.begin(); (i < first) && (rit != mRankings.end()); rit++);

	i = 0;
	for(; (i < count) && (rit != mRankings.end()); rit++)
	{
		rids.push_back(rit->second);
	}
	return true;
}

bool    p3Ranking::getRankDetails(std::string rid, RsRankDetails &details)
{
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

	std::map<std::string, RsRankMsg *>::iterator cit;
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
	if (mRepublish)
	{
		publishMsgs();
		mRepublish = false;
	}
}




/***** NEW CONTENT *****/
std::string p3Ranking::newRankMsg(std::wstring link, std::wstring title, std::wstring comment)
{
	/* generate an id */
	std::string rid = generateRandomLinkId();

	RsRankMsg *msg = new RsRankMsg();

	time_t now = time(NULL);

	msg->PeerId(mOwnId);
	msg->rid = rid;
	msg->title = title;
	msg->timestamp = now;
	msg->link = link;
	msg->comment = comment;

	addRankMsg(msg);

	return rid;
}

bool p3Ranking::updateComment(std::string rid, std::wstring comment)
{
	return true;
}

pqistreamer *createStreamer(std::string file, std::string src, uint32_t bioflags)
{

#if 0
	RsSerialiser *rsSerialiser = new RsSerialiser();
	RsSerialType *serialType = new RsRankSerial(); /* TODO */

	rsSerialiser->addSerialType(serialType);

	BinInterface *bio = BinFileInterface(file.c_str(), bioflags);
	pqistreamer *streamer = new pqistreamer(rsSerialiser, src, bio, 0);

	return streamer;
#endif 
	return NULL;
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
	RsRankMsg *msg = new RsRankMsg();

	time_t now = time(NULL);

	msg->PeerId(mOwnId);
	msg->rid = "0001";
	msg->title = L"Original Awesome Site!";
	msg->timestamp = now - 12345;
	msg->link = L"http://www.retroshare.org";
	msg->comment = L"Retroshares Website";

	addRankMsg(msg);

	msg = new RsRankMsg();
	msg->PeerId(mOwnId);
	msg->rid = "0002";
	msg->title = L"Awesome Site!";
	msg->timestamp = now - 123;
	msg->link = L"http://www.lunamutt.org";
	msg->comment = L"Lunamutt's Website";

	addRankMsg(msg);

	msg = new RsRankMsg();
	msg->PeerId("ALTID");
	msg->rid = "0002";
	msg->title = L"Awesome Site!";
	msg->timestamp = now - 12345;
	msg->link = L"http://www.lunamutt.org";
	msg->comment = L"Lunamutt's Website (TWO) How Long can this comment be!\n";
	msg->comment += L"What happens to the second line?\n";
	msg->comment += L"And a 3rd!";

	addRankMsg(msg);
}

