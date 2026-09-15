/*******************************************************************************
 * gui/statistics/BWGraph.cpp                                                  *
 *                                                                             *
 * Copyright (c) 2012 Retroshare Team <retroshare.project@gmail.com>           *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "BWGraph.h"

#include <time.h>
#include <math.h>
#include "retroshare/rsservicecontrol.h"
#include "retroshare/rspeers.h"

//#define BWGRAPH_DEBUG 1

void BWGraphSource::update()
{
#ifdef BWGRAPH_DEBUG
    std::cerr << "Updating BW graphsource..." << std::endl;
#endif

    std::list<RSTrafficClue> in_clues,out_clues;
    rsConfig->getTrafficInfo(out_clues,in_clues);

    // Add the collected records to the cumulated traffic map

    auto add_traffic_to_cumulated_map = [](const std::list<RSTrafficClue>& incoming_lst,CumulatedTrafficMap& tmap) {
            for(const auto& c:incoming_lst)
            {
                auto& t = tmap[PeerSrvSubsrv(c)];
                t.cumulated_size  += c.size;
                t.cumulated_count += c.count;
#ifdef BWGRAPH_DEBUG
                std::cerr << "Pushing " << c.TS << "  " << c.peer_id << "  " << c.service_id << "  " << (int)c.service_sub_id << " into cumulated map" << std::endl;
#endif
            }
    };

    add_traffic_to_cumulated_map(out_clues,mCumulatedTrafficMap_Out);
    add_traffic_to_cumulated_map(in_clues,mCumulatedTrafficMap_In);

#ifdef BWGRAPH_DEBUG
    std::cerr << "cumulated map is now: " << std::endl;
    for(const auto& it:mCumulatedTrafficMap_Out)
        std::cerr << "  " << it.first.peer_id << "  " << it.first.service_id << "  " << (int)it.first.service_sub_id << ": " << it.second.cumulated_size << "  " << it.second.cumulated_count << std::endl;
#endif

    // Parse the cumulated map and add elements to the traffic clues when missing, converting
    // RSTrafficClue (that misses the cumulated traffic) into a RSTrafficClueExt that includes it.

    auto add_missing_clues_and_convert = [](std::list<RSTrafficClueExt>& clues,const std::list<RSTrafficClue>& incoming_lst,const CumulatedTrafficMap& tmap) {

            std::set<PeerSrvSubsrv> incoming_elements;
            uint64_t ts=time(nullptr);

            // 1 - convert existing traffic clues from incoming list into extended clues with cimulated traffic

            for(const auto& c:incoming_lst)
            {
                auto pss_c = PeerSrvSubsrv(c);
                RSTrafficClueExt e(c);

                auto tm_it = tmap.find(pss_c);

                if(tm_it == tmap.end())
                {
                    RsErr() << "missing value " << c.peer_id << "  " << c.service_id << "  " << (int)c.service_sub_id << " into cumulated map" << std::endl;
                    continue;
                }

                // only insert the cumulated values once since these traffic clues may be added together.

                if(incoming_elements.find(pss_c) == incoming_elements.end())
                {
                    e.cumulated_size  = tm_it->second.cumulated_size;
                    e.cumulated_count = tm_it->second.cumulated_count;
                }
#ifdef BWGRAPH_DEBUG
                std::cerr << "incoming clue: " << c.TS << "  " << c.peer_id << "  " << c.service_id << "  " << (int)c.service_sub_id << " " << c.size << std::endl;
                std::cerr << "pushing new  : " << e.TS << "  " << e.peer_id << "  " << e.service_id << "  " << (int)e.service_sub_id << " " << e.size << "  " << e.cumulated_size << std::endl;
#endif

                clues.push_back(e);
                incoming_elements.insert(pss_c);
            }

            // 2 - look into tmap and possibly add one placeholder traffic clue for each missing element

            for(const auto& it:tmap)
                if(incoming_elements.find(it.first) == incoming_elements.end())
                {
                    RSTrafficClueExt e;
                    e.TS              = ts;
                    e.peer_id         = it.first.peer_id;
                    e.service_id      = it.first.service_id;
                    e.service_sub_id  = it.first.service_sub_id;
                    e.cumulated_count = it.second.cumulated_count;
                    e.cumulated_size  = it.second.cumulated_size ;
                    clues.push_back(e);
#ifdef BWGRAPH_DEBUG
                    std::cerr << "pushing placeholder  : " << e.TS << "  " << e.peer_id << "  " << e.service_id << "  " << (int)e.service_sub_id << " " << e.size << "  " << e.cumulated_size << std::endl;
#endif
                }
    };

    TrafficHistoryChunk thc ;
    add_missing_clues_and_convert(thc.out_rstcl,out_clues,mCumulatedTrafficMap_Out);
    add_missing_clues_and_convert(thc.in_rstcl,in_clues,mCumulatedTrafficMap_In);

    thc.time_stamp = getTime() ;

#ifdef BWGRAPH_DEBUG
    std::cerr << "Traffic detail:" << std::endl;
    for(const auto& tc:thc.out_rstcl)
            std::cerr << "TS=" << tc.TS << " peer=" << tc.peer_id << " service " << tc.service_id << " ("
                      << (int)tc.service_sub_id << ") " << tc.size << " ( " << tc.cumulated_size << ")" << tc.count << " ( " << tc.cumulated_count << ")"<< std::endl;
#endif

    // keep track of them, in case we need to change the sorting

    mTrafficHistory.push_back(thc) ;

    std::set<RsPeerId> fds ;

    // add visible friends/services

    for(const auto& c:thc.out_rstcl)
    {
        fds.insert(c.peer_id) ;
        mVisibleServices.insert(c.service_id) ;
    }
    for(const auto& c:thc.in_rstcl)
    {
        fds.insert(c.peer_id) ;
        mVisibleServices.insert(c.service_id) ;
    }

    for(std::set<RsPeerId>::const_iterator it(fds.begin());it!=fds.end();++it)
    {
        std::string& s(mVisibleFriends[*it]) ;

        if(s.empty())
        {
            RsPeerDetails pd ;
            rsPeers->getPeerDetails(*it,pd) ;

            s = pd.name + " (" + pd.location + ")" ;
        }
    }

#ifdef BWGRAPH_DEBUG
    std::cerr << "  visible friends: " << std::dec << mVisibleFriends.size() << std::endl;
    std::cerr << "  visible service: " << std::dec << mVisibleServices.size() << std::endl;
#endif

    // Now, convert latest data measurement into points. convertTrafficToValues() returns
    // a map of values corresponding to the latest point in time, doing all the requested calculations
    // (sum over friends, sum over services, etc).
    //

    std::map<std::string,float> vals ;

    if(_current_direction == (DIRECTION_UP | DIRECTION_DOWN))
    {
        std::map<std::string,float> vals1,vals2 ;
        convertTrafficClueToValues(thc.out_rstcl,vals1) ;
        convertTrafficClueToValues(thc.in_rstcl,vals2) ;

        for(auto it:vals1) vals[it.first + " (sent)"] = it.second;
        for(auto it:vals2) vals[it.first + " (received)"] = it.second;
    }
    else if(_current_direction & DIRECTION_UP)
        convertTrafficClueToValues(thc.out_rstcl,vals) ;
    else
        convertTrafficClueToValues(thc.in_rstcl,vals) ;

    qint64 ms = getTime() ;

    std::set<std::string> unused_vals ;

    for(std::map<std::string,std::list<std::pair<qint64,float> > >::const_iterator it=_points.begin();it!=_points.end();++it)
        unused_vals.insert(it->first) ;

    for(std::map<std::string,float>::iterator it=vals.begin();it!=vals.end();++it)
    {
        std::list<std::pair<qint64,float> >& lst(_points[it->first]) ;

        if(!lst.empty() && fabsf((float)(lst.back().first - ms)) > _update_period_msecs*1.2 )
        {
            lst.push_back(std::make_pair(lst.back().first,0)) ;
            lst.push_back(std::make_pair(              ms,0)) ;
        }

        lst.push_back(std::make_pair(ms,it->second)) ;

		float& total(_totals[it->first].v) ;

		total += it->second ;

        unused_vals.erase(it->first) ;

        for(std::list<std::pair<qint64,float> >::iterator it2=lst.begin();it2!=lst.end();)
            if( ms - (*it2).first > _time_limit_msecs)
            {
                //std::cerr << "  removing old value with time " << (*it).first/1000.0f << std::endl;
				total -=(*it2).second ;
                it2 = lst.erase(it2) ;
            }
            else
                break ;
    }

    // make sure that all values are fed.

    for(std::set<std::string>::const_iterator it(unused_vals.begin());it!=unused_vals.end();++it)
        _points[*it].push_back(std::make_pair(ms,0)) ;

    // remove empty lists

    float duration = 0.0f;

    for(std::map<std::string,std::list<std::pair<qint64,float> > >::iterator it=_points.begin();it!=_points.end();)
        if(it->second.empty())
		{
			std::map<std::string,std::list<std::pair<qint64,float> > >::iterator tmp(it) ;
			++tmp;
			_totals.erase(it->first) ;
			_points.erase(it) ;
			it=tmp ;
		}
        else
        {
            float d = it->second.back().first - it->second.front().first;

            if(duration < d)
                duration = d ;

            ++it ;
        }

    // also clears history

    for(std::list<TrafficHistoryChunk>::iterator it = mTrafficHistory.begin();it!=mTrafficHistory.end();++it)
    {
#ifdef BWGRAPH_DEBUG
        std::cerr << "TS=" << (*it).time_stamp << ", ms = " << ms << ", diff=" << ms - (*it).time_stamp  << " compared to  " << _time_limit_msecs << std::endl;
#endif

            if( ms - (*it).time_stamp > _time_limit_msecs)
            {
                it = mTrafficHistory.erase(it) ;

#ifdef BWGRAPH_DEBUG
                std::cerr << "Removing 1 item of traffic history" << std::endl;
#endif
            }
            else
                break ;
    }

	_total_duration_seconds = duration/1000.0 ;

    // now update the totals, and possibly convert into an average if the unit requires it.

    // updateTotals();

    // if(_current_unit == UNIT_KILOBYTES)
    //     for(std::map<std::string,ZeroInitFloat>::iterator it(_totals.begin());it!=_totals.end();++it)
    //         it->second.v /= (duration/1000.0) ;

#ifdef BWGRAPH_DEBUG
    std::cerr << "Traffic history has size " << mTrafficHistory.size() << std::endl;
#endif
}

void BWGraphSource::clear()
{
    _total_sent =0;
    _total_recv =0;

    _total_duration_seconds =0;

    mTrafficHistory.clear() ;
    mCumulatedTrafficMap_In.clear();
    mCumulatedTrafficMap_Out.clear();

    mVisibleFriends.clear() ;
    mVisibleServices.clear() ;

    recomputeCurrentCurves() ;
}
void BWGraphSource::getCumulatedValues(std::vector<float>& vals) const
{
	if(_current_unit == UNIT_KILOBYTES && _total_duration_seconds > 0.0)
		for(std::map<std::string,ZeroInitFloat>::const_iterator it = _totals.begin();it!=_totals.end();++it)
			vals.push_back(it->second.v/_total_duration_seconds) ;
	else
		for(std::map<std::string,ZeroInitFloat>::const_iterator it = _totals.begin();it!=_totals.end();++it)
			vals.push_back(it->second.v) ;
}

std::string BWGraphSource::makeSubItemName(uint16_t service_id,uint8_t sub_item_type) const
{
    RsServiceInfoWithNames& s(mServiceInfoMap[service_id]) ;

    if(s.item_names.empty())
		return "item #"+QString("%1").arg(sub_item_type,2,16,QChar('0')).toStdString() ;
    else
    {
        std::map<uint8_t,std::string>::const_iterator it = s.item_names.find(sub_item_type) ;

        if(it == s.item_names.end())
            return "item #"+QString("%1").arg(sub_item_type,2,16,QChar('0')).toStdString() + " (undocumented)";

        return QString("%1").arg(sub_item_type,2,16,QChar('0')).toStdString()+": " + it->second ;
    }
}

void BWGraphSource::convertTrafficClueToValues(const std::list<RSTrafficClueExt>& lst,std::map<std::string,float>& vals) const
{
   auto select_value = [this](const RSTrafficClueExt& tc) -> uint64_t {
       return  (_current_unit == UNIT_KILOBYTES)?
                                (_current_timing==TIMING_INSTANT?tc.size:tc.cumulated_size)
                              :(_current_timing==TIMING_INSTANT?tc.count:tc.cumulated_count) ;
   };

    switch(_friend_graph_type)
	{
	case GRAPH_TYPE_SINGLE:
		switch(_service_graph_type)
		{
		case GRAPH_TYPE_SINGLE:		// single friend, single service => one curve per service sub_id
		{
            std::vector<RSTrafficClueExt> clue_per_sub_id(256) ;

            for(std::list<RSTrafficClueExt>::const_iterator  it(lst.begin());it!=lst.end();++it)
                if(it->peer_id == _current_selected_friend && it->service_id == _current_selected_service)
                    clue_per_sub_id[it->service_sub_id] += *it ;

            for(uint32_t i=0;i<256;++i)
                if(clue_per_sub_id[i].cumulated_count > 0) // checks if i corresponds to an active service
                    vals[makeSubItemName(clue_per_sub_id[i].service_id,i)] = select_value(clue_per_sub_id[i]);
        }
			break ;

		case GRAPH_TYPE_ALL:		// single friend, all services => one curve per service id
		{
            std::map<uint16_t,RSTrafficClueExt> clue_per_id ;

            for(std::list<RSTrafficClueExt>::const_iterator  it(lst.begin());it!=lst.end();++it)
                if(it->peer_id == _current_selected_friend)
                    clue_per_id[it->service_id] += *it ;

            for(std::map<uint16_t,RSTrafficClueExt>::const_iterator it(clue_per_id.begin());it!=clue_per_id.end();++it)
                    vals[mServiceInfoMap[it->first].mServiceName] = select_value(it->second);
		}
			break ;
		case GRAPH_TYPE_SUM:	// single friend, sum services => one curve
		{
            RSTrafficClueExt total ;
            std::map<uint16_t,RSTrafficClueExt> clue_per_id ;

            for(std::list<RSTrafficClueExt>::const_iterator  it(lst.begin());it!=lst.end();++it)
                if(it->peer_id == _current_selected_friend)
                    total += *it ;

            vals[visibleFriendName(_current_selected_friend)] = select_value(total);
		}
		}
		break ;

	case GRAPH_TYPE_ALL:
		switch(_service_graph_type)
		{
		case GRAPH_TYPE_SINGLE: // all friends, single service => one curve per friend for that service
		{
            std::map<RsPeerId,RSTrafficClueExt> clue_per_peer_id;

            for(std::list<RSTrafficClueExt>::const_iterator  it(lst.begin());it!=lst.end();++it)
				if(it->service_id == _current_selected_service)
					clue_per_peer_id[it->peer_id] += *it ;

            for(std::map<RsPeerId,RSTrafficClueExt>::const_iterator it(clue_per_peer_id.begin());it!=clue_per_peer_id.end();++it)
                vals[visibleFriendName(it->first)] = select_value(it->second);
		}
			break ;

		case GRAPH_TYPE_ALL: std::cerr << "(WW) Impossible situation. Cannot draw graph in mode All/All. Reverting to sum." << std::endl;
			/* fallthrough */
		case GRAPH_TYPE_SUM:		// all friends, sum of services => one curve per friend
		{
            RSTrafficClueExt total ;
            std::map<RsPeerId,RSTrafficClueExt> clue_per_peer_id ;

            for(std::list<RSTrafficClueExt>::const_iterator  it(lst.begin());it!=lst.end();++it)
				clue_per_peer_id[it->peer_id] += *it;

            for(std::map<RsPeerId,RSTrafficClueExt>::const_iterator it(clue_per_peer_id.begin());it!=clue_per_peer_id.end();++it)
                vals[visibleFriendName(it->first)] = select_value(it->second);
		}
			break ;
		}
		break ;

	case GRAPH_TYPE_SUM:
		switch(_service_graph_type)
		{
		case GRAPH_TYPE_SINGLE:	// sum of friends, single service => one curve per service sub id summed over all friends
		{
            std::vector<RSTrafficClueExt> clue_per_sub_id(256) ;

            for(std::list<RSTrafficClueExt>::const_iterator  it(lst.begin());it!=lst.end();++it)
				if(it->service_id == _current_selected_service)
                {
					clue_per_sub_id[it->service_sub_id] += *it ;
					clue_per_sub_id[it->service_sub_id].service_id = it->service_id ;
                }

			for(uint32_t i=0;i<256;++i)
                if(clue_per_sub_id[i].cumulated_count > 0) // checks if this corresponds to an active service
                    vals[makeSubItemName(clue_per_sub_id[i].service_id,i)] = select_value(clue_per_sub_id[i]);
		}
			break ;

		case GRAPH_TYPE_ALL:	// sum of friends, all services => one curve per service id summed over all friends
		{
            std::map<uint16_t,RSTrafficClueExt> clue_per_service ;

            for(std::list<RSTrafficClueExt>::const_iterator  it(lst.begin());it!=lst.end();++it)
				clue_per_service[it->service_id] += *it;

            for(std::map<uint16_t,RSTrafficClueExt>::const_iterator it(clue_per_service.begin());it!=clue_per_service.end();++it)
                vals[mServiceInfoMap[it->first].mServiceName] = select_value(it->second);
		}
			break ;

		case GRAPH_TYPE_SUM: 	// sum of friends, sum of services => one single curve covering the total bandwidth
		{
            RSTrafficClueExt total ;

            for(std::list<RSTrafficClueExt>::const_iterator  it(lst.begin());it!=lst.end();++it)
				total += *it;

            vals[QString("Total").toStdString()] = select_value(total);
		}
			break ;
		}
		break ;
	}
}

std::string BWGraphSource::visibleFriendName(const RsPeerId& pid) const
{
    std::map<RsPeerId,std::string>::const_iterator it = mVisibleFriends.find(pid) ;

    if(it != mVisibleFriends.end())
        return it->second;
    else
        return std::string("[unknown]") ;
}

BWGraphSource::BWGraphSource()
	: RSGraphSource()
{

    _total_sent =0;
    _total_recv =0;

    _friend_graph_type = GRAPH_TYPE_SUM;
    _service_graph_type = GRAPH_TYPE_SUM;

    _current_selected_friend.clear() ;
    _current_selected_service = 0;
    _current_unit = UNIT_KILOBYTES;
    _current_direction = DIRECTION_UP;

    RsPeerServiceInfo rspsi ;
    rsServiceControl->getOwnServices(rspsi) ;

    for(std::map<uint32_t,RsServiceInfo>::const_iterator it(rspsi.mServiceList.begin());it!=rspsi.mServiceList.end();++it)
    {
        mServiceInfoMap[ (it->first >> 8) & 0xffff ] = it->second ;

        rsServiceControl->getServiceItemNames(it->first,mServiceInfoMap[(it->first >> 8) & 0xffff].item_names) ;
    }
}

void BWGraphSource::getValues(std::map<std::string,float>& values) const
{
    RsConfigDataRates totalRates;
    rsConfig->getTotalBandwidthRates(totalRates);

    values.insert(std::make_pair(std::string("Bytes in"),1024 * (float)totalRates.mRateIn)) ;
    values.insert(std::make_pair(std::string("Bytes out"),1024 * (float)totalRates.mRateOut)) ;

    _total_sent += 1024 * totalRates.mRateOut * _update_period_msecs/1000.0f ;
    _total_recv += 1024 * totalRates.mRateIn * _update_period_msecs/1000.0f ;
}

QString BWGraphSource::unitName() const
{
    if(_current_timing == TIMING_CUMULATED)
        return tr("KB");

    if(_current_unit == UNIT_KILOBYTES)
        return tr("KB/s");

    return QString();
}

QString BWGraphSource::displayValue(float v) const
{
    if(_current_unit == UNIT_KILOBYTES)
    {
        QString s = niceNumber(v);

        if(_current_timing == TIMING_INSTANT)
            return s + "/s";
        else
            return s ;
    }
    else if(_current_unit == UNIT_COUNT)
    {
	    if(v < 1000)
		    return QString::number(v,'f',0) ;
	    else if(v < 1000*1024)
		    return QString::number(v/1024.0,'f',1) + "k" ;
	    else
		    return QString::number(v/(1024.0*1024),'f',1) + "M" ;
    }
    else
    return QString() ;
}

QString BWGraphSource::legend(int i,float v,bool show_value) const
{
	return RSGraphSource::legend(i,v,show_value) ;
}
QString BWGraphSource::niceNumber(float v) const
{
    if(v < 1000)
        return QString::number(v,'f',2) + " B" ;
    else if(v < 1000*1024)
        return QString::number(v/1024.0,'f',2) + " KB" ;
    else if(v < 1000*1024*1024)
        return QString::number(v/(1024*1024.0),'f',2) + " MB" ;
    else
        return QString::number(v/(1024*1024.0*1024),'f',2) + " GB";
}

void BWGraphSource::setSelector(int selector_type,int selector_value,const std::string& selector_client_string)
{
#ifdef BWGRAPH_DEBUG
    std::cerr << "Setting Graph Source selector to " << selector_type << " - " << selector_value << " - " << selector_client_string << std::endl;
#endif

    bool changed = false ;

    if(selector_type == SELECTOR_TYPE_FRIEND && (_friend_graph_type != selector_value || (selector_value == GRAPH_TYPE_SINGLE && selector_client_string != _current_selected_friend.toStdString())))
    {
        if(selector_value ==  GRAPH_TYPE_SINGLE)
	    {

		    RsPeerId ns(selector_client_string) ;

		    if(!ns.isNull())
		    {
			    _current_selected_friend = ns ;
			    changed = true ;
                _friend_graph_type = selector_value ;
		    }

		    else
			    std::cerr << "(EE) Cannot set current friend to " << selector_client_string << ": unrecognized friend string." << std::endl;
	    }
	    else
	    {
		    changed = true ;
            _friend_graph_type = selector_value ;
	    }
    }
    else if(selector_type == SELECTOR_TYPE_SERVICE
                    && (_service_graph_type != selector_value || (selector_value == GRAPH_TYPE_SINGLE && selector_client_string != QString::number(_current_selected_service,16).toStdString())))
    {
        if(selector_value ==  GRAPH_TYPE_SINGLE)
	    {
		    //bool ok = false ;
		    int tmp = QString::fromStdString(selector_client_string).toInt() ;

		    if(tmp > 0 && tmp < 0x10000)
		    {
			    _current_selected_service = tmp ;

			    changed = true ;
                _service_graph_type = selector_value ;
		    }
                    else
			    std::cerr << "(EE) Cannot set current service to " << selector_client_string << ": unrecognized service string." << std::endl;

	    }
	    else
	    {
		    changed = true ;
            _service_graph_type = selector_value ;
	    }
    }

    // now re-convert all traffic history into the appropriate curves

    if(changed)
	    recomputeCurrentCurves() ;
}
void BWGraphSource::setUnit(int unit)
{
    if(unit == _current_unit)
        return ;

    _current_unit = unit ;

    recomputeCurrentCurves() ;
}

void BWGraphSource::setDirection(int dir)
{
    if(dir == _current_direction)
        return ;

    _current_direction = dir ;

    recomputeCurrentCurves() ;
}

void BWGraphSource::setTiming(int t)
{
    if(t == _current_timing)
        return;

    _current_timing = t;
    recomputeCurrentCurves();
}
void BWGraphSource::recomputeCurrentCurves()
{
#ifdef BWGRAPH_DEBUG
    std::cerr << "BWGraphSource: recomputing current curves." << std::endl;
#endif

    _points.clear() ;

    // now, convert data to current curve points.

    std::set<std::string> used_values_ref ;

    for(std::list<TrafficHistoryChunk>::const_iterator it(mTrafficHistory.begin());it!=mTrafficHistory.end();++it)
    {
	    std::map<std::string,float> vals ;
	    qint64 ms = (*it).time_stamp ;

	    std::set<std::string> unused_values = used_values_ref ;

        if(_current_direction == (DIRECTION_UP | DIRECTION_DOWN))
        {
            std::map<std::string,float> vals1,vals2 ;
            convertTrafficClueToValues((*it).out_rstcl,vals1) ;
            convertTrafficClueToValues((*it).in_rstcl,vals2) ;

            for(auto it:vals1) vals[it.first + " (sent)"] = it.second;
            for(auto it:vals2) vals[it.first + " (received)"] = it.second;
        }
        else if(_current_direction & DIRECTION_UP)
            convertTrafficClueToValues((*it).out_rstcl,vals) ;
        else
		    convertTrafficClueToValues((*it).in_rstcl,vals) ;

	    for(std::map<std::string,float>::iterator it2=vals.begin();it2!=vals.end();++it2)
	    {
		    _points[it2->first].push_back(std::make_pair(ms,it2->second)) ;
		    used_values_ref.insert(it2->first) ;
		    unused_values.erase(it2->first) ;
	    }

	    for(std::set<std::string>::const_iterator it(unused_values.begin());it!=unused_values.end();++it)
		    _points[*it].push_back(std::make_pair(ms,0)) ;
    }

#ifdef BWGRAPH_DEBUG
    std::cerr << "  points() contains " << _points.size() << " curves." << std::endl;
#endif
}

BWGraph::BWGraph(QWidget *parent) : RSGraphWidget(parent)
{
    _local_source = new BWGraphSource() ;

    _local_source->setCollectionTimeLimit(30*60*1000) ; // 30  mins
    _local_source->setCollectionTimePeriod(1000) ;      // collect every second
    _local_source->setDigits(2) ;
    _local_source->start() ;
    _local_source->setUnit(BWGraphSource::UNIT_KILOBYTES) ;
    _local_source->setDirection(BWGraphSource::DIRECTION_UP) ;
    _local_source->setSelector(BWGraphSource::SELECTOR_TYPE_FRIEND,BWGraphSource::GRAPH_TYPE_ALL) ;
    _local_source->setSelector(BWGraphSource::SELECTOR_TYPE_SERVICE,BWGraphSource::GRAPH_TYPE_SUM) ;

    setSource(_local_source) ;

    setTimeScale(1.0f) ; // 1 pixels per second of time.

    setFlags(RSGRAPH_FLAGS_LOG_SCALE_Y) ;
    resetFlags(RSGRAPH_FLAGS_PAINT_STYLE_PLAIN) ;

    setFlags(RSGRAPH_FLAGS_SHOW_LEGEND) ;
}

BWGraph::~BWGraph()
{
    //delete _local_source ;//Will be deleted by RSGraphWidget destructor
}

