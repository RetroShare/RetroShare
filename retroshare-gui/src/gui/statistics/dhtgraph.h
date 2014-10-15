/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2014 RetroShare Team
 * Copyright (c) 2006-2007, crypton
 * Copyright (c) 2006, Matt Edman, Justin Hipple
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

#pragma once

#include <QApplication>
#include <gui/common/RSGraphWidget.h>

#include <retroshare/rsdht.h>
#include <retroshare/rsconfig.h>
#include "dhtgraph.h"

class DHTGraphSource: public RSGraphSource
{
	public:
		virtual int n_values() const
		{
			return 1 ;
		}
		virtual void getValues(std::map<std::string,float>& values) const
		{
			RsConfigNetStatus config;
			rsConfig->getConfigNetStatus(config);

			if (config.DHTActive && config.netDhtOk)
			{
				values.insert(std::make_pair(std::string("RS Net size"),(float)config.netDhtRsNetSize)) ;
				//values.insert(std::make_pair(std::string("GLobal Net size"),(float)config.netDhtNetSize)) ;
			}
			else
			{
				values.insert(std::make_pair(std::string("RS Net size"),0.0f)) ;
				//values.insert(std::make_pair(std::string("GLobal Net size"),0.0f)) ;
			}
		}

		virtual QString unitName() const { return tr("users"); }
};


class DhtGraph : public RSGraphWidget
{
	public:
		DhtGraph(QWidget *parent = 0)
			: RSGraphWidget(parent)
		{
			DHTGraphSource *src = new DHTGraphSource() ;

			src->setCollectionTimeLimit(30*60*1000) ; // 30  mins
			src->setCollectionTimePeriod(1000) ;      // collect every second
			src->start() ;

			addSource(src) ;

			setTimeScale(1.0f) ; // 1 pixels per second of time.
			setScaleParams(0) ;

			resetFlags(RSGRAPH_FLAGS_LOG_SCALE_Y) ;
			setFlags(RSGRAPH_FLAGS_PAINT_STYLE_PLAIN) ;
		}
};
