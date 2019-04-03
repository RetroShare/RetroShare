/*******************************************************************************
 * retroshare-gui/src/gui/im_history/ImHistoryItemPainter.h                    *
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

#ifndef _IMHISTORYITEMPAINTER_H_
#define _IMHISTORYITEMPAINTER_H_

#include <QMetaType>
#include <QString>
#include <QStyleOption>

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
