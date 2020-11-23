/*******************************************************************************
 * gui/common/RSItemDelegate.h                                                 *
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

#ifndef _RSITEMDELEGATE_H
#define _RSITEMDELEGATE_H

#include <QItemDelegate>
#include <QStyledItemDelegate>

class RSItemDelegate : public QItemDelegate
{
public:
    RSItemDelegate(QObject *parent = nullptr);

    void paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint (const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void removeFocusRect(int column);
    void setSpacing(const QSize &spacing);
    QSize spacing() const { return m_spacing; }
private:
    QList<int> m_noFocusRect;
    QSize m_spacing;
};


class RSStyledItemDelegate : public QStyledItemDelegate
{
public:
    RSStyledItemDelegate(QObject *parent = nullptr);

    void paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint (const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void removeFocusRect(int column);
    void setSpacing(const QSize &spacing);
    QSize spacing() const { return m_spacing; }
private:
    QList<int> m_noFocusRect;
    QSize m_spacing;
};

#endif

