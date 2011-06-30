/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2010, RetroShare Team
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

#include "RSItemDelegate.h"

RSItemDelegate::RSItemDelegate(QObject *parent) : QItemDelegate(parent)
{
}

void RSItemDelegate::paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem ownOption (option);

    if (m_noFocusRect.indexOf(index.column()) >= 0) {
        ownOption.state &= ~QStyle::State_HasFocus; // don't show text and focus rectangle
    }

    QItemDelegate::paint (painter, ownOption, index);
}

QSize RSItemDelegate::sizeHint (const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem ownOption (option);

    if (m_noFocusRect.indexOf(index.column()) >= 0) {
        ownOption.state &= ~QStyle::State_HasFocus; // don't show text and focus rectangle
    }

    QSize size = QItemDelegate::sizeHint(ownOption, index);

    size += m_spacing;

    return size;
}

void RSItemDelegate::removeFocusRect(int column)
{
    if (m_noFocusRect.indexOf(column) == -1) {
        m_noFocusRect.push_back(column);
    }
}

void RSItemDelegate::setSpacing(const QSize &spacing)
{
    m_spacing = spacing;
}
