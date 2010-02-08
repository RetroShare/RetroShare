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

#ifndef COLORPEG_H
#define COLORPEG_H

#include <QColor>
#include <QPen>
#include <QRadialGradient>
#include <QGraphicsItem>
#include <iostream>
#include <vector>
#include "colorcode.h"
#include "pegrow.h"
#include "rowhint.h"


class ColorPeg : public QObject, public QGraphicsItem
{
    Q_OBJECT

public:
    ColorPeg(QObject* parent = 0);
    ~ColorPeg();

    int mId;
    void SetId(int id);

    void SetPegType(PegType* pt);
    void SetPegRow(PegRow* pr);

    QRectF boundingRect() const;
    QPainterPath shape() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

    void SetType(const int t);
    void SetEnabled(const bool b);
    void ShowLetter(const bool b);
    void SetCursorShape( Qt::CursorShape shape = Qt::ArrowCursor, const bool force = false);
    PegType* GetPegType();
    int GetType() const;
    QColor GetPenColor() const;
    QColor GetBrushColor() const;

    bool IsBtn() const;
    int GetSort() const;
    void SetBtn(bool b);
    void Reset();

signals:
    void PegReleasedSignal(ColorPeg* cp);
    void PegPressSignal(ColorPeg* cp);
    void PegSortSignal(ColorPeg* cp);

protected:
    PegType* mPegType;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    void mousePressEvent(QGraphicsSceneMouseEvent* e);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* e);

private:
    static const QFont mFont;
    static QFont GetLetterFont();
    static const QBrush mShadowBrush;
    static QBrush GetShadowBrush();
    static const QBrush mOutlineBrush;
    static QBrush GetOutlineBrush();
    static const QBrush mGlossyBrush;
    static QBrush GetGlossyBrush();

    QRectF GetColorRect() const;
    QRectF outlineRect() const;
    void SetIsDragged(bool b);

    PegRow* mPegRow;

    const QBrush mBrush;

    int mType;
    int mSort;
    bool mIsBtn;
    bool mIsDragged;    
    bool mShowLetter;
};



#endif // COLORPEG_H
