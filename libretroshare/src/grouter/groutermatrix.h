/*******************************************************************************
 * libretroshare/src/grouter: groutermatrix.h                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2013 by Cyril Soler <csoler@users.sourceforge.net>                *
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

#include <list>

#include "pgp/rscertificate.h"
#include "retroshare/rsgrouter.h"
#include "groutertypes.h"

struct RsItem;

// The routing matrix records the event clues received from each friend
//
struct RoutingMatrixHitEntry
{
	uint32_t friend_id ;			// not the full key. Gets too big otherwise!
	float weight ;
	rstime_t time_stamp ;
};

struct RoutingTrackEntry
{
	RsPeerId friend_id ;			// not the full key. Gets too big otherwise!
	rstime_t time_stamp ;
};

class GRouterMatrix
{
	public:
		GRouterMatrix() ;

		// Computes the routing probabilities for this id  for the given list of friends.
		// the computation accounts for the time at which the info was received and the
		// weight of each routing hit record.
		//
		bool computeRoutingProbabilities(const GRouterKeyId& id, const std::vector<RsPeerId>& friends, std::vector<float>& probas, float &maximum) const ;

		// Update routing probabilities for each key, accounting for all received events, but without
		// activity information 
		//
		bool updateRoutingProbabilities() ;

		// Record one routing clue. The events can possibly be merged in time buckets.
		//
		bool addRoutingClue(const GRouterKeyId& id,const RsPeerId& source_friend,float weight) ;
		bool addTrackingInfo(const RsGxsMessageId& id,const RsPeerId& source_friend) ;

		bool saveList(std::list<RsItem*>& items) ;
		bool loadList(std::list<RsItem*>& items) ;

        	bool cleanUp() ;
            
		// Dump info in terminal.
		//
		void debugDump() const ;
		void getListOfKnownKeys(std::vector<GRouterKeyId>& key_ids) const ;

        	bool getTrackingInfo(const RsGxsMessageId& id,RsPeerId& source_friend);
	private:
		// returns the friend id, possibly creating a new id.
		//
		uint32_t getFriendId(const RsPeerId& id) ;

		// returns the friend id. If not exist, returns _reverse_friend_indices.size()
		//
		uint32_t getFriendId_const(const RsPeerId& id) const;

		// List of events received and computed routing probabilities
		//
		std::map<GRouterKeyId, std::list<RoutingMatrixHitEntry> > _routing_clues ;       // received routing clues. Should be saved.
		std::map<GRouterKeyId, std::vector<float> >               _time_combined_hits ;  // hit matrix after time-convolution filter
		std::map<RsGxsMessageId,RoutingTrackEntry>                _tracking_clues ;      // who provided the most recent messages

		// This is used to avoid re-computing probas when new events have been received.
		//
		bool _proba_need_updating ;

		// Routing weights. These are the result of a time convolution of the routing clues and weights
		// recorded in _routing_clues.
		//
		std::map<RsPeerId,uint32_t> _friend_indices ;	// index for each friend to lookup in the routing matrix Not saved.
		std::vector<RsPeerId> _reverse_friend_indices ;// SSLid corresponding to each friend index. Saved.
};

