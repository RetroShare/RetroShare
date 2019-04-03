/*******************************************************************************
 * retroshare-gui/src/gui/im_history/ImHistoryItemPainter.cpp                  *
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

void IMHistoryItemPainter::paint(QPainter *painter, const QStyleOptionViewItem &option, EditMode /*mode*/) const
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
