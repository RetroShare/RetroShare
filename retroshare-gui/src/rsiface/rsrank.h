#ifndef RETROSHARE_RANKING_GUI_INTERFACE_H
#define RETROSHARE_RANKING_GUI_INTERFACE_H

/*
 * libretroshare/src/rsiface: rsrank.h
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

#include <inttypes.h>
#include <string>
#include <list>

/* The Main Interface Class - for information about your Peers */
class RsRanks;
extern RsRanks   *rsRanks;

class RsRankComment
{
	public:

	std::string id;
	std::wstring comment;
	time_t	     timestamp;
};
	
class RsRankDetails
{
	public:

	std::string rid;
	std::wstring link;
	std::wstring title;
	float rank;
	bool ownTag;

	std::list<RsRankComment> comments;
};

const uint32_t RS_RANK_SCORE		= 0x0001;
const uint32_t RS_RANK_TIME		= 0x0002;
const uint32_t RS_RANK_ALG		= 0x0003;

std::ostream &operator<<(std::ostream &out, const RsRankDetails &detail);

class RsRanks
{
	public:

	RsRanks()  { return; }
virtual ~RsRanks() { return; }

	/* Set Sort Methods */
virtual bool setSortPeriod(uint32_t period) 		= 0;
virtual bool setSortMethod(uint32_t type)		= 0;
virtual bool clearPeerFilter()				= 0;
virtual bool setPeerFilter(std::list<std::string> peers) = 0;

	/* get Ids */
virtual uint32_t getRankingsCount()			= 0;
virtual float   getMaxRank()				= 0;
virtual bool	getRankings(uint32_t first, uint32_t count, std::list<std::string> &rids) = 0;
virtual bool	getRankDetails(std::string rid, RsRankDetails &details) = 0;

	/* Add New Comment / Msg */
virtual std::string newRankMsg(std::wstring link, std::wstring title, std::wstring comment) = 0;
virtual bool updateComment(std::string rid, std::wstring comment) = 0;

};

#endif
