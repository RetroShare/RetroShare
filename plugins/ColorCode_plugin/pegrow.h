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

#ifndef PEGROW_H
#define PEGROW_H

#include <QObject>
#include <QColor>
#include <QPen>
#include <QRadialGradient>
#include <QGraphicsItem>
#include <iostream>
#include <vector>
#include "colorcode.h"
#include "pegrow.h"
#include "colorpeg.h"

class PegRow : public QObject, public QGraphicsItem
{
    Q_OBJECT

public:
    PegRow(QObject* parent = 0);
    ~PegRow();

    int GetIx() const;
    int GetPegCnt();
    ColorPeg** GetPegs();
    std::vector<int> GetSolution() const;
    void SetIx(const int ix);
    void SetPegCnt(const int pegcnt);
    void SetActive(const bool b);
    bool SnapCP(ColorPeg* cp);
    void ForceSnap(ColorPeg* cp, int posix);
    void RemovePeg(ColorPeg* cp);

    void CloseRow();
    void ClearRow();
    void Reset(const int pegcnt);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    QRectF boundingRect() const;
    QPainterPath shape() const;

signals:
    void RowSolutionSignal(int ix);
    void RemovePegSignal(ColorPeg* cp);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

private:
    std::vector<int> mSolution;

    QColor mPend;
    QColor mPenl;
    QColor mGrad0;
    QColor mGrad1;
    bool mIsActive;
    bool mSolved;
    int mIx;
    int mPegCnt;
    int mXOffs;

    void CheckSolution();
    ColorPeg** mColorPegs;
    QRectF outlineRect() const;
};

#endif // PEGROW_H
