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

#ifndef _IMHISTORYITEMPAINTER_H_
#define _IMHISTORYITEMPAINTER_H_

#include <QMetaType>
#include <QString>
#include <QStyleOption>

#include "IMHistoryItem.h"

class QPainter;

class IMHistoryItemPainter
{
public:
    enum EditMode { /*Editable,*/ ReadOnly };

    IMHistoryItemPainter(const QString &text = "");

    void paint(QPainter *painter, const QStyleOptionViewItem &option, EditMode mode) const;
    QSize sizeHint() const;

private:
    QString itemText;
    QSize size;
};

Q_DECLARE_METATYPE(IMHistoryItemPainter)

#endif // _IMHISTORYITEMPAINTER_H_
