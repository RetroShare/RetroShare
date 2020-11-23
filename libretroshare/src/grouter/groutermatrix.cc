/*******************************************************************************
 * libretroshare/src/grouter: groutermatrix.cc                                 *
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

#include "groutertypes.h"
#include "groutermatrix.h"
#include "grouteritems.h"

//#define ROUTING_MATRIX_DEBUG

GRouterMatrix::GRouterMatrix()
{
	_proba_need_updating = true ;
}

bool GRouterMatrix::addTrackingInfo(const RsGxsMessageId& mid,const RsPeerId& source_friend)
{
    rstime_t now = time(NULL) ;

    RoutingTrackEntry rte ;

    rte.friend_id = source_friend ;
    rte.time_stamp = now ;

    _tracking_clues[mid] = rte ;
#ifdef ROUTING_MATRIX_DEBUG
    std::cerr << "GRouterMatrix::addTrackingInfo(): Added clue mid=" << mid << ", from " << source_friend << " ID=" << source_friend << std::endl;
#endif
    return true ;
}

bool GRouterMatrix::cleanUp()
{
    // remove all tracking entries that have become too old.

#ifdef ROUTING_MATRIX_DEBUG
    std::cerr << "GRouterMatrix::cleanup()" << std::endl;
#endif
    rstime_t now = time(NULL) ;

    for(std::map<RsGxsMessageId,RoutingTrackEntry>::iterator it(_tracking_clues.begin());it!=_tracking_clues.end();)
	    if(it->second.time_stamp + RS_GROUTER_MAX_KEEP_TRACKING_CLUES < now)
	    {
#ifdef ROUTING_MATRIX_DEBUG
		    std::cerr << "  removing old entry msgId=" << it->first << ", from id " << it->second.friend_id << ", obtained " << (now - it->second.time_stamp) << " secs ago." << std::endl;
#endif
		    std::map<RsGxsMessageId,RoutingTrackEntry>::iterator tmp(it) ;
		    ++tmp ;
		    _tracking_clues.erase(it) ;
		    it=tmp ;
	    }
	    else
		    ++it ;
    
    return true ;
}

bool GRouterMatrix::addRoutingClue(const GRouterKeyId& key_id,const RsPeerId& source_friend,float weight) 
{
	// 1 - get the friend index.
	//
	uint32_t fid = getFriendId(source_friend) ;

	// 2 - get the Key map, and add the routing clue.
	//
	rstime_t now = time(NULL) ;

	RoutingMatrixHitEntry rc ;
	rc.weight = weight ;
	rc.time_stamp = now ;
	rc.friend_id = fid ;

	std::list<RoutingMatrixHitEntry>& lst( _routing_clues[key_id] ) ;

	// Prevent flooding. Happens in two scenarii:
	//  1 - a user restarts RS very often => keys get republished for some reason
	//  2 - a user intentionnaly floods a key 
    //
    // Solution is to look for all recorded events, and not add any new event if an event came from the same friend
    // too close in the past. Going through the list is not costly since it is bounded to RS_GROUTER_MATRIX_MAX_HIT_ENTRIES elemts.

    for(std::list<RoutingMatrixHitEntry>::const_iterator mit(lst.begin());mit!=lst.end();++mit)
        if((*mit).friend_id == fid && (*mit).time_stamp + RS_GROUTER_MATRIX_MIN_TIME_BETWEEN_HITS > now)
        {
#ifdef ROUTING_MATRIX_DEBUG
            std::cerr << "GRouterMatrix::addRoutingClue(): too many clues for key " << key_id.toStdString() << " from friend " << source_friend << " in a small interval of " << now - lst.front().time_stamp << " seconds. Flooding?" << std::endl;
#endif
            return false ;
        }

	lst.push_front(rc) ;								// create it if necessary

	// Remove older elements
	//
	uint32_t sz = lst.size() ; // O(n)!

	for(uint32_t i=RS_GROUTER_MATRIX_MAX_HIT_ENTRIES;i<sz;++i)
	{
		lst.pop_back() ;
#ifdef ROUTING_MATRIX_DEBUG
        std::cerr << "Poped one entry" << std::endl;
#endif
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

bool GRouterMatrix::getTrackingInfo(const RsGxsMessageId& mid, RsPeerId &source_friend)
{
    std::map<RsGxsMessageId,RoutingTrackEntry>::const_iterator it = _tracking_clues.find(mid) ;
    
    if(it == _tracking_clues.end())
        return false ;
    
    source_friend = it->second.friend_id;
    
    return true ;
}

void GRouterMatrix::debugDump() const
{
	std::cerr << "    Proba needs up: " << _proba_need_updating << std::endl;
	std::cerr << "    Known keys:     " << _time_combined_hits.size() << std::endl;
	std::cerr << "    Routing events: " << std::endl;
	rstime_t now = time(NULL) ;

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
	std::cerr << "    Tracking clues: " << std::endl;
    
	for(std::map<RsGxsMessageId, RoutingTrackEntry>::const_iterator it(_tracking_clues.begin());it!=_tracking_clues.end();++it)
        	std::cerr << "        " << it->first << ": from " << it->second.friend_id << " " << now - it->second.time_stamp << " secs ago." << std::endl;
}

bool GRouterMatrix::computeRoutingProbabilities(const GRouterKeyId& key_id, const std::vector<RsPeerId>& friends, std::vector<float>& probas, float& maximum) const
{
	// Routing probabilities are computed according to routing clues
	//
	// For a given key, each friend has a known set of routing clues (rstime_t, weight)
	//	We combine these to compute a static weight for each friend/key pair. 
	//	This is performed in updateRoutingProbabilities()
	//
	//	Then for a given list of online friends, the weights are computed into probabilities, 
	//	that always sum up to 1.
	//
#ifdef ROUTING_MATRIX_DEBUG
    if(_proba_need_updating)
        std::cerr << "GRouterMatrix::computeRoutingProbabilities(): matrix is not up to date. Not a real problem, but still..." << std::endl;
#endif

	probas.resize(friends.size(),0.0f) ;
	float total = 0.0f ;

	std::map<GRouterKeyId,std::vector<float> >::const_iterator it2 = _time_combined_hits.find(key_id) ;

	if(it2 == _time_combined_hits.end())
	{
        // The key is not known. In this case, we return a zero probability for all peers.
        //
        float p = 0.0f;//1.0f / friends.size() ;

		probas.clear() ;
		probas.resize(friends.size(),p) ;

#ifdef ROUTING_MATRIX_DEBUG
        std::cerr << "GRouterMatrix::computeRoutingProbabilities(): key id " << key_id.toStdString() << " does not exist! Returning uniform probabilities." << std::endl;
#endif
		return  false ;
	}
	const std::vector<float>& w(it2->second) ;
    	maximum = 0.0f ;
	
	for(uint32_t i=0;i<friends.size();++i)
	{
		uint32_t findex = getFriendId_const(friends[i]) ;

		if(findex >= w.size())
			probas[i] = 0.0f ;
		else
		{
			probas[i] = w[findex] ;
			total += w[findex] ;
            
            		if(maximum < w[findex])
                        	maximum = w[findex] ;
		}
	}

	if(total > 0.0f)
		for(uint32_t i=0;i<friends.size();++i)
			probas[i] /= total ;

	return true ;
}

bool GRouterMatrix::updateRoutingProbabilities()
{
	if(!_proba_need_updating)
		return false ;

	rstime_t now = time(NULL) ;

	for(std::map<GRouterKeyId, std::list<RoutingMatrixHitEntry> >::const_iterator it(_routing_clues.begin());it!=_routing_clues.end();++it)
	{
#ifdef ROUTING_MATRIX_DEBUG
        std::cerr << "      " << it->first.toStdString() << " : " ;
#endif

		std::vector<float>& v(_time_combined_hits[it->first]) ;
		v.clear() ;
		v.resize(_friend_indices.size(),0.0f) ;

		for(std::list<RoutingMatrixHitEntry>::const_iterator it2(it->second.begin());it2!=it->second.end();++it2)
        {
            // Half life period is 7 days.

            float time_difference_in_days = 1 + (now - (*it2).time_stamp ) / (7*86400.0f) ;
			v[(*it2).friend_id] += (*it2).weight / (time_difference_in_days*time_difference_in_days) ;
		}
    }
#ifdef ROUTING_MATRIX_DEBUG
    std::cerr << "  done." << std::endl;
#endif

	_proba_need_updating = false ;
	return true ;
}

bool GRouterMatrix::saveList(std::list<RsItem*>& items) 
{
#ifdef ROUTING_MATRIX_DEBUG
    std::cerr << "  GRoutingMatrix::saveList()" << std::endl;
#endif

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

    for(std::map<RsGxsMessageId,RoutingTrackEntry>::const_iterator it(_tracking_clues.begin());it!=_tracking_clues.end();++it)
    {
	    RsGRouterMatrixTrackItem *item = new RsGRouterMatrixTrackItem ;

	    item->provider_id = it->second.friend_id ;
	    item->time_stamp = it->second.time_stamp ;
	    item->message_id = it->first ;

	    items.push_back(item) ;
    }

    return true ;
}
bool GRouterMatrix::loadList(std::list<RsItem*>& items) 
{
    RsGRouterMatrixFriendListItem *itm1 = NULL ;
    RsGRouterMatrixCluesItem      *itm2 = NULL ;
    RsGRouterMatrixTrackItem      *itm3 = NULL ;

#ifdef ROUTING_MATRIX_DEBUG
    std::cerr << "  GRoutingMatrix::loadList()" << std::endl;
#endif

    for(std::list<RsItem*>::const_iterator it(items.begin());it!=items.end();++it)
    {
	    if(NULL != (itm3 = dynamic_cast<RsGRouterMatrixTrackItem*>(*it)))
	    {
#ifdef ROUTING_MATRIX_DEBUG
		    std::cerr << "    initing tracking clues." << std::endl;
#endif
		    RoutingTrackEntry rte ;
		    rte.friend_id = itm3->provider_id ;
		    rte.time_stamp = itm3->time_stamp ;

		    _tracking_clues[itm3->message_id] = rte;
	    }
	    if(NULL != (itm2 = dynamic_cast<RsGRouterMatrixCluesItem*>(*it)))
	    {
#ifdef ROUTING_MATRIX_DEBUG
		    std::cerr << "    initing routing clues." << std::endl;
#endif

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

