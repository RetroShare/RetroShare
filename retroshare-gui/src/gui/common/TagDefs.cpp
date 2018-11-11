/*******************************************************************************
 * gui/common/TagDefs.cpp                                                      *
 *                                                                             *
 * Copyright (c) 2010, RetroShare Team <retroshare.project@gmail.com>          *
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

#include <QCoreApplication>
#include <QColor>

#include <retroshare/rsmsgs.h>

#include "TagDefs.h"

const QString TagDefs::name(const unsigned int tagId, const std::string &text)
{
    if (tagId >= RS_MSGTAGTYPE_USER) {
        /* no need to be translated */
        return QString::fromStdString(text);
    }

    switch (tagId) {
    case RS_MSGTAGTYPE_IMPORTANT:
        return qApp->translate("TagDefs", "Important");
    case RS_MSGTAGTYPE_WORK:
        return qApp->translate("TagDefs", "Work");
    case RS_MSGTAGTYPE_PERSONAL:
        return qApp->translate("TagDefs", "Personal");
    case RS_MSGTAGTYPE_TODO:
        return qApp->translate("TagDefs", "Todo");
    case RS_MSGTAGTYPE_LATER:
        return qApp->translate("TagDefs", "Later");
    }

    std::cerr << "TagDefs::name: Unknown tag id requested " << tagId;
    return "";
}

const QString TagDefs::labelStyleSheet(const QColor &color)
{
    /* calculate best text color */
    QRgb textColor;
    if (qGray(color.rgb()) < 128) {
        /* dark color, use white */
        textColor = qRgb(255, 255, 255);
    } else {
        /* light color, use black */
        textColor = qRgb(0, 0, 0);
    }

    QColor colorBorder = color.darker(120);
    QColor colorGradient = color.lighter(150);

    return QString("QLabel{padding-left: 2px; padding-right: 2px; color: #%1; background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 #%2, stop:1 #%3); border-radius: 3px; border-color: #%4; border-width: 1; border-style: solid}")
            .arg(textColor & RGB_MASK, 6, 16, QChar('0'))
            .arg(colorGradient.rgb() & RGB_MASK, 6, 16, QChar('0'))
            .arg(color.rgb() & RGB_MASK, 6, 16, QChar('0'))
            .arg(colorBorder.rgb() & RGB_MASK, 6, 16, QChar('0'));
}
