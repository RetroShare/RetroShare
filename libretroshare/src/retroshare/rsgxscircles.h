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

#include "gxs/rstokenservice.h"
#include "gxs/rsgxsifaceimpl.h"

#include "retroshare/rsidentity.h"


/* The Main Interface Class - for information about your Peers */
class RsGxsCircles;
extern RsGxsCircles *rsGxsCircles;


typedef std::string RsGxsCircleId;
typedef std::string RsPeerId;	   // SSL ID.
typedef std::string RsPgpId;
typedef std::string RsCircleInternalId;

#define GXS_PERM_TYPE_PUBLIC            0x0001
#define GXS_PERM_TYPE_EXTERNAL          0x0002
#define GXS_PERM_TYPE_YOUREYESONLY      0x0003

/* Permissions is part of GroupMetaData 
 */

class GxsPermissions
{
public:
	uint32_t mCircleType; // PUBLIC, EXTERNAL or YOUREYESONLY.      
	RsGxsCircleId mCircleId; // If EXTERNAL, otherwise Blank.

	// BELOW IS NOT SERIALISED - BUT MUST BE STORED LOCALLY BY GXS. (If YOUREYESONLY)
	RsPeerId mOriginator;
	RsCircleInternalId mInternalCircle; // if Originator == ownId, otherwise blank.
};


class RsGxsCircleGroup
{
	public:
	RsGroupMetaData mMeta; // includes GxsPermissions, for control of group distribution.

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




class RsGxsCircles: public RsGxsIfaceImpl
{
	public:

	RsGxsCircles(RsGenExchange *gxs)
	:RsGxsIfaceImpl(gxs)  { return; }
virtual ~RsGxsCircles() { return; }

};



#endif
