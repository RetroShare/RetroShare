/*******************************************************************************
 * libretroshare/src/unused: rsdsdv.h                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011 by Robert Fernie <retroshare@lunamutt.com>                   *
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
#ifndef RETROSHARE_DSDV_INTERFACE_H
#define RETROSHARE_DSDV_INTERFACE_H

#include <inttypes.h>
#include <string>
#include <list>

/* The Main Interface Class - for information about your Peers */
class RsDsdv;
extern RsDsdv *rsDsdv;


#define RSDSDV_IDTYPE_PEER	0x0001
#define RSDSDV_IDTYPE_SERVICE	0x0002
#define RSDSDV_IDTYPE_TEST	0x0100

#define RSDSDV_FLAGS_SIGNIFICANT_CHANGE         0x0001
#define RSDSDV_FLAGS_STABLE_ROUTE               0x0002
#define RSDSDV_FLAGS_NEW_ROUTE               	0x0004
#define RSDSDV_FLAGS_OWN_SERVICE                0x0008


class RsDsdvId
{
	public:

	uint32_t mIdType;
	std::string mAnonChunk;
	std::string mHash;
};

class RsDsdvRoute
{
	public:

	RsPeerId 	mNextHop;
	uint32_t 	mSequence;
	uint32_t 	mDistance;
	rstime_t   	mReceived;
	rstime_t   	mValidSince;

};

class RsDsdvTableEntry
{
	public:

	RsDsdvId	mDest;
	bool		mIsStable;
	RsDsdvRoute	mStableRoute;
	//RsDsdvRoute	mFreshestRoute;

	std::map<RsPeerId, RsDsdvRoute> mAllRoutes;

	uint32_t	mFlags;

	/* if we've matched it to someone */
	std::string mMatchedHash;
	bool mMatched;
	bool mOwnSource;
};

std::ostream &operator<<(std::ostream &out, const RsDsdvId &id);
std::ostream &operator<<(std::ostream &out, const RsDsdvRoute &route);
std::ostream &operator<<(std::ostream &out, const RsDsdvTableEntry &entry);


class RsDsdv
{
	public:

	RsDsdv()  { return; }
virtual ~RsDsdv() { return; }

virtual uint32_t getLocalServices(std::list<std::string> &hashes) = 0;
virtual uint32_t getAllServices(std::list<std::string> &hashes) = 0;
virtual int 	 getDsdvEntry(const std::string &hash, RsDsdvTableEntry &entry) = 0;

};

#endif

