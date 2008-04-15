/*
 * libretroshare/src/rsserver: p3rank.cc
 *
 * RetroShare C++ Interface.
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

#include "rsserver/p3rank.h"

RsRanks *rsRanks = NULL;

p3Rank::p3Rank(p3Ranking *ranking)
	:mRank(ranking)
{
	return;
}

p3Rank::~p3Rank() 
{
	return; 
}
        /* needs update? */
bool p3Rank::updated()
{
	return mRank->updated();
}

        /* Set Sort Methods */
bool p3Rank::setSortPeriod(uint32_t period)
{
	return mRank->setSortPeriod(period);
}

bool p3Rank::setSortMethod(uint32_t type)
{
	return mRank->setSortMethod(type);
}

bool p3Rank::clearPeerFilter()
{
	return mRank->clearPeerFilter();
}

bool p3Rank::setPeerFilter(std::list<std::string> peers)
{
	return mRank->setPeerFilter(peers);
}

        /* get Ids */
uint32_t p3Rank::getRankingsCount()
{
	return mRank->getRankingsCount();
}

float   p3Rank::getMaxRank()
{
	return mRank->getMaxRank();
}

bool    p3Rank::getRankings(uint32_t first, uint32_t count, std::list<std::string> &rids)
{
	return mRank->getRankings(first, count, rids);
}

bool    p3Rank::getRankDetails(std::string rid, RsRankDetails &details)
{
	return mRank->getRankDetails(rid, details);
}


        /* Add New Comment / Msg */
std::string p3Rank::newRankMsg(std::wstring link, std::wstring title, std::wstring comment, int32_t score)
{
	return mRank->newRankMsg(link, title, comment, score);
}

bool p3Rank::updateComment(std::string rid, std::wstring comment, int32_t score)
{
	return mRank->updateComment(rid, comment, score);
}

std::string p3Rank::anonRankMsg(std::wstring link, std::wstring title)
{
	return mRank->anonRankMsg(link, title);
}

