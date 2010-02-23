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

#include "rowhint.h"

using namespace std;

const int RowHint::mPegPos[4][5][3] = {
                                            {
                                                {  6, 14, 12 },
                                                { 22, 14, 12 },
                                                {  0,  0,  0 },
                                                {  0,  0,  0 },
                                                {  0,  0,  0 },
                                            },
                                            {
                                                {  6,  6, 12 },
                                                { 22,  6, 12 },
                                                { 14, 22, 12 },
                                                {  0,  0,  0 },
                                                {  0,  0,  0 },
                                            },
                                            {
                                                {  6,  6, 12 },
                                                { 22,  6, 12 },
                                                {  6, 22, 12 },
                                                { 22, 22, 12 },
                                                {  0,  0,  0 },
                                            },
                                            {
                                                {  4,  4, 12 },
                                                { 24,  4, 12 },
                                                { 14, 14, 12 },
                                                {  4, 24, 12 },
                                                { 24, 24, 12 }
                                            }
                                        };

RowHint::RowHint(QObject*)
{
    mIx = -1;
    mPegCnt = 0;

    mPen = QPen(QColor("#808080"));
    mPen.setWidth(2);

    QRadialGradient grad(QPointF(10, 10), 100);
    grad.setColorAt(0, QColor("#d8d8d8"));
    grad.setColorAt(1, QColor("#ffffff"));
    mBrush1 = QBrush(grad);
    grad.setColorAt(0, QColor(216, 216, 216, 51));
    grad.setColorAt(1, QColor(255, 255, 255, 51));
    mBrush0 = QBrush(grad);

    Reset(0, 0);
}

RowHint::~RowHint()
{
    scene()->removeItem(this);
}

void RowHint::Reset(const int pegcnt, const int gamemode)
{
    mActive = true;
    mSolved = false;
    SetPegCnt(pegcnt);
    mHints.clear();
    mHints.assign(mPegCnt, 0);
    SetGameMode(gamemode);
    SetActive(false);
}

int RowHint::GetIx() const
{
    return mIx;
}

void RowHint::SetIx(const int ix)
{
    mIx = ix;
}

void RowHint::SetPegCnt(const int pegcnt)
{
    mPegCnt = pegcnt;
}

void RowHint::SetGameMode(const int gamemode)
{
    mGameMode = gamemode;
}

bool RowHint::IsActive() const
{
    return mActive;
}

void RowHint::SetActive(bool b)
{
    if (b == mActive)
    {
        return;
    }

    mActive = b;
    setAcceptHoverEvents(b);
    setEnabled(b);
    if (b)
    {
        if (mGameMode == ColorCode::MODE_HVM)
        {
            setCursor(Qt::PointingHandCursor);
            setToolTip(tr("Commit Your solution"));
        }
        else if (mGameMode == ColorCode::MODE_MVH)
        {
            mSolved = true;
            mHints.assign(mPegCnt, 0);
            setCursor(Qt::ArrowCursor);
            setToolTip(tr("Click the circles to rate my guess.\nHit Ctrl+H or the corresponding toolbar button to let an impartial part of me do this for you ;-)"));
        }
    }
    else
    {
        setCursor(Qt::ArrowCursor);
        setToolTip(QString(""));
    }
    update(boundingRect());
}

std::vector<int> RowHint::GetHints()
{
    return mHints;
}

void RowHint::DrawHints(std::vector<int> res)
{
    mHints.clear();
    mHints.assign(mPegCnt, 0);
    for (unsigned i = 0; i < res.size(); ++i)
    {
        mHints.at(i) = res.at(i);
    }
    mSolved = true;
    update(boundingRect());
}

void RowHint::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    if (mGameMode == ColorCode::MODE_HVM)
    {
        bool disabled = false;
        if (mActive && !mSolved)
        {
            if (e->button() == Qt::LeftButton)
            {
                SetActive(false);
                emit HintPressedSignal(mIx);
                disabled = true;
            }
        }

        if (!disabled)
        {
            QGraphicsItem::mousePressEvent(e);
        }
    }
    else if (mGameMode == ColorCode::MODE_MVH)
    {
        qreal xm = e->pos().x();
        qreal ym = e->pos().y();

        int posix = mPegCnt - 2;
        for (int i = 0; i < mPegCnt; i++)
        {
            if ( xm >= mPegPos[posix][i][0] && xm <= mPegPos[posix][i][0] + mPegPos[posix][i][2]
                 && ym >= mPegPos[posix][i][1] && ym <= mPegPos[posix][i][1] + mPegPos[posix][i][2])
            {
                ++mHints.at(i);
                if (mHints.at(i) > 2)
                {
                    mHints.at(i) = 0;
                }
            }
        }
        update(boundingRect());
    }
}

QPainterPath RowHint::shape() const
{
    QPainterPath path;
    path.addRect(boundingRect());
    return path;
}

QRectF RowHint::boundingRect() const
{
    const int margin = 1;
    return outlineRect().adjusted(-margin, -margin, 2 * margin, 2 * margin);
}

QRectF RowHint::outlineRect() const
{
    return QRectF(0.0, 0.0, 40.0, 40.0);
}

void RowHint::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /* widget */)
{
    if (mPegCnt == 0)
    {
        return;
    }

    int i;

    QColor pend = QColor("#646568");
    QColor penl = QColor("#ecedef");
    QColor grad0 = QColor("#cccdcf");
    QColor grad1 = QColor("#fcfdff");
    if (mActive)
    {
        pend.setAlpha(0xff);
        penl.setAlpha(0xff);
        grad0.setAlpha(0xff);
        grad1.setAlpha(0xff);
    }
    else if (mSolved)
    {
        pend.setAlpha(0xa0);
        penl.setAlpha(0xa0);
        grad0.setAlpha(0xa0);
        grad1.setAlpha(0xa0);
    }
    else
    {
        pend.setAlpha(0x32);
        penl.setAlpha(0x50);
        grad0.setAlpha(0x32);
        grad1.setAlpha(0x32);
    }
    painter->setPen(Qt::NoPen);

    painter->setBrush(QBrush(penl));
    painter->drawRect(QRectF(0.0, 0.0, 39, 1));
    painter->drawRect(QRectF(0.0, 0.0, 1, 39));
    painter->setBrush(QBrush(pend));
    painter->drawRect(QRectF(0.0, 39.0, 40, 1));
    painter->drawRect(QRectF(39, 0.0, 1, 40));

    QRadialGradient grad(QPointF(10, 10), 100);
    grad.setColorAt(0, grad0);
    grad.setColorAt(1, grad1);
    painter->setBrush(QBrush(grad));
    painter->drawRect(QRectF(1, 1.0, 38.0, 38.0));

    painter->setBrush(Qt::NoBrush);

    int posix = mPegCnt - 2;
    QPointF x0;
    for (i = 0; i < mPegCnt; i++)
    {
        x0 = QPointF(mPegPos[posix][i][0], mPegPos[posix][i][1]);
        QLinearGradient lgrad(x0, QPointF(x0.x() + 10, x0.y() + 10));
        lgrad.setColorAt(0.0, QColor(0, 0, 0, 0xa0));
        lgrad.setColorAt(1.0, QColor(0xff, 0xff, 0xff, 0xa0));
        painter->setPen(QPen(QBrush(lgrad), 0.5));
        painter->drawEllipse(QRectF(mPegPos[posix][i][0], mPegPos[posix][i][1], mPegPos[posix][i][2], mPegPos[posix][i][2]));
    }

    if (mSolved)
    {
        painter->setPen(Qt::NoPen);
        for (unsigned i = 0; i < mHints.size(); i++)
        {
            if (mHints.at(i) == 0)
            {
                continue;
            }

            if (mHints.at(i) == 2)
            {
                painter->setBrush(QBrush(Qt::black));
            }
            else
            {
                painter->setBrush(QBrush(Qt::white));
            }
            painter->drawEllipse(QRectF(mPegPos[posix][i][0], mPegPos[posix][i][1], mPegPos[posix][i][2], mPegPos[posix][i][2]));
        }
    }
}
