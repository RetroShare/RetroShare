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

#include "colorpeg.h"

using namespace std;

const QFont ColorPeg::mFont = ColorPeg::GetLetterFont();
const QBrush ColorPeg::mShadowBrush = ColorPeg::GetShadowBrush();
const QBrush ColorPeg::mOutlineBrush = ColorPeg::GetOutlineBrush();
const QBrush ColorPeg::mGlossyBrush = ColorPeg::GetGlossyBrush();
const QBrush ColorPeg::mNeutralBrush = ColorPeg::GetNeutralBrush();

QFont ColorPeg::GetLetterFont()
{
    QFont lf("Arial", 12, QFont::Bold, false);
    lf.setStyleHint(QFont::SansSerif);

    return lf;
}

QBrush ColorPeg::GetNeutralBrush()
{
    QRadialGradient rgrad(20, 20, 40, 5, 5);
    rgrad.setColorAt(0.0, QColor(0xff, 0xff, 0xff, 0xff));
    rgrad.setColorAt(1.0, QColor(0x80, 0x80, 0x80, 0xff));

    return QBrush(rgrad);
}

QBrush ColorPeg::GetShadowBrush()
{
    QRadialGradient rgrad(22, 22, 22, 22, 22);
    rgrad.setColorAt(0.3, QColor(0, 0, 0, 0xff));
    rgrad.setColorAt(1.0, QColor(0, 0, 0, 0));

    return QBrush(rgrad);
}

QBrush ColorPeg::GetOutlineBrush()
{
    QLinearGradient lgrad(0, 0, 38, 38);
    lgrad.setColorAt(0.0, QColor(0, 0, 0, 0xa0));
    lgrad.setColorAt(1.0, QColor(0xff, 0xff, 0xff, 0xa0));
    return QBrush(lgrad);
}

QBrush ColorPeg::GetGlossyBrush()
{
    QLinearGradient lgrad(20, 4, 20, 20);
    lgrad.setColorAt(0.0, QColor(0xff, 0xff, 0xff, 0x80));
    lgrad.setColorAt(1.0, QColor(0xff, 0xff, 0xff, 0x00));
    return QBrush(lgrad);
}


ColorPeg::ColorPeg(QObject*)
{
    setFlags(QGraphicsItem::GraphicsItemFlags(0));
    mPegType = NULL;
    mPegRow = NULL;
    SetBtn(true);
    mIsDragged = false;
    mShowIndicator = false;
    mHideColor = false;
    mSort = 0;
    mId = -1;
}

ColorPeg::~ColorPeg()
{
}

void ColorPeg::SetPegType(PegType *pt)
{
    mPegType = pt;
    setFlags(ItemIsMovable | ItemIsSelectable);
}

void ColorPeg::SetPegRow(PegRow *pr)
{
    mPegRow = pr;
}

PegType* ColorPeg::GetPegType()
{
    return mPegType;
}

void ColorPeg::SetId(int id)
{
    mId = id;
}

bool ColorPeg::IsBtn() const
{
    return mIsBtn;
}

int ColorPeg::GetSort() const
{
    return mSort;
}

void ColorPeg::SetBtn(bool b)
{
    mIsBtn = b;
    SetCursorShape();
}

int ColorPeg::GetType() const
{
    return mType;
}

void ColorPeg::SetType(const int t)
{
    mType = t;
}

void ColorPeg::Reset()
{
    SetIsDragged(false);
    setSelected(false);
    if (mPegRow != NULL)
    {
        mPegRow->RemovePeg(this);
        mPegRow = NULL;
    }
}

void ColorPeg::SetIsDragged(bool b)
{
    if (false == b)
    {
        update(boundingRect());
    }
    mIsDragged = b;
}

void ColorPeg::SetEnabled(const bool b)
{
    setEnabled(b);
    SetCursorShape();
}

void ColorPeg::SetIndicator(const bool b, const int t, const bool c)
{
    if (mShowIndicator != b || mIndicatorType != t || mHideColor != c)
    {
        mShowIndicator = b;
        mIndicatorType = t;
        mHideColor = c;
        update(boundingRect());
    }
}

void ColorPeg::SetCursorShape(Qt::CursorShape shape, const bool force)
{
    if (!force)
    {
        if (mIsDragged)
        {
            if (mSort == 0)
            {
                shape = Qt::ClosedHandCursor;
            }
            else if (mSort == 1)
            {
                shape = Qt::SizeVerCursor;
            }
            else
            {
                shape = Qt::SplitVCursor;
            }
        }
        else if (isEnabled())
        {
            shape = Qt::OpenHandCursor;
        }
    }
    
    if (cursor().shape() != shape)
    {
        setCursor(QCursor(shape));
    }
}

QVariant ColorPeg::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedHasChanged)
    {
        scene()->update(scene()->sceneRect());
    }
    return QGraphicsItem::itemChange(change, value);
}

void ColorPeg::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    QGraphicsItem::mousePressEvent(e);
    if (mPegType != NULL)
    {
        if (e->button() == Qt::LeftButton)
        {
            Reset();
            SetIsDragged(true);
            if (IsBtn() && (e->modifiers() & Qt::ControlModifier) != 0)
            {
                if ((e->modifiers() & Qt::ShiftModifier) == 0)
                {
                    mSort = 1;
                }
                else
                {
                    mSort = 2;
                }
            }
            else
            {
                mSort = 0;

            }
            emit PegPressSignal(this);
            SetCursorShape();
        }
    }
}

void ColorPeg::mouseReleaseEvent (QGraphicsSceneMouseEvent *e)
{
    QGraphicsItem::mouseReleaseEvent(e);
    if (mPegType != NULL)
    {
        if (e->button() == Qt::LeftButton)
        {
            SetIsDragged(false);
            emit PegReleasedSignal(this);
            SetCursorShape();
        }
    }
}

QPainterPath ColorPeg::shape() const
{
    QPainterPath path;
    path.addEllipse(boundingRect());
    return path;
}

QRectF ColorPeg::boundingRect() const
{
    int margin;
    if (mIsDragged)
    {
        margin = 8;
    }
    else
    {
        margin = 1;
    }
    return outlineRect().adjusted(-margin, -margin, +margin, +margin);
}

QRectF ColorPeg::outlineRect() const
{
    QRectF rect(0.0, 0.0, 36.0, 36.0);
    return rect;
}

QRectF ColorPeg::GetColorRect() const
{
    QRectF rect(1.0, 1.0, 34.0, 34.0);
    return rect;
}

void ColorPeg::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget* /* widget */)
{
    painter->setPen(Qt::NoPen);

    if (!mIsDragged)
    {
        painter->setBrush(ColorPeg::mOutlineBrush);
        painter->drawEllipse(outlineRect());
    }
    else
    {
        painter->translate(QPointF(-2, -2));
        painter->setBrush(ColorPeg::mShadowBrush);
        painter->drawEllipse(QRectF(0, 0, 44, 44));
        painter->translate(QPointF(2, 2));
    }

    if (!mHideColor)
    {
        painter->setBrush(QBrush(*mPegType->grad));
        painter->drawEllipse(GetColorRect());
    }
    else
    {
        painter->setBrush(mNeutralBrush);
        painter->drawEllipse(GetColorRect());
    }

    painter->setBrush(mGlossyBrush);
    painter->drawEllipse(QRectF(5, 3, 24, 20));

    if (mShowIndicator)
    {
        painter->setPen(QPen(QColor("#303133")));
        painter->setRenderHint(QPainter::TextAntialiasing, true);
        painter->setFont(mFont);
        QString ind;
        if (mIndicatorType == Settings::INDICATOR_LETTER)
        {
            ind = QString(mPegType->let);
        }
        else
        {
            ind.setNum((mPegType->ix + 1));
        }
        painter->drawText(QRectF(1.5, 2.0, 32.0, 32.0), Qt::AlignCenter, ind);
    }
}
