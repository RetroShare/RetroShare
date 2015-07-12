#include "BWGraph.h"


void BWGraphSource::update()
{
//    std::cerr << "Updating BW graphsource..." << std::endl;

    std::list<RSTrafficClue> in_rstcl ;
    std::list<RSTrafficClue> out_rstcl ;

    TrafficHistoryChunk thc ;
    rsConfig->getTrafficInfo(thc.out_rstcl,thc.in_rstcl);

    // keep track of them, in case we need to change the sorting

    thc.time_stamp = time(NULL) ;
    mTrafficHistory.push_back(thc) ;

    std::map<std::string,float> vals ;
    convertTrafficClueToValues(thc.out_rstcl,vals) ;

    qint64 ms = getTime() ;

    for(std::map<std::string,float>::iterator it=vals.begin();it!=vals.end();++it)
    {
        std::list<std::pair<qint64,float> >& lst(_points[it->first]) ;

        lst.push_back(std::make_pair(ms,it->second)) ;

        for(std::list<std::pair<qint64,float> >::iterator it2=lst.begin();it2!=lst.end();)
            if( ms - (*it2).first > _time_limit_msecs)
            {
                //std::cerr << "  removing old value with time " << (*it).first/1000.0f << std::endl;
                it2 = lst.erase(it2) ;
            }
            else
                break ;
    }

    // remove empty lists

    for(std::map<std::string,std::list<std::pair<qint64,float> > >::iterator it=_points.begin();it!=_points.end();)
        if(it->second.empty())
    {
        std::map<std::string,std::list<std::pair<qint64,float> > >::iterator tmp(it) ;
        ++tmp;
        _points.erase(it) ;
        it=tmp ;
    }
        else
            ++it ;

    // also clears history

    for(std::list<TrafficHistoryChunk>::iterator it = mTrafficHistory.begin();it!=mTrafficHistory.end();++it)
            if( ms - 1000*(*it).time_stamp > _time_limit_msecs)
                it = mTrafficHistory.erase(it) ;
            else
                break ;

}

void BWGraphSource::convertTrafficClueToValues(const std::list<RSTrafficClue>& lst,std::map<std::string,float>& vals) const
{
	vals.clear() ;

	switch(_friend_graph_type)
	{
	case GRAPH_TYPE_SINGLE:
		switch(_service_graph_type)
		{
		case GRAPH_TYPE_SINGLE:		// single friend, single service => one curve per service sub_id
		{
			std::vector<RSTrafficClue> clue_per_sub_id(256) ;

			for(std::list<RSTrafficClue>::const_iterator  it(lst.begin());it!=lst.end();++it)
				if(it->peer_id == _current_selected_friend && it->service_id == _current_selected_service)
					clue_per_sub_id[it->service_sub_id] += *it ;

			for(uint32_t i=0;i<256;++i)
				if(clue_per_sub_id[i].count > 0)
					vals[QString::number(i,16).toStdString()] = (_current_unit == UNIT_KILOBYTES)?(clue_per_sub_id[i].size/1000.0f):(clue_per_sub_id[i].count) ;
		}
			break ;

		case GRAPH_TYPE_ALL:		// single friend, all services => one curve per service id
		{
			std::map<uint16_t,RSTrafficClue> clue_per_id ;

			for(std::list<RSTrafficClue>::const_iterator  it(lst.begin());it!=lst.end();++it)
				if(it->peer_id == _current_selected_friend)
					clue_per_id[it->service_id] += *it ;

			for(std::map<uint16_t,RSTrafficClue>::const_iterator it(clue_per_id.begin());it!=clue_per_id.end();++it)
				vals[QString::number(it->first,16).toStdString()] = (_current_unit == UNIT_KILOBYTES)?(it->second.size/1000.0f):(it->second.count) ;
		}
			break ;
		case GRAPH_TYPE_SUM:	// single friend, sum services => one curve
		{
			RSTrafficClue total ;
			std::map<uint16_t,RSTrafficClue> clue_per_id ;

			for(std::list<RSTrafficClue>::const_iterator  it(lst.begin());it!=lst.end();++it)
				if(it->peer_id == _current_selected_friend)
					total += *it ;

			vals[_current_selected_friend_name] = (_current_unit == UNIT_KILOBYTES)?(total.size/1000.0f):(total.count) ;
		}
		}
		break ;
	case GRAPH_TYPE_ALL:
		switch(_service_graph_type)
		{
		case GRAPH_TYPE_SINGLE: // all friends, single service => one curve per friend for that service
		{
			std::map<RsPeerId,RSTrafficClue> clue_per_peer_id;

			for(std::list<RSTrafficClue>::const_iterator  it(lst.begin());it!=lst.end();++it)
				if(it->service_id == _current_selected_service)
					clue_per_peer_id[it->peer_id] += *it ;

			for(std::map<RsPeerId,RSTrafficClue>::const_iterator it(clue_per_peer_id.begin());it!=clue_per_peer_id.end();++it)
				vals[it->first.toStdString()] = (_current_unit == UNIT_KILOBYTES)?(it->second.size/1000.0f):(it->second.count) ;
		}
			break ;

		case GRAPH_TYPE_ALL: std::cerr << "(WW) Impossible situation. Cannot draw graph in mode All/All. Reverting to sum." << std::endl;
		case GRAPH_TYPE_SUM:		// all friends, sum of services => one curve per friend
		{
			RSTrafficClue total ;
			std::map<RsPeerId,RSTrafficClue> clue_per_peer_id ;

			for(std::list<RSTrafficClue>::const_iterator  it(lst.begin());it!=lst.end();++it)
				clue_per_peer_id[it->peer_id] += *it;

			for(std::map<RsPeerId,RSTrafficClue>::const_iterator it(clue_per_peer_id.begin());it!=clue_per_peer_id.end();++it)
				vals[it->first.toStdString()] = (_current_unit == UNIT_KILOBYTES)?(it->second.size/1000.0f):(it->second.count) ;
		}
			break ;
		}
		break ;

	case GRAPH_TYPE_SUM:
		switch(_service_graph_type)
		{
		case GRAPH_TYPE_SINGLE:	// sum of friends, single service => one curve per service sub id summed over all friends
		{
			std::vector<RSTrafficClue> clue_per_sub_id(256) ;

			for(std::list<RSTrafficClue>::const_iterator  it(lst.begin());it!=lst.end();++it)
				if(it->service_id == _current_selected_service)
					clue_per_sub_id[it->service_sub_id] += *it ;

			for(uint32_t i=0;i<256;++i)
				if(clue_per_sub_id[i].count > 0)
					vals[QString::number(i,16).toStdString()] = (_current_unit == UNIT_KILOBYTES)?(clue_per_sub_id[i].size/1000.0f):(clue_per_sub_id[i].count) ;
		}
			break ;

		case GRAPH_TYPE_ALL:	// sum of friends, all services => one curve per service id summed over all friends
		{
			std::map<uint16_t,RSTrafficClue> clue_per_service ;

			for(std::list<RSTrafficClue>::const_iterator  it(lst.begin());it!=lst.end();++it)
				clue_per_service[it->service_id] += *it;

			for(std::map<uint16_t,RSTrafficClue>::const_iterator it(clue_per_service.begin());it!=clue_per_service.end();++it)
				vals[QString::number(it->first,16).toStdString()] = (_current_unit == UNIT_KILOBYTES)?(it->second.size/1000.0f):(it->second.count) ;
		}
			break ;

		case GRAPH_TYPE_SUM: 	// sum of friends, sum of services => one single curve covering the total bandwidth
		{
			RSTrafficClue total ;

			for(std::list<RSTrafficClue>::const_iterator  it(lst.begin());it!=lst.end();++it)
				total += *it;

			vals[QString("Total").toStdString()] = (_current_unit == UNIT_KILOBYTES)?(total.size/1000.0f):(total.count) ;
		}
			break ;
		}
		break ;
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

QString BWGraphSource::unitName() const { return tr("KB/s"); }

QString BWGraphSource::displayValue(float v) const
{
    if(v < 1000)
        return QString::number(v,'f',2) + " B/s" ;
    else if(v < 1000*1024)
        return QString::number(v/1024.0,'f',2) + " KB/s" ;
    else
        return QString::number(v/(1024.0*1024),'f',2) + " MB/s" ;
}

QString BWGraphSource::legend(int i,float v) const
{
    if(i==0)
        return RSGraphSource::legend(i,v) + " Total: " + niceNumber(_total_recv) ;
    else
        return RSGraphSource::legend(i,v) + " Total: " + niceNumber(_total_sent) ;
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

void BWGraphSource::setSelector(int selector_class,int selector_type,const std::string& selector_client_string)
{

}

BWGraph::BWGraph(QWidget *parent) : RSGraphWidget(parent)
{
    _local_source = new BWGraphSource() ;

    _local_source->setCollectionTimeLimit(30*60*1000) ; // 30  mins
    _local_source->setCollectionTimePeriod(1000) ;      // collect every second
    _local_source->setDigits(2) ;
    _local_source->start() ;

    setSource(_local_source) ;

    setTimeScale(1.0f) ; // 1 pixels per second of time.

    resetFlags(RSGRAPH_FLAGS_LOG_SCALE_Y) ;
    resetFlags(RSGRAPH_FLAGS_PAINT_STYLE_PLAIN) ;

    setFlags(RSGRAPH_FLAGS_SHOW_LEGEND) ;
}

BWGraphSource *BWGraph::source()
{
    return _local_source ;
}
