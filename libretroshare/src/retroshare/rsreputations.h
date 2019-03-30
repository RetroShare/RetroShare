/*******************************************************************************
 * libretroshare/src/retroshare: rsreputations.h                               *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2015  Cyril Soler <retroshare.team@gmail.com>                 *
 * Copyright (C) 2018  Gioacchino Mazzurco <gio@altermundi.net>                *
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

#include "retroshare/rsids.h"
#include "retroshare/rsgxsifacetypes.h"
#include "serialiser/rsserializable.h"

class RsReputations;

/**
 * Pointer to global instance of RsReputations service implementation
 * @jsonapi{development}
 */
extern RsReputations* rsReputations;


constexpr float RS_REPUTATION_THRESHOLD_DEFAULT = 1.0f;

enum struct RsOpinion : uint8_t
{
	NEGATIVE = 0,
	NEUTRAL = 1,
	POSITIVE = 2
};

enum struct RsReputationLevel : uint8_t
{
	/// local opinion is negative
	LOCALLY_NEGATIVE  = 0x00,

	/// local opinion is neutral and friends are positive in average
	REMOTELY_NEGATIVE = 0x01,

	/// no reputation information
	NEUTRAL           = 0x02,

	/// local opinion is neutral and friends are positive in average
	REMOTELY_POSITIVE = 0x03,

	/// local opinion is positive
	LOCALLY_POSITIVE  = 0x04,

	/// missing info
	UNKNOWN           = 0x05
};

struct RsReputationInfo : RsSerializable
{
	RsReputationInfo() :
	    mOwnOpinion(RsOpinion::NEUTRAL), mFriendsPositiveVotes(0),
	    mFriendsNegativeVotes(0),
	    mFriendAverageScore(RS_REPUTATION_THRESHOLD_DEFAULT),
	    mOverallReputationLevel(RsReputationLevel::NEUTRAL) {}
	virtual ~RsReputationInfo() {}

	RsOpinion mOwnOpinion;

	uint32_t mFriendsPositiveVotes;
	uint32_t mFriendsNegativeVotes;

	float mFriendAverageScore;

	/// this should help clients in taking decisions
	RsReputationLevel mOverallReputationLevel;

	/// @see RsSerializable
	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx ) override
	{
		RS_SERIAL_PROCESS(mOwnOpinion);
		RS_SERIAL_PROCESS(mFriendsPositiveVotes);
		RS_SERIAL_PROCESS(mFriendsNegativeVotes);
		RS_SERIAL_PROCESS(mFriendAverageScore);
		RS_SERIAL_PROCESS(mOverallReputationLevel);
	}
};


class RsReputations
{
public:
	virtual ~RsReputations() {}

	/**
	 * @brief Set own opinion about the given identity
	 * @jsonapi{development}
	 * @param[in] id Id of the identity
	 * @param[in] op Own opinion
	 * @return false on error, true otherwise
	 */
	virtual bool setOwnOpinion(const RsGxsId& id, RsOpinion op) = 0;

	/**
	 * @brief Get own opition about the given identity
	 * @jsonapi{development}
	 * @param[in] id Id of the identity
	 * @param[out] op Own opinion
	 * @return false on error, true otherwise
	 */
	virtual bool getOwnOpinion(const RsGxsId& id, RsOpinion& op) = 0;

	/**
	 * @brief Get reputation data of given identity
	 * @jsonapi{development}
	 * @param[in] id Id of the identity
	 * @param[in] ownerNode Optiona PGP id of the signed identity, accept a null
	 *	(all zero/noninitialized) PGP id
	 * @param[out] info storage for the information
	 * @param[in] stamp if true, timestamo the information
	 * @return false on error, true otherwise
	 */
	virtual bool getReputationInfo(
	        const RsGxsId& id, const RsPgpId& ownerNode, RsReputationInfo& info,
	        bool stamp = true ) = 0;

	/**
	 * @brief Get overall reputation level of given identity
	 * @jsonapi{development}
	 * @param[in] id Id of the identity
	 * @return the calculated reputation level based on available information
	 */
	virtual RsReputationLevel overallReputationLevel(const RsGxsId& id) = 0;

	/**
	 * @brief Enable giving automatic positive opinion when flagging as contact
	 * @jsonapi{development}
	 * @param[in] b true to enable, false to disable
	 */
	virtual void setAutoPositiveOpinionForContacts(bool b) = 0;

	/**
	 * @brief check if giving automatic positive opinion when flagging as
	 *	contact is enbaled
	 * @jsonapi{development}
	 * @return true if enabled, false otherwise
	 */
	virtual bool autoPositiveOpinionForContacts() = 0;

	/**
	 * @brief Set threshold on remote reputation to consider it remotely
	 *	negative
	 * @jsonapi{development}
	 * @param[in] thresh Threshold value
	 */
	virtual void setThresholdForRemotelyNegativeReputation(uint32_t thresh) = 0;

	/**
	 * * @brief Get threshold on remote reputation to consider it remotely
	 *	negative
	 * @jsonapi{development}
	 * @return Threshold value
	 */
	virtual uint32_t thresholdForRemotelyNegativeReputation() = 0;

	/**
	 * @brief Set threshold on remote reputation to consider it remotely
	 *	positive
	 * @jsonapi{development}
	 * @param[in] thresh Threshold value
	 */
	virtual void setThresholdForRemotelyPositiveReputation(uint32_t thresh) = 0;

	/**
	 * @brief Get threshold on remote reputation to consider it remotely
	 *	negative
	 * @jsonapi{development}
	 * @return Threshold value
	 */
	virtual uint32_t thresholdForRemotelyPositiveReputation() = 0;

	/**
	 * @brief Get number of days to wait before deleting a banned identity from
	 *	local storage
	 * @jsonapi{development}
	 * @return number of days to wait, 0 means never delete
	 */
	virtual uint32_t rememberBannedIdThreshold() = 0;

	/**
	 * @brief Set number of days to wait before deleting a banned identity from
	 *	local storage
	 * @jsonapi{development}
	 * @param[in] days number of days to wait, 0 means never delete
	 */
	virtual void setRememberBannedIdThreshold(uint32_t days) = 0;

	/**
	 * @brief This method allow fast checking if a GXS identity is banned.
	 * @jsonapi{development}
	 * @param[in] id Id of the identity to check
	 * @return true if identity is banned, false otherwise
	 */
	virtual bool isIdentityBanned(const RsGxsId& id) = 0;

	/**
	 * @brief Check if automatic banning of all identities signed by the given
	 *	node is enabled
	 * @jsonapi{development}
	 * @param[in] id PGP id of the node
	 * @return true if enabled, false otherwise
	 */
	virtual bool isNodeBanned(const RsPgpId& id) = 0;

	/**
	 * @brief Enable automatic banning of all identities signed by the given
	 *	node
	 * @jsonapi{development}
	 * @param[in] id PGP id of the node
	 * @param[in] b true to enable, false to disable
	 */
	virtual void banNode(const RsPgpId& id, bool b) = 0;


	/**
	 * @deprecated
	 * This returns the reputation level and also the flags of the identity
	 * service for that id. This is useful in order to get these flags without
	 * relying on the async method of p3Identity
	 */
	RS_DEPRECATED
	virtual RsReputationLevel overallReputationLevel(
	        const RsGxsId& id, uint32_t* identity_flags ) = 0;
};
