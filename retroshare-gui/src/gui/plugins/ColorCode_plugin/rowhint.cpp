#include <QtGui>

#include "rowhint.h"

using namespace std;

RowHint::RowHint(QObject*)
{
    //setFlags(QGraphicsItem::GraphicsItemFlags(0));

    mIx = -1;

    mPen = QPen(QColor("#808080"));
    mPen.setWidth(2);

    QRadialGradient grad(QPointF(10, 10), 100);
    grad.setColorAt(0, QColor("#d8d8d8"));
    grad.setColorAt(1, QColor("#ffffff"));
    mBrush1 = QBrush(grad);
    grad.setColorAt(0, QColor(216, 216, 216, 51));
    grad.setColorAt(1, QColor(255, 255, 255, 51));
    mBrush0 = QBrush(grad);

    Reset();
}

RowHint::~RowHint()
{
    scene()->removeItem(this);
}

void RowHint::Reset()
{
    mActive = true;
    mHints.clear();
    mSolved = false;
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
        setCursor(Qt::PointingHandCursor);
        //setOpacity(1);
        setToolTip(tr("Commit Your solution"));
    }
    else
    {
        setCursor(Qt::ArrowCursor);
        //setOpacity(0.2);
        setToolTip(QString(""));
    }
    //scene()->update(mapRectToScene(boundingRect()));
    update(boundingRect());
}

void RowHint::DrawHints(std::vector<int> res)
{
    mHints = res;
    mSolved = true;
    for (unsigned i = 0; i < mHints.size(); ++i)
    {
    }
    update(boundingRect());
}

void RowHint::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    if (mActive && !mSolved)
    {
        if (e->button() == Qt::LeftButton)
        {
            SetActive(false);
            emit HintPressedSignal(mIx);
        }
    }
    QGraphicsItem::mousePressEvent(e);
}

QPainterPath RowHint::shape() const
{
    QPainterPath path;
    path.addRect(boundingRect());
    return path;
}

QRectF RowHint::boundingRect() const
{
    const int Margin = 1;
    return outlineRect().adjusted(-Margin, -Margin, +Margin, +Margin);
}

QRectF RowHint::outlineRect() const
{
    return QRectF(0.0, 0.0, 40.0, 40.0);
}

void RowHint::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /* widget */)
{
    painter->setPen(mPen);
    if (mActive)
    {
        painter->setBrush(mBrush1);
    }
    else
    {
        painter->setBrush(mBrush0);
    }

    QPainterPath path;
    path.addRect(QRectF(0.0, 0.0, 40.0, 40.0));
    for (int i = 0; i < 4; i++)
    {
        //painter->drawEllipse(QRect((i % 2) * 16 + 6, floor(i / 2) * 16 + 6, 12, 12));
        path.addEllipse(QRectF((i % 2) * 16 + 6, floor(i / 2) * 16 + 6, 12, 12));
    }
    //painter->drawRect(QRectF(0.0, 0.0, 40.0, 40.0));
    painter->drawPath(path);

    if (mSolved)
    {
        for (unsigned i = 0; i < mHints.size(); i++)
        {
            if (mHints.at(i) == 2)
            {
                painter->setBrush(QBrush(Qt::black));
            }
            else
            {
                painter->setBrush(QBrush(Qt::white));
            }
            painter->drawEllipse(QRectF((i % 2) * 16 + 6, floor(i / 2) * 16 + 6, 12, 12));
        }
    }
}
