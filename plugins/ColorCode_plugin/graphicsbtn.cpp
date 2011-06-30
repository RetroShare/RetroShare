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
#include <QFontMetrics>
#include "graphicsbtn.h"

using namespace std;

GraphicsBtn::GraphicsBtn()
{
    mWidth = 152;

    setCursor(Qt::PointingHandCursor);
    setAcceptHoverEvents (true);
    mLabel = "";
    mLabelWidth = 0;
    mYOffs = 0;

    InitGraphics();
    SetShapes();

    mFillBrush = &mFillOutBrush;
    mFrameBrush = &mFrameOutBrush;

    mFont = QFont("Arial", 12, QFont::Bold, false);
    mFont.setStyleHint(QFont::SansSerif);
    mFont.setStyleStrategy(QFont::PreferAntialias);
}

void GraphicsBtn::SetWidth(int w)
{
    mWidth = w;
    SetShapes();
}

void GraphicsBtn::SetLabel(QString str)
{
    mLabel = str;
}

void GraphicsBtn::ShowBtn(bool b)
{
    if (b)
    {
        mFillBrush = &mFillOutBrush;
        mFrameBrush = &mFrameOutBrush;
    }
    setVisible(b);
}

void GraphicsBtn::hoverEnterEvent(QGraphicsSceneHoverEvent* e)
{
    mFillBrush = &mFillOverBrush;
    QGraphicsItem::hoverEnterEvent(e);
}

void GraphicsBtn::hoverLeaveEvent(QGraphicsSceneHoverEvent* e)
{
    mFillBrush = &mFillOutBrush;
    QGraphicsItem::hoverLeaveEvent(e);
}

void GraphicsBtn::mousePressEvent(QGraphicsSceneMouseEvent*)
{
    mFrameBrush = &mFrameOverBrush;
    mYOffs = 1;
    update(mRect);
}

void GraphicsBtn::mouseReleaseEvent(QGraphicsSceneMouseEvent* e)
{
    mFrameBrush = &mFrameOutBrush;
    mYOffs = 0;
    if ( e->pos().x() >= 0
         && e->pos().x() <= boundingRect().width()
         && e->pos().y() >= 0
         && e->pos().y() <= boundingRect().height() )
    {
        emit BtnPressSignal(this);
    }
    update(mRect);
}

void GraphicsBtn::InitGraphics()
{
    QLinearGradient fillgrad(0, 2, 0, 36);
    fillgrad.setColorAt(0.0, QColor("#f7f8fa"));
    fillgrad.setColorAt(0.5, QColor("#b9babc"));
    mFillOutBrush = QBrush(fillgrad);

    QLinearGradient fillovergrad(0, 2, 0, 36);
    fillovergrad.setColorAt(0.0, QColor("#f7f8fa"));
    fillovergrad.setColorAt(1.0, QColor("#b9babc"));
    mFillOverBrush = QBrush(fillovergrad);

    QLinearGradient framegrad(0, 0, 0, 40);
    framegrad.setColorAt(1.0, QColor(0, 0, 0, 0xa0));
    framegrad.setColorAt(0.0, QColor(0xff, 0xff, 0xff, 0xa0));
    mFrameOutBrush = QBrush(framegrad);

    QLinearGradient frameovergrad(0, 0, 0, 40);
    frameovergrad.setColorAt(0.0, QColor(0, 0, 0, 0xa0));
    frameovergrad.setColorAt(1.0, QColor(0xff, 0xff, 0xff, 0xa0));
    mFrameOverBrush = QBrush(frameovergrad);
}

void GraphicsBtn::SetShapes()
{
    mRect = QRectF(0, 0, mWidth, 38);
    mRectFill = QRectF(1, 1, mWidth - 2, 36);
}

QRectF GraphicsBtn::boundingRect() const
{
    const double margin = 0.5;
    return mRect.adjusted(-margin, -margin, 2 * margin, 2 * margin);
}

void GraphicsBtn::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget* /* widget */)
{
    painter->setPen(Qt::NoPen);
    painter->setBrush(*mFrameBrush);
    painter->drawRoundedRect(mRect, 20, 20);

    painter->setBrush(*mFillBrush);
    painter->drawRoundedRect(mRectFill, 18, 18);

    if (mLabel != "")
    {
        painter->setRenderHint(QPainter::TextAntialiasing, true);
        painter->setPen(QPen(QColor("#303133")));
        painter->setFont(mFont);
        painter->drawText(mRectFill.adjusted(0, mYOffs, 0, mYOffs), Qt::AlignCenter, mLabel);
    }
}
