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

#include <QTextDocument>
#include <QPainter>
#include <QAbstractTextDocumentLayout>

#include "IMHistoryItemPainter.h"

IMHistoryItemPainter::IMHistoryItemPainter(const QString &text)
{
    itemText = text;

    /* calcultate size */
    QTextDocument doc;
    doc.setHtml(itemText);
    size = doc.size().toSize();
}

QSize IMHistoryItemPainter::sizeHint() const
{
    return size;
}

void IMHistoryItemPainter::paint(QPainter *painter, const QStyleOptionViewItem &option, EditMode mode) const
{
//    if (mode == Editable) {
//        painter->setBrush(option.palette.highlight());
//    } else {
        painter->setBrush(option.palette.foreground());
//    }
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    }

    painter->save();

    QTextDocument doc;
    doc.setHtml(itemText);
    QAbstractTextDocumentLayout::PaintContext context;
    doc.setPageSize(option.rect.size());
    painter->translate(option.rect.x(), option.rect.y());
    doc.documentLayout()->draw(painter, context);

    painter->restore();
}
