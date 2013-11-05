/*
 * libretroshare/src/services: groutermatrix.cc
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

#include "groutermatrix.h"

GRouterMatrix::GRouterMatrix()
{
	_proba_need_updating = true ;
}

bool GRouterMatrix::addRoutingClue(	const GRouterKeyId& key_id,const GRouterServiceId& sid,float distance,
												const std::string& desc_string,const SSLIdType& source_friend) 
{
	// 1 - get the friend index.
	//
	uint32_t fid = getFriendId(source_friend) ;

	// 2 - get the Key map, and add the routing clue.
	//
	time_t now = time(NULL) ;

	RoutingMatrixHitEntry rc ;
	rc.weight = 1.0f / (1.0f + distance) ;
	rc.time_stamp = now ;
	rc.friend_id = fid ;

	_routing_clues[key_id].push_front(rc) ;	// create it if necessary
	_proba_need_updating = true ; 				// always, since we added new clues.

	return true ;
}

uint32_t GRouterMatrix::getFriendId(const SSLIdType& source_friend)
{
	std::map<SSLIdType,uint32_t>::const_iterator it = _friend_indices.find(source_friend) ;

	if(it == _friend_indices.end())
	{
		// add a new friend

		uint32_t new_id = _reverse_friend_indices.size() ;
		_reverse_friend_indices.push_back(source_friend) ;
		_friend_indices[source_friend] = new_id ;

		return new_id ;
	}
	else
		return it->second ;
}

void GRouterMatrix::debugDump() const
{
	std::cerr << "    Proba needs up: " << _proba_need_updating << std::endl;
	std::cerr << "    Routing events: " << std::endl;
	time_t now = time(NULL) ;

	for(std::map<GRouterKeyId, std::list<RoutingMatrixHitEntry> >::const_iterator it(_routing_clues.begin());it!=_routing_clues.end();++it)
	{
		std::cerr << "      " << it->first.toStdString() << " : " ;
		for(std::list<RoutingMatrixHitEntry>::const_iterator it2(it->second.begin());it2!=it->second.end();++it2)
			std::cerr << now - (*it2).time_stamp << " (" << (*it2).friend_id << "," << (*it2).weight << ") " ;

		std::cerr << std::endl;
	}
	std::cerr << "    Routing probabilities: " << std::endl;

	for(std::map<GRouterKeyId, std::vector<float> >::const_iterator it(_time_combined_hits.begin());it!=_time_combined_hits.end();++it)
	{
		std::cerr << it->first.toStdString() << "  :  " ;

		for(uint32_t i=0;i<it->second.size();++i)
			std::cerr << it->second[i] << "   " ;
		std::cerr << std::endl;
	}
}

bool GRouterMatrix::computeRoutingProbabilities(const GRouterKeyId& id, const std::vector<SSLIdType>& friends, std::vector<float>& probas) const
{
	// Routing probabilities are computed according to routing clues
	//
	// For a given key, each friend has a known set of routing clues (time_t, weight)
	//	We combine these to compute a static weight for each friend/key pair. 
	//	This is performed in updateRoutingProbabilities()
	//
	//	Then for a given list of online friends, the weights are computed into probabilities, 
	//	that always sum up to 1.
	//
	return true ;
}

bool GRouterMatrix::updateRoutingProbabilities()
{
	if(!_proba_need_updating)
		return false ;

	time_t now = time(NULL) ;

	for(std::map<GRouterKeyId, std::list<RoutingMatrixHitEntry> >::const_iterator it(_routing_clues.begin());it!=_routing_clues.end();++it)
	{
		std::cerr << "      " << it->first.toStdString() << " : " ;

		std::vector<float>& v(_time_combined_hits[it->first]) ;
		v.clear() ;
		v.resize(_friend_indices.size(),0.0f) ;

		for(std::list<RoutingMatrixHitEntry>::const_iterator it2(it->second.begin());it2!=it->second.end();++it2)
		{
			float time_difference_in_days = 1 + (now - (*it2).time_stamp ) / 86400.0f ;
			v[(*it2).friend_id] += (*it2).weight / (time_difference_in_days*time_difference_in_days) ;
		}
	}
	_proba_need_updating = false ;
	return true ;
}





