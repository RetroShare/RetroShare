#pragma once

#include "retroshare/rsconfig.h"
#include <gui/common/RSGraphWidget.h>

class BWGraphSource: public RSGraphSource
{
public:
    virtual void getValues(std::map<std::string,float>& values) const
    {
    RsConfigDataRates totalRates;
    rsConfig->getTotalBandwidthRates(totalRates);

    values.insert(std::make_pair(std::string("Bytes in"),1024 * (float)totalRates.mRateIn)) ;
    values.insert(std::make_pair(std::string("Bytes out"),1024 * (float)totalRates.mRateOut)) ;

    _total_sent += 1024 * totalRates.mRateOut * _update_period_msecs/1000.0f ;
    _total_recv += 1024 * totalRates.mRateIn * _update_period_msecs/1000.0f ;
    }

    virtual QString unitName() const { return tr("KB/s"); }

    virtual QString displayValue(float v) const
    {
        if(v < 1000)
            return QString::number(v,'f',2) + " B/s" ;
        else if(v < 1000*1024)
            return QString::number(v/1024.0,'f',2) + " KB/s" ;
        else
            return QString::number(v/(1024.0*1024),'f',2) + " MB/s" ;
    }

    virtual QString legend(int i,float v) const
    {
        if(i==0)
            return RSGraphSource::legend(i,v) + " Total: " + niceNumber(_total_recv) ;
        else
            return RSGraphSource::legend(i,v) + " Total: " + niceNumber(_total_sent) ;
    }
    private:
    QString niceNumber(float v) const
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

    mutable float _total_sent ;
    mutable float _total_recv ;
};

class BWGraph: public RSGraphWidget
{
	public:
		BWGraph(QWidget *parent)
			: RSGraphWidget(parent)
		{
			BWGraphSource *src = new BWGraphSource() ;

			src->setCollectionTimeLimit(30*60*1000) ; // 30  mins
            src->setCollectionTimePeriod(1000) ;      // collect every second
            src->setDigits(2) ;
			src->start() ;

			addSource(src) ;

			setTimeScale(1.0f) ; // 1 pixels per second of time.

			resetFlags(RSGRAPH_FLAGS_LOG_SCALE_Y) ;
            resetFlags(RSGRAPH_FLAGS_PAINT_STYLE_PLAIN) ;

            setFlags(RSGRAPH_FLAGS_SHOW_LEGEND) ;
        }
};


