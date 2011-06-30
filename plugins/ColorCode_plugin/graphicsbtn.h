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

#ifndef GRAPHICSBTN_H
#define GRAPHICSBTN_H

#include <QObject>
#include <QGraphicsItem>
#include <QColor>
#include <QPen>
#include <QFont>
#include <QRadialGradient>
#include <iostream>

class GraphicsBtn : public QObject, public QGraphicsItem
{
    Q_OBJECT

public:
    GraphicsBtn();

    void SetWidth(int w);
    void SetLabel(QString str);
    void ShowBtn(bool b);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    QRectF boundingRect() const;

signals:
    void BtnPressSignal(GraphicsBtn* btn);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* e);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* e);
    void mousePressEvent(QGraphicsSceneMouseEvent* e);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* e);

    virtual void InitGraphics();
    virtual void SetShapes();

    QString mLabel;
    int mWidth;
    int mLabelWidth;
    int mYOffs;

    QRectF mRect;
    QRectF mRectFill;
    QRectF mIconRect;

    QBrush mFillOutBrush;
    QBrush mFillOverBrush;
    QBrush* mFillBrush;

    QBrush mFrameOutBrush;
    QBrush mFrameOverBrush;
    QBrush* mFrameBrush;

    QFont mFont;
};

#endif // GRAPHICSBTN_H
