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

#include "background.h"

using namespace std;

BackGround::BackGround(QObject*)
{
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);

    QLinearGradient topgrad(0, 16, 0, 129);
    topgrad.setColorAt(0.0, QColor(0xf8, 0xf8, 0xf8));
    topgrad.setColorAt(0.6, QColor(0xb8, 0xb9, 0xbb));
    topgrad.setColorAt(1, QColor(0xd4, 0xd5, 0xd7));
    mTopGrad = QBrush(topgrad);

    QLinearGradient botgrad(0, 530, 0, 557);
    botgrad.setColorAt(0.0, QColor("#d4d5d7"));
    botgrad.setColorAt(0.3, QColor("#cecfd1"));
    botgrad.setColorAt(1.0, QColor("#b0b1b3"));
    mBotGrad = QBrush(botgrad);

    QLinearGradient lgrad(0, 190, 320, 370);
    lgrad.setColorAt(0.0, QColor(0xff, 0xff, 0xff, 0xa0));
    lgrad.setColorAt(0.49, QColor(0xff, 0xff, 0xff, 0xa0));
    lgrad.setColorAt(0.50, QColor(0, 0, 0, 0x80));
    lgrad.setColorAt(1.0, QColor(0, 0, 0, 0x80));
    mFramePen = QPen(QBrush(lgrad), 1);

    mPend = QColor("#646568");
    mPenl = QColor("#ecedef");
    mGrad0 = QColor("#cccdcf");
    mGrad1 = QColor("#fcfdff");

    mPend.setAlpha(0x32);
    mPenl.setAlpha(0x50);
    mGrad0.setAlpha(0x32);
    mGrad1.setAlpha(0x32);
}

BackGround::~BackGround()
{
    scene()->removeItem(this);
}

QRectF BackGround::boundingRect() const
{
    const int margin = 1;
    return outlineRect().adjusted(-margin, -margin, +margin, +margin);
}

QRectF BackGround::outlineRect() const
{
    return QRectF(0.0, 0.0, 320.0, 560.0);
}

void BackGround::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /* widget */)
{
    int i;

    painter->setPen(Qt::NoPen);

    QRectF cr = QRectF(3, 3, 314, 554);
    QPainterPath cpath;
    cpath.addRoundedRect(cr, 10.1, 10.1);
    painter->setClipPath(cpath);
    painter->setClipping(true);
    
    painter->setBrush(mTopGrad);
    painter->drawRect(QRect(4, 4, 312, 125));
    painter->setBrush(QBrush(QColor("#707173")));
    painter->drawRect(QRect(1, 129, 318, 1));

    painter->setPen(Qt::NoPen);
    for (i = 0; i < 10; ++i)
    {
        painter->setBrush(QBrush(QColor("#9e9fa0")));
        painter->drawRect(QRect(1, i * 40 + 130, 318, 1));
        QLinearGradient rowgrad(0, i * 40 + 131, 0, i * 40 + 169);
        rowgrad.setColorAt(0.0, QColor("#9a9b9d"));
        rowgrad.setColorAt(1.0, QColor("#949597"));
        painter->setBrush(QBrush(rowgrad));
        painter->drawRect(QRect(1, i * 40 + 131, 318, 38));
        painter->setBrush(QBrush(QColor("#88898b")));
        painter->drawRect(QRect(1, i * 40 + 169, 318, 1));

        painter->setBrush(QBrush(mPenl));
        painter->drawRect(QRectF(277, i * 40 + 130, 39, 1));
        painter->drawRect(QRectF(277, i * 40 + 130, 1, 39));
        painter->setBrush(QBrush(mPend));
        painter->drawRect(QRectF(277, i * 40 + 130 + 39, 40, 1));
        painter->drawRect(QRectF(277 + 39, i * 40 + 130, 1, 40));

        QRadialGradient grad(QPointF(277 + 10, i * 40 + 130 + 10), 100);
        grad.setColorAt(0, mGrad0);
        grad.setColorAt(1, mGrad1);
        painter->setBrush(QBrush(grad));
        painter->drawRect(QRectF(277 + 1, i * 40 + 131, 38.0, 38.0));

        painter->setBrush(Qt::NoBrush);
        QPointF x0 = QPointF(279, i * 40 + 130 + 2);
        QLinearGradient lgrad(x0, QPointF(x0.x() + 36, x0.y() + 36));
        lgrad.setColorAt(0.0, QColor(0, 0, 0, 0x80));
        lgrad.setColorAt(1.0, QColor(0xff, 0xff, 0xff, 0xa0));
        painter->setPen(QPen(QBrush(lgrad), 0.5));
        painter->drawEllipse(QRectF(x0.x(), x0.y(), 36, 36));
        painter->setPen(Qt::NoPen);
    }

    painter->setBrush(mBotGrad);
    painter->drawRect(QRect(1, 531, 318, 27));
    painter->setBrush(QBrush(QColor("#eff0f2")));
    painter->drawRect(QRect(1, 530, 318, 1));

    painter->setClipping(false);

    QRectF r = QRectF(3.5, 3.5, 313, 553);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(mFramePen);
    painter->drawRoundedRect(r, 9.8, 9.8);
}


