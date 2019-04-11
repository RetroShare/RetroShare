/*******************************************************************************
 * gui/common/RSItemDelegate.cpp                                               *
 *                                                                             *
 * Copyright (C) 2010 RetroShare Team <retroshare.project@gmail.com>           *
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
