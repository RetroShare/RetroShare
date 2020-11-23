/*******************************************************************************
 * retroshare-gui/src/gui/im_history/ImHistoryItemDelegate.cpp                 *
 *                                                                             *
 * Copyright (C) 2009 by Retroshare Team     <retroshare.project@gmail.com>    *
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
