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


#include <QtGlobal>

#include <retroshare/rsdht.h>
#include <retroshare/rsconfig.h>
#include "dhtgraph.h"

class DHTGraphSource: public RSGraphSource
{
public:
    virtual int n_values() const
    {
        return 2 ;
    }
    virtual void getValues(std::vector<float>& values) const
    {
        RsConfigNetStatus config;
        rsConfig->getConfigNetStatus(config);

        if (config.DHTActive && config.netDhtOk)
        {
            values.push_back(config.netDhtRsNetSize) ;
            values.push_back(config.netDhtNetSize) ;
        }
        else
        {
            values.push_back(0.0f) ;
            values.push_back(0.0f) ;
        }
    }
};

/** Default contructor */
DhtGraph::DhtGraph(QWidget *parent)
: RSGraphWidget(parent)
{
    DHTGraphSource *src = new DHTGraphSource() ;

    src->setCollectionTimeLimit(30*60*1000) ; // 30  mins
    src->setCollectionTimePeriod(1000) ;      // collect every second
    src->start() ;

    addSource(src) ;

    setTimeScale(10.0f) ;
}


