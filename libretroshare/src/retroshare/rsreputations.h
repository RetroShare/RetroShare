/*******************************************************************************
 * libretroshare/src/retroshare: rsreputations.h                               *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2015 by Cyril Soler <retroshare.team@gmail.com>                   *
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

class RsReputations
{
public:
        static const float REPUTATION_THRESHOLD_ANTI_SPAM;
        static const float REPUTATION_THRESHOLD_DEFAULT;
        
	// This is the interface file for the reputation system
	//
	enum Opinion         { OPINION_NEGATIVE = 0, OPINION_NEUTRAL = 1, OPINION_POSITIVE = 2 } ;

	enum ReputationLevel {  REPUTATION_LOCALLY_NEGATIVE  = 0x00, // local opinion is positive
		                    REPUTATION_REMOTELY_NEGATIVE = 0x01, // local opinion is neutral and friends are positive in average
		                    REPUTATION_NEUTRAL           = 0x02, // no reputation information ;
		                    REPUTATION_REMOTELY_POSITIVE = 0x03, // local opinion is neutral and friends are negative in average
		                    REPUTATION_LOCALLY_POSITIVE  = 0x04, // local opinion is negative
		                    REPUTATION_UNKNOWN           = 0x05  // missing info
						 };

	struct ReputationInfo
	{
		ReputationInfo() : mOwnOpinion(OPINION_NEUTRAL),mFriendsPositiveVotes(0),mFriendsNegativeVotes(0), mFriendAverageScore(REPUTATION_THRESHOLD_DEFAULT),mOverallReputationLevel(REPUTATION_NEUTRAL){}
            
		RsReputations::Opinion mOwnOpinion ;

		uint32_t mFriendsPositiveVotes ;
		uint32_t mFriendsNegativeVotes ;

		float mFriendAverageScore ;

		RsReputations::ReputationLevel mOverallReputationLevel;	// this should help clients in taking decisions
	};

	virtual bool setOwnOpinion(const RsGxsId& key_id, const Opinion& op) =0;
	virtual bool getOwnOpinion(const RsGxsId& key_id, Opinion& op) =0;
    virtual bool getReputationInfo(const RsGxsId& id, const RsPgpId &ownerNode, ReputationInfo& info,bool stamp=true) =0;

    // This returns the reputation level and also the flags of the identity service for that id. This is useful in order to get these flags without relying on the async method of p3Identity

    virtual ReputationLevel overallReputationLevel(const RsGxsId& id,uint32_t *identity_flags=NULL)=0;

    // parameters

    virtual void setNodeAutoPositiveOpinionForContacts(bool b) =0;
    virtual bool nodeAutoPositiveOpinionForContacts() =0;

	virtual uint32_t thresholdForRemotelyNegativeReputation()=0;
	virtual uint32_t thresholdForRemotelyPositiveReputation()=0;
	virtual void setThresholdForRemotelyNegativeReputation(uint32_t thresh)=0;
	virtual void setThresholdForRemotelyPositiveReputation(uint32_t thresh)=0;

    virtual void setRememberDeletedNodesThreshold(uint32_t days) =0;
    virtual uint32_t rememberDeletedNodesThreshold() =0;

	// This one is a proxy designed to allow fast checking of a GXS id.
	// it basically returns true if assessment is not ASSESSMENT_OK
        
    virtual bool isIdentityBanned(const RsGxsId& id) =0;

    virtual bool isNodeBanned(const RsPgpId& id) =0;
    virtual void banNode(const RsPgpId& id,bool b) =0;
};

// To access reputations from anywhere
//
extern RsReputations *rsReputations ;
