/*******************************************************************************
 * gui/statistics/BWGraph.h                                                    *
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

#pragma once

#include "retroshare/rsconfig.h"
#include "retroshare/rsservicecontrol.h"
#include <gui/common/RSGraphWidget.h>

class BWGraphSource: public RSGraphSource
{
public:
    struct TrafficHistoryChunk
    {
        time_t time_stamp;
        std::list<RSTrafficClue> out_rstcl ;
        std::list<RSTrafficClue>  in_rstcl ;
    };
    class RsServiceInfoWithNames: public RsServiceInfo
    {
    public:
        RsServiceInfoWithNames(const RsServiceInfo& s) : RsServiceInfo(s) {}
        RsServiceInfoWithNames(){}

		std::map<uint8_t,std::string> item_names ;
    };

    BWGraphSource() ;
	virtual ~BWGraphSource() {}

    enum { SELECTOR_TYPE_FRIEND=0x00, SELECTOR_TYPE_SERVICE=0x01 };
    enum { GRAPH_TYPE_SINGLE=0x00, GRAPH_TYPE_ALL=0x01, GRAPH_TYPE_SUM=0x02 };
    enum { UNIT_KILOBYTES=0x00, UNIT_COUNT=0x01 };
    enum { DIRECTION_UP=0x00, DIRECTION_DOWN=0x01 };

    // re-derived from RSGraphSource

	virtual void getCumulatedValues(std::vector<float>& vals) const;
    virtual void getValues(std::map<std::string,float>& values) const;
    virtual QString displayValue(float v) const;
    virtual QString legend(int i,float v,bool show_value=true) const;
    virtual void update();
    QString unitName() const ;

    // own methdods to control what's used to create displayed info

    void setSelector(int selector_type, int graph_type, const std::string& selector_client_string = std::string()) ;
    void setDirection(int dir) ;
    void setUnit(int unit) ;

    int direction() const { return _current_direction ;}
    int unit() const { return _current_unit ;}
    int friendGraphType() const { return _friend_graph_type ;}
    int serviceGraphType() const { return _service_graph_type ;}

    const std::map<RsPeerId,std::string>& visibleFriends() const { return mVisibleFriends; }
    const std::set<uint16_t>& visibleServices() const { return mVisibleServices; }

protected:
    void convertTrafficClueToValues(const std::list<RSTrafficClue> &lst, std::map<std::string, float> &vals) const;
	std::string makeSubItemName(uint16_t service_id,uint8_t sub_item_type) const;
    void recomputeCurrentCurves() ;
    std::string visibleFriendName(const RsPeerId &pid) const ;

private:
    QString niceNumber(float v) const;

    mutable float _total_sent ;
    mutable float _total_recv ;

    int _friend_graph_type ;
    int _service_graph_type ;

	float _total_duration_seconds ;

    RsPeerId    _current_selected_friend ;
    uint16_t    _current_selected_service ;
    int         _current_unit ;
    int         _current_direction ;

    std::list<TrafficHistoryChunk> mTrafficHistory ;

    std::map<RsPeerId,std::string> mVisibleFriends ;
    std::set<uint16_t> mVisibleServices ;

    mutable std::map<uint16_t,RsServiceInfoWithNames> mServiceInfoMap ;
};

class BWGraph: public RSGraphWidget
{
	public:
        BWGraph(QWidget *parent);
        ~BWGraph();

    void setSelector(int selector_type, int graph_type, const std::string& selector_client_string = std::string())  { _local_source->setSelector(selector_type,graph_type,selector_client_string) ; }
    void setDirection(int dir) { _local_source->setDirection(dir); }
    void setUnit(int unit) { _local_source->setUnit(unit) ;}

    const std::map<RsPeerId,std::string>& visibleFriends() const { return _local_source->visibleFriends(); }
    const std::set<uint16_t>& visibleServices() const { return _local_source->visibleServices(); }
protected:
        BWGraphSource *_local_source ;
};
