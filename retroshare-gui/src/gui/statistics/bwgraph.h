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

    values.insert(std::make_pair(std::string("Bytes in"),(float)totalRates.mRateIn)) ;
    values.insert(std::make_pair(std::string("Bytes out"),(float)totalRates.mRateOut)) ;
    }

    virtual QString unitName() const { return tr("KB/s"); }
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
			src->start() ;

			addSource(src) ;

			setTimeScale(1.0f) ; // 1 pixels per second of time.
			setScaleParams(2) ;

			resetFlags(RSGRAPH_FLAGS_LOG_SCALE_Y) ;
            resetFlags(RSGRAPH_FLAGS_PAINT_STYLE_PLAIN) ;

            setFlags(RSGRAPH_FLAGS_SHOW_LEGEND) ;
        }
};


