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

#ifndef ROWHINT_H
#define ROWHINT_H

#include <QGraphicsItem>
#include <QColor>
#include <QPen>
#include <QRadialGradient>
#include <iostream>
#include <vector>
#include "colorcode.h"

class RowHint : public QObject, public QGraphicsItem
{
    Q_OBJECT

public:
    RowHint(QObject* parent = 0);
    ~RowHint();

    int mIx;
    bool mActive;
    bool mSolved;
    std::vector<int> mHints;

    int GetIx() const;
    void SetIx(const int ix);
    void SetPegCnt(const int pegcnt);
    void SetActive(bool b);
    void DrawHints(std::vector<int> res);
    void Reset(const int pegcnt);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    QRectF boundingRect() const;
    QPainterPath shape() const;

signals:
    void HintPressedSignal(int ix);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* e);

private:
    static const int mPegPos[4][5][3];

    int mPegCnt;

    QPen mPen;
    QBrush mBrush0;
    QBrush mBrush1;

    QRectF outlineRect() const;
};

#endif // ROWHINT_H
