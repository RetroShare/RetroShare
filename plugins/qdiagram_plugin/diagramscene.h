/****************************************************************************
**
** Copyright (C) 2007-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the example classes of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef DIAGRAMSCENE_H
#define DIAGRAMSCENE_H

#include <QGraphicsScene>
#include "diagramitem.h"
#include "diagramdrawitem.h"
#include "diagramtextitem.h"
#include "diagrampathitem.h"

QT_BEGIN_NAMESPACE
class QGraphicsSceneMouseEvent;
class QMenu;
class QPointF;
class QGraphicsLineItem;
class QFont;
class QGraphicsTextItem;
class QColor;
class QFile;
QT_END_NAMESPACE

//! [0]
class DiagramScene : public QGraphicsScene
{
    Q_OBJECT

public:
    enum Mode { InsertItem, InsertLine, InsertText, MoveItem, CopyItem, CopyingItem, InsertDrawItem, Zoom , MoveItems};

    DiagramScene(QMenu *itemMenu, QObject *parent = 0);
    QFont font() const
        { return myFont; }
    QColor textColor() const
        { return myTextColor; }
    QColor itemColor() const
        { return myItemColor; }
    QColor lineColor() const
        { return myLineColor; }
    void setLineColor(const QColor &color);
    void setTextColor(const QColor &color);
    void setItemColor(const QColor &color);
    void setFont(const QFont &font);
    void setArrow(const int i);
    void setGrid(const qreal grid)
    {
    	myGrid=grid;
    }
    void setGridVisible(const bool vis)
    {
    	myGridVisible=vis;
    }
    bool isGridVisible()
    {
    	return myGridVisible;
    }
    void setGridScale(const int k)
    {
    	myGridScale=k;
    }
    bool save(QFile *file);
    bool load(QFile *file);
    QPointF onGrid(QPointF pos);
    void setCursorVisible(bool t);

public slots:
    void setMode(Mode mode,bool m_abort=true);
    void abort(bool keepSelection=false);
    void setItemType(DiagramItem::DiagramType type);
    void setItemType(DiagramDrawItem::DiagramType type);
    void editorLostFocus(DiagramTextItem *item);
    void editorReceivedFocus(DiagramTextItem *item);
    void checkOnGrid();
    void clear();

signals:
    void itemInserted(DiagramItem *item);
    void textInserted(QGraphicsTextItem *item);
    void itemSelected(QGraphicsItem *item);
    void editorHasLostFocus();
    void editorHasReceivedFocus();
    void zoomRect(QPointF p1,QPointF p2);
    void zoom(const qreal factor);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void wheelEvent(QGraphicsSceneWheelEvent *mouseEvent);
    bool event(QEvent *mEvent);
    QGraphicsItem* copy(QGraphicsItem* item);
    void drawBackground(QPainter *p, const QRectF &r);
    void enableAllItems(bool enable=true);


private:
    bool isItemChange(int type);

    DiagramItem::DiagramType myItemType;
    DiagramDrawItem::DiagramType myDrawItemType;
    QMenu *myItemMenu;
    Mode myMode;
    bool leftButtonDown;
    QPointF startPoint;
    QGraphicsLineItem *line;
    QFont myFont;
    DiagramTextItem *textItem;
    QColor myTextColor;
    QColor myItemColor;
    QColor myLineColor;
    DiagramItem *insertedItem;
    DiagramDrawItem *insertedDrawItem;
    DiagramPathItem *insertedPathItem;
    QList<QGraphicsItem *> *copiedItems;
    qreal myDx,myDy;
    DiagramPathItem::DiagramType myArrow;
    qreal myGrid;
    QGraphicsRectItem myCursor;
    qreal myCursorWidth;
    bool myGridVisible;
    int myGridScale;
    QList<QGraphicsItem*> myMoveItems;
    qreal maxZ;
};
//! [0]

#endif
