#pragma once

#include "retroshare/rsconfig.h"
#include <gui/common/RSGraphWidget.h>

class BWGraphSource: public RSGraphSource
{
public:
    enum { SELECTOR_TYPE_FRIEND=0x00, SELECTOR_TYPE_SERVICE=0x01 };
    enum { GRAPH_TYPE_SINGLE=0x00, GRAPH_TYPE_ALL=0x01, GRAPH_TYPE_SUM };

    // re-derived from RSGraphSource

    virtual void getValues(std::map<std::string,float>& values) const;
    virtual QString displayValue(float v) const;
    virtual QString legend(int i,float v) const;
    QString unitName() const ;

    // own methdods to control what's used to create displayed info

    void setSelector(int selector_class,int selector_type,const std::string& selector_client_string = std::string()) ;

    private:
    QString niceNumber(float v) const;

    mutable float _total_sent ;
    mutable float _total_recv ;
};

class BWGraph: public RSGraphWidget
{
	public:
        BWGraph(QWidget *parent);

        BWGraphSource *source() ;

protected:
        BWGraphSource *_local_source ;
};
