#include <QtGui>

#include "colorpeg.h"

using namespace std;


const QBrush ColorPeg::mShadowBrush = ColorPeg::GetShadowBrush();
const QPen ColorPeg::mPen = ColorPeg::GetOutlinePen();

QBrush ColorPeg::GetShadowBrush()
{
    QRadialGradient rgrad(22, 22, 22, 22, 22);
    rgrad.setColorAt(0.3, QColor(0, 0, 0, 0xff));
    rgrad.setColorAt(1.0, QColor(0, 0, 0, 0));

    return QBrush(rgrad);
}

QPen ColorPeg::GetOutlinePen()
{
    QRadialGradient rgrad(20, 20, 32, 0, 0);
    rgrad.setColorAt(0.0, Qt::black);
    rgrad.setColorAt(1.0, Qt::white);
    return QPen(QBrush(rgrad), 2);
}


ColorPeg::ColorPeg(QObject*)
{
    setFlags(QGraphicsItem::GraphicsItemFlags(0));
    mPegType = NULL;
    mPegRow = NULL;
    SetBtn(true);
    mIsDragged = false;
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
    //setFlags(QGraphicsItem::GraphicsItemFlags(0));
    SetCursorShape();
}

void ColorPeg::SetCursorShape(Qt::CursorShape shape, const bool force)
{
    if (!force)
    {
        if (mIsDragged)
        {
            shape = Qt::ClosedHandCursor;
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
    if (change == ItemPositionHasChanged)
    {
    }
    else if (change == ItemPositionChange)
    {
    }
    else if (change == ItemFlagsChange)
    {
    }
    else if (change == ItemSelectedHasChanged)
    {
        scene()->update(scene()->sceneRect());
    }
    return QGraphicsItem::itemChange(change, value);
}

void ColorPeg::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    if (mPegType != NULL)
    {
        if (e->button() == Qt::LeftButton)
        {
            Reset();
            SetIsDragged(true);
            emit PegPressSignal(this);
            SetCursorShape();
        }
    }
    QGraphicsItem::mousePressEvent(e);
}

void ColorPeg::mouseReleaseEvent (QGraphicsSceneMouseEvent *e)
{
    if (mPegType != NULL)
    {
        if (e->button() == Qt::LeftButton)
        {
            SetIsDragged(false);
            emit PegReleasedSignal(this);
            SetCursorShape();
        }
    }
    QGraphicsItem::mouseReleaseEvent(e);
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

void ColorPeg::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget* /* widget */)
{
    /*
    QPen pen(Qt::red);
    pen.setWidth(2);
    if (option->state & QStyle::State_Selected)
    {
        pen.setStyle(Qt::DotLine);
    }
    */


    if (mIsDragged)
    {
        painter->setPen(Qt::NoPen);
        painter->translate(QPointF(-2, -2));
        painter->setBrush(ColorPeg::mShadowBrush);
        painter->drawEllipse(QRectF(0, 0, 44, 44));
        painter->translate(QPointF(2, 2));
    }
    else
    {
        painter->setPen(ColorPeg::mPen);
    }

    painter->setBrush(QBrush(mPegType->grad));
    painter->drawEllipse(outlineRect());
}
