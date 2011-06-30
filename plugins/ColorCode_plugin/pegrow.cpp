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
#include "pegrow.h"
#include "rowhint.h"

using namespace std;

PegRow::PegRow(QObject*)
{
    setAcceptedMouseButtons(0);

    mIx = -1;
    mPegCnt = 0;
    mXOffs = 0;
    mPend = QColor("#646568");
    mPenl = QColor("#ecedef");
    mGrad0 = QColor("#cccdcf");
    mGrad1 = QColor("#fcfdff");

    mColorPegs = NULL;

    Reset(0, 0);
}

PegRow::~PegRow()
{
    for (int i = 0; i < mPegCnt; ++i)
    {
        delete mColorPegs[i];
    }
    delete [] mColorPegs;
    mColorPegs = NULL;
}

bool PegRow::IsActive() const
{
    return mIsActive;
}

void PegRow::Reset(const int pegcnt, const int gamemode)
{
    ClearRow();
    mSolution.clear();
    SetActive(false);
    mSolved = false;
    SetPegCnt(pegcnt);
    SetGameMode(gamemode);
}

void PegRow::ClearRow()
{
    if (mColorPegs == NULL) { return; };

    for (int i = 0; i < mPegCnt; ++i)
    {
        if (mColorPegs[i] != NULL)
        {
            emit(RemovePegSignal(mColorPegs[i]));
        }
    }
}

int PegRow::GetIx() const
{
    return mIx;
}

void PegRow::SetIx(const int ix)
{
    mIx = ix;
}

void PegRow::SetPegCnt(const int pegcnt)
{
    ClearRow();
    for (int i = 0; i < mPegCnt; ++i)
    {
        delete mColorPegs[i];
    }
    delete [] mColorPegs;
    mColorPegs = NULL;

    mPegCnt = pegcnt;
    mColorPegs = new ColorPeg* [mPegCnt];
    for (int i = 0; i < mPegCnt; ++i)
    {
        mColorPegs[i] = NULL;
    }
    SetXOffs();
}

void PegRow::SetGameMode(const int gamemode)
{
    mGameMode = gamemode;
}

void PegRow::SetXOffs()
{
    mXOffs = 100 - mPegCnt * 20;
}

int PegRow::GetPegCnt() const
{
    if (mColorPegs == NULL) { return 0; };

    int cnt = 0;
    int i = 0;
    for (i = 0; i < mPegCnt; ++i)
    {
        if (mColorPegs[i] != NULL)
        {
            ++cnt;
        }
    }
    return cnt;
}

ColorPeg** PegRow::GetPegs()
{
    return mColorPegs;
}

void PegRow::SetActive(const bool b)
{
    mIsActive = b;
    update(boundingRect());
}

bool PegRow::SnapCP(ColorPeg *cp)
{
    if (mColorPegs == NULL)
    {
        return false;
    }

    bool snapped = false;
    if (mIsActive)
    {
        QPointF p = mapFromParent(cp->pos());

        p.rx() += 18;
        p.ry() += 18;

        int ix = -1;
        int i;

        if (p.y() >= 0 && p.y() <= 40)
        {
            for (i = 0; i < mPegCnt; ++i)
            {
                if (p.x() >= mXOffs + i * 40 && p.x() < mXOffs + (i + 1) * 40)
                {
                    if (mColorPegs[i] != NULL)
                    {
                        emit RemovePegSignal(mColorPegs[i]);
                        mColorPegs[i] = NULL;
                    }
                    mColorPegs[i] = cp;
                    cp->SetPegRow(this);
                    ix = i;
                    cp->setPos(mapToParent(mXOffs + i * 40 + 2, 2));
                    break;
                }
            }
        }

        snapped = ix > -1;
        CheckSolution();
    }

    return snapped;
}

void PegRow::ForceSnap(ColorPeg* cp, int posix)
{
    if (mColorPegs == NULL)
    {
        return;
    }

    if (posix > mPegCnt - 1)
    {
        return;
    }

    if (mIsActive)
    {
        if (mColorPegs[posix] != NULL)
        {
            emit RemovePegSignal(mColorPegs[posix]);
            mColorPegs[posix] = NULL;
        }
        mColorPegs[posix] = cp;
        cp->SetPegRow(this);
        cp->setPos(mapToParent(mXOffs + posix * 40 + 2, 2));

        CheckSolution();
    }
}

void PegRow::CloseRow()
{
    mSolved = true;
    if (mColorPegs == NULL)
    {
        
    }
    else
    {
        for (int i = 0; i < mPegCnt; ++i)
        {
            if (mColorPegs[i] != NULL)
            {
                mColorPegs[i]->SetEnabled(false);
            }
        }
    }
    SetActive(false);
}

void PegRow::OpenRow()
{
    mSolved = false;
    if (mColorPegs == NULL)
    {
        
    }
    else
    {
        for (int i = 0; i < mPegCnt; ++i)
        {
            if (mColorPegs[i] != NULL)
            {
                mColorPegs[i]->SetEnabled(true);
            }
        }
    }
    SetActive(true);
}

void PegRow::CheckSolution()
{
    mSolution.clear();
    if (mColorPegs == NULL)
    {
        
    }
    else
    {
        for (int i = 0; i < mPegCnt; ++i)
        {
            if (mColorPegs[i] != NULL)
            {
                mSolution.push_back(mColorPegs[i]->GetPegType()->ix);
            }
        }
    }

    emit RowSolutionSignal(mIx);
}

std::vector<int> PegRow::GetSolution() const
{
    return mSolution;
}

void PegRow::RemovePeg(ColorPeg *cp)
{
    if (mColorPegs == NULL)
    {
        return;
    }

    for (int i = 0; i < mPegCnt; ++i)
    {
        if (mColorPegs[i] == cp)
        {
            mColorPegs[i] = NULL;
            if (mIsActive)
            {
                CheckSolution();
            }
        }
    }
}

QVariant PegRow::itemChange(GraphicsItemChange change, const QVariant &value)
{
    return QGraphicsItem::itemChange(change, value);
}

QPainterPath PegRow::shape() const
{
    QPainterPath path;
    path.addRect(boundingRect());
    return path;
}

QRectF PegRow::boundingRect() const
{
    const int Margin = 1;
    return outlineRect().adjusted(-Margin, -Margin, +Margin, +Margin);
}

QRectF PegRow::outlineRect() const
{
    return QRectF(0.0, 0.0, 200.0, 40.0);
}

void PegRow::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget* /* widget */)
{
    if (mPegCnt == 0)
    {
        return;
    }

    if (mIsActive)
    {
        mPend.setAlpha(0xff);
        mPenl.setAlpha(0xff);
        mGrad0.setAlpha(0xff);
        mGrad1.setAlpha(0xff);
    }
    else if (mSolved)
    {
        mPend.setAlpha(0xa0);
        mPenl.setAlpha(0xa0);
        mGrad0.setAlpha(0xa0);
        mGrad1.setAlpha(0xa0);
    }
    else
    {
        mPend.setAlpha(0x32);
        mPenl.setAlpha(0x50);
        mGrad0.setAlpha(0x32);
        mGrad1.setAlpha(0x32);
    }
    painter->setPen(Qt::NoPen);
    int i;
    for (i = 0; i < mPegCnt; i++)
    {
        painter->setBrush(QBrush(mPenl));
        painter->drawRect(QRectF(mXOffs + i * 40, 0.0, 39, 1));
        painter->drawRect(QRectF(mXOffs + i * 40, 0.0, 1, 39));
        painter->setBrush(QBrush(mPend));
        painter->drawRect(QRectF(mXOffs + i * 40, 39.0, 40, 1));
        painter->drawRect(QRectF(mXOffs + i * 40 + 39, 0.0, 1, 40));

        QRadialGradient grad(QPointF(mXOffs + i * 40 + 10, 10), 100);
        grad.setColorAt(0, mGrad0);
        grad.setColorAt(1, mGrad1);
        painter->setBrush(QBrush(grad));
        painter->drawRect(QRectF(mXOffs + i * 40 + 1, 1.0, 38.0, 38.0));
    }
}
