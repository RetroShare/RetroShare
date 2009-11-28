#ifndef COLORPEG_H
#define COLORPEG_H

#include <QColor>
#include <QPen>
#include <QRadialGradient>
#include <QGraphicsItem>
#include <iostream>
#include <vector>
#include "mainwindow.h"
#include "pegrow.h"
#include "rowhint.h"


class ColorPeg : public QObject, public QGraphicsItem
{
    Q_OBJECT
    //Q_DECLARE_TR_FUNCTIONS(ColorPeg)

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
    void SetCursorShape( Qt::CursorShape shape = Qt::ArrowCursor, const bool force = false);
    PegType* GetPegType();
    int GetType() const;
    QColor GetPenColor() const;
    QColor GetBrushColor() const;

    bool IsBtn() const;
    void SetBtn(bool b);
    void Reset();

signals:
    void PegReleasedSignal(ColorPeg* cp);
    void PegPressSignal(ColorPeg* cp);

protected:
    PegType* mPegType;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    void mousePressEvent(QGraphicsSceneMouseEvent* e);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* e);

private:
    static const QBrush mShadowBrush;
    static QBrush GetShadowBrush();
    static const QPen mPen;
    static QPen GetOutlinePen();

    QRectF outlineRect() const;
    void SetIsDragged(bool b);

    PegRow* mPegRow;

    const QBrush mBrush;

    int mType;
    bool mIsBtn;
    bool mIsDragged;
};



#endif // COLORPEG_H
