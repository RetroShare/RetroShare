/*******************************************************************************
 * libretroshare/src/retroshare: rsgxscircles.h                                *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012  Robert Fernie <retroshare@lunamutt.com>                 *
 * Copyright (C) 2018  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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
#pragma once

#include <cstdint>
#include <string>
#include <list>
#include <set>

#include "retroshare/rstypes.h"
#include "retroshare/rstokenservice.h"
#include "retroshare/rsgxsifacehelper.h"
#include "retroshare/rsidentity.h"
#include "serialiser/rsserializable.h"


class RsGxsCircles;

/**
 * Pointer to global instance of RsGxsCircles service implementation
 * @jsonapi{development}
 */
extern RsGxsCircles* rsGxsCircles;


// TODO: convert to enum
/// The meaning of the different circle types is:
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


struct RsGxsCircleGroup : RsSerializable
{
	virtual ~RsGxsCircleGroup() {}

	RsGroupMetaData mMeta;

	std::set<RsPgpId> mLocalFriends;
	std::set<RsGxsId> mInvitedMembers;
	std::set<RsGxsCircleId> mSubCircles;
#ifdef V07_NON_BACKWARD_COMPATIBLE_CHANGE_UNNAMED
#	error "Add description, and multiple owners/administrators to circles"
	// or better in general to GXS groups
#endif

	/// @see RsSerializable
	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx) override
	{
		RS_SERIAL_PROCESS(mMeta);
		RS_SERIAL_PROCESS(mLocalFriends);
		RS_SERIAL_PROCESS(mInvitedMembers);
		RS_SERIAL_PROCESS(mSubCircles);
	}
};

struct RsGxsCircleMsg : RsSerializable
{
	virtual ~RsGxsCircleMsg() {}

	RsMsgMetaData mMeta;

#ifndef V07_NON_BACKWARD_COMPATIBLE_CHANGE_UNNAMED
	/* This is horrible and should be changed into yet to be defined something
	 * reasonable in next non retrocompatible version */
	std::string stuff;
#endif

	/// @see RsSerializable
	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx) override
	{
		RS_SERIAL_PROCESS(mMeta);
		RS_SERIAL_PROCESS(stuff);
	}
};

struct RsGxsCircleDetails : RsSerializable
{
	RsGxsCircleDetails() :
	    mCircleType(GXS_CIRCLE_TYPE_EXTERNAL), mAmIAllowed(false) {}
	~RsGxsCircleDetails() {}

	RsGxsCircleId mCircleId;
	std::string mCircleName;

	uint32_t mCircleType;
	RsGxsCircleId mRestrictedCircleId;

	/** true when one of load GXS ids belong to the circle allowed list (admin
	 * list & subscribed list). */
	bool mAmIAllowed;

	/// This crosses admin list and subscribed list
	std::set<RsGxsId> mAllowedGxsIds;
	std::set<RsPgpId> mAllowedNodes;

	/// subscription flags for all ids
	std::map<RsGxsId,uint32_t> mSubscriptionFlags;

	/// @see RsSerializable
	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx) override
	{
		RS_SERIAL_PROCESS(mCircleId);
		RS_SERIAL_PROCESS(mCircleName);
		RS_SERIAL_PROCESS(mCircleType);
		RS_SERIAL_PROCESS(mRestrictedCircleId);
		RS_SERIAL_PROCESS(mAmIAllowed);
		RS_SERIAL_PROCESS(mAllowedGxsIds);
		RS_SERIAL_PROCESS(mAllowedNodes);
		RS_SERIAL_PROCESS(mSubscriptionFlags);
	}
};

class RsGxsCircles: public RsGxsIfaceHelper
{
public:

	RsGxsCircles(RsGxsIface& gxs) : RsGxsIfaceHelper(gxs) {}
	virtual ~RsGxsCircles() {}

	/**
	 * @brief Create new circle
	 * @jsonapi{development}
	 * @param[inout] cData input name and flags of the circle, storage for
	 *	generated circle data id etc.
	 * @return false if something failed, true otherwhise
	 */
	virtual bool createCircle(RsGxsCircleGroup& cData) = 0;

	/**
	 * @brief Edit own existing circle
	 * @jsonapi{development}
	 * @param[inout] cData Circle data with modifications, storage for data
	 *	updatedad during the operation.
	 * @return false if something failed, true otherwhise
	 */
	virtual bool editCircle(RsGxsCircleGroup& cData) = 0;

	/**
	 * @brief Get circle details. Memory cached
	 * @jsonapi{development}
	 * @param[in] id Id of the circle
	 * @param[out] details Storage for the circle details
	 * @return false if something failed, true otherwhise
	 */
	virtual bool getCircleDetails(
	        const RsGxsCircleId& id, RsGxsCircleDetails& details ) = 0;

	/**
	 * @brief Get list of known external circles ids. Memory cached
	 * @jsonapi{development}
	 * @param[in] circleIds Storage for circles id list
	 * @return false if something failed, true otherwhise
	 */
	virtual bool getCircleExternalIdList(
	        std::list<RsGxsCircleId>& circleIds ) = 0;

	/**
	 * @brief Get circles summaries list.
	 * @jsonapi{development}
	 * @param[out] circles list where to store the circles summaries
	 * @return false if something failed, true otherwhise
	 */
	virtual bool getCirclesSummaries(std::list<RsGroupMetaData>& circles) = 0;

	/**
	 * @brief Get circles information
	 * @jsonapi{development}
	 * @param[in] circlesIds ids of the circles of which to get the informations
	 * @param[out] circlesInfo storage for the circles informations
	 * @return false if something failed, true otherwhise
	 */
	virtual bool getCirclesInfo(
	        const std::list<RsGxsGroupId>& circlesIds,
	        std::vector<RsGxsCircleGroup>& circlesInfo ) = 0;

	/**
	 * @brief Get circle requests
	 * @jsonapi{development}
	 * @param[in] circleId id of the circle of which the requests are requested
	 * @param[out] requests storage for the circle requests
	 * @return false if something failed, true otherwhise
	 */
	virtual bool getCircleRequests( const RsGxsGroupId& circleId,
	                                std::vector<RsGxsCircleMsg>& requests ) = 0;

	/**
	 * @brief Invite identities to circle
	 * @jsonapi{development}
	 * @param[in] identities ids of the identities to invite
	 * @param[in] circleId Id of the circle you own and want to invite ids in
	 * @return false if something failed, true otherwhise
	 */
	virtual bool inviteIdsToCircle( const std::set<RsGxsId>& identities,
	                                const RsGxsCircleId& circleId ) = 0;

	/**
	 * @brief Request circle membership, or accept circle invitation
	 * @jsonapi{development}
	 * @param[in] ownGxsId Id of own identity to introduce to the circle
	 * @param[in] circleId Id of the circle to which ask for inclusion
	 * @return false if something failed, true otherwhise
	 */
	virtual bool requestCircleMembership(
	        const RsGxsId& ownGxsId, const RsGxsCircleId& circleId ) = 0;

	/**
	 * @brief Leave given circle
	 * @jsonapi{development}
	 * @param[in] ownGxsId Own id to remove from the circle
	 * @param[in] circleId Id of the circle to leave
	 * @return false if something failed, true otherwhise
	 */
	virtual bool cancelCircleMembership(
	        const RsGxsId& ownGxsId, const RsGxsCircleId& circleId ) = 0;

	RS_DEPRECATED_FOR("getCirclesSummaries getCirclesInfo")
	virtual bool getGroupData(
	        const uint32_t& token, std::vector<RsGxsCircleGroup>& groups ) = 0;

	RS_DEPRECATED_FOR(getCirclesRequests)
	virtual bool getMsgData(
	        const uint32_t& token, std::vector<RsGxsCircleMsg>& msgs ) = 0;

	/// make new group
	RS_DEPRECATED_FOR(createCircle)
	virtual void createGroup(uint32_t& token, RsGxsCircleGroup &group) = 0;

	/// update an existing group
	RS_DEPRECATED_FOR("editCircle, inviteIdsToCircle")
	virtual void updateGroup(uint32_t &token, RsGxsCircleGroup &group) = 0;
};
