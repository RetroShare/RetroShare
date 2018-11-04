/*******************************************************************************
 * libretroshare/src/retroshare: rsgxscircles.h                                *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2012 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef RETROSHARE_GXSCIRCLES_INTERFACE_H
#define RETROSHARE_GXSCIRCLES_INTERFACE_H

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

/// The meaning of the different circle types is:
/// TODO: convert to enum
static const uint32_t GXS_CIRCLE_TYPE_UNKNOWN           = 0x0000 ;	/// Used to detect uninizialized values.
static const uint32_t GXS_CIRCLE_TYPE_PUBLIC            = 0x0001 ;	// not restricted to a circle
static const uint32_t GXS_CIRCLE_TYPE_EXTERNAL          = 0x0002 ;	// restricted to an external circle, made of RsGxsId
static const uint32_t GXS_CIRCLE_TYPE_YOUR_FRIENDS_ONLY = 0x0003 ;	// restricted to a subset of friend nodes of a given RS node given by a RsPgpId list
static const uint32_t GXS_CIRCLE_TYPE_LOCAL             = 0x0004 ;	// not distributed at all
static const uint32_t GXS_CIRCLE_TYPE_EXT_SELF          = 0x0005 ;	// self-restricted. Not used, except at creation time when the circle ID isn't known yet. Set to EXTERNAL afterwards.
static const uint32_t GXS_CIRCLE_TYPE_YOUR_EYES_ONLY    = 0x0006 ;	// distributed to nodes signed by your own PGP key only.

static const uint32_t GXS_EXTERNAL_CIRCLE_FLAGS_IN_ADMIN_LIST = 0x0001 ;// user is validated by circle admin
static const uint32_t GXS_EXTERNAL_CIRCLE_FLAGS_SUBSCRIBED    = 0x0002 ;// user has subscribed the group
static const uint32_t GXS_EXTERNAL_CIRCLE_FLAGS_KEY_AVAILABLE = 0x0004 ;// key is available, so we can encrypt for this circle
static const uint32_t GXS_EXTERNAL_CIRCLE_FLAGS_ALLOWED       = 0x0007 ;// user is allowed. Combines all flags above.

static const uint32_t GXS_CIRCLE_FLAGS_IS_EXTERNAL   = 0x0008 ;// user is allowed

/* Permissions is part of GroupMetaData  */

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

    std::set<RsPgpId> mLocalFriends;
    std::set<RsGxsId> mInvitedMembers;
    std::set<RsGxsCircleId> mSubCircles;

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
    		RsGxsCircleDetails() : mCircleType(GXS_CIRCLE_TYPE_EXTERNAL), mAmIAllowed(false) {}
            
        RsGxsCircleId mCircleId;
        std::string mCircleName;

	uint32_t    mCircleType;
        RsGxsCircleId mRestrictedCircleId;
        
        bool mAmIAllowed ;			// true when one of load GXS ids belong to the circle allowed list (admin list & subscribed list).

        std::set<RsGxsId> mAllowedGxsIds;	// This crosses admin list and subscribed list
        std::set<RsPgpId> mAllowedNodes;
        
        std::map<RsGxsId,uint32_t> mSubscriptionFlags ;	// subscription flags for all ids
};

class RsGxsCircles: public RsGxsIfaceHelper
{
public:

	RsGxsCircles(RsGxsIface& gxs) :RsGxsIfaceHelper(gxs) {}
	virtual ~RsGxsCircles() {}

	/* External Interface (Cached stuff) */
	virtual bool getCircleDetails(const RsGxsCircleId &id, RsGxsCircleDetails &details) = 0;
	virtual bool getCircleExternalIdList(std::list<RsGxsCircleId> &circleIds) = 0;
	virtual bool getCirclePersonalIdList(std::list<RsGxsCircleId> &circleIds) = 0;

	/* membership management for external circles */

	virtual bool requestCircleMembership(const RsGxsId& own_gxsid,const RsGxsCircleId& circle_id)=0 ;
	virtual bool cancelCircleMembership(const RsGxsId& own_gxsid,const RsGxsCircleId& circle_id)=0 ;

	/* standard load */
	virtual bool getGroupData(const uint32_t &token, std::vector<RsGxsCircleGroup> &groups) = 0;
	virtual bool getMsgData(const uint32_t &token, std::vector<RsGxsCircleMsg> &msgs) = 0;

	/* make new group */
	virtual void createGroup(uint32_t& token, RsGxsCircleGroup &group) = 0;

	/* update an existing group */
	virtual void updateGroup(uint32_t &token, RsGxsCircleGroup &group) = 0;
};



#endif
