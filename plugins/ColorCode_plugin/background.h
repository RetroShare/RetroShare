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

#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <QGraphicsItem>
#include <QColor>
#include <QPen>
#include <QRadialGradient>
#include <iostream>
#include "colorcode.h"

class BackGround : public QObject, public QGraphicsItem
{
    Q_OBJECT

    public:
        BackGround(QObject* parent = 0);
        ~BackGround();

        QRectF boundingRect() const;
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

    private:
        QRectF outlineRect() const;

        QBrush mTopGrad;
        QBrush mBotGrad;

        QPen mFramePen;

        QColor mPend;
        QColor mPenl;
        QColor mGrad0;
        QColor mGrad1;

};

#endif // BACKGROUND_H
