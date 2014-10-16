/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 20011, RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include <iostream>
#include <QTimer>
#include <QObject>

#include <QPainter>
#include <QStylePainter>

#include <retroshare/rsrtt.h>
#include <retroshare/rspeers.h>
#include "RttStatistics.h"
#include "time.h"

#include "gui/settings/rsharesettings.h"


RttStatistics::RttStatistics(QWidget *parent)
{
	setupUi(this) ;
	
	m_bProcessSettings = false;

    _tunnel_statistics_F->setWidget( _tst_CW = new RttStatisticsGraph(this) ) ;
	_tunnel_statistics_F->setWidgetResizable(true);
	_tunnel_statistics_F->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	_tunnel_statistics_F->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	_tunnel_statistics_F->viewport()->setBackgroundRole(QPalette::NoRole);
	_tunnel_statistics_F->setFrameStyle(QFrame::NoFrame);
	_tunnel_statistics_F->setFocusPolicy(Qt::NoFocus);
	
	// load settings
    processSettings(true);
}

RttStatistics::~RttStatistics()
{

    // save settings
    processSettings(false);
}

void RttStatistics::processSettings(bool bLoad)
{
    m_bProcessSettings = true;

    Settings->beginGroup(QString("RttStatistics"));

    if (bLoad) {
        // load settings

        // state of splitter
        //splitter->restoreState(Settings->value("Splitter").toByteArray());
    } else {
        // save settings

        // state of splitter
        //Settings->setValue("Splitter", splitter->saveState());

    }

    Settings->endGroup();
    
    m_bProcessSettings = false;

}

QString RttGraphSource::unitName() const
{
    return QObject::tr("secs") ;
}

QString RttGraphSource::displayName(int i) const
{
    int n=0 ;
    for(std::map<std::string, std::list<std::pair<qint64,float> > >::const_iterator it=_points.begin();it!=_points.end();++it,++n)
        if(n==i)
            return QString::fromUtf8(rsPeers->getPeerName(RsPeerId(it->first)).c_str()) ;

    return QString() ;
}

void RttGraphSource::getValues(std::map<std::string,float>& vals) const
{
    std::list<RsPeerId> idList;
    rsPeers->getOnlineList(idList);

    vals.clear() ;
    std::list<RsRttPongResult> results ;

    for(std::list<RsPeerId>::const_iterator it(idList.begin());it!=idList.end();++it)
    {
        rsRtt->getPongResults(*it, 1, results);

    vals[(*it).toStdString()] = results.back().mRTT ;
    }
}

RttStatisticsGraph::RttStatisticsGraph(QWidget *parent)
        : RSGraphWidget(parent)
{
    RttGraphSource *src = new RttGraphSource() ;

    src->setCollectionTimeLimit(10*60*1000) ; // 10 mins
    src->setCollectionTimePeriod(1000) ;     // collect every second
    src->setDigits(1) ;
    src->start() ;

    addSource(src) ;

    setTimeScale(2.0f) ; // 1 pixels per second of time.

    resetFlags(RSGRAPH_FLAGS_LOG_SCALE_Y) ;
    resetFlags(RSGRAPH_FLAGS_PAINT_STYLE_PLAIN) ;
    setFlags(RSGRAPH_FLAGS_SHOW_LEGEND) ;
}
