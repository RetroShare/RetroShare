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
#include "solutionrow.h"

using namespace std;

SolutionRow::SolutionRow(QObject*)
{
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    InitGraphics();
}

SolutionRow::~SolutionRow()
{
}

void SolutionRow::InitGraphics()
{
    mRect = QRectF(0.5, 0.5, 219, 39);
    mRectC = QRectF(2, 2, 216, 36);

    QLinearGradient solgrad(0, 0, 0, 40);
    solgrad.setColorAt(0.0, QColor("#9fa0a2"));
    solgrad.setColorAt(1.0, QColor("#8c8d8f"));
    mBgBrush = QBrush(solgrad);

    QLinearGradient framegrad(0, 0, 0, 40);
    framegrad.setColorAt(0.0, QColor("#4e4f51"));
    framegrad.setColorAt(1.0, QColor("#ebecee"));
    mFramePen = QPen(QBrush(framegrad), 1);

    mFont = QFont("Arial", 22, QFont::Bold, false);
    mFont.setStyleHint(QFont::SansSerif);
}

void SolutionRow::SetXOffs()
{
    mXOffs = 110 - mPegCnt * 20;
}

QRectF SolutionRow::boundingRect() const
{
    const double margin = 0.5;
    return mRect.adjusted(-margin, -margin, 2 * margin, 2 * margin);
}

void SolutionRow::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget* /* widget */)
{
    if (mPegCnt == 0)
    {
        return;
    }

    int i;
    painter->setBrush(mBgBrush);
    painter->setPen(mFramePen);
    painter->drawRoundedRect(mRect, 20, 20);

    QPainterPath cpath;
    cpath.addRoundedRect(mRectC, 19, 19);
    painter->setClipPath(cpath);

    painter->setRenderHint(QPainter::TextAntialiasing, true);

    int x0 = (220 - mPegCnt * 40) / 2;
    int xpos;
    for (i = 0; i <= mPegCnt; ++i)
    {
        xpos = x0 + i * 40;
        painter->setPen(Qt::NoPen);
        painter->setBrush(QBrush(QColor(0x7d, 0x7e, 0x80, 0x80)));
        painter->drawRect(xpos - 1, 1, 1, 38);
        painter->setBrush(QBrush(QColor(0xc3, 0xc4, 0xc6, 0x80)));
        painter->drawRect(xpos, 1, 1, 38);

        if (i < mPegCnt)
        {
            painter->setPen(QPen(QColor("#ff9933")));
            painter->setFont(mFont);
            painter->drawText(QRectF(xpos + 3.0, 3.0, 34.0, 34.0), Qt::AlignCenter, QString('?'));
        }
    }
}
