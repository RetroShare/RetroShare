/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2009 The RetroShare Team
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

#include <qvariant.h>
#include <QPainter>

#include "IMHistoryItemDelegate.h"
#include "IMHistoryItemPainter.h"

IMHistoryItemDelegate::IMHistoryItemDelegate(QWidget *parent)
    : QItemDelegate(parent)
{
}

void IMHistoryItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.data().canConvert<IMHistoryItemPainter>()) {
        IMHistoryItemPainter itemPainter = index.data().value<IMHistoryItemPainter>();
        itemPainter.paint(painter, option, IMHistoryItemPainter::ReadOnly);
    } else {
        QItemDelegate::paint(painter, option, index);
    }
}

QSize IMHistoryItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.data().canConvert<IMHistoryItemPainter>()) {
        IMHistoryItemPainter painter = index.data().value<IMHistoryItemPainter>();
        return painter.sizeHint();
    } else {
        return QItemDelegate::sizeHint(option, index);
    }
}
