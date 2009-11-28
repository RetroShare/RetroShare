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

#ifndef DIAGRAMPATHITEM_H
#define DIAGRAMPATHITEM_H

#include <QGraphicsPixmapItem>
#include <QList>

QT_BEGIN_NAMESPACE
class QPixmap;
class QGraphicsItem;
class QGraphicsScene;
class QTextEdit;
class QGraphicsSceneMouseEvent;
class QMenu;
class QGraphicsSceneContextMenuEvent;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;
class QPolygonF;
QT_END_NAMESPACE

//! [0]
class DiagramPathItem : public QGraphicsPathItem
{
public:
    enum { Type = UserType + 6 };
    enum DiagramType { Path, Start, End, StartEnd };

    DiagramPathItem(DiagramType diagramType, QMenu *contextMenu,
        QGraphicsItem *parent = 0, QGraphicsScene *scene = 0);
    DiagramPathItem(QMenu *contextMenu,
    		QGraphicsItem *parent, QGraphicsScene *scene);//constructor fuer Vererbung
    DiagramPathItem(const DiagramPathItem& diagram);//copy constructor

    virtual DiagramPathItem* copy();

    void append(const QPointF point);
    void remove();
    void updateLast(const QPointF point);

    DiagramType diagramType() const
        { return myDiagramType; }

    virtual void setDiagramType(DiagramType type)
		{ myDiagramType=type; }

    QPixmap image() const;
    QPixmap icon();
    QPainterPath getPath() const;
    QList<QPointF> getPoints()
		{ return myPoints; }
    int type() const
        { return Type;}
    void setHandlerWidth(const qreal width)
    {
    	myHandlerWidth = width;
    }

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    void createPath();
    QPainterPath createArrow(QPointF p1, QPointF p2) const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
    QRectF boundingRect() const;
    QPainterPath shape() const;
    bool hasClickedOn(QPointF press_point, QPointF point) const;
    void mousePressEvent(QGraphicsSceneMouseEvent *e);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *e);
    void hoverEnterEvent(QGraphicsSceneHoverEvent *e);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *e);
    QPointF onGrid(QPointF pos);

private:
    DiagramType myDiagramType;
    QMenu *myContextMenu;
    QList<QPointF> myPoints;
    qreal len,breite;
    int mySelPoint,myHoverPoint;
    qreal myHandlerWidth;

};
//! [0]

#endif
