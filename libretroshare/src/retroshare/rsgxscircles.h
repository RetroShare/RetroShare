#ifndef RETROSHARE_GXSCIRCLES_INTERFACE_H
#define RETROSHARE_GXSCIRCLES_INTERFACE_H

/*
 * libretroshare/src/retroshare: rsgxscircles.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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
#include <set>

#include "retroshare/rstypes.h"

//typedef std::string RsGxsCircleId;
//typedef RsPgpId RsPgpId;
//typedef std::string RsCircleInternalId;

#include "retroshare/rstokenservice.h"
#include "retroshare/rsgxsifacehelper.h"

#include "retroshare/rsidentity.h"


/* The Main Interface Class - for information about your Peers */
class RsGxsCircles;
extern RsGxsCircles *rsGxsCircles;

typedef RsPgpId RsPgpId;
//typedef RsGxsCircleId RsCircleInternalId;

#define GXS_CIRCLE_TYPE_PUBLIC            0x0001
#define GXS_CIRCLE_TYPE_EXTERNAL          0x0002
#define GXS_CIRCLE_TYPE_YOUREYESONLY      0x0003
#define GXS_CIRCLE_TYPE_LOCAL		  0x0004

// A special one - used only by Circles themselves - meaning Circle ID == Group ID.
#define GXS_CIRCLE_TYPE_EXT_SELF	  0x0005	

/* Permissions is part of GroupMetaData 
 */

class GxsPermissions
{
public:
	uint32_t mCircleType; // PUBLIC, EXTERNAL or YOUREYESONLY.      
	RsGxsCircleId mCircleId; // If EXTERNAL, otherwise Blank.

	// BELOW IS NOT SERIALISED - BUT MUST BE STORED LOCALLY BY GXS. (If YOUREYESONLY)
	RsPeerId mOriginator;
	RsGxsCircleId mInternalCircle; // if Originator == ownId, otherwise blank.
};


class RsGxsCircleGroup
{
	public:
	RsGroupMetaData mMeta; // includes GxsPermissions, for control of group distribution.

	std::list<RsPgpId> mLocalFriends;
	std::list<RsGxsId> mInvitedMembers;
	std::list<RsGxsCircleId> mSubCircles;

	// Not Serialised.
	// Internally inside rsCircles, this will be turned into:
	// std::list<RsPeerId> mAllowedFriends;
};

class RsGxsCircleMsg
{
	public:
	RsMsgMetaData mMeta;

	// Signature by user signifying that they want to be part of the group.
	// maybe Phase 3.
	std::string stuff;
};

class RsGxsCircleDetails
{
        public:
        RsGxsCircleId mCircleId;
        std::string mCircleName;

	uint32_t    mCircleType;
	bool 	    mIsExternal;

        std::set<RsGxsId> mUnknownPeers;
        std::map<RsPgpId, std::list<RsGxsId> > mAllowedPeers;
};




class RsGxsCircles: public RsGxsIfaceHelper
{
	public:

	RsGxsCircles(RsGxsIface *gxs)
	:RsGxsIfaceHelper(gxs)  { return; }
virtual ~RsGxsCircles() { return; }


	/* External Interface (Cached stuff) */
virtual bool getCircleDetails(const RsGxsCircleId &id, RsGxsCircleDetails &details) = 0;
virtual bool getCircleExternalIdList(std::list<RsGxsCircleId> &circleIds) = 0;
virtual bool getCirclePersonalIdList(std::list<RsGxsCircleId> &circleIds) = 0;

	/* standard load */
virtual bool getGroupData(const uint32_t &token, std::vector<RsGxsCircleGroup> &groups) = 0;

	/* make new group */
virtual bool createGroup(uint32_t& token, RsGxsCircleGroup &group) = 0;


};



#endif
