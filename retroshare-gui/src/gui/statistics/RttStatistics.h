/*******************************************************************************
 * gui/statistics/RttStatistics.h                                              *
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

/*******************************************************************************
 * gui/statistics/RttStatistics.h                                              *
 *******************************************************************************/

#pragma once

#include <QPoint>
#include <QTimer>
#include <QTreeWidget>
#include <QHeaderView>
#include <retroshare/rsrtt.h>
#include <retroshare-gui/RsAutoUpdatePage.h>

#include "ui_RttStatistics.h"
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
    RttStatisticsGraph(QWidget *parent);
};

class RttStatistics: public MainPage, public Ui::RttStatistics
{
    Q_OBJECT

public:
    RttStatistics(QWidget *parent = NULL) ;
    ~RttStatistics();

private slots:
    void updateRttValues(); // Slot for table update

private:
    void processSettings(bool bLoad);
    bool m_bProcessSettings;

    RttStatisticsGraph *_tst_CW ;
    QTimer *m_timer; // Timer for the table
} ;
