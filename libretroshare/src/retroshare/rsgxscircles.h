/*******************************************************************************
 * libretroshare/src/retroshare: rsgxscircles.h                                *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012-2014  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2018-2019  Gioacchino Mazzurco <gio@eigenlab.org>             *
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
#include "util/rsmemory.h"


class RsGxsCircles;

/**
 * Pointer to global instance of RsGxsCircles service implementation
 * @jsonapi{development}
 */
extern RsGxsCircles* rsGxsCircles;

enum class RsGxsCircleType : uint32_t // 32 bit overkill, just for retrocompat
{
	UNKNOWN           = 0, /// Used to detect uninizialized values.
	PUBLIC            = 1, /// Public distribution
	EXTERNAL          = 2, /// Restricted to an external circle

	/** Restricted to a group of friend nodes, the administrator of the circle
	 * behave as a hub for them */
	NODES_GROUP        = 3,

	LOCAL              = 4, /// not distributed at all

	/** Self-restricted. Used only at creation time of self-restricted circles
	 *  when the circle id isn't known yet. Once the circle id is known the type
	 *  is set to EXTERNAL, and the external circle id is set to the id of the
	 *  circle itself.
	 */
	EXT_SELF           = 5,

	/// distributed to nodes signed by your own PGP key only.
	YOUR_EYES_ONLY     = 6
};

// TODO: convert to enum class
static const uint32_t GXS_EXTERNAL_CIRCLE_FLAGS_IN_ADMIN_LIST = 0x0001 ;// user is validated by circle admin
static const uint32_t GXS_EXTERNAL_CIRCLE_FLAGS_SUBSCRIBED    = 0x0002 ;// user has subscribed the group
static const uint32_t GXS_EXTERNAL_CIRCLE_FLAGS_KEY_AVAILABLE = 0x0004 ;// key is available, so we can encrypt for this circle
static const uint32_t GXS_EXTERNAL_CIRCLE_FLAGS_ALLOWED       = 0x0007 ;// user is allowed. Combines all flags above.


struct RsGxsCircleGroup : RsSerializable
{
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

	~RsGxsCircleGroup() override;
};

struct RsGxsCircleMsg : RsSerializable
{
	RsMsgMetaData mMeta;

#ifndef V07_NON_BACKWARD_COMPATIBLE_CHANGE_UNNAMED
	/* This is horrible and should be changed into yet to be defined something
	 * reasonable in next non-retrocompatible version */
	std::string stuff;
#endif

	/// @see RsSerializable
	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx) override
	{
		RS_SERIAL_PROCESS(mMeta);
		RS_SERIAL_PROCESS(stuff);
	}

	~RsGxsCircleMsg() override;
};

struct RsGxsCircleDetails : RsSerializable
{
	RsGxsCircleDetails() :
	    mCircleType(static_cast<uint32_t>(RsGxsCircleType::EXTERNAL)),
	    mAmIAllowed(false),mAmIAdmin(false) {}
	~RsGxsCircleDetails() override;

	RsGxsCircleId mCircleId;
	std::string mCircleName;

	uint32_t mCircleType;
	RsGxsCircleId mRestrictedCircleId;

	/** true when one of load GXS ids belong to the circle allowed list (admin
	 * list & subscribed list). */
	bool mAmIAllowed;

    /// true when we're an administrator of the circle group, meaning that we can add/remove members from the invitee list.
	bool mAmIAdmin;

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
		RS_SERIAL_PROCESS(mAmIAdmin);
		RS_SERIAL_PROCESS(mAllowedGxsIds);
		RS_SERIAL_PROCESS(mAllowedNodes);
		RS_SERIAL_PROCESS(mSubscriptionFlags);
	}
};


enum class RsGxsCircleEventCode: uint8_t
{
	UNKNOWN                   = 0x00,

	/** mCircleId contains the circle id and mGxsId is the id requesting
	 * membership */
	CIRCLE_MEMBERSHIP_REQUEST = 0x01,

	/** mCircleId is the circle that invites me, and mGxsId is my own Id that is
	 * invited */
	CIRCLE_MEMBERSHIP_INVITE  = 0x02,

	/** mCircleId contains the circle id and mGxsId is the id dropping
	 * membership */
	CIRCLE_MEMBERSHIP_LEAVE   = 0x03,

	/// mCircleId contains the circle id and mGxsId is the id of the new member
	CIRCLE_MEMBERSHIP_JOIN    = 0x04,

	/** mCircleId contains the circle id and mGxsId is the id that was revoqued * by admin */
	CIRCLE_MEMBERSHIP_REVOQUED= 0x05,

	/** mCircleId contains the circle id */
	NEW_CIRCLE                = 0x06,

	/** no additional information. Simply means that the info previously from the cache has changed. */
	CACHE_DATA_UPDATED        = 0x07,

};

struct RsGxsCircleEvent: RsEvent
{
	RsGxsCircleEvent()
	    : RsEvent(RsEventType::GXS_CIRCLES),
	      mCircleEventType(RsGxsCircleEventCode::UNKNOWN) {}


	RsGxsCircleEventCode mCircleEventType;
	RsGxsCircleId mCircleId;
	RsGxsId mGxsId;

	///* @see RsEvent @see RsSerializable
	void serial_process(
	        RsGenericSerializer::SerializeJob j,
	        RsGenericSerializer::SerializeContext& ctx ) override
	{
		RsEvent::serial_process(j, ctx);
		RS_SERIAL_PROCESS(mCircleEventType);
		RS_SERIAL_PROCESS(mCircleId);
		RS_SERIAL_PROCESS(mGxsId);
	}

	~RsGxsCircleEvent() override;
};

class RsGxsCircles: public RsGxsIfaceHelper
{
public:

	RsGxsCircles(RsGxsIface& gxs) : RsGxsIfaceHelper(gxs) {}
	virtual ~RsGxsCircles();

	/**
	 * @brief Create new circle
	 * @jsonapi{development}
	 * @param[in] circleName String containing cirlce name
	 * @param[in] circleType Circle type
	 * @param[out] circleId Optional storage to output created circle id
	 * @param[in] restrictedId Optional id of a pre-existent circle that see the
	 *	created circle. Meaningful only if circleType == EXTERNAL, must be null
	 *	in all other cases.
	 * @param[in] authorId Optional author of the circle.
	 * @param[in] gxsIdMembers GXS ids of the members of the circle.
	 * @param[in] localMembers PGP ids of the members if the circle.
	 * @return false if something failed, true otherwhise
	 */
	virtual bool createCircle(
	        const std::string& circleName, RsGxsCircleType circleType,
	        RsGxsCircleId& circleId = RS_DEFAULT_STORAGE_PARAM(RsGxsCircleId),
	        const RsGxsCircleId& restrictedId = RsGxsCircleId(),
	        const RsGxsId& authorId = RsGxsId(),
	        const std::set<RsGxsId>& gxsIdMembers = std::set<RsGxsId>(),
	        const std::set<RsPgpId>& localMembers = std::set<RsPgpId>() ) = 0;

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
	 * @brief Get specific circle request
	 * @jsonapi{development}
	 * @param[in] circleId id of the circle of which the requests are requested
	 * @param[in] msgId id of the request
	 * @param[out] msg storage for the circle request
	 * @return false if something failed, true otherwhise
	 */
	virtual bool getCircleRequest(const RsGxsGroupId& circleId,
	                              const RsGxsMessageId& msgId,
	                              RsGxsCircleMsg& msg) =0;

	/**
	 * @brief Invite identities to circle (admin key is required)
	 * @jsonapi{development}
	 * @param[in] identities ids of the identities to invite
	 * @param[in] circleId Id of the circle you own and want to invite ids in
	 * @return false if something failed, true otherwhise
	 */
	virtual bool inviteIdsToCircle( const std::set<RsGxsId>& identities,
	                                const RsGxsCircleId& circleId ) = 0;

	/**
	 * @brief Remove identities from circle (admin key is required)
	 * @jsonapi{development}
	 * @param[in] identities ids of the identities to remove from the invite list
	 * @param[in] circleId Id of the circle you own and want to invite ids in
	 * @return false if something failed, true otherwhise
	 */
	virtual bool revokeIdsFromCircle( const std::set<RsGxsId>& identities,
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

	/// default base URL used for circle links @see exportCircleLink
	static const std::string DEFAULT_CIRCLE_BASE_URL;

	/// Circle link query field used to store circle name @see exportCircleLink
	static const std::string CIRCLE_URL_NAME_FIELD;

	/// Circle link query field used to store circle id @see exportCircleLink
	static const std::string CIRCLE_URL_ID_FIELD;

	/// Circle link query field used to store circle data @see exportCircleLink
	static const std::string CIRCLE_URL_DATA_FIELD;

	/**
	 * @brief Get link to a circle
	 * @jsonapi{development}
	 * @param[out] link storage for the generated link
	 * @param[in] circleId Id of the circle of which we want to generate a link
	 * @param[in] includeGxsData if true include the circle GXS group data so
	 *	the receiver can request circle membership even if the circle hasn't
	 *	propagated through GXS to her yet
	 * @param[in] baseUrl URL into which to sneak in the RetroShare circle link
	 *	radix, this is primarly useful to induce applications into making the
	 *	link clickable, or to disguise the RetroShare circle link into a
	 *	"normal" looking web link. If empty the circle data link will be
	 *	outputted in plain base64 format.
	 * @param[out] errMsg optional storage for error message, meaningful only in
	 *	case of failure
	 * @return false if something failed, true otherwhise
	 */
	virtual bool exportCircleLink(
	        std::string& link, const RsGxsCircleId& circleId,
	        bool includeGxsData = true,
	        const std::string& baseUrl = RsGxsCircles::DEFAULT_CIRCLE_BASE_URL,
	        std::string& errMsg = RS_DEFAULT_STORAGE_PARAM(std::string) ) = 0;

	/**
	 * @brief Import circle from full link
	 * @param[in] link circle link either in radix or link format
	 * @param[out] circleId optional storage for parsed circle id
	 * @param[out] errMsg optional storage for error message, meaningful only in
	 *	case of failure
	 * @return false if some error occurred, true otherwise
	 */
	virtual bool importCircleLink(
	        const std::string& link,
	        RsGxsCircleId& circleId = RS_DEFAULT_STORAGE_PARAM(RsGxsCircleId),
	        std::string& errMsg = RS_DEFAULT_STORAGE_PARAM(std::string) ) = 0;

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


/// @deprecated Used to detect uninizialized values.
RS_DEPRECATED_FOR("RsGxsCircleType::UNKNOWN")
static const uint32_t GXS_CIRCLE_TYPE_UNKNOWN           = 0x0000;

/// @deprecated not restricted to a circle
RS_DEPRECATED_FOR("RsGxsCircleType::PUBLIC")
static const uint32_t GXS_CIRCLE_TYPE_PUBLIC            = 0x0001;

/// @deprecated restricted to an external circle, made of RsGxsId
RS_DEPRECATED_FOR("RsGxsCircleType::EXTERNAL")
static const uint32_t GXS_CIRCLE_TYPE_EXTERNAL          = 0x0002;

/// @deprecated restricted to a subset of friend nodes of a given RS node given
/// by a RsPgpId list
RS_DEPRECATED_FOR("RsGxsCircleType::NODES_GROUP")
static const uint32_t GXS_CIRCLE_TYPE_YOUR_FRIENDS_ONLY = 0x0003;

/// @deprecated not distributed at all
RS_DEPRECATED_FOR("RsGxsCircleType::LOCAL")
static const uint32_t GXS_CIRCLE_TYPE_LOCAL             = 0x0004;

/// @deprecated self-restricted. Not used, except at creation time when the
/// circle ID isn't known yet. Set to EXTERNAL afterwards.
RS_DEPRECATED_FOR("RsGxsCircleType::EXT_SELF")
static const uint32_t GXS_CIRCLE_TYPE_EXT_SELF          = 0x0005;

/// @deprecated distributed to nodes signed by your own PGP key only.
RS_DEPRECATED_FOR("RsGxsCircleType::YOUR_EYES_ONLY")
static const uint32_t GXS_CIRCLE_TYPE_YOUR_EYES_ONLY    = 0x0006;
