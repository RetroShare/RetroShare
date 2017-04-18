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


#ifndef _RSITEMDELEGATE_H
#define _RSITEMDELEGATE_H

#include <QItemDelegate>

class RSItemDelegate : public QItemDelegate
{
public:
    explicit RSItemDelegate(QObject *parent = 0);

    void paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint (const QStyleOptionViewItem &option, const QModelIndex &index) const;

    void removeFocusRect(int column);
    void setSpacing(const QSize &spacing);
    QSize spacing() const { return m_spacing; }
private:
    QList<int> m_noFocusRect;
    QSize m_spacing;
};

#endif

