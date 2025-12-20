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

#pragma once

#include <QTreeWidget>
#include <QTimer>
#include <QHeaderView>
#include <retroshare-gui/RsAutoUpdatePage.h>
#include "ui_RttStatistics.h"

class RttStatistics: public MainPage, public Ui::RttStatistics
{
    Q_OBJECT

public:
    RttStatistics(QWidget *parent = NULL);
    ~RttStatistics();

private slots:
    void updateRttValues();

private:
    void processSettings(bool bLoad);
    bool m_bProcessSettings;

    QTimer *m_timer;
};
