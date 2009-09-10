#ifndef ROWHINT_H
#define ROWHINT_H

#include <QGraphicsItem>
#include <QColor>
#include <QPen>
#include <QRadialGradient>
#include <iostream>
#include <vector>
#include "mainwindow.h"

class RowHint : public QObject, public QGraphicsItem
{
    Q_OBJECT

public:
    RowHint(QObject* parent = 0);
    ~RowHint();

    int mIx;
    bool mActive;
    bool mSolved;
    std::vector<int> mHints;

    int GetIx() const;
    void SetIx(const int ix);
    void SetActive(bool b);
    void DrawHints(std::vector<int> res);
    void Reset();

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    QRectF boundingRect() const;
    QPainterPath shape() const;

signals:
    void HintPressedSignal(int ix);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* e);

private:
    QPen mPen;
    QBrush mBrush0;
    QBrush mBrush1;

    QRectF outlineRect() const;
};

#endif // ROWHINT_H
