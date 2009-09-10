#ifndef PEGROW_H
#define PEGROW_H

#include <QObject>
#include <QColor>
#include <QPen>
#include <QRadialGradient>
#include <QGraphicsItem>
#include <iostream>
#include <vector>
#include "mainwindow.h"
#include "pegrow.h"
#include "colorpeg.h"

class PegRow : public QObject, public QGraphicsItem
{
    Q_OBJECT

public:
    PegRow(QObject* parent = 0);
    ~PegRow();

    int GetIx() const;
    int GetPegCnt();
    ColorPeg** GetPegs();
    std::vector<int> GetSolution() const;
    void SetIx(const int ix);
    void SetActive(const bool b);
    bool SnapCP(ColorPeg* cp);
    void ForceSnap(ColorPeg* cp, int posix);
    void RemovePeg(ColorPeg* cp);

    void CloseRow();
    void ClearRow();
    void Reset();

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    QRectF boundingRect() const;
    QPainterPath shape() const;

signals:
    void RowSolutionSignal(int ix);
    void RemovePegSignal(ColorPeg* cp);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    //void contextMenuEvent(QGraphicsSceneContextMenuEvent* e);

private slots:
    //void FillRandSlot();

private:
    std::vector<int> mSolution;
    QPen mPen;
    bool mIsActive;
    int mIx;

    void CheckSolution();
    ColorPeg* mColorPegs[4];
    QRectF outlineRect() const;
    /*
    void InitActions();
    void InitMenus();
    QAction* FillRandAct;
    QAction* ClearRowAct;
    QMenu* ContextMenu;
    */
};

#endif // PEGROW_H
