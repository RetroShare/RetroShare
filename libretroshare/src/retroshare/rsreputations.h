/*
 * libretroshare/src/services: rsreputation.h
 *
 * Services for RetroShare.
 *
 * Copyright 2015 by Cyril Soler
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
 * Please report all bugs and problems to "csoler@users.sourceforge.net".
 *
 */

#pragma once

#include "retroshare/rsids.h"
#include "retroshare/rsgxsifacetypes.h"

class RsReputations
{
public:
	// This is the interface file for the reputation system
	//
	enum Opinion    { OPINION_NEGATIVE = 0, OPINION_NEUTRAL = 1, OPINION_POSITIVE = 2 } ;
    	enum Assessment { ASSESSMENT_BAD = 0, ASSESSMENT_OK = 1 } ;

	struct ReputationInfo
	{
		RsReputations::Opinion mOwnOpinion ;
		float mOverallReputationScore ;
     		RsReputations::Assessment mAssessment;	// this should help clients in taking decisions
	};

	virtual bool setOwnOpinion(const RsGxsId& key_id, const Opinion& op) =0;
	virtual bool getReputationInfo(const RsGxsId& id,ReputationInfo& info) =0 ;
        
        // This one is a proxy designed to allow fast checking of a GXS id.
        // it basically returns true if assessment is ASSESSMENT_OK
        
	virtual bool isIdentityOk(const RsGxsId& id) =0;
};

// To access reputations from anywhere
//
extern RsReputations *rsReputations ;
