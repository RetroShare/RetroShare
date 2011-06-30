/* ColorCode, a free MasterMind clone with built in solver
 * Copyright (C) 2009  Dirk Laebisch
 * http://www.laebisch.com/
 *
 * ColorCode is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ColorCode is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ColorCode. If not, see <http://www.gnu.org/licenses/>.
*/

#include <QtGui>

#include "msg.h"

using namespace std;

Msg::Msg()
{
    mFont = QFont("Arial", 12, QFont::Bold, false);
    mFont.setStyleHint(QFont::SansSerif);
    mFont.setPixelSize(16);
    mLay = new QTextLayout();
    mLay->setFont(mFont);
    mLay->setTextOption(QTextOption(Qt::AlignHCenter));
    mUpdateRect = QRectF(0, 0, 10, 10);
}

Msg::~Msg()
{
}

void Msg::ShowMsg(const QString str)
{
    mUpdateRect = boundingRect();

    mLay->setText(str);
    int leading = -3;
    qreal h = 0;
    qreal maxw = 0;
    qreal maxh = 0;
    mLay->beginLayout();

    while (1)
    {
        QTextLine line = mLay->createLine();
        if (!line.isValid())
        {
            break;
        }

        line.setLineWidth(280);
        h += leading;
        line.setPosition(QPointF(0, h));
        h += line.height();
        maxw = qMax(maxw, line.naturalTextWidth());
    }
    mLay->endLayout();

    float ypos = 4 + (70 - mLay->boundingRect().height()) / 2;

    maxw = qMax(mUpdateRect.width(), mLay->boundingRect().width());
    maxh = qMax(mUpdateRect.height(), mLay->boundingRect().height() + ypos);

    mUpdateRect = QRectF(0, 0, maxw, maxh + ypos);

    update(boundingRect());
}

QRectF Msg::boundingRect() const
{
    return mUpdateRect;
}

void Msg::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /* widget */)
{
    painter->setRenderHint(QPainter::TextAntialiasing, true);
    painter->setPen(QPen(QColor("#303133")));
    float ypos = 4 + (70 - mLay->boundingRect().height()) / 2;
    mLay->draw(painter, QPointF(0, ypos));
}
