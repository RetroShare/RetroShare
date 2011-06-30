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

#ifndef SOLROW_H
#define SOLROW_H

#include <QGraphicsItem>
#include <iostream>

class SolRow : public QGraphicsItem
{
public:
    SolRow();

    void SetState(const int pegcnt, const bool solved);
    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

private:
    QRectF mRect;
    QRectF mRectC;
    QBrush mBgBrush;
    QPen mFramePen;
    QFont mFont;

    int mPegCnt;
    bool mSolved;
};

#endif // SOLROW_H
