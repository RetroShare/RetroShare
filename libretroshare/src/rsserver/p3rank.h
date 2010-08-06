#ifndef RETROSHARE_P3_RANKING_INTERFACE_H
#define RETROSHARE_P3_RANKING_INTERFACE_H

/*
 * libretroshare/src/rsserver: p3rank.h
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

#include "retroshare/rsrank.h"
#include "services/p3ranking.h"

class p3Rank: public RsRanks
{
	public:

	p3Rank(p3Ranking *ranking);
virtual ~p3Rank(); 

        /* needs update? */
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

	private:

	p3Ranking *mRank;
};

#endif
