/*******************************************************************************
 * gui/statistics/turtlegraph.h                                                *
 *                                                                             *
 * Copyright (c) 2011 Retroshare Team <retroshare.project@gmail.com>           *
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

#include "gui/settings/rsharesettings.h"

#include "retroshare/rsturtle.h"
#include <gui/common/RSGraphWidget.h>

class TurtleGraphSource: public RSGraphSource
{
	public:
		virtual void getValues(std::map<std::string,float>& values) const
		{
			TurtleTrafficStatisticsInfo info ;
			rsTurtle->getTrafficStatistics(info) ;

            values.insert(std::make_pair(QObject::tr("TR up").toStdString(),(float)info.tr_up_Bps)) ;
            values.insert(std::make_pair(QObject::tr("TR dn").toStdString(),(float)info.tr_dn_Bps)) ;
            values.insert(std::make_pair(QObject::tr("Data up").toStdString(),(float)info.data_up_Bps)) ;
            values.insert(std::make_pair(QObject::tr("Data dn").toStdString(),(float)info.data_dn_Bps)) ;
            values.insert(std::make_pair(QObject::tr("Data forward").toStdString(),(float)info.unknown_updn_Bps)) ;
		}

    virtual QString displayValue(float v) const
    {
        if(v < 1000)
            return QString::number(v,'f',2) + " B/s" ;
        else if(v < 1000*1024)
            return QString::number(v/1024.0,'f',2) + " KB/s" ;
        else
            return QString::number(v/(1024.0*1024),'f',2) + " MB/s" ;
    }
};

class TurtleGraph: public RSGraphWidget
{
	public:
		TurtleGraph(QWidget *parent)
			: RSGraphWidget(parent)
		{
            TurtleGraphSource *src = new TurtleGraphSource() ;

			src->setCollectionTimeLimit(30*60*1000) ; // 30  mins
            src->setCollectionTimePeriod(1000) ;      // collect every second
            src->setDigits(2) ;
			src->start() ;

			setSource(src) ;

			setTimeScale(1.0f) ; // 1 pixels per second of time.

			resetFlags(RSGRAPH_FLAGS_LOG_SCALE_Y) ;
			resetFlags(RSGRAPH_FLAGS_PAINT_STYLE_PLAIN) ;
			setFlags(RSGRAPH_FLAGS_SHOW_LEGEND) ;

			int graphColor = Settings->valueFromGroup("BandwidthStatsWidget", "cmbGraphColor", 0).toInt();

			if(graphColor==0)
				resetFlags(RSGraphWidget::RSGRAPH_FLAGS_DARK_STYLE);
			else
				setFlags(RSGraphWidget::RSGRAPH_FLAGS_DARK_STYLE);
        }
};


