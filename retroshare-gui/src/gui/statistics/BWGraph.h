#pragma once

#include "retroshare/rsconfig.h"
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

    enum { SELECTOR_TYPE_FRIEND=0x00, SELECTOR_TYPE_SERVICE=0x01 };
    enum { GRAPH_TYPE_SINGLE=0x00, GRAPH_TYPE_ALL=0x01, GRAPH_TYPE_SUM=0x02 };
    enum { UNIT_KILOBYTES=0x00, UNIT_COUNT=0x01 };

    // re-derived from RSGraphSource

    virtual void getValues(std::map<std::string,float>& values) const;
    virtual QString displayValue(float v) const;
    virtual QString legend(int i,float v) const;
    virtual void update();
    QString unitName() const ;

    // own methdods to control what's used to create displayed info

    void setSelector(int selector_class,int selector_type,const std::string& selector_client_string = std::string()) ;

protected:
    void convertTrafficClueToValues(const std::list<RSTrafficClue> &lst, std::map<std::string, float> &vals) const;

private:
    QString niceNumber(float v) const;

    mutable float _total_sent ;
    mutable float _total_recv ;

    int _friend_graph_type ;
    int _service_graph_type ;

    RsPeerId _current_selected_friend ;
    std::string _current_selected_friend_name ;
    uint16_t _current_selected_service ;
    int _current_unit ;

    std::list<TrafficHistoryChunk> mTrafficHistory ;
};

class BWGraph: public RSGraphWidget
{
	public:
        BWGraph(QWidget *parent);

        BWGraphSource *source() ;

protected:
        BWGraphSource *_local_source ;
};
