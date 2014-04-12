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

#include "groutertypes.h"
#include "groutermatrix.h"
#include "grouteritems.h"

GRouterMatrix::GRouterMatrix()
{
	_proba_need_updating = true ;
}

bool GRouterMatrix::addRoutingClue(const GRouterKeyId& key_id,const RsPeerId& source_friend,float weight) 
{
	// 1 - get the friend index.
	//
	uint32_t fid = getFriendId(source_friend) ;

	// 2 - get the Key map, and add the routing clue.
	//
	time_t now = time(NULL) ;

	RoutingMatrixHitEntry rc ;
	rc.weight = weight ;
	rc.time_stamp = now ;
	rc.friend_id = fid ;

	std::list<RoutingMatrixHitEntry>& lst( _routing_clues[key_id] ) ;

	// Prevent flooding. Happens in two scenarii:
	//  1 - a user restarts RS very often => keys get republished for some reason
	//  2 - a user intentionnaly floods a key 
	//
	if(!lst.empty() && lst.front().time_stamp + RS_GROUTER_MATRIX_MIN_TIME_BETWEEN_HITS > now)
	{
		std::cerr << "GRouterMatrix::addRoutingClue(): too clues for key " << key_id.toStdString() << " in a small interval of " << now - lst.front().time_stamp << " seconds. Flooding?" << std::endl;
		return false ;
	}

	lst.push_front(rc) ;								// create it if necessary

	// Remove older elements
	//
	uint32_t sz = lst.size() ; // O(n)!

	for(uint32_t i=RS_GROUTER_MATRIX_MAX_HIT_ENTRIES;i<sz;++i)
	{
		lst.pop_back() ;
		std::cerr << "Poped one entry" << std::endl;
	}

	_proba_need_updating = true ; 				// always, since we added new clues.

	return true ;
}
uint32_t GRouterMatrix::getFriendId_const(const RsPeerId& source_friend) const
{
	std::map<RsPeerId,uint32_t>::const_iterator it = _friend_indices.find(source_friend) ;

	if(it == _friend_indices.end())
		return _reverse_friend_indices.size() ;
	else
		return it->second ;
}
uint32_t GRouterMatrix::getFriendId(const RsPeerId& source_friend)
{
	std::map<RsPeerId,uint32_t>::const_iterator it = _friend_indices.find(source_friend) ;

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

void GRouterMatrix::getListOfKnownKeys(std::vector<GRouterKeyId>& key_ids) const
{
	key_ids.clear() ;

	for(std::map<GRouterKeyId,std::vector<float> >::const_iterator it(_time_combined_hits.begin());it!=_time_combined_hits.end();++it)
		key_ids.push_back(it->first) ;
}

void GRouterMatrix::debugDump() const
{
	std::cerr << "    Proba needs up: " << _proba_need_updating << std::endl;
	std::cerr << "    Known keys:     " << _time_combined_hits.size() << std::endl;
	std::cerr << "    Routing events: " << std::endl;
	time_t now = time(NULL) ;

	for(std::map<GRouterKeyId, std::list<RoutingMatrixHitEntry> >::const_iterator it(_routing_clues.begin());it!=_routing_clues.end();++it)
	{
		std::cerr << "      " << it->first.toStdString() << " : " ;
		for(std::list<RoutingMatrixHitEntry>::const_iterator it2(it->second.begin());it2!=it->second.end();++it2)
			std::cerr << now - (*it2).time_stamp << " (" << (*it2).friend_id << "," << (*it2).weight << ") " ;

		std::cerr << std::endl;
	}
	std::cerr << "    Routing values: " << std::endl;

	for(std::map<GRouterKeyId, std::vector<float> >::const_iterator it(_time_combined_hits.begin());it!=_time_combined_hits.end();++it)
	{
		std::cerr << "      " << it->first.toStdString() << "  :  " ;

		for(uint32_t i=0;i<it->second.size();++i)
			std::cerr << it->second[i] << "   " ;
		std::cerr << std::endl;
	}
}

bool GRouterMatrix::computeRoutingProbabilities(const GRouterKeyId& key_id, const std::vector<RsPeerId>& friends, std::vector<float>& probas) const
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
	if(_proba_need_updating)
		std::cerr << "GRouterMatrix::computeRoutingProbabilities(): matrix is not up to date. Not a real problem, but still..." << std::endl;

	probas.resize(friends.size(),0.0f) ;
	float total = 0.0f ;

	std::map<GRouterKeyId,std::vector<float> >::const_iterator it2 = _time_combined_hits.find(key_id) ;

	if(it2 == _time_combined_hits.end())
	{
		// The key is not known. In this case, we return equal probabilities for all peers. 
		//
		float p = 1.0f / friends.size() ;

		probas.clear() ;
		probas.resize(friends.size(),p) ;

		std::cerr << "GRouterMatrix::computeRoutingProbabilities(): key id " << key_id.toStdString() << " does not exist! Returning uniform probabilities." << std::endl;
		return  false ;
	}
	const std::vector<float>& w(it2->second) ;
	
	for(uint32_t i=0;i<friends.size();++i)
	{
		uint32_t findex = getFriendId_const(friends[i]) ;

		if(findex >= w.size())
			probas[i] = 0.0f ;
		else
		{
			probas[i] = w[findex] ;
			total += w[findex] ;
		}
	}

	if(total > 0.0f)
		for(uint32_t i=0;i<friends.size();++i)
			probas[i] /= total ;

	return true ;
}

bool GRouterMatrix::updateRoutingProbabilities()
{
	std::cerr << "Updating routing probabilities..." << std::endl;

	if(!_proba_need_updating)
	{
		std::cerr << "  not needed." << std::endl;
		return false ;
	}

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
	std::cerr << "  done." << std::endl;

	_proba_need_updating = false ;
	return true ;
}

bool GRouterMatrix::saveList(std::list<RsItem*>& items) 
{
	std::cerr << "  GRoutingMatrix::saveList()" << std::endl;

	RsGRouterMatrixFriendListItem *item = new RsGRouterMatrixFriendListItem ;

	item->reverse_friend_indices = _reverse_friend_indices ;
	items.push_back(item) ;

	for(std::map<GRouterKeyId,std::list<RoutingMatrixHitEntry> >::const_iterator it(_routing_clues.begin());it!=_routing_clues.end();++it)
	{
		RsGRouterMatrixCluesItem *item = new RsGRouterMatrixCluesItem ;

		item->destination_key = it->first ;
		item->clues = it->second ;

		items.push_back(item) ;
	}

	return true ;
}
bool GRouterMatrix::loadList(std::list<RsItem*>& items) 
{
	RsGRouterMatrixFriendListItem *itm1 = NULL ;
	RsGRouterMatrixCluesItem      *itm2 = NULL ;

	std::cerr << "  GRoutingMatrix::loadList()" << std::endl;

	for(std::list<RsItem*>::const_iterator it(items.begin());it!=items.end();++it)
	{
		if(NULL != (itm2 = dynamic_cast<RsGRouterMatrixCluesItem*>(*it)))
		{
			std::cerr << "    initing routing clues." << std::endl;

			_routing_clues[itm2->destination_key] = itm2->clues ;
			_proba_need_updating = true ;	// notifies to re-compute all the info.
		}
		if(NULL != (itm1 = dynamic_cast<RsGRouterMatrixFriendListItem*>(*it)))
		{
			_reverse_friend_indices = itm1->reverse_friend_indices ;
			_friend_indices.clear() ;

			for(uint32_t i=0;i<_reverse_friend_indices.size();++i)
				_friend_indices[_reverse_friend_indices[i]] = i ;

			_proba_need_updating = true ;	// notifies to re-compute all the info.
		}
	}

	return true ;
}

