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

typedef uint32_t GRouterServiceId ;
typedef uint64_t GRouterMsgPropagationId ;

class RsReputations
{
public:
	// This is the interface file for the reputation system
	//
	enum Opinion { OPINION_NEGATIVE = -1, OPINION_NEUTRAL = 0, OPINION_POSITIVE = 1 } ;

	struct ReputationInfo
	{
		RsReputations::Opinion mOwnOpinion ;
		float mOverallReputationScore ;
	};

	virtual bool setOwnOpinion(const RsGxsId& key_id, const Opinion& op) =0;
	virtual bool getReputationInfo(const RsGxsId& id,ReputationInfo& info) =0 ;
};

// To access reputations from anywhere
//
extern RsReputations *rsReputations ;
