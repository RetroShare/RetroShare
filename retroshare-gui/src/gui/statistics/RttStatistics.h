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

#pragma once

#include <QPoint>
#include <retroshare/rsrtt.h>
#include "ui_RttStatistics.h"
#include "RsAutoUpdatePage.h"
#include <gui/common/RSGraphWidget.h>

class RttStatisticsWidget ;

class RttGraphSource: public RSGraphSource
{
public:
    RttGraphSource() {}

    virtual void getValues(std::map<std::string,float>& vals) const ;
    virtual QString unitName() const ;
    virtual QString displayName(int i) const ;
};

class RttStatisticsGraph: public RSGraphWidget
{
public:
    explicit RttStatisticsGraph(QWidget *parent);
};

class RttStatistics: public MainPage, public Ui::RttStatistics
{
public:
    explicit RttStatistics(QWidget *parent = NULL) ;
    ~RttStatistics();

private:
    void processSettings(bool bLoad);
    bool m_bProcessSettings;

    RttStatisticsGraph *_tst_CW ;
} ;




