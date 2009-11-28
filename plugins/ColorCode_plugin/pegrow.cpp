#include <QtGui>

#include "colorpeg.h"
#include "pegrow.h"
#include "rowhint.h"

using namespace std;

PegRow::PegRow(QObject*)
{
    mIx = -1;

    //setFlags(QGraphicsItem::GraphicsItemFlags(0));

    mPen = QPen(QColor("#808080"));

    for (int i = 0; i < 4; ++i)
    {
        mColorPegs[i] = NULL;
    }

    Reset();
}

PegRow::~PegRow()
{
}

void PegRow::Reset()
{
    ClearRow();
    SetActive(false);
}

void PegRow::ClearRow()
{
    for (int i = 0; i < 4; ++i)
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

int PegRow::GetPegCnt()
{
    int cnt = 0;
    int i = 0;
    for (i = 0; i < 4; ++i)
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
    if (b)
    {
        setOpacity(1.0);
    }
    else
    {
        setOpacity(0.5);
    }
    update(boundingRect());
}

bool PegRow::SnapCP(ColorPeg *cp)
{

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
            for (i = 0; i < 4; ++i)
            {
                if (p.x() >= i * 40 && p.x() < (i + 1) * 40)
                {
                    if (mColorPegs[i] != NULL)
                    {
                        emit RemovePegSignal(mColorPegs[i]);
                        mColorPegs[i] = NULL;
                    }
                    mColorPegs[i] = cp;
                    cp->SetPegRow(this);
                    ix = i;
                    cp->setPos(mapToParent(i * 40 + 2, 2));
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
    if (mIsActive)
    {
        if (mColorPegs[posix] != NULL)
        {
            emit RemovePegSignal(mColorPegs[posix]);
            mColorPegs[posix] = NULL;
        }
        mColorPegs[posix] = cp;
        cp->SetPegRow(this);
        cp->setPos(mapToParent(posix * 40 + 2, 2));

        CheckSolution();
    }
}

void PegRow::CloseRow()
{
    for (int i = 0; i < 4; ++i)
    {
        if (mColorPegs[i] != NULL)
        {
            mColorPegs[i]->SetEnabled(false);
        }
    }
    SetActive(false);
}

void PegRow::CheckSolution()
{
    mSolution.clear();

    for (int i = 0; i < 4; ++i)
    {
        if (mColorPegs[i] != NULL)
        {
            //mColorPegs[i]->setEnabled(false);
            //mColorPegs[i]->setFlags(QGraphicsItem::GraphicsItemFlags(0));
            mSolution.push_back(mColorPegs[i]->GetPegType()->ix);
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
    for (int i = 0; i < 4; ++i)
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
    return QRectF(0.0, 0.0, 160.0, 40.0);
}

void PegRow::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* /* widget */)
{
    mPen.setWidth(2);
    if (option->state & QStyle::State_Selected)
    {
        mPen.setStyle(Qt::DotLine);
    }

    painter->setPen(mPen);

    int i;
    for (i = 0; i < 4; i++)
    {
        QRadialGradient grad(QPointF(i * 40 + 10, 10), 100);
        grad.setColorAt(0, QColor("#cccccc"));
        grad.setColorAt(1, QColor("#ffffff"));
        painter->setBrush(QBrush(grad));
        painter->drawRect(QRectF(i * 40, 0.0, 40.0, 40.0));
    }
}
