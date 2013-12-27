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

class RsItem ;

// The routing matrix records the event clues received from each friend
//
struct RoutingMatrixHitEntry
{
	uint32_t friend_id ;			// not the full key. Gets too big otherwise!
	float weight ;
	time_t time_stamp ;
};

class GRouterMatrix
{
	public:
		GRouterMatrix() ;

		// Computes the routing probabilities for this id  for the given list of friends.
		// the computation accounts for the time at which the info was received and the
		// weight of each routing hit record.
		//
		bool computeRoutingProbabilities(const GRouterKeyId& id, const std::list<SSLIdType>& friends, std::map<SSLIdType,float>& probas) const ;

		// Update routing probabilities for each key, accounting for all received events, but without
		// activity information 
		//
		bool updateRoutingProbabilities() ;

		// Record one routing clue. The events can possibly be merged in time buckets.
		//
		bool addRoutingClue(const GRouterKeyId& id,const GRouterServiceId& sid,float distance,const std::string& desc_string,const SSLIdType& source_friend) ;

		// Dump info in terminal.
		//
		void debugDump() const ;

		bool saveList(std::list<RsItem*>& items) ;
		bool loadList(std::list<RsItem*>& items) ;

	private:
		// returns the friend id, possibly creating a new id.
		//
		uint32_t getFriendId(const SSLIdType& id) ;

		// returns the friend id. If not exist, returns _reverse_friend_indices.size()
		//
		uint32_t getFriendId_const(const SSLIdType& id) const;

		// List of events received and computed routing probabilities
		//
		std::map<GRouterKeyId, std::list<RoutingMatrixHitEntry> > _routing_clues ;			// received routing clues. Should be saved.
		std::map<GRouterKeyId, std::vector<float> > 					 _time_combined_hits ; 	// hit matrix after time-convolution filter

		// This is used to avoid re-computing probas when new events have been received.
		//
		bool _proba_need_updating ;

		// Routing weights. These are the result of a time convolution of the routing clues and weights
		// recorded in _routing_clues.
		//
		std::map<SSLIdType,uint32_t> _friend_indices ;	// index for each friend to lookup in the routing matrix Not saved.
		std::vector<SSLIdType> _reverse_friend_indices ;// SSLid corresponding to each friend index. Saved.

};

