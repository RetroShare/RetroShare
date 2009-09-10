#include <QtGui>

#include "msg.h"

using namespace std;

Msg::Msg()
{
    /*
    setFont(QFont("Arial", 14, QFont::Bold, false));
    setDefaultTextColor(QColor("#cccccc"));
    setTextWidth(300);

    mTd = new QTextDocument();
    mTd->setDefaultFont(QFont("Arial", 14, QFont::Bold, false));
    mTd->setTextWidth(300);


    //QAbstractTextDocumentLayout* lay = mTd->documentLayout();

    setDocument(mTd);
    */
    mFont = QFont("Arial", 14, QFont::Bold, false);
    mFont.setStyleHint(QFont::SansSerif);
    mLay = new QTextLayout();
    mLay->setFont(mFont);
    mLay->setTextOption(QTextOption(Qt::AlignHCenter));
    mUpdateRect = QRectF(0, 0, 10, 10);
}

Msg::~Msg()
{
}

void Msg::ShowMsg(const QString str)
{
    //setPlainText(str);
    //mTd->clear();
    //mTd->setPlainText(str);
    mUpdateRect = boundingRect();

    mLay->setText(str);
    int leading = -3;//fontMetrics.leading();
    qreal h = 10;
    qreal maxw = 0;
    qreal maxh = 0;
    mLay->beginLayout();

    while (1)
    {
        QTextLine line = mLay->createLine();
        if (!line.isValid())
        {
            break;
        }

        line.setLineWidth(280);
        h += leading;
        line.setPosition(QPointF(0, h));
        h += line.height();
        maxw = qMax(maxw, line.naturalTextWidth());
    }
    mLay->endLayout();


    maxw = qMax(mUpdateRect.width(), mLay->boundingRect().width());
    maxh = qMax(mUpdateRect.height(), mLay->boundingRect().height() + 8);
    mUpdateRect = QRectF(0, 0, maxw, maxh);

    update(boundingRect());
}

QRectF Msg::boundingRect() const
{
    return mUpdateRect;
}

void Msg::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /* widget */)
{
    //painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(QColor("#f0f0f0")));
    mLay->draw(painter, QPoint(0, 0));
}
