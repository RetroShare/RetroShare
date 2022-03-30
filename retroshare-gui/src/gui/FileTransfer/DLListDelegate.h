/*******************************************************************************
 * retroshare-gui/src/gui/FileTransfer/DLListDelegate.h                        *
 *                                                                             *
 * Copyright 2007 by Crypton         <retroshare.project@gmail.com>            *
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

#include <QAbstractItemDelegate>
#include "xprogressbar.h"

class QModelIndex;
class QPainter;

class DLListDelegate: public QAbstractItemDelegate
{
public:
    DLListDelegate(QObject *parent=0);
    virtual ~DLListDelegate(){}
    void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const;

    static constexpr int COLUMN_NAME        =  0;
    static constexpr int COLUMN_SIZE        =  1;
    static constexpr int COLUMN_COMPLETED   =  2;
    static constexpr int COLUMN_DLSPEED     =  3;
    static constexpr int COLUMN_PROGRESS    =  4;
    static constexpr int COLUMN_SOURCES     =  5;
    static constexpr int COLUMN_STATUS      =  6;
    static constexpr int COLUMN_PRIORITY    =  7;
    static constexpr int COLUMN_REMAINING   =  8;
    static constexpr int COLUMN_DOWNLOADTIME=  9;
    static constexpr int COLUMN_ID          = 10;
    static constexpr int COLUMN_LASTDL      = 11;
    static constexpr int COLUMN_PATH        = 12;
    static constexpr int COLUMN_COUNT       = 13;

    static constexpr float PRIORITY_NULL     = 0.0;
    static constexpr float PRIORITY_FASTER   = 0.1;
    static constexpr float PRIORITY_AVERAGE  = 0.2;
    static constexpr float PRIORITY_SLOWER   = 0.3;
};

