/*
 * libretroshare/src/services: groutermatrix.h
 *
 * Services for RetroShare.
 *
 * Copyright 2013 by Cyril Soler
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

#include <list>

#include "pgp/rscertificate.h"
#include "groutertypes.h"
#include "rsgrouter.h"

// The routing matrix records the event clues received from each friend
//
struct RoutingMatrixHitEntry
{
	float weight ;
	time_t time_stamp ;
};

// The map indexes for each friend the list of recent routing clues received.
//
struct RoutingMatrixFriendKeyData
{
	std::list<RoutingMatrixHitEntry> routing_clues ;
	float probability ;
};

class GRouterMatrix
{
	public:
		// Computes the routing probabilities for this id  for the given list of friends.
		// the computation accounts for the time at which the info was received and the
		// weight of each routing hit record.
		//
		bool computeRoutingProbabilities(const GRouterKeyId& id, const std::vector<SSLIdType>& friends, std::vector<float>& probas) const ;

		// Remove oldest entries.
		//
		bool autoWash() ;

		// Update routing probabilities for each key, accounting for all received events, but without
		// activity information 
		//
		bool updateRoutingProbabilities() ;

		// Record one routing clue. The events can possibly be merged in time buckets.
		//
		bool addRoutingClue(const GRouterKeyId& id,const GRouterServiceId& sid,float distance,const std::string& desc_string,const SSLIdType& source_friend) ;

	private:
		// List of events received and computed routing probabilities
		//
		std::map<GRouterKeyId, RoutingMatrixFriendKeyData> _routing_info ;

		// This is used to avoid re-computing probas when new events have been received.
		//
		bool _proba_need_updating ;
};

