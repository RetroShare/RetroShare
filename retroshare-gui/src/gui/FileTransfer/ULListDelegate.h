/*******************************************************************************
 * retroshare-gui/src/gui/FileTransfer/ULListDelegate.h                        *
 *                                                                             *
 * Copyright (c) 2007 Crypton         <retroshare.project@gmail.com>           *
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

class QModelIndex;
class QPainter;

class ULListDelegate: public QAbstractItemDelegate
{
public:
    ULListDelegate(QObject *parent=0);
    ~ULListDelegate();
    void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const;

    static constexpr int COLUMN_UNAME        = 0;
    static constexpr int COLUMN_UPEER        = 1;
    static constexpr int COLUMN_USIZE        = 2;
    static constexpr int COLUMN_UTRANSFERRED = 3;
    static constexpr int COLUMN_ULSPEED      = 4;
    static constexpr int COLUMN_UPROGRESS    = 5;
    static constexpr int COLUMN_UHASH        = 6;
    static constexpr int COLUMN_UCOUNT       = 7;
};
