/*******************************************************************************
 * gui/statistics/BwCtrlWindow.h                                               *
 *                                                                             *
 * Copyright (c) 2012 Robert Fernie   <retroshare.project@gmail.com>           *
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

#include <QMainWindow>

#include <QAbstractItemDelegate>

#include "RsAutoUpdatePage.h"
#include "gui/common/RSGraphWidget.h"
#include "ui_BwCtrlWindow.h"

// Defines for download list list columns
#define COLUMN_RSNAME 0
#define COLUMN_PEERID 1
#define COLUMN_IN_RATE 2
#define COLUMN_IN_MAX 3
#define COLUMN_IN_QUEUE 4
#define COLUMN_IN_ALLOC 5
#define COLUMN_IN_ALLOC_SENT 6
#define COLUMN_OUT_RATE 7
#define COLUMN_OUT_MAX 8
#define COLUMN_OUT_QUEUE 9
#define COLUMN_OUT_ALLOC 10
#define COLUMN_OUT_ALLOC_SENT 11
#define COLUMN_ALLOWED RECVD 12
#define COLUMN_COUNT 13


class QModelIndex;
class QPainter;
class BWListDelegate ;

class BwCtrlWindow : public RsAutoUpdatePage,  public Ui::BwCtrlWindow
{
    Q_OBJECT
public:

    BwCtrlWindow(QWidget *parent = 0);
    ~BwCtrlWindow();

    void updateBandwidth();

public slots:
    virtual void updateDisplay() ;

protected:
    BWListDelegate *BWDelegate;

};
